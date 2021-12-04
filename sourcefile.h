#ifndef SOURCEFILE_H
#define SOURCEFILE_H
#include <QHash>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>

class SourceFile
{
    quint64 _size;
    QString _fileName;
    QString _canonicalPath;
    QDateTime _lastModified;
    bool _exists;

    void load(const QFileInfo& info);
public:
    inline SourceFile(const QFileInfo& info) {
        load(info);
    }
    SourceFile() {
        _exists = false;
        _size = 0;
    }
    inline quint64 size() const {
        return _size;
    }
    inline QString fileName() const {
        return _fileName;
    }
    inline QString completeBaseName() const {
        int p = _fileName.lastIndexOf(QChar('.'));
        if(p == -1) return _fileName;
        return _fileName.left(p);
    }
    inline QString canonicalPath() const {
        return _canonicalPath;
    }
    inline QString canonicalFilePath() const {
        return QDir(_canonicalPath).absoluteFilePath(_fileName);
    }
    inline QDateTime lastModified() const {
        return _lastModified;
    }
    bool refresh();
    inline bool exists() const {
        return _exists;
    }

    QString par2name;
};

typedef QHash<QString, SourceFile> SrcFileList;

#endif // SOURCEFILE_H
