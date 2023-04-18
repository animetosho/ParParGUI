#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileInfo>
#include "optionsdialog.h"
#include "par2outinfo.h"
#include "sourcefile.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);

private slots:
    void on_btnAbout_clicked();

    void on_btnOptions_clicked();

    void on_txtInsliceCount_valueChanged(int arg1);

    void on_btnSourceAdd_clicked();

    void on_btnSourceAddDir_clicked();

    void on_btnSourceDel_clicked();

    void on_btnSourcePathBrowse_clicked();

    void on_btnDestPreview_clicked();

    void on_cboSourcePaths_currentIndexChanged(int index);

    void on_btnDestFileBrowse_clicked();

    void on_txtOutsliceRatio_valueChanged(double arg1);

    void on_txtOutsliceCount_valueChanged(int arg1);

    void on_cboDestDist_currentIndexChanged(int index);

    void on_btnSourceAdv_clicked();

    void on_btnSourceAdv2_clicked();

    void on_btnComment_clicked();

    void on_btnSourceSetFiles_clicked();

    void on_btnSourceSetDir_clicked();

    void on_txtSourcePath_editingFinished();

    void on_btnSourceRefresh_clicked();

    void on_txtDestFiles_valueChanged(int arg1);

    void on_txtDestCount_valueChanged(int arg1);

    void on_txtDestMaxCount_valueChanged(int arg1);

    void on_btnCopyCmd_clicked();

    void on_btnCreate_clicked();

    void on_txtDestFile_textEdited(const QString &arg1);

    void on_txtInsliceCount_editingFinished();

    void on_optInsliceSize_toggled(bool checked);

    void on_optInsliceCount_toggled(bool checked);

    void on_txtOutsliceRatio_editingFinished();

    void on_optOutsliceRatio_toggled(bool checked);

    void on_optOutsliceCount_toggled(bool checked);

    void on_optOutsliceSize_toggled(bool checked);

    void on_optDestFiles_toggled(bool checked);

    void on_optDestCount_toggled(bool checked);

    void on_optDestSize_toggled(bool checked);

    void on_optDestMaxLfile_toggled(bool checked);

    void on_optDestMaxCount_toggled(bool checked);

    void on_optDestMaxSize_toggled(bool checked);

    void on_txtDestOffset_valueChanged(int arg1);

    void on_txtOutsliceCount_editingFinished();

    void on_txtInsliceSize_valueChanged(quint64 size, bool finished);

    void on_txtOutsliceSize_valueChanged(quint64 size, bool finished);

    void on_txtDestSize_valueChanged(quint64 size, bool finished);

    void on_txtDestMaxSize_valueChanged(quint64 size, bool finished);

    void on_stkSource_filesDropped(const QStringList &);

private:
    Ui::MainWindow *ui;
    OptionsDialog dlgOptions;
    Par2OutInfo outPreview;

    void rescale();
    void adjustExpansion(bool allowExpand);

    void sourceAddFiles(const QStringList& files);
    void sourceAddDir(const QString& dir);

public: // grant access for SliceCountSpinBox / Par2OutInfo
    quint64 optionSliceMultiple;
    int optionSliceLimit;
    SrcFileList par2SrcFiles;
    quint64 par2SrcSize; // cached value
    int par2FileCount;   // cached value
    quint64 par2SliceSize; // cached value - only used for recovery slice calc
    QString par2Comment;
private:
    QString srcFilesCommonPath() const;
    void reloadSourceFiles();

    QStringList getCmdArgs(QHash<QString, QString>& env) const;
    QByteArray getCmdFilelist(bool nullSep) const;
    QList<Par2RecoveryFile> getOutputFiles();

    bool srcBaseChosen;
    bool destFileChosen;
    void autoSelectDestFile();
    bool checkSourceFileCount(const QString& title = QString());
    void updateSrcFilesState();

    void txtOutsliceCount_updated(bool editingFinished);

    void updateInsliceInfo();
    void updateOutsliceInfo(bool setMax = true);
    void updateDestInfo(bool setMax = true);
    void updateDestPreview();
};
#endif // MAINWINDOW_H
