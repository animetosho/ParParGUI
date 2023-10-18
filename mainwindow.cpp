#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "./optionsdialog.h"
#include "./createprogress.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include "settings.h"
#include "util.h"
#include "progressdialog.h"
#include "par2calc.h"
#include "sourcefilelistitem.h"
#include "outpreviewlistitem.h"
#include "clientinfo.h"
#include <math.h>
#include <QSet>

#include <QFile>
#include <QDir>
#include <QClipboard>
#include <QTemporaryFile>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , dlgOptions(this)
    , outPreview(this)
{
    ui->setupUi(this);

    ui->tvDest->hide();

    ui->txtInsliceCount->setMainWindow(this);

    const auto& settings = Settings::getInstance();
    ui->stkSource->setCurrentIndex(settings.uiExpSource() ? 1 : 0);
    ui->btnDestPreview->setChecked(settings.uiExpDest());
    // handle things which change in Designer
    ui->stkDestSizing->setCurrentIndex(0);
    this->resize(600, 500); // set an initial size to expand on

    ui->tvSource->sortByColumn(0, Qt::SortOrder::AscendingOrder);
    ui->tvDest->sortByColumn(0, Qt::SortOrder::AscendingOrder);

    ui->txtInsliceSize->blockSignals(true);
    ui->txtInsliceSize->setText(settings.allocSliceSize());
    ui->txtInsliceSize->blockSignals(false);
    ui->txtInsliceCount->blockSignals(true);
    ui->txtInsliceCount->setValue(settings.allocSliceCount());
    ui->txtInsliceCount->blockSignals(false);
    switch(settings.allocSliceMode()) {
    case SettingsDefaultAllocIn::ALLOC_IN_SIZE:
        ui->optInsliceSize->blockSignals(true);
        ui->optInsliceSize->setChecked(true);
        ui->optInsliceSize->blockSignals(false);
        break;
    case SettingsDefaultAllocIn::ALLOC_IN_COUNT:
        ui->optInsliceCount->blockSignals(true);
        ui->optInsliceCount->setChecked(true);
        ui->optInsliceCount->blockSignals(false);
        break;
    case SettingsDefaultAllocIn::ALLOC_IN_RATIO:
        // handle later
        break;
    }

    ui->txtOutsliceRatio->blockSignals(true);
    ui->txtOutsliceRatio->setValue(settings.allocRecoveryRatio());
    ui->txtOutsliceRatio->blockSignals(false);
    ui->txtOutsliceCount->blockSignals(true);
    ui->txtOutsliceCount->setValue(settings.allocRecoveryCount());
    ui->txtOutsliceCount->blockSignals(false);
    ui->txtOutsliceSize->blockSignals(true);
    ui->txtOutsliceSize->setText(settings.allocRecoverySize());
    ui->txtOutsliceSize->blockSignals(false);
    switch(settings.allocRecoveryMode()) {
    case SettingsDefaultAllocRec::ALLOC_REC_RATIO:
        ui->optOutsliceRatio->blockSignals(true);
        ui->optOutsliceRatio->setChecked(true);
        ui->optOutsliceRatio->blockSignals(false);
        break;
    case SettingsDefaultAllocRec::ALLOC_REC_COUNT:
        ui->optOutsliceCount->blockSignals(true);
        ui->optOutsliceCount->setChecked(true);
        ui->optOutsliceCount->blockSignals(false);
        break;
    case SettingsDefaultAllocRec::ALLOC_REC_SIZE:
        ui->optOutsliceSize->blockSignals(true);
        ui->optOutsliceSize->setChecked(true);
        ui->optOutsliceSize->blockSignals(false);
        break;
    }

    par2SrcSize = 0;
    par2FileCount = 0;
    srcBaseChosen = false;
    destFileChosen = false;


    auto& clientInfo = ClientInfo::getInstance();
    connect(&clientInfo, &ClientInfo::failed, this, [=](const QString& error) {
        QMessageBox::warning(this, tr("ParPar Execute Failed"), tr("Failed to retrieve information from ParPar client. Please ensure that ParPar is available, executable and/or configured in the Options dialog.\n\nDetail: %1").arg(error));
    });
    connect(&clientInfo, &ClientInfo::updated, this, [=]() {
        // mostly because the creator could've changed
        this->updateDestPreview();
    });

    this->optionSliceMultiple = 4;
    this->optionSliceLimit = settings.sliceLimit();
    auto updateMultiple = [=](bool binaryChanged) {
        const auto& settings = Settings::getInstance();
        quint64 newMultiple = sizeToBytes(settings.sliceMultiple());
        if(!newMultiple) newMultiple = 4;
        else if(newMultiple & 3) // invalid multiple - can't be used
            newMultiple = 4;
        if(this->optionSliceLimit != settings.sliceLimit()) {
            this->optionSliceLimit = settings.sliceLimit();
            this->optionSliceMultiple = newMultiple;
            this->updateInsliceInfo();
            this->checkSourceFileCount(tr("Change slice limit"));
        }
        else if(this->optionSliceMultiple != newMultiple) {
            this->optionSliceMultiple = newMultiple;
            this->updateInsliceInfo();
        }

        if(binaryChanged) { // TODO: maybe update regardless of change (e.g. if user renamed files correct)
            // dest preview will be updated by the following, so don't need to double update
            ClientInfo::getInstance().refresh();
        } else {
            this->updateDestPreview(); // a number of options affect this, so always regen
        }
    };
    connect(&dlgOptions, &OptionsDialog::settingsUpdated, this, updateMultiple);
    updateMultiple(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::rescale() {
    int w = ui->tvSource->width();
    ui->tvSource->header()->setUpdatesEnabled(false);
    ui->tvSource->header()->resizeSection(0, w>400 ? w-250 : 150);
    ui->tvSource->header()->resizeSection(1, 150);
    ui->tvSource->header()->resizeSection(2, 80);
    ui->tvSource->header()->setUpdatesEnabled(true);

    w = ui->tvDest->width();
    ui->tvDest->header()->setUpdatesEnabled(false);
    ui->tvDest->header()->resizeSection(0, w>320 ? w-240 : 80);
    ui->tvDest->header()->resizeSection(1, 80);
    ui->tvDest->header()->resizeSection(2, 70);
    ui->tvDest->header()->resizeSection(3, 70);
    ui->tvDest->header()->setUpdatesEnabled(true);
}

void MainWindow::adjustExpansion(bool allowExpand) {
    bool inExp = ui->stkSource->currentIndex() == 1;
    bool outExp = ui->btnDestPreview->isChecked();
    ui->splitter->setUpdatesEnabled(false);
    for(int i=0; i<ui->splitter->count(); i++)
        ui->splitter->handle(i)->setEnabled(inExp && outExp);
    ui->splitter->setUpdatesEnabled(true);

    ui->tvDest->setVisible(outExp);
    ui->btnDestPreview->setArrowType(outExp ? Qt::UpArrow : Qt::DownArrow);
    ui->stkSource->setUpdatesEnabled(false);
    if(inExp) {
        ui->stkSource->setMinimumHeight(ui->stkSourceAdv->minimumHeight());
        ui->stkSource->setMaximumHeight(ui->stkSourceAdv->maximumHeight());
    } else {
        ui->stkSource->setFixedHeight(ui->stkSourceBasic->layout()->sizeHint().height());
    }
    ui->stkSource->setUpdatesEnabled(true);

    ui->stkDestSizing->setFixedHeight(ui->stkDestSizing->currentWidget()->layout()->sizeHint().height());
    ui->stkDestSizing->updateGeometry();

    auto destOptsMargin = ui->fraDestOpts->layout()->contentsMargins();
    destOptsMargin.setBottom(outExp ? destOptsMargin.top() : 0);
    ui->fraDestOpts->layout()->setContentsMargins(destOptsMargin);

    auto destPolicy = ui->grpDest->sizePolicy();
    destPolicy.setVerticalPolicy(outExp ? QSizePolicy::Expanding : QSizePolicy::Maximum);
    ui->grpDest->setUpdatesEnabled(false);
    ui->grpDest->setSizePolicy(destPolicy);
    ui->grpDest->adjustSize();
    ui->grpDest->setUpdatesEnabled(true);

    this->setUpdatesEnabled(false);
    if(inExp || outExp) {
        this->setMaximumHeight(16777215);
        ui->scrollArea->setMaximumHeight(16777215);

        if(allowExpand) {
            // TODO: detect screen height and restrict resize?
            /*
            // the default QTreeWidget size is a bit too high - try restricting it and let the layout compute the appropriate size
            ui->tvSource->setFixedHeight(100);
            int scrollDiff = ui->scrollAreaContents->layout()->sizeHint().height() - ui->scrollAreaContents->height();
            if(scrollDiff > 0) {
                // try to eliminate scrollbar
                this->resize(this->width(), this->height() + scrollDiff);
            }
            ui->tvSource->setMaximumHeight(INT_MAX);
            */

            int wantedMinHeight = this->layout()->sizeHint().height();
            if(inExp) wantedMinHeight += 150;
            if(outExp) wantedMinHeight += ui->tvDest->minimumHeight();
            if(this->height() < wantedMinHeight)
                this->resize(this->width(), wantedMinHeight);
        }
    } else {
        // for some reason, the layout engine overscales by default, so manually size the major containers
        auto scrollMargins = ui->scrollAreaContents->layout()->contentsMargins();
        int scrollHeight = ui->stkSourceBasic->layout()->sizeHint().height()
                + ui->fraSlices->sizeHint().height()
                + ui->grpDest->sizeHint().height()
                + scrollMargins.bottom() + scrollMargins.top()
                + ui->scrollAreaContents->layout()->spacing()*2;
            //ui->scrollArea->setFixedHeight(scrollHeight);
            this->setFixedHeight(scrollHeight + ui->lytBottomButtons->sizeHint().height());
    }
    this->setMinimumWidth(575); // for some reason, this can get lost?
    this->setUpdatesEnabled(true);
}

void MainWindow::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);

    on_cboDestDist_currentIndexChanged(ui->cboDestDist->currentIndex());
    adjustExpansion(true); // TODO: above calls adjustExpansion - need to avoid a double-call
    rescale();


    /*
    // check ParPar executable
    bool parparMissing;
    auto parparBin = Settings::getInstance().parparBin(&parparMissing);
    if(!parparMissing) {
        for(const auto& part : parparBin) {
            if(!QFile::exists(part)) {
                parparMissing = true;
                break;
            }
        }
    }
    if(parparMissing) {
        dlgOptions.open();
        QMessageBox::warning(&dlgOptions, tr("ParPar Executable"), tr("The ParPar executable was not found. Please configure it in this Options dialog."));
        dlgOptions.focusWidget(); // TODO: this still doesn't seem to prioritize focus onto the dialog
    } else {
        QFileInfo binInfo(parparBin[0]);
        if(binInfo.isReadable() && !binInfo.isExecutable()) {
            // possibly common problem on Unixes where the executable bit isn't set
            if(QMessageBox::Yes == QMessageBox::warning(this, tr("ParPar Executable"), tr("The Node/ParPar executable is not executable. Do you want to try and enable it?"), QMessageBox::Yes | QMessageBox::No)) {
                QFile exe(parparBin[0]);
                auto ans = QMessageBox::Retry;
                while(ans == QMessageBox::Retry) {
                    if(exe.setPermissions(exe.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther))
                        break;
                    ans = QMessageBox::critical(this, tr("ParPar Executable"), tr("Failed to set executable permissions to the Node/ParPar executable."), QMessageBox::Retry | QMessageBox::Ignore);
                }
            }
        }
    }
    */
}
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    rescale();
}

