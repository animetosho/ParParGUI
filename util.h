#ifndef UTIL_H
#define UTIL_H

#include <QString>

QString friendlySize(quint64 s);
quint64 sizeToBytes(QString s);
QString escapeShellArg(QString arg);

#endif // UTIL_H
