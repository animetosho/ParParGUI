#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QSettings>

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

private:
    Ui::OptionsDialog *ui;

    void loadSettings();
};

#endif // OPTIONSDIALOG_H
