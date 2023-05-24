#include "optionsdialog.h"
#include "ui_optionsdialog.h"
#include <QSysInfo>
#include <QThread>
#include <QFileDialog>
#include "settings.h"
#include <QMessageBox>
#include <QSharedPointer>
#include "parparclient.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QProgressDialog>
#include <QProgressBar>
#include "util.h"

OptionsDialog::OptionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OptionsDialog)
{
    ui->setupUi(this);

    auto arch = QSysInfo::currentCpuArchitecture();
    if(arch == "arm" || arch == "arm64") {
        ui->cboProcKernel->addItems({"shuffle-neon", "clmul-neon"});
        ui->cboHashKernel->addItems({"simd", "crc", "simd-crc"});
        ui->cboMD5Kernel->addItems({"neon"});
    }
    if(arch == "arm64") {
        ui->cboProcKernel->addItems({"shuffle128-sve", "shuffle128-sve2", "shuffle2x128-sve2", "shuffle512-sve2", "clmul-sve2"});
        ui->cboMD5Kernel->addItems({"sve2"});
    }
    if(arch == "i386" || arch == "x86_64") {
        ui->cboProcKernel->addItems({"xor-sse", "xorjit-sse"});
        if(arch == "x86_64")
            ui->cboProcKernel->addItems({"xorjit-avx2", "xorjit-avx512"});
        ui->cboProcKernel->addItems({"shuffle-sse", "shuffle-avx", "shuffle-avx2", "shuffle-avx512", "shuffle-vbmi", "shuffle2x-avx2", "shuffle2x-avx512", "affine-sse", "affine-avx2", "affine-avx512", "affine2x-sse", "affine2x-avx2", "affine2x-avx512"});
        ui->cboHashKernel->addItems({"simd", "crc", "simd-crc", "bmi", "avx512"});
        ui->cboMD5Kernel->addItems({"sse", "avx2", "xop", "avx512f", "avx512vl"});
    }

    devicesListed = false;
    ui->tabWidget->setCurrentIndex(0);
    loadSettings();
}

OptionsDialog::~OptionsDialog()
{
    delete ui;
}

