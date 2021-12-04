#include "sourcefile.h"

void SourceFile::load(const QFileInfo& info)
{
    _size = info.size();
    _fileName = info.fileName();
    _canonicalPath = info.canonicalPath();
    _lastModified = info.lastModified();
    _exists = info.exists();
}

bool SourceFile::refresh()
{
    bool changed = false;
    QFileInfo info(canonicalFilePath());
    if(_size != info.size() || _lastModified != info.lastModified() || _exists != info.exists())
        changed = true;
    load(info);
    return changed;
}
