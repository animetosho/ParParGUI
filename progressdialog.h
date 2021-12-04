#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QProgressDialog>
#include <QObject>

class ProgressDialog : public QProgressDialog
{
private:
    int incValue;
public:
    explicit ProgressDialog(QWidget* parent, const QString& labelText);

    int progressMask;
    inline void inc() {
        incValue++;
        // setValue seems to be rather slow, so defer updating it
        if((incValue & progressMask) == progressMask)
            setValue(incValue);
    }

    inline void end() {
        setValue(maximum());
    }
};

#endif // PROGRESSDIALOG_H
