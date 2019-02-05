#ifndef KCALCDISPLAY2_H_
#define KCALCDISPLAY2_H_ value

#include <QLineEdit>
#include "knumber.h"

class KCalcDisplay2 : public QLineEdit
{
    Q_OBJECT

public:
    explicit KCalcDisplay2(QWidget *parent = nullptr);

    enum Event {
        EventReset, // resets display
        EventClear, // if no error reset display
        EventError,
        EventChangeSign
    };

    bool sendEvent(Event event);

    void changeSettings();
    QSize sizeHint() const override;
    void initStyleOption(QStyleOptionFrame *option) const;
};

#endif
