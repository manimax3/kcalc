#include "kcalc_parser.h"

#include <QLocale>
#include <QChar>
#include <QQueue>

#include <tuple>

namespace {
template<typename T>
constexpr bool isOneOf(T &&)
{
    return false;
}

template<typename T, typename B, typename... R>
constexpr bool isOneOf(T &&t, B &&b, R &&... r)
{
    return t == b || isOneOf(t, r...);
}
}

bool KCalcParser::isValidDigit(const QChar &ch, NumBase numberMode)
{
    switch (ch.toUpper().toLatin1()) {
    case 'F':
    case 'E':
    case 'D':
    case 'C':
    case 'B':
    case 'A':
        return numberMode == NB_HEX;
    case '9':
    case '8':
        return isOneOf(numberMode, NB_DECIMAL, NB_HEX);
    case '7':
    case '6':
    case '5':
    case '4':
    case '3':
    case '2':
        return isOneOf(numberMode, NB_OCTAL, NB_DECIMAL, NB_HEX);
    case '1':
    case '0':
        return isOneOf(numberMode, NB_BINARY, NB_OCTAL, NB_DECIMAL, NB_HEX);
    default:
        return false;
    }

    return false;
}

QStringView KCalcParser::findOperator(QString::Iterator position) const
{
    // TODO FIXME cosh <=> cos
    const QStringView current(position, currentExpression.end() - position);
    for (const auto &key : infixParsers.keys()) {
        if (current.startsWith(key, Qt::CaseInsensitive)) {
            return key;
        }
    }

    for (const auto &key : prefixParsers.keys()) {
        if (current.startsWith(key, Qt::CaseInsensitive)) {
            return key;
        }
    }

    return QStringView();
}

void KCalcParser::tokenize()
{
    const auto end = currentExpression.end();
    const auto start = currentExpression.begin();

    while (position != end) {

        if (position->isSpace()) {
            ++position;
            continue;
        }

        auto foundFunction = findOperator(position);
        if (foundFunction.length() > 0) {
            Token token {
                OPERATOR,
                foundFunction.toString(),
                position - start
            };

            position += foundFunction.length();
            tokens_.push_back(token);
            continue;
        }

        if (isValidDigit(*position, getNumBase())) { // TODO
            QString numberValue;
            while (position != end && isValidDigit(*position, getNumBase()) && findOperator(position).length() == 0) {
                numberValue.append(*position);
                ++position;
            }

            if (position != end && *position == QLocale().decimalPoint()) {
                numberValue.append(*position);
                ++position;
                while (position != end && isValidDigit(*position, getNumBase()) && findOperator(position).length() == 0) {
                    numberValue.append(*position);
                    ++position;
                }
            }

            tokens_.push_back(Token { NUMBER, std::move(numberValue), position - start });
            continue;
        }

        tokens_.push_back(Token { INVALID, *position++, position - start - 1 });
    }
}

void KCalcParser::setNumBase(NumBase numbase)
{
    numberBase_ = numbase;
}

NumBase KCalcParser::getNumBase() const
{
    return numberBase_;
}

void KCalcParser::setAngleMode(AngleMode anglemode)
{
    angleMode_ = anglemode;
}

AngleMode KCalcParser::getAngleMode() const
{
    return angleMode_;
}

const KCalcParser::Token &KCalcParser::peak() const
{
    return tokens_.front();
}

KCalcParser::Token KCalcParser::consume()
{
    const auto token = tokens_.front();
    tokens_.pop_front();
    return token;
}

void KCalcParser::expect(TokenType token)
{
    if (tokens_.size() == 0) {
        emit foundInvalidToken(currentExpression.length());
        return;
    }

    auto next = consume();
    if (next.type != token) {
        emit foundInvalidToken(next.debugPos);
    }
}
void KCalcParser::expect(TokenType type, const QString &value)
{
    if (tokens_.size() == 0) {
        emit foundInvalidToken(currentExpression.length());
        return;
    }

    auto next = consume();
    if (next.type != type || next.value != value) {
        emit foundInvalidToken(next.debugPos);
    }
}

