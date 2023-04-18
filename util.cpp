#include "util.h"
#include <QStringList>
#include <QRegularExpression>
#include <QLocale>
#include <math.h>

QString friendlySize(quint64 s) {
    //const QVector<QChar> units{'B', 'K', 'M', 'G', 'T', 'P', 'E'};
    const QString units("BKMGTPE");
    double v = s;
    int ui = 0;
    for(; ui < units.size(); ui++) {
        if(v < 10000) break;
        v /= 1024;
    }
    if(ui)
        return QLocale().toString(round(v*100) / 100, 'f', 2) + units.at(ui);
    else
        // bytes - no decimal needed
        return QLocale().toString((int)v) + units.at(ui);
}

quint64 sizeToBytes(QString size) {
    if(size.isEmpty() || size == QLocale().decimalPoint()) return 0;
    // extract unit from text
    auto cUnit = size.at(size.size()-1);
    if(cUnit < '0' || cUnit > '9')
        size = size.left(size.length()-1);
    else
        cUnit = 'B';

    double val = size.toDouble();
    switch(cUnit.toUpper().toLatin1()) {
    case 'E':
        val *= 1024;
        // fallthrough
    case 'P':
        val *= 1024;
        // fallthrough
    case 'T':
        val *= 1024;
        // fallthrough
    case 'G':
        val *= 1024;
        // fallthrough
    case 'M':
        val *= 1024;
        // fallthrough
    case 'K':
        val *= 1024;
        // fallthrough
    case 'B':
        break;
    }
    return val;
}

QString escapeShellArg(QString arg) {
#ifdef Q_OS_WINDOWS
    QRegularExpression basic;
    // we'll special case our recovery % option
    if(arg.count('%') == 1) // % is only problematic if there's more than one of it
        basic.setPattern("^[a-zA-Z0-9\\-_=.,:/\\\\%]+$");
    else
        basic.setPattern("^[a-zA-Z0-9\\-_=.,:/\\\\]+$");
    if(basic.match(arg).hasMatch())
        return arg;
    // I think this is correct...
    return QString("\"%1\"").arg(arg.replace("\"", "\"\"").replace("%", "\"^%\"").replace("!", "\"^!\"").replace("\\", "\\\\").replace("\n", "\"^\n\"").replace("\r", "\"^\r\""));
#else
    QRegularExpression basic("^[a-zA-Z0-9\\-_=.,:/]+$");
    if(basic.match(arg).hasMatch())
        return arg;
    return QString("'%1'").arg(arg.replace("'", "'\\''"));
#endif
}
