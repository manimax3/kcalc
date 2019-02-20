#include "kcalcdisplay2.h"

#include <QApplication>
#include <QStyle>
#include <QStyleOption>
#include <QStack>
#include <QPainter>

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

void KCalcDisplay2::paintEvent(QPaintEvent *e) {

    QLineEdit::paintEvent(e);
	QPainter painter(this);

	QStyleOptionFrame option;
	initStyleOption(&option);

	// draw the status texts using half of the normal
	// font size but not smaller than 7pt
	QFont fnt(font());
	fnt.setPointSize(qMax((fnt.pointSize() / 2), 7));
	painter.setFont(fnt);
	
	QFontMetrics fm(fnt);
	const uint w = fm.width(QStringLiteral("________"));
	const uint h = fm.height();

	for (int n = 0; n < 4; ++n) {
		painter.drawText(5 + n * w, h, str_status_[n]);
	}
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

void KCalcDisplay2::setStatusText(int i, const QString &text) {

    if (i < 4) {
        str_status_[i] = text;
	}
	
    update();
}