void OptionsDialog::loadSettings()
{
    auto& settings = Settings::getInstance();
    bool chk;

    bool isSystemExecutable;
    auto ppBin = settings.parparBin(&isSystemExecutable);
    if(ppBin.length() > 1) {
        ui->chkPathNode->setChecked(true);
        ui->txtPathNode->setText(ppBin[0]);
        ui->txtPathParPar->setText(ppBin[1]);
    } else if(!isSystemExecutable) {
        ui->txtPathParPar->setText(ppBin[0]);
    }
    ui->cboUnicode->setCurrentIndex(settings.unicode());
    ui->cboCharset->setCurrentText(settings.charset());
    ui->txtPacketMin->setValue(settings.packetRepMin());
    ui->txtPacketMax->setValue(settings.packetRepMax());
    ui->chkStdNaming->setChecked(settings.stdNaming());

    chk = !settings.sliceMultiple().isEmpty();
    ui->chkSliceMultiple->setChecked(chk);
    ui->txtSliceMultiple->setEnabled(chk);
    ui->txtSliceMultiple->setText(chk ? settings.sliceMultiple() : "750K");
    ui->txtSliceLimit->setValue(settings.sliceLimit());

    ui->cboAllocInMode->setCurrentIndex(settings.allocSliceMode());
    on_cboAllocInMode_currentIndexChanged(ui->cboAllocInMode->currentIndex());
    ui->txtAllocInSize->setText(settings.allocSliceSize());
    ui->txtAllocInCount->setValue(settings.allocSliceCount());
    ui->txtAllocInRatio->setValue(settings.allocSliceRatio());
    ui->cboAllocRecMode->setCurrentIndex(settings.allocRecoveryMode());
    on_cboAllocRecMode_currentIndexChanged(ui->cboAllocRecMode->currentIndex());
    ui->txtAllocRecRatio->setValue(settings.allocRecoveryRatio());
    ui->txtAllocRecCount->setValue(settings.allocRecoveryCount());
    ui->txtAllocRecSize->setText(settings.allocRecoverySize());

    ui->chkRunClose->setChecked(settings.runClose());


    ui->txtReadSize->setText(settings.readSize());
    ui->txtReadBufs->setValue(settings.readBuffers());
    ui->txtHashQueue->setMaximum(settings.readBuffers());
    ui->txtHashQueue->setValue(settings.hashQueue());
    ui->cboHashKernel->setCurrentText(settings.hashMethod());
    ui->txtMinChunk->setText(settings.minChunk());
    ui->txtChunkReadThreads->setMaximum(settings.readBuffers());
    ui->txtChunkReadThreads->setValue(settings.chunkReadThreads());
    ui->txtRecBufs->setValue(settings.recBuffers());
    ui->txtRecHashBatch->setMaximum(settings.recBuffers());
    ui->txtRecHashBatch->setValue(settings.hashBatch());
    ui->cboMD5Kernel->setCurrentText(settings.md5Method());
    ui->chkOutputSync->setChecked(settings.outputSync());

    chk = settings.procBatch() >= 0;
    ui->chkProcBatch->setChecked(chk);
    ui->txtProcBatch->setEnabled(chk);
    ui->txtProcBatch->setValue(chk ? settings.procBatch() : 12);
    chk = !settings.memLimit().isEmpty();
    ui->chkMemLimit->setChecked(chk);
    ui->txtMemLimit->setEnabled(chk);
    ui->txtMemLimit->setText(chk ? settings.memLimit() : "256M");
    chk = !settings.gfMethod().isEmpty();
    ui->chkProcKernel->setChecked(chk);
    ui->cboProcKernel->setEnabled(chk);
    if(chk) ui->cboProcKernel->setCurrentText(settings.gfMethod());
    chk = !settings.tileSize().isEmpty();
    ui->chkTileSize->setChecked(chk);
    ui->txtTileSize->setEnabled(chk);
    ui->txtTileSize->setText(chk ? settings.tileSize() : "32K");
    chk = settings.threadNum() >= 0;
    ui->chkThreads->setChecked(chk);
    ui->txtThreads->setEnabled(chk);
    ui->txtThreads->setValue(chk ? settings.threadNum() : QThread::idealThreadCount());
    ui->txtCpuMinChunk->setText(settings.cpuMinChunk());

    // TODO: load defaults for processing options (e.g. GF kernel)

    checkIOPreset();

    openclSettings.clear();
    for(const auto& dev : settings.openclDevices()) {
        QString key = dev.name.toLower();
        if(openclSettings.contains(key)) {
            int idx = 1;
            QString idxStr("::%1");
            while(openclSettings.contains(key + idxStr.arg(idx)))
                idx++;
            key += idxStr.arg(idx);
        }
        openclSettings.insert(key, dev);
    }

    ui->chkOpencl->blockSignals(ui->tabWidget->currentIndex() != 2);
    ui->chkOpencl->setChecked(!openclSettings.isEmpty());
    ui->chkOpencl->blockSignals(false);
}

void OptionsDialog::on_chkProcBatch_stateChanged(int arg1)
{
    ui->txtProcBatch->setEnabled(ui->chkProcBatch->isChecked());
}
void OptionsDialog::on_chkMemLimit_stateChanged(int arg1)
{
    ui->txtMemLimit->setEnabled(ui->chkMemLimit->isChecked());
}
void OptionsDialog::on_chkProcKernel_stateChanged(int arg1)
{
    ui->cboProcKernel->setEnabled(ui->chkProcKernel->isChecked());
}
void OptionsDialog::on_chkThreads_stateChanged(int arg1)
{
    ui->txtThreads->setEnabled(ui->chkThreads->isChecked());
}

