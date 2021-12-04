#ifndef CREATEPROGRESS_H
#define CREATEPROGRESS_H

#include <QDialog>
#include <QProcess>
#include <QTimer>
#include <QElapsedTimer>
#include <QSystemTrayIcon>
#include <QAbstractButton>

#ifdef Q_OS_WINDOWS
#include <shobjidl.h>
#endif

namespace Ui {
class CreateProgress;
}

class CreateProgress : public QDialog
{
    Q_OBJECT

public:
    explicit CreateProgress(QWidget *parent = nullptr);
    void run(QStringList args, const QByteArray& inFiles, const QString& baseOutput_, const QString& outDir_, const QStringList& outFiles_);
    ~CreateProgress();

private slots:
    void timerTick();
    void gotStdout();
    void finished(int exitCode, QProcess::ExitStatus exitStatus);
    void notificationClicked();

    void on_CreateProgress_rejected();

    void on_btnBackground_clicked();

    void on_btnPause_clicked();

    void on_buttonBox_accepted();

    void on_btnNotify_clicked();

private:
    Ui::CreateProgress *ui;
    QProcess parpar;
    QString baseOutput;
    QString outDir;
    QStringList outFiles;

    QString stdoutBuffer;
    bool isCancelled;

    void deleteOutput();
    void ended(const QString& error, bool showOutput);

    QTimer timer;
    quint64 elapsedTimeBase;
    QElapsedTimer elapsedTime;

    void updateTitlePerc();
#ifdef Q_OS_WINDOWS
    ITaskbarList3* pTL;
    HWND hWnd;
#endif
    QSystemTrayIcon sysTrayIcon;

    void init();
    void rerun();
};

#endif // CREATEPROGRESS_H
