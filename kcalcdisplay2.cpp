#include "kcalcdisplay2.h"

#include <QApplication>
#include <QStyle>
#include <QStyleOption>
#include <QStack>
#include <QValidator>

#include <knotification.h>
#include <KLocalizedString>

#include "kcalc_settings.h"

namespace {
class KCalcDisplayValidator : public QValidator
{
public:
    explicit KCalcDisplayValidator(KCalcDisplay2 *display, QWidget *parent = nullptr)
        : QValidator(parent), display_(display)
    {
    }

    void fixup(QString &input) const override
    {
        Q_UNUSED(input);
        // TODO: What can we fix? Brackets, Grouping
        // This is only run after returnPressed() so find a way to trigger this manually
        qDebug() << "fixupt";
    }

    QValidator::State validate(QString &input, int &pos) const override
    {
        Q_UNUSED(input);
        Q_UNUSED(pos);

        // TODO: Things we need to check for:
        //  1. Brackets validity
        //  2. Operators validity
        //  3. Correct number system used ie FF not valid in Dec mode
        //  3. Correct number grouping
        //
        
        /*
         * Checking the whole string every time is way too expensive
         * So we need to check incrementally
         */

        return validateBrackets(input);
    }

    /*
     * Stack based validating? What should we return if wrong? Invalid or Intermediate
     * Intermediate: Unclosed brackets
     * Invalid: Closed brackets without matching open bracket
     */
    QValidator::State validateBrackets(QString &input) const
    {
        int bracketCounter = 0;

        for (const auto &ch : input) {
            if (ch == QLatin1Char('(')) {
                ++bracketCounter;
            } else if (ch == QLatin1Char(')')) {
                --bracketCounter;
            }

            if(bracketCounter < 0) {
                return QValidator::Invalid;
            }
        }

        if(bracketCounter > 0) {
            return QValidator::Intermediate;
        }

        return QValidator::Acceptable;
    }

    // For these operations we propbably need to tokenize the string

    QValidator::State validateNumberSystem(QString &input) const
    {
        /*
         * 1. Skip whitespace and characters until valid digit in any number system
         * 2. Iterate digits and check if number system is valid
         * 3. keep track of
         */

        Q_UNUSED(input);
        return QValidator::Acceptable;
    }

    QValidator::State validateNumberGrouping(QString &input) const
    {
        Q_UNUSED(input);
        return QValidator::Acceptable;
    }

    QValidator::State validateOperators(QString &input) const
    {
        Q_UNUSED(input);
        return QValidator::Acceptable;
    }

private:
    KCalcDisplay2 *display_;
};
}

KCalcDisplay2::KCalcDisplay2(QWidget *parent)
    : QLineEdit(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    setBackgroundRole(QPalette::Base);
    setForegroundRole(QPalette::Text);
    setAlignment(Qt::Alignment(Qt::AlignRight | Qt::AlignVCenter));

    setValidator(new KCalcDisplayValidator(this, this));
    connect(this, &KCalcDisplay2::textChanged, this, [this]() {

        return;
        if (!this->hasAcceptableInput()) {
            /* this->clear(); */
            qDebug() << "Cleared";
        }

    });

    // TODO: Frame style sunken

    sendEvent(EventReset);
}

bool KCalcDisplay2::sendEvent(Event event)
{
    switch (event) {
    case EventClear:
        clear();
        return true;
    case EventReset:
        clear();
        return true;
    default:
        return true;
    }
}

void KCalcDisplay2::initStyleOption(QStyleOptionFrame *option) const
{
    if (!option) {
        return;
    }

    option->initFrom(this);
    option->state &= ~QStyle::State_HasFocus; // don't draw focus highlight

    // TODO:
    /* if (frameShadow() == QFrame::Sunken) { */
    option->state |= QStyle::State_Sunken;
    /* } else if (frameShadow() == QFrame::Raised) { */
    option->state |= QStyle::State_Raised;
    /* } */

    option->lineWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, option, this);
    option->midLineWidth = 0;
}

void KCalcDisplay2::changeSettings()
{
    QPalette pal = palette();
    pal.setColor(QPalette::Text, KCalcSettings::foreColor());
    pal.setColor(QPalette::Base, KCalcSettings::backColor());
    setPalette(pal);

    setFont(KCalcSettings::displayFont());

    setBeep(KCalcSettings::beep());
    setGroupDigits(KCalcSettings::groupDigits());
    setBinaryGrouping(KCalcSettings::binaryGrouping());
    setOctalGrouping(KCalcSettings::octalGrouping());
    setHexadecimalGrouping(KCalcSettings::hexadecimalGrouping());
}

QSize KCalcDisplay2::sizeHint() const
{
    QSize size = QLineEdit::sizeHint();

    QFont fnt(font());
    fnt.setPointSize(qMax(((fnt.pointSize() * 3) / 4), 7));

    const QFontMetrics fm(fnt);
    size.setHeight(size.height() + fm.height());

    QStyleOptionFrame option;
    initStyleOption(&option);

    /* TODO FIX this return (style()->sizeFromContents(QStyle::CT_LineEdit, &option, size.expandedTo(QApplication::globalStrut()), this)); */
    return size;
}
