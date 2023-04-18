#ifndef CLIENTINFO_H
#define CLIENTINFO_H

#include "parparclient.h"

class ClientInfo : public QObject
{
    Q_OBJECT

    ClientInfo(QObject* parent = nullptr);
    ParParClient parpar;
    QString _version;
    QString _creator;

signals:
    void updated();
    void failed(const QString& error);

public:
    static ClientInfo& getInstance()
    {
        static ClientInfo instance;
        return instance;
    }

    static inline QString& version()
    {
        return getInstance()._version;
    }
    static inline QString& creator()
    {
        return getInstance()._creator;
    }

    void refresh();
};

#endif // CLIENTINFO_H
