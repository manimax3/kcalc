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

void KCalcTokenizer::addFunctionToken(QString functionName)
{
    bool found = false;
    for (const auto &func : function_Names_) {
        if (func.compare(functionName), Qt::CaseInsensitive) {
            found = true;
            break;
        }
    }

    if (!found) {
        function_Names_.append(functionName);
    }
}

bool KCalcTokenizer::isValidDigit(const QChar &ch, NumMode numberMode) const
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

QStringView KCalcTokenizer::followsFunction(const QString::Iterator &input, const QString::Iterator &end) const
{
    QStringView remainder(input, end - input);

    for (const auto &func : function_Names_) {
        if (remainder.startsWith(func, Qt::CaseInsensitive)) {
            return func;
        }
    }

    return QStringLiteral();
}

QList<KCalcToken> KCalcTokenizer::parse(QString expression, NumMode numberMode) const
{
    static const auto EMPTY = QStringLiteral();
    auto begin = expression.begin();
    const auto end = expression.end();
    QList<KCalcToken> tokens;

    while (begin != end) {

        if (begin->isSpace()) {
            ++begin;
            continue;
        }

        auto follFunction = followsFunction(begin, end);
        if (follFunction.length() > 0) {
            tokens.push_back({ FUNCTION, follFunction.toString() });
            begin += follFunction.length();
            continue;
        }

        const QChar currentChar = *begin;
        switch (currentChar.toLatin1()) {
        case '+':
            ++begin;
            tokens.push_back({ OP_PLUS, EMPTY });
            continue;
        case '-':
            ++begin;
            tokens.push_back({ OP_MINUS, EMPTY });
            continue;
        case '*':
            ++begin;
            tokens.push_back({ OP_MULT, EMPTY });
            continue;
        case '/':
            ++begin;
            tokens.push_back({ OP_DIV, EMPTY });
            continue;
        case '^':
            ++begin;
            tokens.push_back({ OP_PWR, EMPTY });
            continue;
        case '(':
            ++begin;
            tokens.push_back({ BRACKET_OPEN, EMPTY });
            continue;
        case ')':
            ++begin;
            tokens.push_back({ BRACKET_CLOSE, EMPTY });
            continue;
        }

        if (isValidDigit(currentChar, numberMode)) {
            QString number(currentChar);
            while (++begin != end && isValidDigit(*begin, numberMode) && !(followsFunction(begin, end).length() > 0)) {
                number.append(*begin);
            }

            if (begin != end && *begin == QLocale().decimalPoint()) {
                number.append(QLocale().decimalPoint());
                while (++begin != end && isValidDigit(*begin, numberMode) && !(followsFunction(begin, end).length() > 0)) {
                    number.append(*begin);
                }
            }

            if (number.length() > 0) {
                tokens.push_back({ NUMBER, number });
                continue;
            }
        }

        tokens.push_back({ INVALID, *begin++ });
    }

    return tokens;
}

NodeEvaluator::NodeEvaluator(int precedence, Associativity associativity, Notation notation, int operandCount)
    : precedence_(precedence), associativity_(associativity), notation_(notation), operandCount_(operandCount)
{
}

int NodeEvaluator::getPrecedence() const
{
    return precedence_;
}

Associativity NodeEvaluator::getAssociativity() const
{
    return associativity_;
}

Notation NodeEvaluator::getNotation() const
{
    return notation_;
}

int NodeEvaluator::getOperandCount() const
{
    return operandCount_;
}

KCalcParser::KCalcParser(QObject *parent)
    : QObject(parent)
{
    invalidNodeEvaluator_ = new InvalidNode;
}

KCalcParser::~KCalcParser()
{
    for (auto &mode : evaluators_) {
        for (auto &node : mode) {
            delete node;
        }
    }

    delete invalidNodeEvaluator_;
}

void KCalcParser::registerNode(const QString &mode, NodeEvaluator *node)
{
    // TODO Some checks here
    evaluators_[mode].push_back(node);
}

void KCalcParser::setActiveMode(const QString &mode)
{
    activeMode_ = mode;
}

NodeEvaluator *KCalcParser::findEvaluator(const KCalcToken &token) const
{
    const auto &evList = evaluators_[activeMode_];
    for (const auto &ev : evList) {
        if (ev->accepts(token)) {
            return ev;
        }
    }

    return nullptr;
}

InvalidNode *KCalcParser::getInvalidNodeEvaluator()
{
    return invalidNodeEvaluator_;
}

