#include "clientinfo.h"
#include "settings.h"
#include <QJsonDocument>
#include <QJsonObject>

ClientInfo::ClientInfo(QObject* parent) : QObject(parent)
{
    connect(&parpar, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &ClientInfo::finished);
    // treat failing to start, like a crash
    connect(&parpar, &QProcess::errorOccurred, this, [this](QProcess::ProcessError err) {
        if(err == QProcess::FailedToStart) {
            this->finished(0, QProcess::ExitStatus::CrashExit);
        }
        // let other handler handle other errors
    });
    isRunning = false;
    timer.setInterval(10000);
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, this, [this]() {
        this->parpar.kill();
    });

    // default values
    _version = "0.0.0";
    _creator = "ParPar v0.0.0 [https://animetosho.org/app/parpar]";
}

void ClientInfo::refresh()
{
    // create process
    auto cmd = Settings::getInstance().parparBin();
    QStringList args{"--client-info"};
    if(cmd.length() > 1)
        args.prepend(cmd[1]);
    isRunning = true;
    parpar.start(cmd[0], args);
    timer.start();
}

void ClientInfo::finished(int exitCode, QProcess::ExitStatus exitStatus)
{
    timer.stop();
    isRunning = false;
    if(exitStatus != QProcess::ExitStatus::NormalExit || exitCode != 0) {
        emit failed(); // TODO: pass back diagnostic info to help see what's wrong
    } else {
        const auto data = QJsonDocument::fromJson(parpar.readAllStandardOutput());

        if(data.isNull() || !data.isObject())
            emit failed();
        else {
            const auto doc = data.object();
            _version = doc.value("version").toString();
            _creator = doc.value("creator").toString();

            emit updated();
        }
    }
}

