#ifndef PAR2CALC_H
#define PAR2CALC_H

#include <QHash>
#include <QString>
#include <QFileInfo>
#include "sourcefile.h"

class Par2Calc
{
public:
    static quint64 sliceSizeFromCount(int& count, quint64 multiple, int limit, const SrcFileList& files, int fileCount, int moveDir = 0);
    static int sliceCountFromSize(quint64& size, quint64 multiple, int limit, const SrcFileList& files, int fileCount);
    static int maxSliceCount(quint64 multiple, int limit, const SrcFileList& files);

    static int round_down_pow2(int v);
};

#endif // PAR2CALC_H
