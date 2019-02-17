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
        BRACKET_CLOSE,
        INVALID,
        EOS
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

    class InfixParser
    {
    public:
        virtual ~InfixParser() = default;
        virtual void parse(KCalcParser &parser, const Token &token) = 0;
        virtual int getPrecedence() const = 0;
        virtual bool rightAssociative() const { return false; }
    };

    class PrefixParser
    {
    public:
        virtual ~PrefixParser() = default;
        virtual void parse(KCalcParser &parser, const Token &token) = 0;
    };

    void registerParser(const QString &name, InfixParser *parser);
    void registerParser(const QString &name, PrefixParser *parser);

    void parseExpression(const QString &expression);

    Token consume();
    const Token &peak() const;
    void parse(int p = 0);
    void expect(TokenType token);

    void addDefaultParser();

    QList<Token> output;

Q_SIGNALS:
    void unexpectedToken(const Token &token, TokenType expected);

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
public:
    void parse(KCalcParser &parser, const KCalcParser::Token &token) override
    {
        parser.output.push_back(token);
    }
};

class AdditionParser : public KCalcParser::InfixParser
{
public:
    void parse(KCalcParser &parser, const KCalcParser::Token &token) override;
    int getPrecedence() const override { return 20; }
};

class SubtractionParser : public KCalcParser::InfixParser
{
public:
    void parse(KCalcParser &parser, const KCalcParser::Token &token) override
    {
        parser.parse(this->getPrecedence());
        parser.output.push_back(token);
    }

    int getPrecedence() const override { return 20; }
};

class NegationParser : public KCalcParser::PrefixParser
{
public:
    void parse(KCalcParser &parser, const KCalcParser::Token &token) override
    {
        parser.parse(100);
        parser.output.push_back(token);
    }
};

class GroupParser : public KCalcParser::PrefixParser
{
public:
    void parse(KCalcParser &parser, const KCalcParser::Token &token) override
    {
        parser.parse(0);
        parser.expect(KCalcParser::BRACKET_CLOSE);
    }
};

class MultiplicationParser : public KCalcParser::InfixParser
{
public:
    void parse(KCalcParser &parser, const KCalcParser::Token &token) override
    {
        parser.parse(this->getPrecedence());
        parser.output.push_back(token);
    }

    int getPrecedence() const override { return 40; }
};

#endif