KNumber KCalcParser::parseExpression(const QString &expression)
{
    currentExpression = expression;
    position = currentExpression.begin();
    tokens_.clear();
    operands_.clear();
    tokenize();
    parse();

    for (const auto &remainder : tokens_) {
        foundInvalidToken(remainder.debugPos);
    }

    if (operands_.size() > 0) {
        return operands_.top();
    }

    return KNumber::Zero;
}

void KCalcParser::parse(int p)
{
    if (tokens_.size() == 0) {
        return;
    }

    const auto start = consume();

    if (start.type == OPERATOR) {
        auto *parser = findPrefixParser(start.value);

        if (!parser) {
            emit foundInvalidToken(start.debugPos);
            return;
        }

        parser->parse(*this, start, parser->precedence);

        if (operands_.size() < 1) {
            // todo error
            return;
        }

        operands_.push(parser->eval(operands_.pop()));
    } else if (start.type == NUMBER) {

        bool ok;
        // The old kcalcdisplay did the conversion like this
        // but KNumber *should* be able to do this directly
        const auto number = start.value.toULongLong(&ok, getNumBase());
        if (!ok) {
            emit foundInvalidToken(start.debugPos);
        }
        operands_.push(KNumber(number));
    } else if (start.type == INVALID) {
        emit foundInvalidToken(start.debugPos);
        parse(p);
    }

    while (tokens_.size() > 0) {
        auto next = peak();

        if (next.value == QStringLiteral(")")) {
            break;
        }

        if (next.type == INVALID) {
            emit foundInvalidToken(consume().debugPos);
            continue;
        }

        auto *infparser = findInfixParser(next.value);

        if (!infparser) {
            foundInvalidToken(consume().debugPos);
            continue;
        }

        if (infparser->precedence <= p) {
            break;
        }

        int opCount = infparser->parse(*this, consume(), infparser->precedence);
        if (operands_.size() < opCount) {
            // todo error
            return;
        }

        QList<KNumber> ops;
        for (int i = 0; i < opCount; ++i) {
            ops.push_front(operands_.pop());
        }
        operands_.push(infparser->eval(ops));
    }
}

void KCalcParser::registerInfixParser(const QString &name, int precedence,
                                      ParseFunc parse, EvaluateFunc eval, bool leftassociative)
{
    infixParsers[name] = InfixParser { precedence, leftassociative, parse, eval };
}

void KCalcParser::registerPrefixParser(const QString &name, int precedence,
                                       PrefParseFunc parse, PrefEvaluateFunc eval)
{
    prefixParsers[name] = PrefixParser { precedence, parse, eval };
}

KCalcParser::PrefixParser *KCalcParser::findPrefixParser(const QString &name)
{
    const auto parser = prefixParsers.find(name);
    return parser != prefixParsers.end() ? &(*parser) : nullptr;
}

KCalcParser::InfixParser *KCalcParser::findInfixParser(const QString &name)
{
    const auto parser = infixParsers.find(name);
    return parser != infixParsers.end() ? &(*parser) : nullptr;
}

