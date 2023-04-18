#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>

enum /*class*/ SettingsUnicode {
    AUTO, EXCLUDE, INCLUDE
};
enum SettingsDefaultAllocIn {
    ALLOC_IN_SIZE, ALLOC_IN_COUNT, ALLOC_IN_RATIO
};
enum SettingsDefaultAllocRec {
    ALLOC_REC_RATIO, ALLOC_REC_COUNT, ALLOC_REC_SIZE
};


struct OpenclDevice {
    QString name;
    float alloc;
    QString memLimit;
    QString minChunk;
    QString gfMethod;
    int batch;
    unsigned iters;
    int outputs;

    OpenclDevice() {
        alloc = 0.0;
        memLimit = "";
        minChunk = "32K";
        gfMethod = "lookup";
        batch = 0;
        iters = 0;
        outputs = 0;
    }
};

class Settings
{
private:
    QSettings settings;
    Settings() : settings("ParParGUI.ini", QSettings::IniFormat) {}
public:
    static Settings& getInstance()
    {
        static Settings* instance = new Settings();
        return *instance;
    }
    void reset()
    {
        settings.clear();
    }

    SettingsUnicode unicode() const {
        auto value = settings.value("PAR2/Unicode", "auto").toString().toLower();
        if(value == "true") return SettingsUnicode::INCLUDE;
        if(value == "false") return SettingsUnicode::EXCLUDE;
        return SettingsUnicode::AUTO;
    }
    void setUnicode(const SettingsUnicode value) {
        if(value == SettingsUnicode::INCLUDE)
            settings.setValue("PAR2/Unicode", "true");
        else if(value == SettingsUnicode::EXCLUDE)
            settings.setValue("PAR2/Unicode", "false");
        else
            settings.setValue("PAR2/Unicode", "auto");
    }

    QString charset() const {
        return settings.value("PAR2/LocalCharset", "utf8").toString();
    }
    void setCharset(const QString& value) {
        settings.setValue("PAR2/LocalCharset", value);
    }

    int packetRepMin() const {
        return settings.value("PAR2/PacketRepetitionMin", 1).toInt();
    }
    void setPacketRepMin(int value) {
        settings.setValue("PAR2/PacketRepetitionMin", value);
    }

    int packetRepMax() const {
        return settings.value("PAR2/PacketRepetitionMax", 16).toInt();
    }
    void setPacketRepMax(int value) {
        settings.setValue("PAR2/PacketRepetitionMax", value);
    }

    bool stdNaming() const {
        return settings.value("PAR2/StdNaming", false).toBool();
    }
    void setStdNaming(const bool value) {
        settings.setValue("PAR2/StdNaming", value);
    }

    QString readSize() const {
        return settings.value("Tuning/ReadSize", "4M").toString();
    }
    void setReadSize(const QString& value) {
        settings.setValue("Tuning/ReadSize", value);
    }

    int readBuffers() const {
        return settings.value("Tuning/ReadBuffers", 8).toInt();
    }
    void setReadBuffers(const int value) {
        settings.setValue("Tuning/ReadBuffers", value);
    }

    int hashQueue() const {
        return settings.value("Tuning/HashQueue", 5).toInt();
    }
    void setHashQueue(const int value) {
        settings.setValue("Tuning/HashQueue", value);
    }

    QString minChunk() const {
        return settings.value("Tuning/MinChunk", "128K").toString();
    }
    void setMinChunk(const QString& value) {
        settings.setValue("Tuning/MinChunk", value);
    }

    int chunkReadThreads() const {
        return settings.value("Tuning/ChunkReadThreads", 2).toInt();
    }
    void setChunkReadThreads(const int value) {
        settings.setValue("Tuning/ChunkReadThreads", value);
    }

    int recBuffers() const {
        return settings.value("Tuning/RecBuffers", 12).toInt();
    }
    void setRecBuffers(const int value) {
        settings.setValue("Tuning/RecBuffers", value);
    }

    int hashBatch() const {
        return settings.value("Tuning/HashBatch", 8).toInt();
    }
    void setHashBatch(const int value) {
        settings.setValue("Tuning/HashBatch", value);
    }

    int procBatch() const {
        const auto val = settings.value("Tuning/ProcBatch", "auto");
        if(val.toString().toLower() == "auto") return -1;
        return val.toInt();
    }
    void setProcBatch(const int value) {
        if(value < 0)
            settings.setValue("Tuning/ProcBatch", "auto");
        else
            settings.setValue("Tuning/ProcBatch", value);
    }