void OptionsDialog::accept()
{
    int packetRepMin = ui->txtPacketMin->value();
    if(packetRepMin < 16 && packetRepMin > ui->txtPacketMax->value()) {
        QMessageBox::warning(this, tr("Update Options"), tr("The packet repetition minimum value cannot exceed the maximum value"));
        return;
    }

    quint64 defSliceAllocSize = 0;
    if(ui->cboAllocInMode->currentIndex() == SettingsDefaultAllocIn::ALLOC_IN_SIZE) {
        defSliceAllocSize = ui->txtAllocInSize->getBytes();
        if(defSliceAllocSize == 0 || (defSliceAllocSize & 3)) {
            QMessageBox::warning(this, tr("Update Options"), tr("The slice size must be a multiple of 4 bytes"));
            return;
        }

        if(ui->cboAllocRecMode->currentIndex() == SettingsDefaultAllocRec::ALLOC_REC_SIZE) {
            if(ui->txtAllocRecSize->getBytes() % defSliceAllocSize) {
                QMessageBox::warning(this, tr("Update Options"), tr("The amount of recovery data should be a multiple of the slice size"));
                return;
            }
        }
    }
    if(ui->cboAllocInMode->currentIndex() == SettingsDefaultAllocIn::ALLOC_IN_COUNT) {
        if(ui->txtAllocInCount->value() > ui->txtSliceLimit->value()) {
            QMessageBox::warning(this, tr("Update Options"), tr("The number of source slices cannot exceed the slice limt"));
            return;
        }
    }

    if(ui->cboAllocInMode->currentIndex() == SettingsDefaultAllocIn::ALLOC_IN_RATIO) {
        if(ui->txtAllocInRatio->value() <= 0.0) {
            QMessageBox::warning(this, tr("Update Options"), tr("The slice count÷size ratio must be greater than 0"));
            return;
        }
    }

    bool useMultiple = ui->chkSliceMultiple->isChecked();
    if(useMultiple) {
        auto multipleSize = ui->txtSliceMultiple->getBytes();
        if(multipleSize == 0 || (multipleSize & 3)) {
            QMessageBox::warning(this, tr("Update Options"), tr("The slice size multiple must be a multiple of 4 bytes"));
            return;
        }
        if(defSliceAllocSize && defSliceAllocSize % multipleSize) {
            QMessageBox::warning(this, tr("Update Options"), tr("The selected slice size is not a multiple of the selected slice size multiple"));
            return;
        }
    }

    if(ui->txtMinChunk->getBytes() & 1) {
        QMessageBox::warning(this, tr("Update Options"), tr("The Min chunk size must be a multiple of 2 bytes"));
        return;
    }

    if(ui->txtCpuMinChunk->getBytes() & 1) {
        QMessageBox::warning(this, tr("Update Options"), tr("The CPU Min chunk size must be a multiple of 2 bytes"));
        return;
    }

    if(ui->txtChunkReadThreads->value() > ui->txtReadBufs->value()) {
        QMessageBox::warning(this, tr("Update Options"), tr("The number of chunking read threads cannot exceed the number of read buffers"));
        return;
    }

    QList<OpenclDevice> oclDevices;
    if(ui->chkOpencl->isChecked()) {
        float totalAlloc = 0;
        QHashIterator<QString, OpenclDevice> it(openclSettings);
        while(it.hasNext()) {
            it.next();
            const auto& dev = it.value();
            if(dev.alloc <= 0.0) continue;
            oclDevices.append(dev);
            totalAlloc += dev.alloc;
            if(sizeToBytes(dev.minChunk) & 1) {
                QMessageBox::warning(this, tr("Update Options"), tr("The Min chunk size for device \"%1\" must be a multiple of 2 bytes").arg(dev.name));
                return;
            }
        }
        if(totalAlloc > 100.0) {
            QMessageBox::warning(this, tr("Update Options"), tr("The total allocation across OpenCL devices cannot exceed 100%"));
            return;
        }
    }

    auto& settings = Settings::getInstance();
    // we won't bother verifying the existence of the binary as the user could be using system binaries
    const auto& currentBin = settings.parparBin();
    bool useNode = ui->chkPathNode->isChecked();
    bool binChanged = useNode != currentBin.length() > 1;
    const auto& binParpar = ui->txtPathParPar->text();
    if(useNode) {
        const auto& binNode = ui->txtPathNode->text();
        if(!binChanged && (binParpar != currentBin[1] || binNode != currentBin[0]))
            binChanged = true;
        settings.setParparBin(binParpar, binNode);
    } else {
        if(!binChanged && binParpar != currentBin[0])
            binChanged = true;
        settings.setParparBin(binParpar);
    }

    settings.setUnicode(static_cast<SettingsUnicode>(ui->cboUnicode->currentIndex()));
    settings.setCharset(ui->cboCharset->currentText());
    settings.setPacketRepMin(packetRepMin);
    settings.setPacketRepMax(ui->txtPacketMax->value());
    settings.setStdNaming(ui->chkStdNaming->isChecked());

    settings.setSliceMultple(useMultiple ? ui->txtSliceMultiple->getSizeString() : "");
    settings.setSliceLimit(ui->txtSliceLimit->value());
    switch(ui->cboAllocInMode->currentIndex()) {
    case SettingsDefaultAllocIn::ALLOC_IN_SIZE:
        settings.setAllocSliceSize(ui->txtAllocInSize->text());
        break;
    case SettingsDefaultAllocIn::ALLOC_IN_COUNT:
        settings.setAllocSliceCount(ui->txtAllocInCount->value());
        break;
    case SettingsDefaultAllocIn::ALLOC_IN_RATIO:
        settings.setAllocSliceRatio(ui->txtAllocInRatio->value());
        break;
    }
    switch(ui->cboAllocRecMode->currentIndex()) {
    case SettingsDefaultAllocRec::ALLOC_REC_RATIO:
        settings.setAllocRecoveryRatio(ui->txtAllocRecRatio->value());
        break;
    case SettingsDefaultAllocRec::ALLOC_REC_COUNT:
        settings.setAllocRecoveryCount(ui->txtAllocRecCount->value());
        break;
    case SettingsDefaultAllocRec::ALLOC_REC_SIZE:
        settings.setAllocRecoverySize(ui->txtAllocRecSize->text());
        break;
    }

    settings.setRunClose(ui->chkRunClose->isChecked());


    settings.setReadSize(ui->txtReadSize->getSizeString());
    settings.setReadBuffers(ui->txtReadBufs->value());
    settings.setHashQueue(ui->txtHashQueue->value());
    settings.setHashMethod(ui->cboHashKernel->currentText());
    settings.setMinChunk(ui->txtMinChunk->getSizeString());
    settings.setChunkReadThreads(ui->txtChunkReadThreads->value());
    settings.setRecBuffers(ui->txtRecBufs->value());
    settings.setHashBatch(ui->txtRecHashBatch->value());
    settings.setMd5Method(ui->cboMD5Kernel->currentText());
    settings.setOutputSync(ui->chkOutputSync->isChecked());

    settings.setProcBatch(ui->chkProcBatch->isChecked() ? ui->txtProcBatch->value() : -1);
    settings.setMemLimit(ui->chkMemLimit->isChecked() ? ui->txtMemLimit->getSizeString() : "");
    settings.setGfMethod(ui->chkProcKernel->isChecked() ? ui->cboProcKernel->currentText() : "");
    settings.setTileSize(ui->chkTileSize->isChecked() ? ui->txtTileSize->getSizeString() : "");
    settings.setThreadNum(ui->chkThreads->isChecked() ? ui->txtThreads->value() : -1);
    settings.setCpuMinChunk(ui->txtCpuMinChunk->getSizeString());

    settings.setOpenclDevices(oclDevices);

    QDialog::accept();
    emit settingsUpdated(binChanged);

}


