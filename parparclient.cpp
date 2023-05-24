#include "parparclient.h"
#include "settings.h"
#include <QJsonDocument>
#include <QJsonObject>

ParParClient::ParParClient(QObject *parent)
    : QObject{parent}
{
    connect(&parpar, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &ParParClient::finished);
    // treat failing to start, like a crash
    connect(&parpar, &QProcess::errorOccurred, this, [this](QProcess::ProcessError err) {
        if(err == QProcess::FailedToStart) {
            this->finished(0, QProcess::ExitStatus::CrashExit);
        }
        // let other handler handle other errors
    });
    isRunning = false;
    isCancelled = false;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, this, [this]() {
        this->parpar.kill();
    });
}

void ParParClient::run(const QStringList& _args, int timeout)
{
    auto cmd = Settings::getInstance().parparBin();
    QStringList args{"--json"};
    args.append(_args);
    if(cmd.length() > 1)
        args.prepend(cmd[1]);
    isRunning = true;
    isCancelled = false;
    stdoutBuffer.clear();
    if(timeout)
    {
        timer.setInterval(timeout);
        timer.start();
    }
    parpar.start(cmd[0], args);
}

void ParParClient::finished(int exitCode, QProcess::ExitStatus exitStatus)
{
    timer.stop();
    isRunning = false;

    if(isCancelled) {
        emit failed(QString());
    } else if(exitStatus != QProcess::ExitStatus::NormalExit) {
        emit failed(tr("ParPar process crashed or failed to start"));
    } else if(exitCode != 0) {
        emit failed(tr("ParPar failure (exit code: %1)").arg(exitCode));
    } else {
        const auto data = QJsonDocument::fromJson(parpar.readAllStandardOutput());

        if(data.isNull() || !data.isObject()) {
            auto message = QString::fromUtf8(parpar.readAllStandardError());
            if(message.isEmpty()) message = "No output received";
            emit failed(message);
        } else
            emit output(data.object());
    }
}

void ParParClient::kill()
{
    isCancelled = true;
    parpar.kill();
}