QList<QPair<KCalcToken, NodeEvaluator *>> KCalcParser::parseTokens(const QList<KCalcToken> &tokens)
{
    QList<QPair<KCalcToken, NodeEvaluator *>> matchedNodes;
    QStack<QPair<KCalcToken, NodeEvaluator *>> tokenStack;

    for (const auto &token : tokens) {
        auto *evaluator = findEvaluator(token);

        if (token.type == INVALID) {
            emit foundInvalidTokens();
            continue;
        }

        if (!evaluator) {
            matchedNodes.push_back({ token, getInvalidNodeEvaluator() });
            emit foundUnhandledTokens();
            continue;
        }

        if (token.type == NUMBER) {
            matchedNodes.push_back({ token, evaluator });
            continue;
        }

        if (token.type == FUNCTION) {
            tokenStack.push({ token, evaluator });
            continue;
        }

        if (isOneOf(token.type, OP_PLUS, OP_MINUS, OP_MULT, OP_DIV, OP_PWR)) {
            while (tokenStack.size() > 0 && ((tokenStack.top().first.type == FUNCTION) || (tokenStack.top().second->getPrecedence() > evaluator->getPrecedence()) || (tokenStack.top().second->getPrecedence() == evaluator->getPrecedence() && tokenStack.top().second->getAssociativity() == LEFT))
                   && tokenStack.top().first.type != BRACKET_OPEN) {
                matchedNodes.push_back(tokenStack.pop());
            }
            tokenStack.push({ token, evaluator });
            continue;
        }

        if (token.type == BRACKET_OPEN) {
            tokenStack.push({ token, evaluator });
            continue;
        }

        if (token.type == BRACKET_CLOSE) {
            while (tokenStack.size() > 0 && tokenStack.top().first.type != BRACKET_OPEN) {
                matchedNodes.push_back(tokenStack.pop());
            }

            if (tokenStack.size() == 0) {
                emit foundMismatchedBrackets();
            } else {
                tokenStack.pop();
            }
        }
    }

    while (tokenStack.size() > 0) {
        if (isOneOf(tokenStack.top().first.type, BRACKET_OPEN, BRACKET_CLOSE)) {
            emit foundMismatchedBrackets();
            tokenStack.pop();
            continue;
        }

        matchedNodes.push_back(tokenStack.pop());
    }

    return matchedNodes;
}

PowerNode::PowerNode()
    : NodeEvaluator(4, RIGHT)
{
}
bool PowerNode::accepts(const KCalcToken &token) const
{
    return token.type == OP_PWR;
}

KNumber PowerNode::evaluate(const KCalcToken &, QList<KNumber> numbers)
{
    Q_UNUSED(numbers);
    return KNumber::Zero;
}

MultiplicationNode::MultiplicationNode()
    : NodeEvaluator(3, LEFT)
{
}
bool MultiplicationNode::accepts(const KCalcToken &token) const
{
    return isOneOf(token.type, OP_DIV, OP_MULT);
}

KNumber MultiplicationNode::evaluate(const KCalcToken &, QList<KNumber>)
{
    return KNumber::Zero;
}

AdditionNode::AdditionNode()
    : NodeEvaluator(2, LEFT)
{
}

bool AdditionNode::accepts(const KCalcToken &token) const
{
    return isOneOf(token.type, OP_PLUS, OP_MINUS);
}

KNumber AdditionNode::evaluate(const KCalcToken &, QList<KNumber>)
{
    return KNumber::Zero;
}

BracketNode::BracketNode()
    : NodeEvaluator(1, LEFT)
{
}

bool BracketNode::accepts(const KCalcToken &token) const
{
    return isOneOf(token.type, BRACKET_OPEN, BRACKET_CLOSE);
}

KNumber BracketNode::evaluate(const KCalcToken &, QList<KNumber>)
{
    return KNumber::Zero;
}

FunctionNode::FunctionNode()
    : NodeEvaluator(0, LEFT)
{
}

bool FunctionNode::accepts(const KCalcToken &token) const
{
    return token.type == FUNCTION;
}

KNumber FunctionNode::evaluate(const KCalcToken &, QList<KNumber>)
{
    return KNumber::Zero;
}

NumberNode::NumberNode()
    : NodeEvaluator(-1, LEFT)
{
}

bool NumberNode::accepts(const KCalcToken &token) const
{
    return token.type == NUMBER;
}

KNumber NumberNode::evaluate(const KCalcToken &, QList<KNumber>)
{
    return KNumber::Zero;
}

InvalidNode::InvalidNode()
    : NodeEvaluator(-1, LEFT)
{
}

bool InvalidNode::accepts(const KCalcToken &) const
{
    return true; // We accept all tokens as valid
}

KNumber InvalidNode::evaluate(const KCalcToken &, QList<KNumber>)
{
    Q_ASSERT("Tried to evaluate invalid node" && false);
    return KNumber::Zero;
}

#include <iostream>
int main()
{
    KCalcParser parser;
    auto &tokenizer = parser.getTokenizer();
    tokenizer.addFunctionToken(QStringLiteral("sin"));
    tokenizer.addFunctionToken(QStringLiteral("cos"));
    tokenizer.addFunctionToken(QStringLiteral("func"));

    parser.setActiveMode(QStringLiteral("main"));
    parser.registerNode(QStringLiteral("main"), new PowerNode);
    parser.registerNode(QStringLiteral("main"), new AdditionNode);
    parser.registerNode(QStringLiteral("main"), new MultiplicationNode);
    parser.registerNode(QStringLiteral("main"), new BracketNode);
    parser.registerNode(QStringLiteral("main"), new FunctionNode);
    parser.registerNode(QStringLiteral("main"), new NumberNode);

    auto list = tokenizer.parse(QStringLiteral("10 (+ 20 + 30"), HEX);
    auto result = parser.parseTokens(list);

    /* auto list = tokenizer.parse(); */

    for (const auto &t : list) {
        std::cout << t.type << ":" << t.value.toStdString() << std::endl;
    }

    for (const auto &r : result) {
        std::cout << r.first.type << " " << r.first.value.toStdString() << "\n";
    }

    return 0;
}
