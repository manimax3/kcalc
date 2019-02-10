#ifndef KCALC_PARSER_H
#define KCALC_PARSER_H value

/* #include "knumber/knumber.h" */
#include <QStack>
#include <QMap>

struct KNumber {
    static KNumber Zero;
};

enum TokenType {
    NUMBER, // 0
    FUNCTION, // 1
    BRACKET_OPEN, // 2
    BRACKET_CLOSE, // 3
    OP_PLUS, // 4
    OP_MINUS, // 5
    OP_MULT, // 6
    OP_DIV, // 7
    OP_PWR, // 8
    INVALID // 9
};

// TODO Dont use this enum
enum NumMode {
    DEC,
    HEX,
    OCT,
    BIN
};

struct KCalcToken {
    TokenType type;
    QString value;
};

class KCalcTokenizer
{
public:
    void addFunctionToken(QString functionName);
    bool isValidDigit(const QChar &ch, NumMode numberMode) const;
    QStringView followsFunction(const QString::Iterator &input, const QString::Iterator &end) const;

    QList<KCalcToken> parse(QString expression, NumMode numberMode) const;

private:
    QList<QString> function_Names_;
};

enum Associativity {
    LEFT,
    RIGHT
};

enum Notation {
    INFIX,
    POSTFIX,
    PREFIX
};

class NodeEvaluator
{
public:
    explicit NodeEvaluator(
            int precedence = 0,
            Associativity associativity = LEFT,
            Notation notation = INFIX,
            int operandsCount = 2);
    virtual ~NodeEvaluator() = default;
    virtual bool accepts(const KCalcToken &token) const = 0;
    virtual KNumber evaluate(const KCalcToken &token, QList<KNumber> operands) = 0;

    int getPrecedence() const;
    Associativity getAssociativity() const;
    Notation getNotation() const;
    int getOperandCount() const;

private:
    int precedence_;
    Associativity associativity_;
    Notation notation_;
    int operandCount_;
};

class KCalcParser
{
public:
    KCalcTokenizer &getTokenizer() { return tokenizer_; };

    void registerNode(const QString &mode, NodeEvaluator *NodeEvaluator);
    void setActiveMode(const QString &mode);
    QList<QPair<KCalcToken, NodeEvaluator *>> parseTokens(const QList<KCalcToken> &tokesn);

private:
    NodeEvaluator *findEvaluator(const KCalcToken &token) const;

    QMap<QString, QList<NodeEvaluator *>> evaluators_;
    KCalcTokenizer tokenizer_;
    QString activeMode_;
};

class PowerNode : public NodeEvaluator
{
public:
    explicit PowerNode();
    bool accepts(const KCalcToken &token) const override;
    KNumber evaluate(const KCalcToken &, QList<KNumber>) override;
};

class MultiplicationNode : public NodeEvaluator
{
public:
    explicit MultiplicationNode();
    bool accepts(const KCalcToken &token) const override;
    KNumber evaluate(const KCalcToken &, QList<KNumber>) override;
};

class AdditionNode : public NodeEvaluator
{
public:
    explicit AdditionNode();
    bool accepts(const KCalcToken &token) const override;
    KNumber evaluate(const KCalcToken &, QList<KNumber>) override;
};

class BracketNode : public NodeEvaluator
{
public:
    explicit BracketNode();
    bool accepts(const KCalcToken &token) const override;
    KNumber evaluate(const KCalcToken &, QList<KNumber>) override;
};

class FunctionNode : public NodeEvaluator
{
public:
    explicit FunctionNode();
    bool accepts(const KCalcToken &token) const override;
    KNumber evaluate(const KCalcToken &, QList<KNumber>) override;
};

class NumberNode : public NodeEvaluator
{
public:
    explicit NumberNode();
    bool accepts(const KCalcToken &token) const override;
    KNumber evaluate(const KCalcToken &, QList<KNumber>) override;
};

#endif
