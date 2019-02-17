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
                position - start
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

            tokens_.push_back(Token { NUMBER, std::move(numberValue), position - start });
            continue;
        }

        if (position->toLatin1() == ')') {
            tokens_.push_back(Token { BRACKET_CLOSE, *position++, position - start });
            continue;
        }

        tokens_.push_back(Token { INVALID, *position++, position - start });
    }
}

const KCalcParser::Token &KCalcParser::peak() const
{
    if (tokens_.size() > 0) {
        return tokens_.front();
    } else {
        static Token t { EOS, QStringLiteral(), -1 };
        return t;
    }
}

KCalcParser::Token KCalcParser::consume()
{
    if (tokens_.size() > 0) {
        const auto token = tokens_.front();
        tokens_.pop_front();
        return token;
    } else {
        static Token t { EOS, QStringLiteral(), -1 };
        return t;
    }
}

void KCalcParser::expect(TokenType token)
{
    auto next = consume();
    if (next.type != token)
        emit unexpectedToken(next, token);
}

void KCalcParser::parseExpression(const QString &expression)
{
    currentExpression = expression;
    position = currentExpression.begin();
    tokenize();
}

void KCalcParser::parse(int p)
{
    const auto start = consume();
    auto *parser = start.type != NUMBER ? findPrefixParser(start.value)
                                        : new NumberParser;

    if (!parser) {
        emit unexpectedToken(start, OPERATOR);
        return;
    }

    parser->parse(*this, start);

    while (tokens_.size() > 0) {
        auto next = peak();
        if (next.type == INVALID) {
            emit unexpectedToken(next, OPERATOR);
            break;
        }

        auto *infparser = findInfixParser(next.value);

        if (!infparser) {
            unexpectedToken(next, OPERATOR);
            break;
        }

        if (infparser->getPrecedence() <= p) {
            break;
        }

        infparser->parse(*this, consume());
    }
}

void KCalcParser::registerParser(const QString &name, InfixParser *parser)
{
    infixParsers[name] = parser;
}

void KCalcParser::registerParser(const QString &name, PrefixParser *parser)
{
    prefixParsers[name] = parser;
}

KCalcParser::PrefixParser *KCalcParser::findPrefixParser(const QString &name)
{
    const auto parser = prefixParsers.find(name);
    return parser != prefixParsers.end() ? *parser : nullptr;
}

KCalcParser::InfixParser *KCalcParser::findInfixParser(const QString &name)
{
    const auto parser = infixParsers.find(name);
    return parser != infixParsers.end() ? *parser : nullptr;
}

void KCalcParser::addDefaultParser()
{
    registerParser(QStringLiteral("+"), new AdditionParser);
    registerParser(QStringLiteral("*"), new MultiplicationParser);
    registerParser(QStringLiteral("-"), new SubtractionParser);
    registerParser(QStringLiteral("-"), new NegationParser);
    registerParser(QStringLiteral("("), new GroupParser);
}

void AdditionParser::parse(KCalcParser &parser, const KCalcParser::Token &token)
{
    const int precedence = getPrecedence();
    parser.parse(precedence);
    parser.output.push_back(token);
}