void MainWindow::on_btnAbout_clicked()
{
    QMessageBox::information(this, tr("About ParPar GUI"), QString("ParPar GUI v%1\nParPar v%2")
                             .arg(QCoreApplication::applicationVersion(),
                                  ClientInfo::version())
                             );
}


void MainWindow::on_btnOptions_clicked()
{
    dlgOptions.open();
}

void MainWindow::on_txtInsliceCount_valueChanged(int value)
{
    ui->optInsliceCount->blockSignals(true);
    ui->optInsliceCount->setChecked(true);
    ui->optInsliceCount->blockSignals(false);
    par2SliceSize = Par2Calc::sliceSizeFromCount(value, optionSliceMultiple, optionSliceLimit, par2SrcFiles, par2FileCount);
    ui->txtInsliceSize->setBytesApprox(par2SliceSize, true);

    updateOutsliceInfo(false);
}
void MainWindow::on_txtInsliceCount_editingFinished()
{
    if(!ui->optInsliceCount->isChecked()) return;

    int newValue = ui->txtInsliceCount->value();
    par2SliceSize = Par2Calc::sliceSizeFromCount(newValue, optionSliceMultiple, optionSliceLimit, par2SrcFiles, par2FileCount);
    if(newValue != ui->txtInsliceCount->value()) {
        ui->txtInsliceCount->blockSignals(true);
        ui->txtInsliceCount->setValue(newValue);
        ui->txtInsliceCount->blockSignals(false);
    }
    ui->txtInsliceSize->setBytesApprox(par2SliceSize, true);

    updateOutsliceInfo();
}


void MainWindow::on_btnSourceAdd_clicked()
{
    auto files = QFileDialog::getOpenFileNames(this, tr("Add source files"), ui->txtSourcePath->text(), tr("All files (*.*)"));
    if(!files.isEmpty()) {
        // TODO: select items that were just added
        sourceAddFiles(files);
        if(ui->txtDestFile->text().isEmpty())
            autoSelectDestFile();
        checkSourceFileCount();
        updateSrcFilesState();
    }
}

void MainWindow::on_btnSourceAddDir_clicked()
{
    auto dir = QFileDialog::getExistingDirectory(this, tr("Add source files from directory"), ui->txtSourcePath->text());
    if(!dir.isEmpty()) {
        sourceAddDir(dir);
        if(ui->txtDestFile->text().isEmpty())
            autoSelectDestFile();
        checkSourceFileCount();
        updateSrcFilesState();
    }
}


static void sourceDelHelper(QTreeWidgetItem* item, SrcFileList& files, quint64& totalSize, int& totalCount, bool isRecur = false)
{
    int children = item->childCount();
    while(children--)
        sourceDelHelper(item->child(children), files, totalSize, totalCount, true);

    auto key = item->data(0, Qt::UserRole).toString();
    if(!key.isEmpty()) { // key will be empty for directories
        if(files[key].size()) {
            totalSize -= files[key].size();
            totalCount--;
        }
        files.remove(key);
    }

    auto parent = item->parent();
    delete item;
    // remove empty parents
    if(isRecur) return;
    while(parent && parent->childCount() == 0) {
        item = parent;
        parent = item->parent();
        delete item;
    }
}
void MainWindow::on_btnSourceDel_clicked()
{
    auto selections = ui->tvSource->selectedItems();
    ui->tvSource->setUpdatesEnabled(false);
    while(!selections.isEmpty()) {
        sourceDelHelper(selections.at(0), par2SrcFiles, par2SrcSize, par2FileCount);
        selections = ui->tvSource->selectedItems();
    }
    ui->tvSource->setUpdatesEnabled(true);
    updateSrcFilesState();
    if(par2SrcFiles.isEmpty()) {
        // assume we're starting afresh
        srcBaseChosen = false;
        destFileChosen = false;
    }
}

