#include "kcalc_parser.h"

#include <QLocale>
#include <QChar>
#include <QDebug>

#include <array>

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

void KCalcTokenizer::setExpressionString(const QString &expression)
{
    expression_ = expression;

    for (const auto &ch : expression) {
        if (ch.isSpace())
            continue;
        expression_.append(ch);
    }
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
        return current_Mode_ == HEX;
    case '9':
    case '8':
        return (current_Mode_ == DEC || current_Mode_ == HEX);
    case '7':
    case '6':
    case '5':
    case '4':
    case '3':
    case '2':
        return (current_Mode_ == OCT || current_Mode_ == DEC || current_Mode_ == HEX);
    case '1':
    case '0':
        return (current_Mode_ == BIN || current_Mode_ == OCT || current_Mode_ == DEC || current_Mode_ == HEX);
    default:
        return false;
    }

    return false;
}

bool KCalcTokenizer::followsFunction(const QString::Iterator &input, const QString::Iterator &end) const
{
    QStringView remainder(input, end - input);

    for (const auto &func : function_Names_) {
        if (remainder.startsWith(func, Qt::CaseInsensitive)) {
            return true;
        }
    }

    return false;
}

KCalcToken KCalcTokenizer::numberMatcher(QString::Iterator &input, const QString::Iterator &end, int pos)
{
    auto input_org = input;

    KCalcToken token {
        QString(),
        pos,
        INVALID
    };

    while (isValidDigit(*input) && input != end && !followsFunction(input, end)) {
        token.value.append(*input);
        ++input;
    }

    // TODO: Without a further check this allows .1
    if (input != end && *input == QLocale().decimalPoint()) {
        token.value.append(*(input++));

        while (isValidDigit(*input) && input != end && !followsFunction(input, end)) {
            token.value.append(*input);
            ++input;
        }
    }

    token.type = input_org == input ? INVALID : NUMBER;
    return token;
}

KCalcToken KCalcTokenizer::functionMatcher(QString::Iterator &input, const QString::Iterator &end, int pos)
{
    /* skipWhiteSpace(input, end, pos); */
    const auto readeableCharacters = end - input;
    QString start;

    // TODO We can do that more efficient
    for (int i = 0; i < readeableCharacters; ++i) {
        start.append(*input);
        ++input;
    }

    input -= readeableCharacters;

    for (const auto &func : function_Names_) {
        if (start.startsWith(func, Qt::CaseInsensitive)) {
            input += func.length();
            return { func, pos, FUNCTION_NAME };
        }
    }

    return { QString(), pos, INVALID };
}

/* void KCalcTokenizer::skipWhiteSpace(QString::Iterator &input, const QString::Iterator &end, int &pos) */
/* { */
/*     while (input != end && input->isSpace()) { */
/*         ++input; */
/*         ++pos; */
/*     } */
/* } */

QList<KCalcToken> KCalcTokenizer::parse()
{
    auto begin = expression_.begin();
    const auto end = expression_.end();
    QList<KCalcToken> tokens;

    // TODO Maybe add an overload without int pos
    /* int skipper = 0; */
    /* skipWhiteSpace(begin, end, skipper); */

    while (begin != end) {
        KCalcToken token = functionMatcher(begin, end, expression_.begin() - begin);

        if (token.type == INVALID) {
            token = numberMatcher(begin, end, expression_.begin() - begin);
        }

        if (token.type == INVALID) {
            token.startPos = expression_.begin() - begin;
            token.value.append(*begin);
            ++begin;
        }

        tokens.push_back(token);
    }

    return tokens;
}

#include <iostream>
int main()
{
    KCalcTokenizer tokenizer;
    tokenizer.addFunctionToken(QStringLiteral("sin"));
    tokenizer.addFunctionToken(QStringLiteral("cos"));
    tokenizer.addFunctionToken(QStringLiteral("func"));
    tokenizer.addFunctionToken(QStringLiteral("+"));
    tokenizer.addFunctionToken(QStringLiteral("-"));
    tokenizer.addFunctionToken(QStringLiteral("^"));
    tokenizer.addFunctionToken(QStringLiteral("*"));
    tokenizer.addFunctionToken(QStringLiteral("/"));
    tokenizer.addFunctionToken(QStringLiteral("("));
    tokenizer.addFunctionToken(QStringLiteral(")"));

    tokenizer.setNumberMode(HEX); // TODO: Doesnt work
    tokenizer.setExpressionString(QStringLiteral("sIn(F*AA)+100,0fffunc() AAcos+")); // TODO: Anything at the end of a string doesnt work

    auto list = tokenizer.parse();

    for (const auto &t : list) {
        std::cout << t.type << ":" << t.value.toStdString() << std::endl;
    }

    return 0;
}
