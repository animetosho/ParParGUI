#ifndef SLICECOUNTSPINBOX_H
#define SLICECOUNTSPINBOX_H

#include <QSpinBox>
#include <QObject>
#include "mainwindow.h"

class SliceCountSpinBox : public QSpinBox
{
private:
    MainWindow* mainWin;
public:
    SliceCountSpinBox(QWidget *parent = nullptr);
    inline void setMainWindow(MainWindow* win) {
        mainWin = win;
    }

protected:
    void stepBy(int steps) override;
};

#endif // SLICECOUNTSPINBOX_H
