#ifndef CLIENTINFO_H
#define CLIENTINFO_H

#include <QProcess>
#include <QTimer>

class ClientInfo : public QObject
{
    Q_OBJECT

    ClientInfo(QObject* parent = nullptr);
    QProcess parpar;
    QTimer timer;
    bool isRunning;
    QString _version;
    QString _creator;

private slots:
    void finished(int exitCode, QProcess::ExitStatus exitStatus);

signals:
    void updated();
    void failed();

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