void OptionsDialog::on_btnReset_clicked()
{
    Settings::getInstance().reset();
    loadSettings();
}


void OptionsDialog::on_chkPathNode_toggled(bool checked)
{
    ui->txtPathNode->setEnabled(checked);
    ui->btnPathNode->setEnabled(checked);
}


void OptionsDialog::on_btnPathParPar_clicked()
{
    auto file = QFileDialog::getOpenFileName(this, tr("Select ParPar script or binary"), ui->txtPathParPar->text(), tr("ParPar script/binary (parpar.js;parpar"
                                                                                                                   #ifdef Q_OS_WINDOWS
                                                                                                                       ".exe;parpar.cmd"
                                                                                                                   #endif
                                                                                                                       ");;All files (*.*)"));
    if(!file.isEmpty()) {
        ui->txtPathParPar->setText(file.replace("/", QDir::separator()));

        if(file.endsWith(".js", Qt::CaseInsensitive)) {
#ifdef Q_OS_WINDOWS
            // Linux may have the ability to execute scripts directly
            ui->chkPathNode->setChecked(true);
#endif
        } else {
            ui->chkPathNode->setChecked(false);
        }
    }
}


void OptionsDialog::on_btnPathNode_clicked()
{
    auto file = QFileDialog::getOpenFileName(this, tr("Select Node.js binary"), ui->txtPathNode->text(), tr("Node.js binary (node"
                                                                                                                   #ifdef Q_OS_WINDOWS
                                                                                                                       ".exe"
                                                                                                                   #endif
                                                                                                                       ");;All files (*.*)"));
    if(!file.isEmpty()) {
        ui->txtPathNode->setText(file.replace("/", QDir::separator()));
    }
}