    QString memLimit() const {
        const auto val = settings.value("Tuning/MemLimit", "auto").toString();
        if(val.toLower() == "auto") return "";
        return val;
    }
    void setMemLimit(const QString& value) {
        if(value.isEmpty())
            settings.setValue("Tuning/MemLimit", "auto");
        else
            settings.setValue("Tuning/MemLimit", value);
    }

    QString gfMethod() const {
        const auto val = settings.value("Tuning/GFMethod", "auto").toString();
        if(val.toLower() == "auto") return "";
        return val;
    }
    void setGfMethod(const QString& value) {
        if(value.isEmpty())
            settings.setValue("Tuning/GFMethod", "auto");
        else
            settings.setValue("Tuning/GFMethod", value);
    }

    QString tileSize() const {
        const auto val = settings.value("Tuning/LoopTileSize", "auto").toString();
        if(val.toLower() == "auto") return "";
        return val;
    }
    void setTileSize(const QString& value) {
        if(value.isEmpty())
            settings.setValue("Tuning/LoopTileSize", "auto");
        else
            settings.setValue("Tuning/LoopTileSize", value);
    }

    QString hashMethod() const {
        const auto val = settings.value("Tuning/HashMethod", "auto").toString();
        if(val.isEmpty()) return "auto";
        return val.toLower();
    }
    void setHashMethod(const QString& value) {
        if(value.isEmpty())
            settings.setValue("Tuning/HashMethod", "auto");
        else
            settings.setValue("Tuning/HashMethod", value);
    }

    QString md5Method() const {
        const auto val = settings.value("Tuning/MD5Method", "auto").toString();
        if(val.isEmpty()) return "auto";
        return val.toLower();
    }
    void setMd5Method(const QString& value) {
        if(value.isEmpty())
            settings.setValue("Tuning/MD5Method", "auto");
        else
            settings.setValue("Tuning/MD5Method", value);
    }

    bool outputSync() const {
        return settings.value("Tuning/OutputSync", false).toBool();
    }
    void setOutputSync(bool value) {
        settings.setValue("Tuning/OutputSync", value);
    }


    int threadNum() const {
        const auto val = settings.value("Tuning/Threads", "auto");
        if(val.toString().toLower() == "auto") return -1;
        return val.toInt();
    }
    void setThreadNum(const int value) {
        if(value < 0)
            settings.setValue("Tuning/Threads", "auto");
        else
            settings.setValue("Tuning/Threads", value);
    }

    QString cpuMinChunk() const {
        return settings.value("Tuning/CpuMinChunk", "128K").toString();
    }
    void setCpuMinChunk(const QString& value) {
        settings.setValue("Tuning/CpuMinChunk", value);
    }

    QList<OpenclDevice> openclDevices() const {
        QList<OpenclDevice> devices;
        int i=0;
        while(1) {
            const auto skey = QString("OpenCL/%1_") + QString::number(i++);
            const auto& devKey = skey.arg("Device");
            if(!settings.contains(devKey)) break;

            OpenclDevice dev;
            dev.name = settings.value(devKey).toString();
            dev.alloc = settings.value(skey.arg("AllocationPercent"), dev.alloc).toFloat();
            if(dev.alloc <= 0.0) continue; // invalid allocation
            dev.memLimit = settings.value(skey.arg("MemLimit"), dev.memLimit).toString();
            dev.minChunk = settings.value(skey.arg("MinChunk"), dev.minChunk).toString();
            dev.gfMethod = settings.value(skey.arg("GFMethod"), dev.gfMethod).toString();
            dev.batch = settings.value(skey.arg("ProcBatch"), dev.batch).toInt();
            dev.iters = settings.value(skey.arg("Iterations"), dev.iters).toUInt();
            dev.outputs = settings.value(skey.arg("Outputs"), dev.outputs).toInt();
            devices.append(dev);
        }
        return devices;
    }
    void setOpenclDevices(const QList<OpenclDevice>& value) {
        settings.remove("OpenCL");
        settings.beginGroup("OpenCL");
        for(int i=0; i<value.size(); i++) {
            const auto& dev = value.at(i);
            const auto is = QString::number(i);
            settings.setValue(QString("Device_%1").arg(is), dev.name);
            settings.setValue(QString("AllocationPercent_%1").arg(is), dev.alloc);
            settings.setValue(QString("MemLimit_%1").arg(is), dev.memLimit);
            settings.setValue(QString("MinChunk_%1").arg(is), dev.minChunk);
            settings.setValue(QString("GFMethod_%1").arg(is), dev.gfMethod);
            settings.setValue(QString("ProcBatch_%1").arg(is), dev.batch);
            settings.setValue(QString("Iterations_%1").arg(is), dev.iters);
            settings.setValue(QString("Outputs_%1").arg(is), dev.outputs);
        }
        settings.endGroup();
    }

