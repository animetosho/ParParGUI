#include "slicecountspinbox.h"
#include "par2calc.h"
#include "settings.h"

SliceCountSpinBox::SliceCountSpinBox(QWidget *parent)
    : QSpinBox(parent), mainWin(nullptr)
{

}

void SliceCountSpinBox::stepBy(int steps)
{
    if(steps == 0) return;
    int target = value() + steps;
    // fix up target
    if(mainWin)
        Par2Calc::sliceSizeFromCount(target, mainWin->optionSliceMultiple, mainWin->optionSliceLimit, mainWin->par2SrcFiles, mainWin->par2FileCount, steps);

    if(target < minimum()) target = minimum();
    else if(target > maximum()) target = maximum();
    if(target != value())
        setValue(target);
}
