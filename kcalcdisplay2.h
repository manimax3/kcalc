#ifndef KCALCDISPLAY2_H_
#define KCALCDISPLAY2_H_ value

#include <QLineEdit>
#include "knumber.h"

enum NumBase {
    NB_BINARY = 2,
    NB_OCTAL = 8,
    NB_DECIMAL = 10,
    NB_HEX = 16
};

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

    void setBeep(bool flag) {};
    void setGroupDigits(bool flag) {};
    void setTwosComplement(bool flag) {};
    void setBinaryGrouping(int digits) {};
    void setOctalGrouping(int digits) {};
    void setHexadecimalGrouping(int digits) {};

    void changeSettings();
    QSize sizeHint() const override;
    void initStyleOption(QStyleOptionFrame *option) const;

    void setStatusText(int i, const QString &text);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    bool beep_ = false;
    bool groupdigits_ = true;
    int binaryGrouping_;
    int octalGrouping_;
    int hexadecimalGrouping_;
    NumBase num_base_ = NB_DECIMAL;

    QString str_status_[4];
};

#endif
