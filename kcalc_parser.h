#ifndef KCALC_PARSER_H
#define KCALC_PARSER_H value

#include "knumber/knumber.h"
#include <QStack>
#include <QMap>

enum TokenType {
    NUMBER,
    BRACK_CLOSE_MISSING,
    FUNCTION_NAME,
    INVALID
};

// TODO Dont use this enum
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
    bool followsFunction(const QString::Iterator &input, const QString::Iterator &end) const;

    /**
     * Gurantees:
     *  - input != end
     *  - input points to an unread element
     *  - input needs to be increaded for every read element
     *  TODO: Those functions should be private
     */
    KCalcToken functionMatcher(QString::Iterator &input, const QString::Iterator &end, int pos);
    KCalcToken numberMatcher(QString::Iterator &input, const QString::Iterator &end, int pos);

    QList<KCalcToken> parse();

private:
    NumMode current_Mode_;
    QString expression_;
    QList<QString> function_Names_;
};

class KCalcParser
{
public:
    enum Associativity {
        LEFT,
        RIGHT
    };

    enum Notation {
        INFIX,
        POSTFIX,
        PREFIX
    };

    class Node
    {
    public:
        explicit Node(const QString &identifier, int precedence, Associativity associativity, Notation notation, int operandCount);
        virtual ~Node() = default;

        virtual KNumber evaluateNode(const QString &nodeValue, QList<KNumber> operands) const = 0;

        const QString &getIdentifier() const;
        int getPrecedence() const;
        Associativity getAssociativity() const;
        Notation getNotation() const;
        int getOperandCount() const;

    private:
        QString identifier_;
        int precedence_;
        Associativity associativity_;
        Notation notation_;
        int operandCount_;
    };

    // TODO: Some other parts of kcalc should already have enums for the different modes
    KCalcTokenizer &getTokenizer() { return tokenizer_; };

    void registerNode(const QString &mode, TokenType type, Node *node);
    void setActiveMode(const QString &mode);

    /**
     * Function witch does some parsing and determines some validity questions
     * Use case could be for the valdiator
     *  - Concatenate invalid character runs
     *  - Generate the tokens
     *  - Convert token values
     */
    QList<KCalcToken> preParse(const QString &expression);
    QList<QPair<Node *, QString>> parseTokens(const QList<KCalcToken> &tokens);

private:
    // Mapes the current mode and token type to an accepting node
    // --> Only one node per type
    QMap<QPair<QString, TokenType>, Node *> nodes_;
    QString activeMode_;
    KCalcTokenizer tokenizer_;
};

#endif
