#ifndef KCALC_PARSER_H
#define KCALC_PARSER_H value

#include <QStack>

class ExpressionToken {
    public:
        virtual ~ExpressionTokenType() = default;
};

class ExpressionTokenType
{
public:
    enum Associativity {
        LEFT,
        RIGHT
    };

    explicit ExpressionTokenType(int precedence, Associativity associativity = LEFT);
    virtual ~ExpressionTokenType() = default;

    int getPrecedence() const;
    Associativity getAssociativity() const;

    virtual ExpressionToken *matchSequence(const QString &seq, int &pos) = 0;

private:
    int precedence_;
    Associativity associativity_;
};

#endif
