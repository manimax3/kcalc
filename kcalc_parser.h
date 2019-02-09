#ifndef KCALC_PARSER_H
#define KCALC_PARSER_H value

#include <QStack>

enum TokenType {
    NUMBER,
    PLUS,
    MINUS,
    MULT,
    DIV,
    BRACK_OPEN,
    BRACK_CLOSE,
    FUNCTION_NAME,
    PWR,
    INVALID
};

enum NumMode {
    DEC,
    HEX,
    OCT,
    BIN
};

struct KCalcToken {
    QString value;
    int startPos;
    TokenType type;
};

class KCalcTokenizer
{
public:
    void addFunctionToken(QString functionName);
    void setNumberMode(NumMode mode);

    void setExpressionString(const QString &expression);

    bool isValidDigit(const QChar &ch) const;

    /**
     * Gurantees:
     *  - input != end
     *  - input points to an unread element
     *  - input needs to be increaded for every read element
     */
    KCalcToken functionMatcher(QString::Iterator &input, const QString::Iterator &end, int pos);
    KCalcToken numberMatcher(QString::Iterator &input, const QString::Iterator &end, int pos);
    KCalcToken operatorMatcher(QString::Iterator &input, const QString::Iterator &end, int pos);

    QList<KCalcToken> parse();

private:
    NumMode current_Mode_;
    QString expression_;
    QList<QString> function_Names_;
};

#endif