static void recurseAddDir(const QDir& dir, SrcFileList& dest, ProgressDialog& progress)
{
    // TODO: options for symlinks/system/hidden (system probably undesirable on Unix) files
    auto list = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);

    // since we don't know how deep this is going to get, we'll overallocate the progress and fix it at the end
    progress.setMaximum(progress.maximum() + list.size()*2);

    for(auto& info : list) {
        if(progress.wasCanceled()) return;
        auto key = info.canonicalFilePath();
        if(info.isDir()) {
            recurseAddDir(key, dest, progress);
        } else {
#ifdef Q_OS_WINDOWS
            key = key.toLower();
#endif
            dest.insert(key, info);
        }
        progress.inc();
    }
    progress.setMaximum(progress.maximum() - list.size());
}
void MainWindow::sourceAddFiles(const QStringList &files)
{
    ProgressDialog progress(this, tr("Adding files..."));
    progress.setMaximum(files.size());

    par2SrcFiles.reserve(par2SrcFiles.size() + files.size());
    for(const auto& file : files) {
        if(progress.wasCanceled()) break;

        QFileInfo info(file);
        if(info.isDir()) {
            recurseAddDir(file, par2SrcFiles, progress);
        } else {
            auto key = info.canonicalFilePath();
#ifdef Q_OS_WINDOWS
            key = key.toLower();
#endif
            par2SrcFiles.insert(key, info);
            progress.inc();
        }
    }
    progress.end();

    if(!srcBaseChosen)
        ui->txtSourcePath->setText(srcFilesCommonPath().replace("/", QDir::separator()));
    reloadSourceFiles();
}
void MainWindow::sourceAddDir(const QString& dir)
{
    ProgressDialog progress(this, tr("Adding files..."));
    progress.setMaximum(1); // we don't know the maximum, so add it as we go; can't set to 0 as it'd close the window

    bool previouslyEmpty = par2SrcFiles.isEmpty();
    recurseAddDir(dir, par2SrcFiles, progress);
    progress.end();

    QString basePath = dir;
    if(!previouslyEmpty)
        basePath = srcFilesCommonPath();
    if(!srcBaseChosen)
        ui->txtSourcePath->setText(basePath.replace("/", QDir::separator()));
    reloadSourceFiles();
}

void MainWindow::on_btnSourcePathBrowse_clicked()
{
    auto dir = QFileDialog::getExistingDirectory(this, tr("Select base path"), ui->txtSourcePath->text());
    if(!dir.isEmpty()) {
        ui->txtSourcePath->setText(dir.replace("/", QDir::separator()));
        srcBaseChosen = true;
        reloadSourceFiles();
        updateDestPreview();
    }
}


void MainWindow::on_btnDestPreview_clicked()
{
    bool show = ui->btnDestPreview->isChecked();

    adjustExpansion(show);
    ui->scrollArea->ensureWidgetVisible(ui->tvDest);
    Settings::getInstance().setUiExpDest(show);

    updateDestPreview();
}


void MainWindow::on_cboSourcePaths_currentIndexChanged(int index)
{
    bool enablePath = (index == 1);
    ui->txtSourcePath->setEnabled(enablePath);
    ui->btnSourcePathBrowse->setEnabled(enablePath);

    ui->tvSource->setItemsExpandable(index != 0);
    ui->tvSource->setRootIsDecorated(index != 0);
    reloadSourceFiles();
    updateDestPreview();
}


void MainWindow::on_btnDestFileBrowse_clicked()
{
    auto file = QFileDialog::getSaveFileName(this, tr("Select destination recovery base file"), ui->txtDestFile->text(), tr("PAR2 files (*.par2)"));
    if(!file.isEmpty()) {
        ui->txtDestFile->setText(file.replace("/", QDir::separator()));
        destFileChosen = true;
        updateSrcFilesState();
        updateDestPreview();
    }
}


void MainWindow::on_txtInsliceSize_valueChanged(quint64 size, bool finished)
{
    if(finished) {
        if(!ui->optInsliceSize->isChecked()) return;
    } else {
        ui->optInsliceSize->blockSignals(true);
        ui->optInsliceSize->setChecked(true);
        ui->optInsliceSize->blockSignals(false);
    }

    par2SliceSize = size;
    int count = Par2Calc::sliceCountFromSize(par2SliceSize, optionSliceMultiple, optionSliceLimit, par2SrcFiles, par2FileCount);
    if(finished && size != par2SliceSize) {
        ui->txtInsliceSize->setBytes(par2SliceSize);
    }
    ui->txtInsliceCount->blockSignals(true);
    ui->txtInsliceCount->setValue(count);
    ui->txtInsliceCount->blockSignals(false);

    updateOutsliceInfo(finished);
}

void MainWindow::on_txtOutsliceRatio_valueChanged(double arg1)
{
    ui->optOutsliceRatio->blockSignals(true);
    ui->optOutsliceRatio->setChecked(true);
    ui->optOutsliceRatio->blockSignals(false);

    int srcSlices = ui->txtInsliceCount->value();
    int destSlices = ceil(srcSlices * (arg1/100));
    if(destSlices > 65535) destSlices = 65535;
    ui->txtOutsliceCount->blockSignals(true);
    ui->txtOutsliceCount->setValue(destSlices);
    ui->txtOutsliceCount->blockSignals(false);

    ui->txtOutsliceSize->setBytesApprox(par2SliceSize * destSlices, true);

    updateDestInfo(false);
}
void MainWindow::on_txtOutsliceRatio_editingFinished()
{
    if(!ui->optOutsliceRatio->isChecked()) return;

    double val = ui->txtOutsliceRatio->value();
    int srcSlices = ui->txtInsliceCount->value();
    int destSlices = ceil(srcSlices * (val/100));
    if(destSlices > 65535) {
        val = 65535*100;
        val /= srcSlices;
        ui->txtOutsliceRatio->blockSignals(true);
        ui->txtOutsliceRatio->setValue(val);
        ui->txtOutsliceRatio->blockSignals(false);
    } else {
        ui->txtOutsliceCount->blockSignals(true);
        ui->txtOutsliceCount->setValue(destSlices);
        ui->txtOutsliceCount->blockSignals(false);

        ui->txtOutsliceSize->setBytesApprox(par2SliceSize * destSlices, true);
        updateDestInfo();
    }
}
void MainWindow::txtOutsliceCount_updated(bool editingFinished)
{
    int val = ui->txtOutsliceCount->value();
    double perc = val*100;
    perc /= ui->txtInsliceCount->value();
    ui->txtOutsliceRatio->blockSignals(true);
    ui->txtOutsliceRatio->setValue(perc);
    ui->txtOutsliceRatio->blockSignals(false);
    ui->txtOutsliceSize->setBytesApprox(par2SliceSize * val, true);

    updateDestInfo(editingFinished);
}
void MainWindow::on_txtOutsliceCount_valueChanged(int arg1)
{
    ui->optOutsliceCount->blockSignals(true);
    ui->optOutsliceCount->setChecked(true);
    ui->optOutsliceCount->blockSignals(false);
    txtOutsliceCount_updated(false);
}
void MainWindow::on_txtOutsliceCount_editingFinished()
{
    if(!ui->optOutsliceCount->isChecked()) return;
    txtOutsliceCount_updated(true);
}

void MainWindow::on_txtOutsliceSize_valueChanged(quint64 size, bool finished)
{
    if(finished) {
        if(!ui->optOutsliceSize->isChecked()) return;
    } else {
        ui->optOutsliceSize->blockSignals(true);
        ui->optOutsliceSize->setChecked(true);
        ui->optOutsliceSize->blockSignals(false);
    }

    int destSlices = (size + par2SliceSize-1) / par2SliceSize; // round up
    if(destSlices > 65535) {
        destSlices = 65535;
        if(finished) {
            size = 65535 * par2SliceSize;
            ui->txtOutsliceSize->setBytes(size, true);
            return;
        }
    }
    ui->txtOutsliceCount->blockSignals(true);
    ui->txtOutsliceCount->setValue(destSlices);
    ui->txtOutsliceCount->blockSignals(false);

    double perc = destSlices*100;
    perc /= ui->txtInsliceCount->value();
    ui->txtOutsliceRatio->blockSignals(true);
    ui->txtOutsliceRatio->setValue(perc);
    ui->txtOutsliceRatio->blockSignals(false);

    updateDestInfo(finished);
}



void MainWindow::on_cboDestDist_currentIndexChanged(int index)
{
    int selection = ui->cboDestDist->currentIndex();
    if(selection == 0 || selection == 2) {
        ui->stkDestSizing->setCurrentIndex(0);
        adjustExpansion(false);
    } else {
        ui->stkDestSizing->setCurrentIndex(selection/2 + 1);
        adjustExpansion(true);
    }
    updateDestPreview();
}


