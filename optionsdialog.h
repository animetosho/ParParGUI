#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QSettings>
#include <QTreeWidgetItem>
#include <QHash>
#include "settings.h"

namespace Ui {
class OptionsDialog;
}

class OptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OptionsDialog(QWidget *parent = nullptr);
    ~OptionsDialog();

signals:
    void settingsUpdated(bool binaryChanged);

public slots:
    void accept() override;

private slots:
    void on_chkProcBatch_stateChanged(int arg1);

    void on_chkMemLimit_stateChanged(int arg1);

    void on_chkProcKernel_stateChanged(int arg1);

    void on_chkThreads_stateChanged(int arg1);

    void on_btnReset_clicked();

    void on_chkPathNode_toggled(bool checked);

    void on_btnPathParPar_clicked();

    void on_btnPathNode_clicked();

    void on_chkSliceMultiple_toggled(bool checked);

    void on_chkTileSize_stateChanged(int arg1);

    void on_txtReadBufs_editingFinished();

    void on_txtRecBufs_editingFinished();

    void on_tabWidget_currentChanged(int index);

    void on_treeDevices_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

    void on_txtPacketMin_valueChanged(int arg1);

    void on_cboIOPreset_currentIndexChanged(int index);

    void on_txtReadSize_valueChanged(quint64 , bool );

    void on_txtReadBufs_valueChanged(int arg1);

    void on_txtHashQueue_valueChanged(int arg1);

    void on_txtChunkReadThreads_valueChanged(int arg1);

    void on_txtRecBufs_valueChanged(int arg1);

    void on_chkOpencl_toggled(bool checked);

    void on_txtOclAlloc_valueChanged(double arg1);

    void on_chkOclMemory_toggled(bool checked);

    void on_chkOclBatch_toggled(bool checked);

    void on_chkOclIters_toggled(bool checked);

    void on_chkOclOutputs_toggled(bool checked);

    void on_cboAllocInMode_currentIndexChanged(int index);

    void on_cboAllocRecMode_currentIndexChanged(int index);

    void on_txtOclBatch_editingFinished();

    void on_txtOclMemory_textChanged(const QString &arg1);

    void on_cboOclKernel_currentIndexChanged(int index);

    void on_txtOclIters_editingFinished();

    void on_txtOclOutputs_editingFinished();

    void on_txtOclMinChunk_textChanged(const QString &arg1);

private:
    Ui::OptionsDialog *ui;
    QHash<QString, OpenclDevice> openclSettings;

    void loadSettings();
    void rescale();
    void checkIOPreset();
    bool devicesListed;
    void loadOpenclDevices();
    void fillDeviceList(bool opencl);
    void openclOptChanged();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

};

#endif // OPTIONSDIALOG_H
