#include "sizeedit.h"
#include <QRegularExpressionValidator>
#include <QLocale>
#include "util.h"
#include <QKeyEvent>
#include <cmath>

SizeEdit::SizeEdit(QWidget *parent) : QLineEdit(parent)
{
    QString dec = QRegularExpression::escape(QLocale().decimalPoint());
    // validator is lax to allow users maximum flexibility when editing; fix it up on editingFinished
    setValidator(new QRegularExpressionValidator(QRegularExpression(QString("^\\d*(") + dec + "\\d*)?[BKMGTPE]?$"), this));

    connect(this, &SizeEdit::editingFinished, this, &SizeEdit::onEditingFinished);
    updateActive = false;
    connect(this, &SizeEdit::textEdited, this, &SizeEdit::onTextEdited);
}

void SizeEdit::setUnit(QChar unit)
{
    auto sVal = text();
    if(sVal.isEmpty() || sVal == QLocale().decimalPoint()) sVal = "1";
    // strip unit from text if present
    auto cUnit = sVal.at(sVal.size()-1);
    if(cUnit < '0' || cUnit > '9')
        sVal = sVal.left(sVal.length()-1);

    double val = sVal.toDouble();
    if(unit == 'B') {
        val = floor(val);
    }
    auto newText = QString::number(val) + unit.toUpper();
    setText(newText);
    setSelection(newText.length()-1, 1);

    emit textEdited(newText);
}

void SizeEdit::keyPressEvent(QKeyEvent* event)
{
    switch(event->key()) {
    case Qt::Key_B:
        setUnit('B');
        return;
    case Qt::Key_K:
        setUnit('K');
        return;
    case Qt::Key_M:
        setUnit('M');
        return;
    case Qt::Key_G:
        setUnit('G');
        return;
    case Qt::Key_T:
        setUnit('T');
        return;
    case Qt::Key_P:
        setUnit('P');
        return;
    case Qt::Key_E:
        setUnit('E');
        return;
    default:
        QLineEdit::keyPressEvent(event);
    }
}

quint64 SizeEdit::getBytes() const
{
    return sizeToBytes(text());
}

QString SizeEdit::getSizeString() const
{
    auto ret = text();
    QLocale l;
    ret.replace(l.groupSeparator(), "").replace(l.decimalPoint(), ".");

    // append B for pure sizes (needed for ParPar to know it's a size)
    if(ret.isEmpty()) return ret;

    auto cUnit = ret.at(ret.size()-1).toUpper();
    if(cUnit >= 'A' && cUnit <= 'Z')
        return ret;
    return ret + "B";
}

void SizeEdit::setBytesApprox(quint64 bytes, bool noTrigger)
{
    QString s = friendlySize(bytes);
    s.replace(QLocale().groupSeparator(), "");
    if(noTrigger) blockSignals(true);
    setText(s);
    if(noTrigger) blockSignals(false);
}

void SizeEdit::setBytes(quint64 bytes, bool noTrigger)
{
    // modification to friendlySize: only pick a larger unit if it can be represented exactly
    QStringList units{"B", "K", "M", "G", "T", "P", "E"};
    quint64 v = bytes;
    QString s;
    int ui = 0;
    for(; ui < units.size(); ui++) {
        if(v < 10000) break;
        if((v % 1024) != 0) {
            // if fractional part can be represented exactly, do the divide and stop
            if((v*100) % 1024 == 0) {
                v = (v*100) / 1024;
                s = QLocale().toString(static_cast<double>(v) / 100, 'f', 2) + units[ui+1];
            }
            break;
        }

        v /= 1024;
    }
    if(s.isEmpty())
        s = QLocale().toString(v) + units[ui];
    if(noTrigger) blockSignals(true);
    setText(s.replace(QLocale().groupSeparator(), ""));
    if(noTrigger) blockSignals(false);
}

void SizeEdit::onTextEdited(const QString& text)
{
    updateActive = true;
    emit valueChanged(getBytes(), false);
}
void SizeEdit::onEditingFinished()
{
    if(updateActive) {
        updateActive = false;

        // fix text
        auto sVal = text();
        if(sVal.isEmpty() || sVal == QLocale().decimalPoint()) sVal = "1";
        // strip unit from text if present
        auto cUnit = sVal.at(sVal.size()-1);
        if(cUnit < '0' || cUnit > '9') {
            sVal = sVal.left(sVal.length()-1);
            if(cUnit == '.') cUnit = QChar(0);
            if(sVal.isEmpty() || sVal == QLocale().decimalPoint()) sVal = "1";
        } else
            cUnit = QChar(0);

        double val = sVal.toDouble();
        if(cUnit == 'B' || cUnit == 0) {
            val = floor(val);
        }
        auto newText = QString::number(val);
        if(cUnit != 0) newText += cUnit.toUpper();
        blockSignals(true);
        setText(newText);
        blockSignals(false);
        emit updateFinished();
        emit valueChanged(getBytes(), true);
    }
}