void OptionsDialog::on_chkSliceMultiple_toggled(bool checked)
{
    ui->txtSliceMultiple->setEnabled(checked);
}


void OptionsDialog::on_chkTileSize_stateChanged(int arg1)
{
    ui->txtTileSize->setEnabled(ui->chkTileSize->isChecked());
}


void OptionsDialog::on_txtReadBufs_editingFinished()
{
    ui->txtHashQueue->setMaximum(ui->txtReadBufs->value());
    ui->txtChunkReadThreads->setMaximum(ui->txtReadBufs->value());
}


void OptionsDialog::on_txtRecBufs_editingFinished()
{
    ui->txtRecHashBatch->setMaximum(ui->txtRecBufs->value());
}

void OptionsDialog::rescale()
{
    int w = ui->treeDevices->width();
    ui->treeDevices->header()->setUpdatesEnabled(false);
    ui->treeDevices->header()->resizeSection(0, w>190 ? w-70 : 120);
    ui->treeDevices->header()->resizeSection(1, 60);
    ui->treeDevices->header()->setUpdatesEnabled(true);
}

void OptionsDialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    //rescale();
}
void OptionsDialog::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    rescale();
}

void OptionsDialog::on_tabWidget_currentChanged(int index)
{
    if(index == 2 && !devicesListed) {
        devicesListed = true;
        rescale(); // doesn't seem to do it on window load :|
        on_chkOpencl_toggled(ui->chkOpencl->isChecked());
    }
}

static bool updatingOpenclOpt = false;

