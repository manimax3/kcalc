#include "kcalc_parser.h"

#include <QLocale>
#include <QChar>
#include <QDebug>

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

void KCalcTokenizer::setNumberMode(NumMode mode)
{
    current_Mode_ = mode;
}

void KCalcTokenizer::setExpressionString(QString expression)
{
    expression_ = std::move(expression);
    current_Pos_ = 0;
    tokens_.clear();
}

bool KCalcTokenizer::isValidDigit(const QChar &ch) const
{
    switch (ch.toUpper().toLatin1()) {
    case 'F':
    case 'E':
    case 'D':
    case 'C':
    case 'B':
    case 'A':
        if (current_Mode_ == HEX) {
            return true;
        }
    case '9':
    case '8':
        if (current_Mode_ == DEC || current_Mode_ == HEX) {
            return true;
        }
    case '7':
    case '6':
    case '5':
    case '4':
    case '3':
    case '2':
        if (current_Mode_ == OCT || current_Mode_ == DEC || current_Mode_ == HEX) {
            return true;
        }
    case '1':
    case '0':
        if (current_Mode_ == BIN || current_Mode_ == OCT || current_Mode_ == DEC || current_Mode_ == HEX) {
            return true;
        }
    default:
        return false;
    }

    return false;
}

int KCalcTokenizer::isNumberNext() const
{
    int i = current_Pos_;

    while (i < expression_.length() && isValidDigit(expression_.at(i))) {
        ++i;
    }

    if (i > current_Pos_ && i < expression_.length() && expression_.at(i) == QLocale().decimalPoint()) {
        // At least one digit found and are now at a decimal point
        const int prev = ++i;
        while (i < expression_.length() && isValidDigit(expression_.at(i))) {
            ++i;
        }

        if (prev == i) {
            // No further digits found
            // TODO: Should this be valid input: 12.
            /* return -1; */
        }
    }

    return i - current_Pos_;
}

QStringView KCalcTokenizer::isFunctionNext() const
{
    const QString start = expression_.right(expression_.length() - current_Pos_);
    for (const auto &func : function_Names_) {
        if (start.startsWith(func, Qt::CaseInsensitive)) {
            return func;
        }
    }

    return QStringView();
}

KCalcToken KCalcTokenizer::readNextToken()
{
    while (current_Pos_ < expression_.length() && expression_.at(current_Pos_).isSpace())
        current_Pos_++;

    if (current_Pos_ >= expression_.length()) {
        return { QString(), 0, INVALID };
    }

    const QChar currentChar = expression_.at(current_Pos_);

    if (currentChar == QLatin1Char('+')) {
        return { QStringLiteral("+"), current_Pos_++, PLUS };
    } else if (currentChar == QLatin1Char('-')) {
        return { QStringLiteral("-"), current_Pos_++, MINUS };
    } else if (currentChar == QLatin1Char('*')) {
        return { QStringLiteral("*"), current_Pos_++, MULT };
    } else if (currentChar == QLatin1Char('/')) {
        return { QStringLiteral("/"), current_Pos_++, DIV };
    } else if (currentChar == QLatin1Char('(')) {
        return { QStringLiteral("("), current_Pos_++, BRACK_OPEN };
    } else if (currentChar == QLatin1Char(')')) {
        return { QStringLiteral(")"), current_Pos_++, BRACK_CLOSE };
    } else if (currentChar == QLatin1Char('^')) {
        return { QStringLiteral("^"), current_Pos_++, PWR };
    }

    const QStringView function = isFunctionNext();

    if (function.length() > 0) {
        KCalcToken token { QString(function.toString()), current_Pos_, FUNCTION_NAME };
        current_Pos_ += function.length();
        return token;
    }

    const int numberFound = isNumberNext();

    if (numberFound > 0) {
        KCalcToken token { expression_.right(expression_.length() - current_Pos_).left(numberFound),
                           current_Pos_,
                           NUMBER };
        current_Pos_ += numberFound;
        return token;
    }

    return { QString(), 0, INVALID };
}

QList<KCalcToken> KCalcTokenizer::parse()
{
    // TODO: Do-while loop instead
    KCalcToken currentToken = readNextToken();
    while (currentToken.type != INVALID && current_Pos_ != expression_.length()) {
        tokens_.push_back(currentToken);
        currentToken = readNextToken();
    }
    return tokens_;
}

#include <iostream>
int main()
{
    KCalcTokenizer tokenizer;
    tokenizer.addFunctionToken(QStringLiteral("sin"));
    tokenizer.addFunctionToken(QStringLiteral("cos"));
    tokenizer.addFunctionToken(QStringLiteral("func"));

    tokenizer.setNumberMode(DEC); // TODO: Doesnt work
    tokenizer.setExpressionString(QStringLiteral("sIn(F*AA)+100,0 cos+")); // TODO: Anything at the end of a string doesnt work

    auto list = tokenizer.parse();

    for (const auto &t : list) {
        std::cout << t.value.toStdString() << std::endl;
    }

    return 0;
}
