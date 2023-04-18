#ifndef PARPARCLIENT_H
#define PARPARCLIENT_H

#include <QProcess>
#include <QTimer>

class ParParClient : public QObject
{
    Q_OBJECT

    QProcess parpar;
    QTimer timer;
    bool isRunning;
    QString stdoutBuffer;
    bool isCancelled;

public:
    explicit ParParClient(QObject *parent = nullptr);

private slots:
    void finished(int exitCode, QProcess::ExitStatus exitStatus);

signals:
    void output(const QJsonObject& m);
    void failed(const QString& error);

public:
    void run(const QStringList& args, int timeout = 10000);
    void kill();
};

#endif // PARPARCLIENT_H
