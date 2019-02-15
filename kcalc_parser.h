#ifndef KCALC_PARSER_H
#define KCALC_PARSER_H value

#include "knumber/knumber.h"
#include <QStack>
#include <QMap>
#include <QObject>

class KCalcParser : public QObject
{
    Q_OBJECT

public:
    enum TokenType {
        NUMBER,
        OPERATOR,
        INVALID
    };

    enum NumBase {
        HEX,
        DEC,
        OCT,
        BIN
    };

    struct Token {
        TokenType type;
        QString value;
        long debugPos;
    };

    class Expression
    {
    public:
        virtual ~Expression() = default;
        virtual KNumber evaluate() const = 0;
    };

    class InfixParser
    {
    public:
        virtual ~InfixParser() = default;
        virtual Expression *parse(KCalcParser &parser, Expression *lhs, const Token &token) = 0;
        virtual int getPrecedence() const = 0;
        virtual bool rightAssociative() const { return false; }
    };

    class PrefixParser
    {
    public:
        virtual ~PrefixParser() = default;
        virtual Expression *parse(KCalcParser &parser, const Token &token) = 0;
    };

    void registerParser(const QString &name, InfixParser *parser);
    void registerParser(const QString &name, PrefixParser *parser);

    void parseExpression(const QString &expression);

    Token consume();
    const Token &peak() const;
    Expression *parse(int p = 0);

    void addDefaultParser();

private:
    static bool isValidDigit(const QChar &ch, NumBase base);
    void tokenize();

    QStringView findOperator(QString::Iterator position) const;
    InfixParser *findInfixParser(const QString &value);
    PrefixParser *findPrefixParser(const QString &value);

    QString currentExpression;
    QString::Iterator position;
    QMap<QString, InfixParser *> infixParsers;
    QMap<QString, PrefixParser *> prefixParsers;
    QList<Token> tokens_;
};

class NumberParser : public KCalcParser::PrefixParser
{
    class NumberExpression : public KCalcParser::Expression
    {
    public:
        explicit NumberExpression(const KCalcParser::Token &token)
            : token(token)
        {
        }

        KNumber evaluate() const override
        {
            return KNumber(token.value);
        }

    private:
        KCalcParser::Token token;
    };

    KCalcParser::Expression *parse(KCalcParser &parser, const KCalcParser::Token &token) override
    {
        return new NumberExpression(token);
    }
};

class AdditionParser : public KCalcParser::InfixParser
{
public:
    class AdditionExpression : public KCalcParser::Expression
    {
    public:
        explicit AdditionExpression(Expression *lhs, Expression *rhs);
        KNumber evaluate() const override;

    private:
        Expression *lhs, *rhs;
    };

    KCalcParser::Expression *parse(KCalcParser &parser, KCalcParser::Expression *lhs, const KCalcParser::Token &token) override;
    int getPrecedence() const override { return 20; }
};

#endif