void OptionsDialog::on_treeDevices_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    if(!current) return;
    int type = current->data(0, Qt::UserRole).toInt();
    ui->stkComputeOpt->setCurrentIndex(type);

    if(type == 1) { // OpenCL device
        // load data
        const auto& opts = openclSettings.value(current->data(1, Qt::UserRole).toString(), OpenclDevice());
        updatingOpenclOpt = true;
        ui->txtOclAlloc->setValue(opts.alloc);
        ui->chkOclMemory->setChecked(!opts.memLimit.isEmpty());
        if(!opts.memLimit.isEmpty())
            ui->txtOclMemory->setText(opts.memLimit);
        else
            ui->txtOclMemory->setText("128M");
        ui->txtOclMinChunk->setText(opts.minChunk);
        ui->cboOclKernel->setCurrentText(opts.gfMethod);
        ui->chkOclBatch->setChecked(opts.batch > 0);
        ui->txtOclBatch->setValue(opts.batch > 0 ? opts.batch : 4);
        ui->chkOclIters->setChecked(opts.iters > 0);
        ui->txtOclIters->setValue(opts.iters > 0 ? opts.iters : 1);
        ui->chkOclOutputs->setChecked(opts.outputs > 0);
        ui->txtOclOutputs->setValue(opts.outputs > 0 ? opts.outputs : 1);
        updatingOpenclOpt = false;
    }
}


void OptionsDialog::on_txtPacketMin_valueChanged(int arg1)
{
    ui->txtPacketMax->setEnabled(arg1 < 16);
}

static constexpr struct {
    quint64 readSize;
    int readBuffers;
    int hashQueue;
    int chunkReadThreads;
    int recBuffers;
} ioPresets[]{
    {8*1048576, 4, 3, 2, 12}, // HDD
    {4*1048576, 8, 5, 2, 12}, // default
    {2*1048576, 16, 5, 4, 16} // SSD
};

void OptionsDialog::checkIOPreset()
{
    int idx = 0;
    for(const auto& preset : ioPresets) {
        if(ui->txtReadSize->getBytes() == preset.readSize
                && ui->txtReadBufs->value() == preset.readBuffers
                && ui->txtHashQueue->value() == preset.hashQueue
                && ui->txtChunkReadThreads->value() == preset.chunkReadThreads
                && ui->txtRecBufs->value() == preset.recBuffers)
            break;

        idx++;
    }
    if(idx != ui->cboIOPreset->currentIndex()) {
        ui->cboIOPreset->blockSignals(true);
        ui->cboIOPreset->setCurrentIndex(idx);
        ui->cboIOPreset->blockSignals(false);
    }
}

void OptionsDialog::on_cboIOPreset_currentIndexChanged(int index)
{
    if(index >= ui->cboIOPreset->count()-1) {
        // re-eval
        checkIOPreset();
    } else {
        const auto& preset = ioPresets[index];
        const auto blockSignals = [this](bool block) {
            ui->txtReadSize->blockSignals(block);
            ui->txtReadBufs->blockSignals(block);
            ui->txtHashQueue->blockSignals(block);
            ui->txtChunkReadThreads->blockSignals(block);
            ui->txtRecBufs->blockSignals(block);
        };
        blockSignals(true);
        ui->txtReadSize->setBytes(preset.readSize);
        ui->txtReadBufs->setValue(preset.readBuffers);
        on_txtReadBufs_editingFinished(); // update maximums
        ui->txtHashQueue->setValue(preset.hashQueue);
        ui->txtChunkReadThreads->setValue(preset.chunkReadThreads);
        ui->txtRecBufs->setValue(preset.recBuffers);
        on_txtRecBufs_editingFinished();
        blockSignals(false);
    }
}


void OptionsDialog::on_txtReadSize_valueChanged(quint64 , bool )
{
    checkIOPreset();
}
void OptionsDialog::on_txtReadBufs_valueChanged(int arg1)
{
    checkIOPreset();
}
void OptionsDialog::on_txtHashQueue_valueChanged(int arg1)
{
    checkIOPreset();
}
void OptionsDialog::on_txtChunkReadThreads_valueChanged(int arg1)
{
    checkIOPreset();
}
void OptionsDialog::on_txtRecBufs_valueChanged(int arg1)
{
    checkIOPreset();
}


