#include "progressdialog.h"
#include <QProgressBar>

ProgressDialog::ProgressDialog(QWidget* parent, const QString& labelText)
    : QProgressDialog(parent)
{
    setLabelText(labelText);
    setMinimumDuration(1000);
    setWindowModality(Qt::WindowModal);

    auto* pb = new QProgressBar(this);
    pb->setAlignment(Qt::AlignCenter);
    setBar(pb);

    progressMask = 0xff;
    incValue = 0;
}
