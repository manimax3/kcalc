#include "kcalcdisplay2.h"

#include <QApplication>
#include <QStyle>
#include <QStyleOption>

#include <knotification.h>
#include <KLocalizedString>

#include "kcalc_settings.h"

KCalcDisplay2::KCalcDisplay2(QWidget *parent)
    : QLineEdit(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    setBackgroundRole(QPalette::Base);
    setForegroundRole(QPalette::Text);
    setAlignment(Qt::Alignment(Qt::AlignRight | Qt::AlignVCenter));

    // TODO: Frame style sunken

    sendEvent(EventReset);
}

bool KCalcDisplay2::sendEvent(Event event)
{
    Q_UNUSED(event);
    return true;
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

    // Other settings
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