static QJsonArray oclPlatforms;
void OptionsDialog::fillDeviceList(bool opencl)
{
    auto& tree = this->ui->treeDevices;
    // add items
    QList<QTreeWidgetItem*> topItems;
    topItems.reserve(oclPlatforms.size() + 1);

    auto cpuItem = new QTreeWidgetItem(static_cast<QTreeWidget*>(nullptr), QStringList{"CPU", "100%"});
    cpuItem->setTextAlignment(1, Qt::AlignRight);
    cpuItem->setData(0, Qt::UserRole, 0);
    topItems.append(cpuItem);

    if(opencl) {
        float cpuAlloc = 100;
        QSet<QString> seenDevices;
        for(const auto& _plat : oclPlatforms) {
            const auto plat = _plat.toObject();
            auto platItem = new QTreeWidgetItem(static_cast<QTreeWidget*>(nullptr), QStringList{plat.value("name").toString(), ""});
            platItem->setTextAlignment(1, Qt::AlignRight);
            platItem->setData(0, Qt::UserRole, 2);

            for(const auto& _dev : plat.value("devices").toArray()) {
                const auto& dev = _dev.toObject();
                QString name = dev.value("name").toString();

                // determine device key
                QString key = name.toLower();
                if(seenDevices.contains(key)) {
                    int idx = 1;
                    QString idxStr("::%1");
                    while(seenDevices.contains(key + idxStr.arg(idx)))
                        idx++;
                    key += idxStr.arg(idx);
                }
                seenDevices.insert(key);
                // create backing store here as well
                if(!openclSettings.contains(key))
                    openclSettings.insert(key, OpenclDevice());
                openclSettings[key].name = name;

                auto alloc = openclSettings[key].alloc;
                cpuAlloc -= alloc;
                QString allocStr;
                if(alloc > 0.0) allocStr = QLocale().toString(alloc, 'f', 2) + "%";
                auto devItem = new QTreeWidgetItem(platItem, QStringList{name, allocStr});
                devItem->setTextAlignment(1, Qt::AlignRight);
                devItem->setData(0, Qt::UserRole, 1);
                devItem->setData(1, Qt::UserRole, key);

                platItem->addChild(devItem);
            }
            topItems.append(platItem);
        }
        if(cpuAlloc > 0.0)
            topItems[0]->setText(1, QLocale().toString(cpuAlloc, 'f', 2) + "%");

        // prune unrecognised devices
        QMutableHashIterator<QString, OpenclDevice> it(openclSettings);
        while(it.hasNext()) {
            it.next();
            if(!seenDevices.contains(it.key()))
                it.remove();
        }
    }

    tree->setUpdatesEnabled(false);
    tree->clear();
    tree->addTopLevelItems(topItems);
    for(auto item : topItems)
        if(item->childCount() > 0)
            item->setExpanded(true);
    tree->setCurrentItem(topItems[0]);
    tree->setUpdatesEnabled(true);
}

static bool haveOclDevices = false; // TODO: should be class local, so it gets retried on dialog reopen
void OptionsDialog::loadOpenclDevices()
{
    if(haveOclDevices) return;
    haveOclDevices = true;

    auto progress = new QProgressDialog(this);
    progress->setLabelText(tr("Querying OpenCL devices..."));
    progress->setMinimumDuration(100);
    progress->setValue(0);
    progress->setWindowModality(Qt::WindowModal);
    auto* pb = new QProgressBar(progress);
    pb->setAlignment(Qt::AlignCenter);
    progress->setBar(pb);
    progress->setRange(0, 0);

    auto parpar = new ParParClient(this);
    connect(parpar, &ParParClient::failed, this, [=](const QString& error) {
        progress->setValue(0); // closes window
        delete progress;
        haveOclDevices = false;
        ui->chkOpencl->setChecked(false);
        parpar->deleteLater();

        if(!error.isEmpty()) // isEmpty = cancelled
            QMessageBox::warning(this, tr("OpenCL Options"), tr("Failed to query list of OpenCL devices.\n%1").arg(error));
    });
    connect(parpar, &ParParClient::output, this, [=](const QJsonObject& obj) {
        oclPlatforms = obj.value("platforms").toArray();
        progress->setValue(0); // closes window
        delete progress;
        fillDeviceList(ui->chkOpencl->isChecked());
        parpar->deleteLater();
    });
    connect(progress, &QProgressDialog::canceled, this, [=]() {
        parpar->kill();
    });
    parpar->run({"--opencl-list"});
}

