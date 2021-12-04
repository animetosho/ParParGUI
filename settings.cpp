#include "settings.h"
#include <QFile>
#include <QDir>

static QString pathConverter(const QString& fn)
{
    return QDir().absoluteFilePath(fn).replace("/", QDir::separator());
}
QStringList Settings::parparBin(bool* isSystemExecutable) const
{
    // check setting
    auto pathParPar = settings.value("ParPar/Bin", "").toString();
    auto pathNode = settings.value("ParPar/Node", "").toString();
    if(!pathParPar.isEmpty()) {
        if(isSystemExecutable) *isSystemExecutable = false; // well... I guess it's possibly true, but we wouldn't know
        if(!pathNode.isEmpty()) {
            if(pathNode != "node") // system executable - can't make absolute
                pathNode = pathConverter(pathNode);
            return {pathConverter(pathNode), pathConverter(pathParPar)};
        } else {
            if(pathParPar != "parpar")
                pathParPar = pathConverter(pathParPar);
            return {pathParPar};
        }
    }

    // if not set, scan for it

    QString exe = "";
    QString curdir = "./";
#ifdef Q_OS_WINDOWS
    exe = ".exe";
    curdir = "";
#endif

    if(isSystemExecutable) *isSystemExecutable = false;

    // compiled binary
    if(QFile::exists(QString("parpar") + exe))
        return {pathConverter(QString("parpar") + exe)};
#ifdef Q_OS_WINDOWS
    // TODO: test if this works
    if(QFile::exists("parpar.cmd"))
        return {pathConverter("parpar.cmd")};
#endif

    if(QFile::exists("bin/parpar.js")) {
        // find included node
        if(QFile::exists(QString("bin/node") + exe))
            return {pathConverter(QString("bin/node") + exe), pathConverter("bin/parpar.js")};
        if(QFile::exists(QString("node") + exe))
            return {pathConverter(QString("node") + exe), pathConverter("bin/parpar.js")};
        // use system node
#ifdef Q_OS_WINDOWS
        if(isSystemExecutable) *isSystemExecutable = true;
        return {QString("node"), pathConverter("bin/parpar.js")};
#else
        return {pathConverter("bin/parpar.js")};
#endif
    }

    if(isSystemExecutable) *isSystemExecutable = true;
    return {"parpar"};
}

static QString relPathConverter(const QString& file)
{
    if(file.isEmpty()) return file;

    QDir cd;
    QFileInfo info(file);
    if(info.isAbsolute()) {
        QString relPath = cd.relativeFilePath(file).replace("/", QDir::separator());
#ifdef Q_OS_WINDOWS
        // on Windows, can't use relative paths if on different drives (or drive <> UNC path)
        if(cd.absolutePath().left(2).compare(file.left(2), Qt::CaseInsensitive) == 0)
#else
        // on *nix, path in current dir should have preceeding './'
        // (we don't really require it, because we're not executing over a shell, but it distinguishes a local binary vs system binary)
        if(!relPath.contains(QDir::separator()))
            return QString(".") + QDir::separator() + relPath;
#endif
        return relPath;
    }
    return file;
}
void Settings::setParparBin(const QString& parpar, const QString& node)
{
    // convert to relative paths to allow portability
    settings.setValue("ParPar/Bin", relPathConverter(parpar));
    settings.setValue("ParPar/Node", relPathConverter(node));
}