void MainWindow::on_btnSourceAdv_clicked()
{
    ui->btnSourceAdv->setChecked(false);
    ui->stkSource->setCurrentIndex(1);
    ui->btnSourceAdv2->focusWidget();

    adjustExpansion(true);
    Settings::getInstance().setUiExpSource(true);
}


void MainWindow::on_btnSourceAdv2_clicked()
{
    ui->btnSourceAdv2->setChecked(true);
    ui->stkSource->setCurrentIndex(0);
    ui->btnSourceAdv->focusWidget();

    adjustExpansion(false);
    Settings::getInstance().setUiExpSource(false);
}


void MainWindow::on_btnComment_clicked()
{
    bool accepted;
    auto newComment = QInputDialog::getMultiLineText(this, tr("PAR2 Comment"), tr("Enter a comment for this PAR2"), par2Comment, &accepted);
    if(accepted) {
        par2Comment = newComment;
#ifdef Q_OS_WINDOWS
        if(par2Comment.length() > 5000) { // command limit is 32767 chars, but we'll warn the user if they could exceed the 8191 limit of cmd.exe
            QMessageBox::warning(this, tr("Set Comment"), tr("Comment has been set, however long comments may cause issues with Windows' command length limits. Keeping comments short is recommended."));
        }
#endif
        updateDestPreview();
        ui->btnComment->setText(par2Comment.isEmpty() ? tr("Set Comme&nt...") : tr("Edit Comme&nt..."));
    }
}


void MainWindow::on_btnSourceSetFiles_clicked()
{
    auto files = QFileDialog::getOpenFileNames(this, tr("Add source files"), "", tr("All files (*.*)"));
    if(!files.isEmpty()) {
        par2SrcFiles.clear();
        srcBaseChosen = false;
        sourceAddFiles(files);
        autoSelectDestFile();
        checkSourceFileCount();
        updateSrcFilesState();
    }

}


void MainWindow::on_btnSourceSetDir_clicked()
{
    auto dir = QFileDialog::getExistingDirectory(this, tr("Add source files from directory"), "");
    if(!dir.isEmpty()) {
        par2SrcFiles.clear();
        srcBaseChosen = false;
        sourceAddDir(dir);
        autoSelectDestFile();
        checkSourceFileCount();
        updateSrcFilesState();
    }
}

QString MainWindow::srcFilesCommonPath() const
{
    if(par2SrcFiles.isEmpty()) return "";
    auto keys = par2SrcFiles.keys();
    QString common = keys.at(0);
    int p = common.lastIndexOf('/');
    if(p < 0) return ""; // invalid
    common = common.left(p+1);
    for(int i=1; i<keys.length(); i++) {
        while(!keys.at(i).startsWith(common)) {
            p = common.lastIndexOf('/', -2);
            if(p < 0) return "";
            common = common.left(p+1);
        }
    }
    if(common.isEmpty()) return "";

    // retrieve proper case from source list
    for(const auto& key : keys) {
        if(key.startsWith(common)) { // should always be true
            QString name = par2SrcFiles[key].canonicalFilePath();
            return QDir(name.left(common.length())).absolutePath();
        }
    }
    return ""; // invalid
}

void MainWindow::reloadSourceFiles()
{
    int pathOpt = ui->cboSourcePaths->currentIndex();
    auto tv = ui->tvSource;
    par2SrcSize = 0;
    par2FileCount = 0;
    if(par2SrcFiles.isEmpty()) {
        tv->clear();
        return;
    }
    tv->setUpdatesEnabled(false);
    tv->clear();

    ProgressDialog progress(this, tr("Building file list..."));
    progress.setMaximum(par2SrcFiles.size() + 1);
    progress.setCancelButton(nullptr);
    // TODO: also disable window close button

    if(pathOpt == 0) {
        // add without pathing
        QList<QTreeWidgetItem*> items;
        auto srcKeys = par2SrcFiles.keys();
        for(const auto& key : srcKeys) {
            auto& file = par2SrcFiles[key];
            auto item = SourceFileListItem::create(nullptr, file);
            item->setData(0, Qt::UserRole, key);
            items.append(item);

            if(file.size()) {
                par2SrcSize += file.size();
                par2FileCount++;
            }
            file.par2name = file.fileName();
            progress.inc();
        }
        tv->addTopLevelItems(items);
    } else {
        // if absolute, add base path as first item
        QString basePath = "";
        bool isRelative = (pathOpt == 1);
        if(isRelative) {
            basePath = ui->txtSourcePath->text();
            basePath = basePath.replace(QDir::separator(), "/");
            if(!basePath.isEmpty()) {
                if(!basePath.endsWith('/')) basePath += "/";
            } else // if there's no common path defined, treat as absolute
                isRelative = false;
        }
        // TODO: for absolute paths (and relative as well) support merging single directories
        QDir baseDir(basePath);

        QHash<QString, QTreeWidgetItem*> dirNodes;
        QList<QTreeWidgetItem*> topNodes;
        auto it = QMutableHashIterator<QString, SourceFile>(par2SrcFiles);
        while(it.hasNext()) {
            it.next();
            auto& file = it.value();

            auto relPath = isRelative ? baseDir.relativeFilePath(file.canonicalFilePath()) : file.canonicalFilePath();
#ifdef Q_OS_WINDOWS
            auto pathKey = relPath.toLower().split('/');
#else
            auto pathKey = relPath.split('/');
#endif
            if(pathKey.length() > 1) {
                // child node - ensure folder nodes are present
                QString key = pathKey[0];
                if(!dirNodes.contains(key)) {
                    // create top level directory
                    auto dirItem = SourceFileListItem::create(nullptr, relPath.left(key.length()));
                    topNodes.append(dirItem);
                    dirNodes.insert(key, dirItem);
                }
                for(int i=1; i<pathKey.length()-1; i++) {
                    QString newKey = key + "/" + pathKey[i];
                    if(!dirNodes.contains(newKey)) {
                        auto dirItem = SourceFileListItem::create(
                                    dirNodes[key],
                                    relPath.mid(key.length()+1, pathKey[i].length())
                                    );
                        dirNodes.insert(newKey, dirItem);
                        dirNodes[key]->addChild(dirItem);
                    }
                    key = newKey;
                }
                // insert actual node
                auto newItem = SourceFileListItem::create(dirNodes[key], file);
                newItem->setData(0, Qt::UserRole, it.key());
                dirNodes[key]->addChild(newItem);

                file.par2name = relPath;
                file.par2name.replace("/", QDir::separator());
            } else {
                // top level file
                auto newItem = SourceFileListItem::create(nullptr, file);
                newItem->setData(0, Qt::UserRole, it.key());
                topNodes.append(newItem);

                file.par2name = file.fileName();
            }

            if(file.size()) {
                par2SrcSize += file.size();
                par2FileCount++;
            }
            progress.inc();
        }
        tv->addTopLevelItems(topNodes);
        // expand top-level folders
        for(auto node : topNodes) {
            if(node->childCount() > 0)
                node->setExpanded(true);
        }
    }
    tv->setUpdatesEnabled(true);
    progress.end();
}

static QString wrapFile(const QString& val)
{
    if(!val.isEmpty() && val.at(0) == '-')
        return QString(".") + QDir::separator() + val;
    return val;
}

