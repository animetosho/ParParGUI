#ifndef SIZEEDIT_H
#define SIZEEDIT_H

#include <QLineEdit>

class SizeEdit : public QLineEdit
{
    Q_OBJECT
public:
    SizeEdit(QWidget *parent = nullptr);

    quint64 getBytes() const;
    void setBytesApprox(quint64 bytes, bool noTrigger = false);
    void setBytes(quint64 bytes, bool noTrigger = false);
    QString getSizeString() const;

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setUnit(QChar unit);
    bool updateActive;

private slots:
    void onEditingFinished();
    void onTextEdited(const QString& text);
signals:
    void valueChanged(quint64 size, bool finished);
    void updateFinished();
};

#endif // SIZEEDIT_H
