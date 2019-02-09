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

    void setExpressionString(QString expression);

    bool isValidDigit(const QChar &ch) const;
    int isNumberNext() const;
    QStringView isFunctionNext() const;

    KCalcToken readNextToken();
    QList<KCalcToken> parse();

private:
    NumMode current_Mode_;
    QString expression_;
    int current_Pos_ = 0;
    QList<KCalcToken> tokens_;
    QList<QString> function_Names_;
};

#endif