void KCalcParser::addDefaultParser()
{
    registerInfixParser(QStringLiteral("+"), 10, [](KCalcParser &parser, const KCalcParser::Token &, int precedence) {
        parser.parse(precedence);
        return 2; }, [](const QList<KNumber> &operands) { return operands[0] + operands[1]; });

    registerInfixParser(QStringLiteral("-"), 10, [](KCalcParser &parser, const KCalcParser::Token &, int precedence) {
        parser.parse(precedence);
        return 2; }, [](const QList<KNumber> &operands) { return operands[0] - operands[1]; });

    registerInfixParser(QStringLiteral("*"), 20, [](KCalcParser &parser, const KCalcParser::Token &, int precedence) {
        parser.parse(precedence);
        return 2; }, [](const QList<KNumber> &operands) { return operands[0] * operands[1]; });

    registerInfixParser(QStringLiteral("/"), 20, [](KCalcParser &parser, const KCalcParser::Token &, int precedence) {
        parser.parse(precedence); return 2; }, [](const QList<KNumber> &operands) { return operands[0] / operands[1]; });

    registerInfixParser(QStringLiteral("^"), 25, [](KCalcParser &parser, const KCalcParser::Token &, int precedence) {
        parser.parse(precedence); return 2; }, [](const QList<KNumber> &operands) { return operands[0].pow(operands[1]); },
                        false);

    registerPrefixParser(QStringLiteral("-"), 30, [](KCalcParser &parser, const KCalcParser::Token &, int precedence) { parser.parse(precedence); }, [](KNumber operand) { return -operand; });

    registerPrefixParser(QStringLiteral("("), 0, [](KCalcParser &parser, const KCalcParser::Token &, int) {
        parser.parse();
        parser.expect(KCalcParser::INVALID, QStringLiteral(")")); }, [](KNumber operand) { return operand; });

    registerPrefixParser(QStringLiteral("sin"), 50, [](KCalcParser &parser, const KCalcParser::Token &, int) {
        parser.expect(KCalcParser::OPERATOR, QStringLiteral("("));
        parser.parse();
        parser.expect(KCalcParser::INVALID, QStringLiteral(")")); }, [](KNumber operand) { return operand.sin(); });

    registerPrefixParser(QStringLiteral("cos"), 50, [](KCalcParser &parser, const KCalcParser::Token &, int) {
        parser.expect(KCalcParser::OPERATOR, QStringLiteral("("));
        parser.parse();
        parser.expect(KCalcParser::INVALID, QStringLiteral(")")); }, [](KNumber operand) { return operand.cos(); });

    registerPrefixParser(QStringLiteral("func"), 50, [](KCalcParser &parser, const KCalcParser::Token &, int) {
        parser.expect(KCalcParser::OPERATOR, QStringLiteral("("));
        parser.parse();
        parser.expect(KCalcParser::INVALID, QStringLiteral(")")); }, [](KNumber operand) { return operand + KNumber(10); });

    registerPrefixParser(QStringLiteral("tan"), 50, [](KCalcParser &parser, const KCalcParser::Token &, int) {
        parser.expect(KCalcParser::OPERATOR, QStringLiteral("("));
        parser.parse();
        parser.expect(KCalcParser::INVALID, QStringLiteral(")")); }, [](KNumber operand) { return operand.tan(); });

    registerPrefixParser(QStringLiteral("log"), 50, [](KCalcParser &parser, const KCalcParser::Token &, int) {
        parser.expect(KCalcParser::OPERATOR, QStringLiteral("("));
        parser.parse();
        parser.expect(KCalcParser::INVALID, QStringLiteral(")")); }, [](KNumber operand) { return operand.log10(); });

    registerPrefixParser(QStringLiteral("ln"), 50, [](KCalcParser &parser, const KCalcParser::Token &, int) {
        parser.expect(KCalcParser::OPERATOR, QStringLiteral("("));
        parser.parse();
        parser.expect(KCalcParser::INVALID, QStringLiteral(")")); }, [](KNumber operand) { return operand.ln(); });

    registerInfixParser(QStringLiteral("!"), 50, [](KCalcParser &, const KCalcParser::Token &, int) { return 1; }, [](const QList<KNumber> &operand) { return operand[0].factorial(); });

    registerInfixParser(QStringLiteral("mod"), 5, [](KCalcParser &parser, const KCalcParser::Token &, int precedence) { 
            parser.parse(precedence);
            return 2; }, [](const QList<KNumber> &operand) { return operand[0] % operand[1]; });
}
