#include "clientinfo.h"
#include "settings.h"
#include <QJsonDocument>
#include <QJsonObject>

ClientInfo::ClientInfo(QObject* parent) : QObject(parent), parpar(parent)
{
    connect(&parpar, &ParParClient::failed, this, &ClientInfo::failed);
    // treat failing to start, like a crash
    connect(&parpar, &ParParClient::output, this, [this](const QJsonObject& doc) {
        _version = doc.value("version").toString();
        _creator = doc.value("creator").toString();

        emit updated();
    });

    // default values
    _version = "0.0.0";
    _creator = "ParPar v0.0.0 [https://animetosho.org/app/parpar]";
}

void ClientInfo::refresh()
{
    parpar.run({"--version"});
}

