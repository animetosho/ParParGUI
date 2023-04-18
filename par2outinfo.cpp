#include "par2outinfo.h"
#include <QString>
#include "settings.h"
#include "mainwindow.h"
#include "clientinfo.h"
#include <cmath>

static int strLen(const QString& s)
{
    const auto charset = Settings::getInstance().charset();
    if(charset == "latin1" || charset == "ascii")
        return s.toLatin1().length();
    return s.toUtf8().length();
}
static bool hasUnicode(const QString& s)
{
    for(const QChar& c : qAsConst(s)) {
        if(c.unicode() > 127) return true;
    }
    return false;
}
static int unicodeLen(const QString& s)
{
    auto opt = Settings::getInstance().unicode();
    if(opt == EXCLUDE) return 0;
    bool include = true;
    if(opt == AUTO)
        include = hasUnicode(s);
    if(!include) return 0;
    return s.length() * 2;
}

int Par2OutInfo::pktSizeFileDesc(const QString& filename)
{
    int uniLen = unicodeLen(filename);
    int nameLen = strLen(filename);
    nameLen = (nameLen + 3) & ~3; // multiple of 4 padding

    int pktLen = 64 + 56 + nameLen;
    if(uniLen) {
        uniLen = (uniLen + 3) & ~3;
        pktLen += 64 + 16 + uniLen;
    }
    return pktLen;
}

int Par2OutInfo::pktSizeComment() const
{
    int uniLen = unicodeLen(win->par2Comment);
    int len = strLen(win->par2Comment);
    len = (len + 3) & ~3; // multiple of 4 padding

    int pktLen = 64 + len;
    if(uniLen) {
        uniLen = (uniLen + 3) & ~3;
        pktLen += 64 + 16 + uniLen;
    }
    return pktLen;
}

int Par2OutInfo::pktSizeCreator()
{
    int len = ClientInfo::creator().toUtf8().length();
    len = (len + 3) & ~3; // multiple of 4 padding
    return 64 + len;
}

void Par2OutInfo::updateCritPacketSizes()
{
    critPacketSizes.clear();
    int numFiles = win->par2SrcFiles.size();
    critPacketSizes.reserve(numFiles * 2 + 3);
    for(const auto& file : qAsConst(win->par2SrcFiles)) {
        // add fileDesc
        critPacketSizes.append(pktSizeFileDesc(file.par2name));

        // add ifsc
        int slices = (file.size() + win->par2SliceSize-1) / win->par2SliceSize;
        if(slices > 0)
            critPacketSizes.append(64 + 16 + 20*slices);
    }

    // add comment packet
    if(!win->par2Comment.isEmpty()) {
        critPacketSizes.append(pktSizeComment());
    }

    // add main packet
    critPacketSizes.append(64 + 12 + numFiles*16);

    // creator packet not included as it's not a critical packet

    totalCritSize = 0;
    for(const auto& size : critPacketSizes)
        totalCritSize += size;
}

QList<Par2RecoveryFile> Par2OutInfo::getOutputList(int sliceCount, int distMode, int sliceLimit, int sliceOffset)
{
    updateCritPacketSizes();
    QList<Par2RecoveryFile> ret;
    if(distMode == 0) {
        // single file
        ret.append(makeRecFile(sliceCount, sliceOffset));
        return ret;
    }

    ret.append(makeRecFile(0, 0)); // index file

    if(distMode == 1) { // uniform
        int numFiles = sliceLimit;
        int slicePos = 0;
        ret.reserve(1 + numFiles);
        while(numFiles--) {
            int slicesThisFile = (sliceCount-slicePos + numFiles) / (numFiles+1);
            //if(slicesThisFile > sliceLimit) slicesThisFile = sliceLimit;
            ret.append(makeRecFile(slicesThisFile, slicePos+sliceOffset));
            slicePos += slicesThisFile;
        }
        return ret;
    }
    // pow2
    if(sliceCount > 0) {
        ret.reserve(1 + 16 + sliceCount/sliceLimit); // rough over-estimate
        int slicePos = 0, slices = 1;
        int totalCount = sliceCount + sliceOffset;
        while(slicePos < totalCount) {
            if(slices > sliceLimit) slices = sliceLimit;

            int fSlices = slices;
            if(fSlices+slicePos > totalCount) fSlices = totalCount-slicePos;

            if(slicePos+fSlices > sliceOffset) {
                if(sliceOffset > slicePos)
                    ret.append(makeRecFile(fSlices - (sliceOffset-slicePos), sliceOffset));
                else
                    ret.append(makeRecFile(fSlices, slicePos));
            }

            slicePos += slices;
            slices *= 2;
        }
    }
    return ret;
}


// ported from par2gen.js/_rfPush
Par2RecoveryFile Par2OutInfo::makeRecFile(int sliceCount, int sliceOffset)
{
    int critTotalSize = totalCritSize;
    quint64 recvSize = 68 + win->par2SliceSize;

    if(1 /*pow2 repetition*/ && sliceCount) {
        int critCopies = static_cast<int>(round(log(static_cast<double>(sliceCount)) / log(2.0)));
        if(critCopies < 1) critCopies = 1;
        critTotalSize *= critCopies;
    }

    return {
        sliceOffset, sliceCount, critTotalSize + pktSizeCreator() + sliceCount*recvSize
    };
}

QString Par2OutInfo::fileExt(int numSlices, int sliceOffset, int totalSlices)
{
    if(numSlices == 0) return ".par2";
    int digits = QString::number(totalSlices).length();
    if(digits < 2) digits = 2;

    if(Settings::getInstance().stdNaming()) {
        return QString(".vol%1-%2.par2")
                .arg(sliceOffset, digits, 10, QChar('0'))
                .arg(sliceOffset + numSlices, digits, 10, QChar('0'));
    } else {
        return QString(".vol%1+%2.par2")
                .arg(sliceOffset, digits, 10, QChar('0'))
                .arg(numSlices, digits, 10, QChar('0'));
    }
}

QString Par2OutInfo::nameSafeLen(QString name)
{
    // allocate enough space in the name for PAR2 extension (and our uniquifier)
    // assume a max of 255 bytes allowed for a filename, so if the byte length exceeds 235, we'll need to shorten the name
    auto name8 = (name + " - 2.vol12345+12345.par2").toLocal8Bit();
    // the algorithm to shorten the string is somewhat dumb, but since we can't be too far above the max length, this shouldn't iterate too many times
    while(name8.length() > 255) {
        name.chop(1);
        name8 = (name + " - 2.vol12345+12345.par2").toLocal8Bit();
    }
    return name;

}