QStringList MainWindow::getCmdArgs(QHash<QString, QString>& env) const
{
    QStringList list;
    // slice sizing
    if(ui->optInsliceSize->isChecked())
        list << "--input-slices" << ui->txtInsliceSize->getSizeString();
    if(ui->optInsliceCount->isChecked())
        list << "--input-slices" << QString::number(ui->txtInsliceCount->value());
    if(ui->optOutsliceRatio->isChecked())
        list << "--recovery-slices" << ui->txtOutsliceRatio->text();
    if(ui->optOutsliceCount->isChecked())
        list << "--recovery-slices" << QString::number(ui->txtOutsliceCount->value());
    if(ui->optOutsliceSize->isChecked())
        list << "--recovery-slices" << ui->txtOutsliceSize->getSizeString();

    // input options
    switch(ui->cboSourcePaths->currentIndex()) {
    case 0: list << "--filepath-format" << "basename"; break;
    case 1: list << "--filepath-format" << "path" << "--filepath-base" << wrapFile(ui->txtSourcePath->text()); break;
    case 2: list << "--filepath-format" << "keep"; break;
    }

    // output options
    list << "--out" << wrapFile(ui->txtDestFile->text()) << "--overwrite";
    if(ui->txtDestOffset->value() > 0)
        list << "--recovery-offset" << QString::number(ui->txtDestOffset->value());
    if(ui->cboDestDist->isEnabled()) {
        switch(ui->cboDestDist->currentIndex()) {
        case 0: list << "--slice-dist" << "equal" << "--noindex"; break;
        case 1: list << "--slice-dist" << "uniform";
            if(ui->optDestFiles->isChecked())
                list << "--recovery-files" << QString::number(ui->txtDestFiles->value());
            if(ui->optDestCount->isChecked())
                list << "--slices-per-file" << QString::number(ui->txtDestCount->value());
            if(ui->optDestSize->isChecked())
                list << "--slices-per-file" << ui->txtDestSize->getSizeString();
            break;
        case 2: list << "--slice-dist" << "pow2"; break;
        case 3: list << "--slice-dist" << "pow2";
            if(ui->optDestMaxLfile->isChecked())
                list << "--slices-per-file" << "1l";
            if(ui->optDestMaxCount->isChecked())
                list << "--slices-per-file" << QString::number(ui->txtDestMaxCount->value());
            if(ui->optDestMaxSize->isChecked())
                list << "--slices-per-file" << ui->txtDestMaxSize->getSizeString();
            break;
        }
    }

    if(!par2Comment.isEmpty()) {
        if(par2Comment.at(0) == '-')
            list << QString("--comment=") + par2Comment;
        else
            list << "--comment" << par2Comment;
    }

    // processing options
    const auto& settings = Settings::getInstance();
    if(settings.unicode() != AUTO)
        list << (settings.unicode() == INCLUDE ? "--unicode" : "--no-unicode");
    if(settings.charset() != "utf8")
        list << "--ascii-charset" << settings.charset();
    if(settings.packetRepMin() != 1)
        list << "--min-packet-redundancy" << QString::number(settings.packetRepMin());
    if(settings.packetRepMax() != 16 && settings.packetRepMin() < 16)
        list << "--max-packet-redundancy" << QString::number(settings.packetRepMax());
    if(settings.stdNaming())
        list << "--std-naming";
    // TODO: should we include the slice size multiple?

    if(settings.outputSync())
        list << "--write-sync";

    list << "--seq-read-size" << settings.readSize()
         << "--read-buffers" << QString::number(settings.readBuffers())
         << "--read-hash-queue" << QString::number(settings.hashQueue())
         << "--min-chunk-size" << settings.minChunk()
         << "--chunk-read-threads" << QString::number(settings.chunkReadThreads())
         << "--recovery-buffers" << QString::number(settings.recBuffers())
         << "--md5-batch-size" << QString::number(settings.hashBatch())
         << "--cpu-minchunk" << settings.cpuMinChunk();
    if(settings.hashMethod() != "auto")
        list << "--hash-method" << settings.hashMethod();
    if(settings.md5Method() != "auto")
        list << "--md5-method" << settings.md5Method();
    if(settings.procBatch() >= 0)
        list << "--proc-batch-size" << QString::number(settings.procBatch());
    if(!settings.memLimit().isEmpty())
        list << "--memory" << settings.memLimit();
    if(!settings.gfMethod().isEmpty())
        list << "--method" << settings.gfMethod();
    if(!settings.tileSize().isEmpty())
        list << "--loop-tile-size" << settings.tileSize();
    if(settings.threadNum() >= 0)
        list << "--threads" << QString::number(settings.threadNum());

    const auto openclDevices = settings.openclDevices();
    for(const auto& dev : openclDevices) {
        QString devLine("device=%1,process=%2,minchunk=%3,method=%4");
        QString devName(dev.name);
        devName.remove(',');
        devLine = devLine.arg(devName, QString::number(dev.alloc) + "%", dev.minChunk, dev.gfMethod);
        if(!dev.memLimit.isEmpty())
            devLine += QString(",memory=") + dev.memLimit;
        if(dev.batch)
            devLine += QString(",batch-size=") + QString::number(dev.batch);
        if(dev.iters)
            devLine += QString(",iter-count=") + QString::number(dev.iters);
        if(dev.outputs)
            devLine += QString(",grouping=") + QString::number(dev.outputs);
        list << "--opencl" << devLine;
    }

    if(settings.chunkReadThreads() >= 3)
        env.insert("UV_THREADPOOL_SIZE", QString::number(settings.chunkReadThreads()+2));

    return list;
}
QByteArray MainWindow::getCmdFilelist(bool nullSep) const
{
    QByteArray fileList;
    fileList.reserve(par2SrcFiles.size() * 256); // rough allocation
    auto it = QHashIterator<QString, SourceFile>(par2SrcFiles);
    while(it.hasNext()) {
        it.next();
        auto file = it.value().canonicalFilePath();
        fileList.append(file.replace("/", QDir::separator()).toUtf8()); // TODO: do we want to support UCS2 in case of badly encoded filenames? does that even work in Qt?
        if(nullSep)
            fileList.append(static_cast<char>(0));
        else
            fileList.append("\r\n", 2);
    }
    return fileList;
}

void MainWindow::on_txtSourcePath_editingFinished()
{
    // TODO: simply losing focus sets this - it should only be set if some change was made
    srcBaseChosen = !ui->txtSourcePath->text().isEmpty();
    reloadSourceFiles();
    updateDestPreview();
}


void MainWindow::on_btnSourceRefresh_clicked()
{
    ProgressDialog progress(this, tr("Refreshing file info..."));
    progress.setMaximum(par2SrcFiles.size());

    auto keys = par2SrcFiles.keys();
    bool changed = false;
    for(const auto& key : keys) {
        if(progress.wasCanceled()) break;
        changed = changed || par2SrcFiles[key].refresh();
        if(!par2SrcFiles[key].exists())
            par2SrcFiles.remove(key);
        progress.inc();
    }
    progress.end();
    if(changed) {
        reloadSourceFiles();
        updateSrcFilesState();
    }
}


void MainWindow::on_txtDestFiles_valueChanged(int arg1)
{
    ui->optDestFiles->blockSignals(true);
    ui->optDestFiles->setChecked(true);
    ui->optDestFiles->blockSignals(false);

    int slices = ui->txtOutsliceCount->value();
    if(arg1 > slices) arg1 = slices;
    int slicesPerFile = (slices + arg1-1) / arg1;
    ui->txtDestCount->blockSignals(true);
    ui->txtDestCount->setValue(slicesPerFile);
    ui->txtDestCount->blockSignals(false);
    // TODO: the following needs to include PAR2 overheads!
    ui->txtDestSize->setBytesApprox(slicesPerFile * par2SliceSize, true);

    if(ui->stkDestSizing->currentIndex() == 1)
        updateDestPreview();
}
void MainWindow::on_txtDestCount_valueChanged(int arg1)
{
    ui->optDestCount->blockSignals(true);
    ui->optDestCount->setChecked(true);
    ui->optDestCount->blockSignals(false);

    int slices = ui->txtOutsliceCount->value();
    if(arg1 > slices) arg1 = slices;
    int files = (slices + arg1-1) / arg1;
    ui->txtDestFiles->blockSignals(true);
    ui->txtDestFiles->setValue(files);
    ui->txtDestFiles->blockSignals(false);

    int slicesPerFile = (slices + files-1) / files;
    // TODO: the following needs to include PAR2 overheads!
    ui->txtDestSize->setBytesApprox(slicesPerFile * par2SliceSize, true);

    if(ui->stkDestSizing->currentIndex() == 1)
        updateDestPreview();
}
void MainWindow::on_txtDestSize_valueChanged(quint64 size, bool finished)
{
    if(finished) {
        if(!ui->optDestSize->isChecked()) return;
    } else {
        ui->optDestSize->blockSignals(true);
        ui->optDestSize->setChecked(true);
        ui->optDestSize->blockSignals(false);
    }

    // TODO: the following needs to include PAR2 overheads!
    int slices = ui->txtOutsliceCount->value();
    int slicesPerFile = (size + par2SliceSize/2) / par2SliceSize; // we'll do rounding here to be consistent with the other size option; ParPar allows all ceil/floor/round
    if(slicesPerFile > slices) {
        slicesPerFile = slices;
        if(finished) {
            quint64 newSize = slices * par2SliceSize;
            ui->txtDestSize->setBytes(newSize, true);
            return;
        }
    }
    if(slicesPerFile < 1) {
        slicesPerFile = 1;
        if(finished) {
            ui->txtDestSize->setBytes(par2SliceSize, true);
            return;
        }
    }
    int files = (slices + slicesPerFile-1) / slicesPerFile;
    ui->txtDestFiles->blockSignals(true);
    ui->txtDestFiles->setValue(files);
    ui->txtDestFiles->blockSignals(false);

    slicesPerFile = (slices + files-1) / files;
    ui->txtDestCount->blockSignals(true);
    ui->txtDestCount->setValue(slicesPerFile);
    ui->txtDestCount->blockSignals(false);

    if(ui->stkDestSizing->currentIndex() == 1)
        updateDestPreview();
}


