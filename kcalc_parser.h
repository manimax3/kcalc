#ifndef KCALC_PARSER_H
#define KCALC_PARSER_H value

#include "knumber/knumber.h"
#include <QStack>
#include <QMap>
#include <QObject>
#include <QDebug>

class KCalcParser : public QObject
{
    Q_OBJECT

public:
    enum TokenType {
        NUMBER,
        OPERATOR,
        BRACKET_CLOSE,
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

    using ParseFunc = int (*)(KCalcParser &, const Token &, int);
    using EvaluateFunc = KNumber (*)(const QList<KNumber> &operands);

    using PrefParseFunc = void (*)(KCalcParser &, const Token &, int);
    using PrefEvaluateFunc = KNumber (*)(KNumber operand);

    struct InfixParser {
        int precedence;
        bool leftassociative;
        ParseFunc parse;
        EvaluateFunc eval;
    };

    struct PrefixParser {
        int precedence;
        PrefParseFunc parse;
        PrefEvaluateFunc eval;
    };

    void registerInfixParser(const QString &name,
                             int precedence,
                             ParseFunc parse,
                             EvaluateFunc eval,
                             bool leftassociative = true);

    void registerPrefixParser(const QString &name,
                              int precedence,
                              PrefParseFunc parse,
                              PrefEvaluateFunc eval);

    KNumber parseExpression(const QString &expression);

    Token consume();
    const Token &peak() const;
    void parse(int p = 0);
    void expect(TokenType token);
    void expect(TokenType type, const QString &value);

    void addDefaultParser();

Q_SIGNALS:
    void expectedToken(KCalcParser::TokenType type, QString value);
    void foundInvalidToken(KCalcParser::Token token);

private:
    static bool isValidDigit(const QChar &ch, NumBase base);
    void tokenize();

    QStringView findOperator(QString::Iterator position) const;
    InfixParser *findInfixParser(const QString &value);
    PrefixParser *findPrefixParser(const QString &value);

    QString currentExpression;
    QString::Iterator position;
    QMap<QString, InfixParser> infixParsers;
    QMap<QString, PrefixParser> prefixParsers;
    QList<Token> tokens_;
    QStack<KNumber> operands_;
};

Q_DECLARE_METATYPE(KCalcParser::TokenType);
Q_DECLARE_METATYPE(KCalcParser::Token);
#endif