    QStringList parparBin(bool* isSystemExecutable = nullptr) const;
    void setParparBin(const QString &parpar, const QString &node = QString());

    bool runBackground() const {
        return settings.value("Running/Background", false).toBool();
    }
    void setRunBackground(const bool value) {
        settings.setValue("Running/Background", value);
    }

    bool runNotification() const {
        return settings.value("Running/Notification", false).toBool();
    }
    void setRunNotification(const bool value) {
        settings.setValue("Running/Notification", value);
    }

    bool runClose() const {
        return settings.value("Running/AutoClose", false).toBool();
    }
    void setRunClose(const bool value) {
        settings.setValue("Running/AutoClose", value);
    }

    bool uiExpSource() const {
        return settings.value("UI/ExpandSource", false).toBool();
    }
    void setUiExpSource(const bool value) {
        settings.setValue("UI/ExpandSource", value);
    }

    bool uiExpDest() const {
        return settings.value("UI/ExpandDest", false).toBool();
    }
    void setUiExpDest(const bool value) {
        settings.setValue("UI/ExpandDest", value);
    }

    SettingsDefaultAllocIn allocSliceMode() const {
        if(settings.contains("UI/AllocSliceCount"))
            return SettingsDefaultAllocIn::ALLOC_IN_COUNT;
        if(settings.contains("UI/AllocSliceRatio"))
            return SettingsDefaultAllocIn::ALLOC_IN_RATIO;
        return SettingsDefaultAllocIn::ALLOC_IN_SIZE;
    }

    QString allocSliceSize() const {
        return settings.value("UI/AllocSliceSize", "1M").toString();
    }
    void setAllocSliceSize(const QString& value) {
        settings.remove("UI/AllocSliceCount");
        settings.remove("UI/AllocSliceRatio");
        settings.setValue("UI/AllocSliceSize", value);
    }

    int allocSliceCount() const {
        return settings.value("UI/AllocSliceCount", 1000).toInt();
    }
    void setAllocSliceCount(int value) {
        settings.remove("UI/AllocSliceSize");
        settings.remove("UI/AllocSliceRatio");
        settings.setValue("UI/AllocSliceCount", value);
    }

    float allocSliceRatio() const {
        return settings.value("UI/AllocSliceRatio", 1.0).toFloat();
    }
    void setAllocSliceRatio(float value) {
        settings.remove("UI/AllocSliceCount");
        settings.remove("UI/AllocSliceSize");
        settings.setValue("UI/AllocSliceRatio", value);
    }

    SettingsDefaultAllocRec allocRecoveryMode() const {
        if(settings.contains("UI/AllocRecoveryCount"))
            return SettingsDefaultAllocRec::ALLOC_REC_COUNT;
        if(settings.contains("UI/AllocRecoverySize"))
            return SettingsDefaultAllocRec::ALLOC_REC_SIZE;
        return SettingsDefaultAllocRec::ALLOC_REC_RATIO;
    }

    float allocRecoveryRatio() const {
        return settings.value("UI/AllocRecoveryRatio", 10.0).toFloat();
    }
    void setAllocRecoveryRatio(float value) {
        settings.remove("UI/AllocRecoveryCount");
        settings.remove("UI/AllocRecoverySize");
        settings.setValue("UI/AllocRecoveryRatio", value);
    }

    int allocRecoveryCount() const {
        return settings.value("UI/AllocRecoveryCount", 100).toInt();
    }
    void setAllocRecoveryCount(int value) {
        settings.remove("UI/AllocRecoveryRatio");
        settings.remove("UI/AllocRecoverySize");
        settings.setValue("UI/AllocRecoveryCount", value);
    }

    QString allocRecoverySize() const {
        return settings.value("UI/AllocRecoverySize", "100M").toString();
    }
    void setAllocRecoverySize(const QString& value) {
        settings.remove("UI/AllocRecoveryRatio");
        settings.remove("UI/AllocRecoveryCount");
        settings.setValue("UI/AllocRecoverySize", value);
    }

    QString sliceMultiple() const {
        return settings.value("UI/SizeMultiple", "").toString();
    }
    void setSliceMultple(const QString& value) {
        settings.setValue("UI/SizeMultiple", value);
    }

    int sliceLimit() const {
        return settings.value("UI/SliceLimit", 32768).toInt();
    }
    void setSliceLimit(int value) {
        settings.setValue("UI/SliceLimit", value);
    }
};

#endif // SETTINGS_H