void MainWindow::on_txtDestMaxCount_valueChanged(int arg1)
{
    ui->optDestMaxCount->blockSignals(true);
    ui->optDestMaxCount->setChecked(true);
    ui->optDestMaxCount->blockSignals(false);

    if(arg1 > 32768) arg1 = 32768;
    // TODO: need to include PAR2 overheads
    ui->txtDestMaxSize->setBytesApprox(arg1 * par2SliceSize, true);

    if(ui->stkDestSizing->currentIndex() == 3)
        updateDestPreview();
}
void MainWindow::on_txtDestMaxSize_valueChanged(quint64 size, bool finished)
{
    if(finished) {
        if(!ui->optDestMaxSize->isChecked()) return;
    } else {
        ui->optDestMaxSize->blockSignals(true);
        ui->optDestMaxSize->setChecked(true);
        ui->optDestMaxSize->blockSignals(false);
    }

    // TODO: need to include PAR2 overheads
    int slicesFile = (size + par2SliceSize/2) / par2SliceSize;
    if(slicesFile > 32768) {
        slicesFile = 32768;
        if(finished) {
            ui->txtDestMaxSize->setBytes(32768 * par2SliceSize, true);
            return;
        }
    }
    ui->txtDestMaxCount->blockSignals(true);
    ui->txtDestMaxCount->setValue(slicesFile);
    ui->txtDestMaxCount->blockSignals(false);

    if(ui->stkDestSizing->currentIndex() == 3)
        updateDestPreview();
}



