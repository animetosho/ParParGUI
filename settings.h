#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>

enum /*class*/ SettingsUnicode {
    AUTO, EXCLUDE, INCLUDE
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

    QString sliceMultiple() const {
        return settings.value("UI/SizeMultiple", "").toString();
    }
    void setSliceMultple(const QString& value) {
        settings.setValue("UI/SizeMultiple", value);
    }
};

#endif // SETTINGS_H
