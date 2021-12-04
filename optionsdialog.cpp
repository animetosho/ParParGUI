#include "optionsdialog.h"
#include "ui_optionsdialog.h"
#include <QSysInfo>
#include <QThread>
#include <QFileDialog>
#include "settings.h"
#include <QMessageBox>

OptionsDialog::OptionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OptionsDialog)
{
    ui->setupUi(this);

    auto arch = QSysInfo::currentCpuArchitecture();
    if(arch == "arm" || arch == "arm64")
        ui->cboProcKernel->addItems({"shuffle-neon", "clmul-neon"});
    if(arch == "arm64")
        ui->cboProcKernel->addItems({"shuffle128-sve", "shuffle128-sve2", "shuffle2x128-sve2", "shuffle512-sve2", "clmul-sve2"});
    if(arch == "i386" || arch == "x86_64") {
        ui->cboProcKernel->addItems({"xor-sse", "xorjit-sse"});
        if(arch == "x86_64")
            ui->cboProcKernel->addItems({"xorjit-avx2", "xorjit-avx512"});
        ui->cboProcKernel->addItems({"shuffle-sse", "shuffle-avx", "shuffle-avx2", "shuffle-avx512", "shuffle-vbmi", "shuffle2x-avx2", "shuffle2x-avx512", "affine-sse", "affine-avx2", "affine-avx512", "affine2x-sse", "affine2x-avx2", "affine2x-avx512"});
    }

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
    ui->chkStdNaming->setChecked(settings.stdNaming());

    chk = !settings.sliceMultiple().isEmpty();
    ui->chkSliceMultiple->setChecked(chk);
    ui->txtSliceMultiple->setEnabled(chk);
    ui->txtSliceMultiple->setText(chk ? settings.sliceMultiple() : "750K");


    ui->txtReadSize->setText(settings.readSize());
    ui->txtReadBufs->setValue(settings.readBuffers());
    ui->txtHashQueue->setMaximum(settings.readBuffers());
    ui->txtHashQueue->setValue(settings.hashQueue());
    ui->txtMinChunk->setText(settings.minChunk());
    ui->txtRecBufs->setValue(settings.recBuffers());
    ui->txtRecHashBatch->setMaximum(settings.recBuffers());
    ui->txtRecHashBatch->setValue(settings.hashBatch());

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

    // TODO: load defaults for processing options (e.g. GF kernel)
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
    bool useMultiple = ui->chkSliceMultiple->isChecked();
    if(useMultiple) {
        if(ui->txtSliceMultiple->getBytes() & 3) {
            QMessageBox::warning(this, tr("Update Options"), tr("The slice size multiple must be a multiple of 4 bytes"));
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
    settings.setStdNaming(ui->chkStdNaming->isChecked());

    settings.setSliceMultple(useMultiple ? ui->txtSliceMultiple->getSizeString() : "");


    settings.setReadSize(ui->txtReadSize->getSizeString());
    settings.setReadBuffers(ui->txtReadBufs->value());
    settings.setHashQueue(ui->txtHashQueue->value());
    settings.setMinChunk(ui->txtMinChunk->getSizeString());
    settings.setRecBuffers(ui->txtRecBufs->value());
    settings.setHashBatch(ui->txtRecHashBatch->value());

    settings.setProcBatch(ui->chkProcBatch->isChecked() ? ui->txtProcBatch->value() : -1);
    settings.setMemLimit(ui->chkMemLimit->isChecked() ? ui->txtMemLimit->getSizeString() : "");
    settings.setGfMethod(ui->chkProcKernel->isChecked() ? ui->cboProcKernel->currentText() : "");
    settings.setTileSize(ui->chkTileSize->isChecked() ? ui->txtTileSize->getSizeString() : "");
    settings.setThreadNum(ui->chkThreads->isChecked() ? ui->txtThreads->value() : -1);

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
}


void OptionsDialog::on_txtRecBufs_editingFinished()
{
    ui->txtRecHashBatch->setMaximum(ui->txtRecBufs->value());
}