void MainWindow::on_btnCopyCmd_clicked()
{
    QString cmd = Settings::getInstance().parparBin().join(" ");

    QHash<QString, QString> env;
    auto args = getCmdArgs(env);
    for(const auto& arg : args)
        cmd += QString(" ") + escapeShellArg(arg);

    // input files
    QString fileList;
    auto it = QHashIterator<QString, SourceFile>(par2SrcFiles);
    while(it.hasNext()) {
        it.next();
        auto file = it.value().canonicalFilePath();
        fileList += QString(" ") + escapeShellArg(file.replace("/", QDir::separator()));
    }

    if(!env.empty()) {
        auto it = QHashIterator<QString, QString>(env);
        QString envStr = "";
        while(it.hasNext()) {
            it.next();
#ifdef Q_OS_WINDOWS
            envStr += QString("SET %1=%2\r\n").arg(it.key(), it.value());
#else
            envStr += it.key() + "=" + escapeShellArg(it.value()) + " ";
#endif
        }
        cmd = envStr + cmd;
    }

#ifdef Q_OS_WINDOWS
    // handle max limits, from https://stackoverflow.com/questions/3205027/maximum-length-of-command-line-string
    if(cmd.length()+fileList.length() > 8191) {
        if(cmd.length() < 7500) {
            // offer to write out file list
            auto answer = QMessageBox::warning(this, tr("Copy Command"), tr("The generated command would likely exceed Windows' maximum command length. Would you like to write the list of files to a file, and reference that instead?"), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            if(answer == QMessageBox::Cancel) return;
            if(answer == QMessageBox::Yes) {
                QTemporaryFile listFile(QDir::tempPath() + "/parpargui-filelist.txt");
                listFile.setAutoRemove(false);
                listFile.open();
                listFile.write(getCmdFilelist(false));
                listFile.close();
                QGuiApplication::clipboard()->setText(cmd + " --input-file " + listFile.fileName().replace("/", QDir::separator()));
                QMessageBox::information(this, tr("Copy Command"), tr("The ParPar command has been copied to the clipboard."));

                return;
            }
        }

        QGuiApplication::clipboard()->setText(cmd + fileList);
        QMessageBox::warning(this, tr("Copy Command"), tr("The ParPar command has been copied to the clipboard, but may exceed Windows' maximum command length."));
        return;
    }
#endif

    QGuiApplication::clipboard()->setText(cmd + fileList);
    QMessageBox::information(this, tr("Copy Command"), tr("The ParPar command has been copied to the clipboard."));
}

QList<Par2RecoveryFile> MainWindow::getOutputFiles()
{
    if(par2FileCount == 0)
        return outPreview.getOutputList(0, 0, 0, 0);

    int sliceCount = ui->txtOutsliceCount->value();
    int sliceLimit = 32768;
    int distMode = ui->cboDestDist->currentIndex();
    if(distMode == 1)
        sliceLimit = ui->txtDestFiles->value();
    if(distMode == 3)
        sliceLimit = ui->txtDestMaxCount->value();
    return outPreview.getOutputList(sliceCount, distMode, sliceLimit, ui->txtDestOffset->value());
}

void MainWindow::on_btnCreate_clicked()
{
    if(!checkSourceFileCount(tr("Create PAR2")))
        return;

    // check dupe filenames (should only be possible if path names are discarded, but always double-check anyway)
    QSet<QString> srcNames;
    QSet<QString> dupeNames;
    QHashIterator<QString, SourceFile> it(par2SrcFiles);
    while(it.hasNext()) {
        it.next();
        const auto& name = it.value().par2name;
        if(srcNames.contains(name))
            dupeNames.insert(name);
        else
            srcNames.insert(name);
    }
    if(dupeNames.size() > 0) {
        QMessageBox::warning(this, tr("Create PAR2"), tr("%1 duplicate file name(s) were found in the source file list. Please remove all duplicate names before proceeding.")
                                                   .arg(dupeNames.size()));
        return;
    }


    QFileInfo output(ui->txtDestFile->text());
    auto outputFiles = getOutputFiles();

    QString outputBaseName = output.fileName();
    if(outputBaseName.endsWith(".par2", Qt::CaseInsensitive))
        outputBaseName = outputBaseName.left(outputBaseName.length()-5);
    int sliceCount = ui->txtOutsliceCount->value();
    int distMode = ui->cboDestDist->currentIndex();

    QStringList outputFilenames;
    outputFilenames.reserve(outputFiles.count());
    // check if any file would be overwritten
    QString exists;
    bool tooLong = false;
    QDir outputDir = output.dir();
    for(const auto& outFile : outputFiles) {
        QString name = outputBaseName + Par2OutInfo::fileExt(outFile.count, outFile.offset, sliceCount);
        if(distMode == 0)
            name = output.fileName();
        if(outputDir.exists(name)) {
            exists += QString("\n") + name;
        }
        outputFilenames.append(name);

        // assume Windows allows 255 UCS2 chars, otherwise 255 bytes, in the file name
#ifdef _WINDOWS
        if(name.length() > 255)
#else
        if(name.toLocal8Bit().length() > 255)
#endif
            tooLong = true;
    }
    if(tooLong) {
        QMessageBox::warning(this, tr("Create PAR2"), tr("One or more output file names are too long. Please shorten the output file name."));
        return;
    }
    if(!exists.isEmpty()) {
        auto answer = QMessageBox::warning(this, tr("Create PAR2"), tr("The following output file(s) already exist. Do you want to overwrite them?%1")
                                           .arg(exists), QMessageBox::Yes | QMessageBox::No);
        if(answer != QMessageBox::Yes) return;
    }

    // ensure directories for output exist
    if(!outputDir.exists(".") && !outputDir.mkpath(".")) {
        QMessageBox::critical(this, tr("Create PAR2"), tr("Failed to create directory for output."));
        return;
    }

    // launch progress window
    QHash<QString, QString> env;
    auto args = getCmdArgs(env);
    CreateProgress w(this);
    w.run(args, env, getCmdFilelist(true), output.fileName(), output.absolutePath(), outputFilenames);
    w.exec();
}


void MainWindow::on_txtDestFile_textEdited(const QString &arg1)
{
    destFileChosen = !arg1.isEmpty();
    updateSrcFilesState();
}

void MainWindow::autoSelectDestFile()
{
    if(destFileChosen) return;
    if(par2SrcFiles.isEmpty()) return;

    QString target;
    // if single file, use that name
    if(par2SrcFiles.size() == 1) {
        const auto& file = par2SrcFiles.cbegin().value();
        target = QDir(file.canonicalPath()).absoluteFilePath(Par2OutInfo::nameSafeLen(file.completeBaseName()));
    } else {
        // otherwise, use common path
        QDir dir(ui->txtSourcePath->text());
        if(dir.dirName().isEmpty()) return; // root directory - no name available
        target = QDir(dir.canonicalPath()).absoluteFilePath(Par2OutInfo::nameSafeLen(dir.dirName()));
    }

    target.replace("/", QDir::separator());

    if(QFile::exists(target + ".par2")) {
        for(int i=2; i<10; i++) {
            QString testName = target + " - " + QString::number(i) + ".par2";
            if(!QFile::exists(testName)) {
                ui->txtDestFile->setText(testName);
                return;
            }
        }
    }
    ui->txtDestFile->setText(target + ".par2");
}

bool MainWindow::checkSourceFileCount(const QString& title)
{
    if(par2FileCount > 32768) {
        QMessageBox::warning(this,
                             title.isEmpty() ? tr("Add source files") : title,
                             tr("PAR2 supports a maximum of 32768 (non-empty) files per archive. Currently, there are %1 files loaded. Please remove files to bring the count under the limit before proceeding.")
                             .arg(par2FileCount));
        return false;
    }
    if(par2FileCount > optionSliceLimit) {
        QMessageBox::warning(this,
                             title.isEmpty() ? tr("Add source files") : title,
                             tr("A slice count limit of %1 has been set, which is less than the %2 currently loaded files. Please remove files to bring the count under the limit before proceeding.")
                             .arg(optionSliceLimit, par2FileCount));
        return false;
    }
    return true;
}

void MainWindow::updateSrcFilesState()
{
    bool hasFiles = !par2SrcFiles.isEmpty();
    bool hasSource = par2FileCount > 0;
    bool hasDest = !ui->txtDestFile->text().isEmpty();
    ui->grpSrcData->setEnabled(hasSource);
    ui->grpRecData->setEnabled(hasSource);
    //ui->btnDestPreview->setEnabled(hasSource);
    ui->btnCopyCmd->setEnabled(hasFiles && hasDest);
    ui->btnCreate->setEnabled(hasFiles && hasDest);

    QString infoStr;
    if(!par2SrcFiles.isEmpty()) {
        if(par2SrcFiles.size() > par2FileCount) {
            infoStr = tr("%1 / %2+%3 file(s)").arg(
                        friendlySize(par2SrcSize),
                        QLocale().toString(par2FileCount),
                        QLocale().toString(par2SrcFiles.size()-par2FileCount));
        } else {
            infoStr = tr("%1 / %2 file(s)").arg(
                        friendlySize(par2SrcSize),
                        QLocale().toString(par2SrcFiles.size()));
        }
    }
    ui->lblSourceInfo->setText(infoStr);
    ui->lblSourceInfo2->setText(infoStr);

    if(hasSource) {
        updateInsliceInfo();
        // the update will appropriately enable the sizing controls
    } else {
        ui->cboDestDist->setEnabled(hasSource);
        ui->txtDestOffset->setEnabled(hasSource);
        ui->stkDestSizing->setEnabled(hasSource);
        updateDestPreview();
    }
}


void MainWindow::on_optInsliceSize_toggled(bool checked)
{
    if(!checked) return;

    int value = ui->txtInsliceCount->value();
    par2SliceSize = Par2Calc::sliceSizeFromCount(value, optionSliceMultiple, optionSliceLimit, par2SrcFiles, par2FileCount);
    ui->txtInsliceSize->setBytes(par2SliceSize, true);
}
void MainWindow::on_optInsliceCount_toggled(bool checked)
{
    if(!checked) return;

    on_txtInsliceCount_editingFinished();
}
void MainWindow::updateInsliceInfo()
{
    if(par2FileCount < 1) return;
    ui->txtInsliceCount->blockSignals(true);
    ui->txtInsliceCount->setMinimum(par2FileCount);
    ui->txtInsliceCount->setMaximum(Par2Calc::maxSliceCount(optionSliceMultiple, optionSliceLimit, par2SrcFiles));
    ui->txtInsliceCount->blockSignals(false);

    if(Settings::getInstance().allocSliceMode() == SettingsDefaultAllocIn::ALLOC_IN_RATIO) {
        double _count = (double)par2SrcSize;
        _count = sqrt(_count * Settings::getInstance().allocSliceRatio() / 100);
        int count = (std::min)((int)round(_count), optionSliceLimit);
        if(count < 1) count = 1;
        ui->txtInsliceCount->setValue(count);
        ui->optInsliceCount->setChecked(true);
        on_txtInsliceCount_editingFinished();
        return;
    }

    if(ui->optInsliceCount->isChecked())
        on_txtInsliceCount_editingFinished();
    if(ui->optInsliceSize->isChecked())
        on_txtInsliceSize_valueChanged(ui->txtInsliceSize->getBytes(), true);
}



void MainWindow::on_optOutsliceRatio_toggled(bool checked)
{
    if(!checked) return;

    on_txtOutsliceRatio_editingFinished();
}
void MainWindow::on_optOutsliceCount_toggled(bool checked)
{
    if(!checked) return;

    txtOutsliceCount_updated(true);
}
void MainWindow::on_optOutsliceSize_toggled(bool checked)
{
    if(!checked) return;

    on_txtOutsliceSize_valueChanged(ui->txtOutsliceSize->getBytes(), true);
}
void MainWindow::updateOutsliceInfo(bool setMax)
{
    int sliceCount = ui->txtInsliceCount->value();

    // also updates the 'padding' info
    quint64 padding = sliceCount * par2SliceSize - par2SrcSize;
    ui->lblInslicePadding->setText(friendlySize(padding) + " (" + QLocale().toString((double)(padding * 100) / (par2SrcSize + padding), 'f', 2) + "%)");

    double ratioMax = 6553500.0 / sliceCount;
    ui->txtOutsliceRatio->blockSignals(true);
    ui->txtOutsliceRatio->setMaximum(setMax ? ratioMax : 6553500.0);
    ui->txtOutsliceRatio->blockSignals(false);
    if(ui->optOutsliceRatio->isChecked()) {
        if(setMax)
            on_txtOutsliceRatio_editingFinished();
        else
            on_txtOutsliceRatio_valueChanged(ui->txtOutsliceRatio->value());
    }
    if(ui->optOutsliceCount->isChecked())
        on_txtOutsliceCount_valueChanged(ui->txtOutsliceCount->value());
    if(ui->optOutsliceSize->isChecked())
        on_txtOutsliceSize_valueChanged(ui->txtOutsliceSize->getBytes(), true);
}

void MainWindow::on_optDestFiles_toggled(bool checked)
{
    if(!checked) return;

    on_txtDestFiles_valueChanged(ui->txtDestFiles->value());
}
void MainWindow::on_optDestCount_toggled(bool checked)
{
    if(!checked) return;

    on_txtDestCount_valueChanged(ui->txtDestCount->value());
}
void MainWindow::on_optDestSize_toggled(bool checked)
{
    if(!checked) return;

    int slices = ui->txtDestCount->value();
    // TODO: include PAR2 overhead
    ui->txtDestSize->setBytes(slices * par2SliceSize);
}
void MainWindow::on_optDestMaxLfile_toggled(bool checked)
{
    if(!checked) return;

    quint64 maxSize = 0;
    for(const auto& file : qAsConst(par2SrcFiles)) {
        if(file.size() > maxSize)
            maxSize = file.size();
    }
    int slices = (maxSize + par2SliceSize-1) / par2SliceSize;
    if(slices > 32768) slices = 32768;
    ui->txtDestMaxCount->blockSignals(true);
    ui->txtDestMaxCount->setValue(slices);
    ui->txtDestMaxCount->blockSignals(false);

    ui->txtDestMaxSize->setBytesApprox(slices * par2SliceSize, true);

    if(ui->stkDestSizing->currentIndex() == 3)
        updateDestPreview();
}
void MainWindow::on_optDestMaxCount_toggled(bool checked)
{
    if(!checked) return;
    on_txtDestMaxCount_valueChanged(ui->txtDestMaxCount->value());
}
void MainWindow::on_optDestMaxSize_toggled(bool checked)
{
    if(!checked) return;

    int slices = ui->txtDestMaxCount->value();
    ui->txtDestMaxSize->setBytes(slices * par2SliceSize);
}

void MainWindow::updateDestInfo(bool setMax)
{
    int slices = ui->txtOutsliceCount->value();
    ui->cboDestDist->setEnabled(slices > 0);
    ui->txtDestOffset->setEnabled(slices > 0);
    ui->stkDestSizing->setEnabled(slices > 0);
    ui->txtDestOffset->setMaximum(65535 - (setMax ? slices : 0));
    if(slices > 0) {
        ui->txtDestFiles->blockSignals(true);
        ui->txtDestFiles->setMaximum(setMax ? slices : 65535);
        ui->txtDestFiles->blockSignals(false);
        ui->txtDestCount->blockSignals(true);
        ui->txtDestCount->setMaximum(setMax ? slices : 65535);
        ui->txtDestCount->blockSignals(false);
        if(ui->optDestFiles->isChecked())
            on_txtDestFiles_valueChanged(ui->txtDestFiles->value());
        if(ui->optDestCount->isChecked())
            on_txtDestCount_valueChanged(ui->txtDestCount->value());
        if(ui->optDestSize->isChecked())
            on_txtDestSize_valueChanged(ui->txtDestSize->getBytes(), setMax);
        if(ui->optDestMaxLfile->isChecked())
            on_optDestMaxLfile_toggled(true);
        if(ui->optDestMaxCount->isChecked())
            on_txtDestMaxCount_valueChanged(ui->txtDestMaxCount->value());
        if(ui->optDestMaxSize->isChecked())
            on_txtDestMaxSize_valueChanged(ui->txtDestMaxSize->getBytes(), setMax);

        int sliceDist = ui->stkDestSizing->currentIndex();
        if(sliceDist == 0 || sliceDist == 2)
            updateDestPreview();
    } else
        updateDestPreview();
}

void MainWindow::updateDestPreview()
{
    if(!ui->btnDestPreview->isChecked()) return;
    if(par2SrcFiles.isEmpty()) {
        ui->tvDest->clear();
        return;
    }

    // TODO: consider moving this to a thread as updates can be slow
    int sliceCount = ui->txtOutsliceCount->value();
    int distMode = ui->cboDestDist->currentIndex();
    const auto files = getOutputFiles();

    QString fileName = QFileInfo(ui->txtDestFile->text()).fileName();
    QString baseName = fileName;
    if(baseName.endsWith(".par2", Qt::CaseInsensitive))
        baseName = baseName.left(baseName.length()-5);

    QList<QTreeWidgetItem*> items;
    items.reserve(files.size());
    QStringList itemText{"", "", "", ""};
    if(distMode == 0)
        itemText[0] = fileName;
    else {
        itemText[0] = baseName;
        itemText[0].reserve(baseName.length() + 20); // ".vol12345+12345.par2".length == 20
    }

    // input efficiency * par2SliceSize * 100
    double efficiencyCoeff = (double)(par2SrcSize *100) / ui->txtInsliceCount->value();
    QLocale locale;

    quint64 lastSize = 0;
    double lastEfficiency = 0;
    quint64 totalSize = 0;
    for(const auto& file : files) {
        if(distMode != 0) {
            itemText[0].replace(baseName.length(), 20, Par2OutInfo::fileExt(file.count, file.offset, sliceCount));
        }
        if(lastSize != file.size) {
            // for large number of files (slowest case), most files will have the same size/slices, so reuse values in such case
            itemText[1] = friendlySize(file.size);
            itemText[2] = locale.toString(file.count);
            lastEfficiency = (double)(file.count * efficiencyCoeff) / file.size;
            itemText[3] = locale.toString(lastEfficiency, 'f', 2) + "%";
            lastSize = file.size;
        }
        totalSize += file.size;
        auto item = new OutPreviewListItem(static_cast<QTreeWidgetItem*>(nullptr), itemText);
        item->setTextAlignment(1, Qt::AlignRight);
        item->setTextAlignment(2, Qt::AlignRight);
        item->setTextAlignment(3, Qt::AlignRight);
        // sort keys
        item->setData(0, Qt::UserRole, file.count == 0 && file.offset == 0 ? -1 : file.offset);
        item->setData(1, Qt::UserRole, file.size);
        item->setData(2, Qt::UserRole, file.count);
        item->setData(3, Qt::UserRole, lastEfficiency);
        items.append(item);
    }

    // total item
    if(items.count() > 1) {
        itemText[0] = tr("[Total]");
        itemText[1] = friendlySize(totalSize);
        itemText[2] = locale.toString(sliceCount);
        lastEfficiency = (double)(sliceCount * efficiencyCoeff) / totalSize;
        itemText[3] = locale.toString(lastEfficiency, 'f', 2) + "%";
        auto item = new OutPreviewListItem(static_cast<QTreeWidgetItem*>(nullptr), itemText);
        item->setTextAlignment(1, Qt::AlignRight);
        item->setTextAlignment(2, Qt::AlignRight);
        item->setTextAlignment(3, Qt::AlignRight);
        QFont boldFont;
        boldFont.setBold(true);
        item->setFont(0, boldFont);
        item->setFont(1, boldFont);
        item->setFont(2, boldFont);
        item->setFont(3, boldFont);
        // sort keys
        item->setData(0, Qt::UserRole, 65536);
        item->setData(1, Qt::UserRole, totalSize);
        item->setData(2, Qt::UserRole, sliceCount);
        item->setData(3, Qt::UserRole, lastEfficiency);
        items.append(item);
    }

    ui->tvDest->setUpdatesEnabled(false);
    ui->tvDest->clear(); // TODO: clear seems to be very slow
    ui->tvDest->addTopLevelItems(items);
    ui->tvDest->setUpdatesEnabled(true);
}


void MainWindow::on_txtDestOffset_valueChanged(int arg1)
{
    updateDestPreview();
}




void MainWindow::on_stkSource_filesDropped(const QStringList& files)
{
    bool clearExisting = ui->stkSource->currentIndex() == 0;

    if(clearExisting) {
        par2SrcFiles.clear();
        srcBaseChosen = false;
    }
    sourceAddFiles(files);
    if(clearExisting || ui->txtDestFile->text().isEmpty())
        autoSelectDestFile();
    checkSourceFileCount();
    updateSrcFilesState();
}

