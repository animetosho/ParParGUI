#ifndef PAR2OUTINFO_H
#define PAR2OUTINFO_H

class MainWindow;
#include <QtGlobal>
#include <QVector>

class Par2RecoveryFile {
public:
    int offset;
    int count;
    quint64 size;
};

class Par2OutInfo
{
private:
    MainWindow* win;

public:
    // ugly strong coupling, but who really cares?
    Par2OutInfo(MainWindow* mainWin) : win(mainWin) {
    }

    void updateCritPacketSizes();
    QList<Par2RecoveryFile> getOutputList(int sliceCount, int distMode, int sliceLimit, int sliceOffset);

    static QString fileExt(int numSlices, int sliceOffset, int totalSlices);

private:
    // state variables because stateless went out of fashion
    QVector<quint64> critPacketSizes;
    quint64 totalCritSize;

    static int pktSizeFileDesc(const QString& filename);
    int pktSizeComment() const;
    static int pktSizeCreator();

    Par2RecoveryFile makeRecFile(int sliceCount, int sliceOffset);
};

#endif // PAR2OUTINFO_H
