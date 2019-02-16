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
        return numberMode == HEX;
    case '9':
    case '8':
        return (numberMode == DEC || numberMode == HEX);
    case '7':
    case '6':
    case '5':
    case '4':
    case '3':
    case '2':
        return (numberMode == OCT || numberMode == DEC || numberMode == HEX);
    case '1':
    case '0':
        return (numberMode == BIN || numberMode == OCT || numberMode == DEC || numberMode == HEX);
    default:
        return false;
    }

    return false;
}

QStringView KCalcParser::findOperator(QString::Iterator position) const
{
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
        auto foundFunction = findOperator(position);
        if (foundFunction.length() > 0) {
            Token token {
                OPERATOR,
                foundFunction.toString(),
                start - position
            };

            position += foundFunction.length();
            tokens_.push_back(token);
            continue;
        }

        if (isValidDigit(*position, HEX)) { // TODO
            QString numberValue;
            while (position != end && isValidDigit(*position, HEX) && findOperator(position).length() == 0) {
                numberValue.append(*position);
                ++position;
            }

            if (position != end && *position == QLocale().decimalPoint()) {
                numberValue.append(*position);
                ++position;
                while (position != end && isValidDigit(*position, HEX) && findOperator(position).length() == 0) {
                    numberValue.append(*position);
                    ++position;
                }
            }

            tokens_.push_back(Token { NUMBER, std::move(numberValue), start - position });
            continue;
        }

        tokens_.push_back(Token { INVALID, *position++, start - position });
    }
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

void KCalcParser::parseExpression(const QString &expression)
{
    currentExpression = expression;
    position = currentExpression.begin();
    tokenize();
}

KCalcParser::Expression *KCalcParser::parse(int p)
{
    const auto start = consume();
    auto *parser = start.type != NUMBER ? findPrefixParser(start.value)
                                        : new NumberParser;

    if (!parser) {
        // TODO ERROR
    }

    auto *left = parser->parse(*this, start);

    while (tokens_.size() > 0) {
        auto next = peak();
        if (next.type == INVALID) {
            // TODO ERROR
            break;
        }

        auto *infparser = findInfixParser(next.value);

        if (!infparser) {
            // TODO ERROR
            break;
        }

        if (infparser->getPrecedence() <= p) {
            break;
        }

        left = infparser->parse(*this, left, consume());
    }

    return left;
}

void KCalcParser::registerParser(const QString &name, InfixParser *parser)
{
    infixParsers[name] = parser;
}

void KCalcParser::registerParser(const QString &name, PrefixParser *parser)
{
    prefixParsers[name] = parser;
}

KCalcParser::PrefixParser* KCalcParser::findPrefixParser(const QString &name)
{
    const auto parser = prefixParsers.find(name);
    return parser != prefixParsers.end() ? *parser : nullptr;
}

KCalcParser::InfixParser* KCalcParser::findInfixParser(const QString &name)
{
    const auto parser = infixParsers.find(name);
    return parser != infixParsers.end() ? *parser : nullptr;
}

void KCalcParser::addDefaultParser()
{
    registerParser(QStringLiteral("+"), new AdditionParser);
}

AdditionParser::AdditionExpression::AdditionExpression(Expression *lhs, Expression *rhs)
    : lhs(lhs), rhs(rhs)
{
}

KNumber AdditionParser::AdditionExpression::evaluate() const
{
    return lhs->evaluate() + rhs->evaluate();
}

KCalcParser::Expression *AdditionParser::parse(KCalcParser &parser, KCalcParser::Expression *lhs, const KCalcParser::Token &token)
{
    Q_UNUSED(token);
    const int precedence = getPrecedence();
    return new AdditionExpression(lhs, parser.parse(precedence));
}