void OptionsDialog::on_chkOpencl_toggled(bool checked)
{
    // load OpenCL if not already
    if(checked && !haveOclDevices)
        loadOpenclDevices();
    else
        fillDeviceList(checked);
}

void OptionsDialog::openclOptChanged()
{
    ui->grpOpenclOpts->setEnabled(ui->txtOclAlloc->value() > 0);
    ui->txtOclMemory->setEnabled(ui->chkOclMemory->isChecked());
    ui->txtOclBatch->setEnabled(ui->chkOclBatch->isChecked());
    ui->txtOclIters->setEnabled(ui->chkOclIters->isChecked());
    ui->txtOclOutputs->setEnabled(ui->chkOclOutputs->isChecked());

    if(updatingOpenclOpt) return;
    const auto key = ui->treeDevices->currentItem()->data(1, Qt::UserRole).toString();
    auto& dev = openclSettings[key];
    dev.alloc = ui->txtOclAlloc->value();
    dev.memLimit = ui->chkOclMemory->isChecked() ? ui->txtOclMemory->text() : "";
    dev.minChunk = ui->txtOclMinChunk->text();
    dev.gfMethod = ui->cboOclKernel->currentText();
    dev.batch = ui->chkOclBatch->isChecked() ? ui->txtOclBatch->value() : 0;
    dev.iters = ui->chkOclIters->isChecked() ? ui->txtOclIters->value() : 0;
    dev.outputs = ui->chkOclOutputs->isChecked() ? ui->txtOclOutputs->value() : 0;

    QString allocStr;
    if(dev.alloc > 0.0)
        allocStr = QLocale().toString(dev.alloc, 'f', 2) + "%";
    ui->treeDevices->currentItem()->setText(1, allocStr);

    float cpuAlloc = 100.0;
    for(const auto& dev : openclSettings)
        cpuAlloc -= dev.alloc;
    allocStr = "";
    if(cpuAlloc > 0.0)
        allocStr = QLocale().toString(cpuAlloc, 'f', 2) + "%";
    ui->treeDevices->topLevelItem(0)->setText(1, allocStr);
}

void OptionsDialog::on_txtOclAlloc_valueChanged(double arg1)
{
    openclOptChanged();
}
void OptionsDialog::on_chkOclMemory_toggled(bool checked)
{
    openclOptChanged();
}
void OptionsDialog::on_chkOclBatch_toggled(bool checked)
{
    openclOptChanged();
}
void OptionsDialog::on_chkOclIters_toggled(bool checked)
{
    openclOptChanged();
}
void OptionsDialog::on_chkOclOutputs_toggled(bool checked)
{
    openclOptChanged();
}
void OptionsDialog::on_txtOclBatch_editingFinished()
{
    openclOptChanged();
}
void OptionsDialog::on_txtOclMemory_textChanged(const QString &arg1)
{
    openclOptChanged();
}
void OptionsDialog::on_cboOclKernel_currentIndexChanged(int index)
{
    openclOptChanged();
}
void OptionsDialog::on_txtOclIters_editingFinished()
{
    openclOptChanged();
}
void OptionsDialog::on_txtOclOutputs_editingFinished()
{
    openclOptChanged();
}
void OptionsDialog::on_txtOclMinChunk_textChanged(const QString &arg1)
{
    openclOptChanged();
}

void OptionsDialog::on_cboAllocInMode_currentIndexChanged(int index)
{
    ui->stkAllocIn->setCurrentIndex(index);
}


void OptionsDialog::on_cboAllocRecMode_currentIndexChanged(int index)
{
    ui->stkAllocRec->setCurrentIndex(index);
}



