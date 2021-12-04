#include "createprogress.h"
#include "ui_createprogress.h"
#include "settings.h"
#include <QDir>
#include <QMessageBox>

#ifdef Q_OS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
// from https://stackoverflow.com/a/11010508/459150
typedef LONG (NTAPI *NtSuspendResumeProcess)(IN HANDLE ProcessHandle);
#elif defined(Q_OS_UNIX)
#include <sys/resource.h>
#include <signal.h>
#endif

#ifdef Q_OS_WINDOWS
#define WIN_PROGRESS(f, ...) if(pTL) pTL->f(hWnd, __VA_ARGS__)
#else
#define WIN_PROGRESS(f, ...) (void)0
#endif

void CreateProgress::init()
{
    isCancelled = false;

    ui->lblTime->setText("");
    ui->txtMessage->hide();
    this->setFixedHeight(this->layout()->sizeHint().height());
    this->setSizeGripEnabled(false);

    elapsedTimeBase = 0;
    stdoutBuffer.clear();
    ui->progressBar->setMaximum(0);
    WIN_PROGRESS(SetProgressState, TBPF_INDETERMINATE);

    sysTrayIcon.hide();
}

CreateProgress::CreateProgress(QWidget *parent) :
    QDialog(parent, Qt::Dialog | Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint)
    , ui(new Ui::CreateProgress)
    , sysTrayIcon(this)
{
    ui->setupUi(this);

    connect(&parpar, &QProcess::readyReadStandardOutput, this, &CreateProgress::gotStdout);
    connect(&parpar, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &CreateProgress::finished);

    connect(&timer, &QTimer::timeout, this, &CreateProgress::timerTick);
    timer.setInterval(1000);

#if !defined(Q_OS_WINDOWS) && !defined(Q_OS_UNIX)
    ui->btnBackground->hide();
    ui->btnPause->hide();
#endif

#ifdef Q_OS_WINDOWS
    HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pTL));
    if (SUCCEEDED(hr)) {
        hr = pTL->HrInit();
        if (!SUCCEEDED(hr)) {
            pTL->Release();
            pTL = nullptr;
        }
    } else
        pTL = nullptr;
    // only the parent window has a taskbar icon, so need to attach it there
    // TODO: probably better to try and search for the QMainWindow instance rather than assume the parent
    hWnd = reinterpret_cast<HWND>(reinterpret_cast<QWidget*>(this->parent())->winId());
#endif

    // TODO: get application icon instead
    QPixmap p(16, 16);
    p.fill(Qt::transparent);
    sysTrayIcon.setIcon(QIcon(p));
    sysTrayIcon.setToolTip(tr("ParPar notification icon"));
    connect(&sysTrayIcon, &QSystemTrayIcon::messageClicked, this, &CreateProgress::notificationClicked);
    connect(&sysTrayIcon, &QSystemTrayIcon::activated, this, [=](QSystemTrayIcon::ActivationReason reason) {
        this->notificationClicked();
    });

    init();

    const auto& settings = Settings::getInstance();
    ui->btnBackground->setChecked(settings.runBackground());
    ui->btnNotify->setChecked(settings.runNotification());

    // TODO: consider adding decimal percentage bar: https://www.qtcentre.org/threads/70885-QProgressBar-with-in-decimal?s=bdfd413197898978b8ef6ea933c3e67e&p=307578#post307578
}

void CreateProgress::run(QStringList args, const QByteArray& inFiles, const QString& baseOutput_, const QString& outDir_, const QStringList& outFiles_)
{
    baseOutput = baseOutput_;
    outDir = outDir_;
    outFiles = outFiles_;

    auto cmd = Settings::getInstance().parparBin();
    parpar.setProgram(cmd[0]);
    if(cmd.length() > 1)
        args.prepend(cmd[1]);

    args << "--quiet" << "--progress" << "stdout" << "--input-file0" << "-";
    parpar.setArguments(args);

    this->setWindowTitle(tr("ParPar - Creating %2")
                         .arg(baseOutput));

    // send stdin when process starts
    connect(&parpar, &QProcess::started, this, [=]() {
        ui->lblStatus->setText("");
        ui->btnBackground->setEnabled(true);
        ui->btnNotify->setEnabled(true);
        ui->btnPause->setEnabled(true);

        if(ui->btnBackground->isChecked())
            this->on_btnBackground_clicked();

        this->parpar.write(inFiles);
        this->parpar.closeWriteChannel();
    });
    // TODO: disconnect this signal after start?
    // - or see if it's even needed
    connect(&parpar, &QProcess::errorOccurred, this, [this](QProcess::ProcessError err) {
        if(err == QProcess::FailedToStart) {
            // it seems that this is often called too soon - before the window has set up
            this->ended(tr("Failed to execute ParPar"), false);
            /*
            this->isCancelled = true;
            this->close();
            QMessageBox::critical(reinterpret_cast<QWidget*>(this->parent()), tr("Create PAR2"), tr("ParPar failed to launch"));
            */
        }
        // for other errors let finished handler handle it
    });

    elapsedTime.start();
    timer.start();
    timerTick();
    parpar.start();
}

void CreateProgress::gotStdout()
{
    stdoutBuffer += QString::fromUtf8(parpar.readAllStandardOutput());

    // find last % and update display
    int p = stdoutBuffer.lastIndexOf('%');
    if(p > 0) {
        // extract percentage
        int i = p;
        while(i--) {
            auto c = stdoutBuffer.at(i);
            if(c != '.' && !c.isDigit())
                break;
        }
        i++;
        if(i == p) return; // got a % without a preceeding number

        bool ok;
#if QT_VERSION >= 0x060000
        int progress = QStringView{stdoutBuffer}.mid(i, p-i).toFloat(&ok) * 100;
#else
        int progress = stdoutBuffer.midRef(i, p-i).toFloat(&ok) * 100;
#endif
        if(!ok) return; // why oh why???

        ui->progressBar->setMaximum(10000);
        ui->progressBar->setValue(progress);
        WIN_PROGRESS(SetProgressState, TBPF_NORMAL);
        WIN_PROGRESS(SetProgressValue, progress, 10000);

        updateTitlePerc();
        stdoutBuffer = stdoutBuffer.mid(p+1);
    }
}

void CreateProgress::updateTitlePerc()
{
    float perc = ui->progressBar->value();
    perc /= 100;
    this->setWindowTitle(tr("ParPar - %1% creating %2").arg(
                             QLocale().toString(perc, 'f', 2),
                             baseOutput
                             ));
}

CreateProgress::~CreateProgress()
{
    delete ui;
#ifdef Q_OS_WINDOWS
    WIN_PROGRESS(SetProgressState, TBPF_NOPROGRESS);
    if(pTL) pTL->Release();
#endif
}

void CreateProgress::finished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if(isCancelled) return;
    if(exitStatus != QProcess::ExitStatus::NormalExit) {
        this->ended(tr("ParPar process crashed"), true);
    } else if(exitCode != 0) {
        ended(tr("PAR2 creation failed (exit code: %1)").arg(exitCode), true);
    } else {
        ended("", false);
    }
}

void CreateProgress::ended(const QString &error, bool showOutput)
{
    bool success = error.isEmpty();
    ui->progressBar->setEnabled(false);
    ui->lblTime->setEnabled(false);
    ui->btnBackground->setEnabled(false);
    ui->btnNotify->setEnabled(false);
    ui->btnPause->setEnabled(false);
    if(success) {
        ui->lblStatus->setText(tr("PAR2 successfully created"));
        this->setWindowTitle(tr("ParPar - Finished creating %1").arg(baseOutput));
        WIN_PROGRESS(SetProgressState, TBPF_NOPROGRESS);
    } else {
        ui->lblStatus->setText(error);
        this->setWindowTitle(tr("ParPar - Failed creating %1").arg(baseOutput));
        WIN_PROGRESS(SetProgressState, TBPF_ERROR);

        ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Retry);
    }
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Close"));
    if(ui->progressBar->maximum() == 0) {
        // process never started - stop scrolling progress bar
        ui->progressBar->setMaximum(1);
        ui->progressBar->setTextVisible(false);
        WIN_PROGRESS(SetProgressState, TBPF_NOPROGRESS);
    } else {
        quint64 time = elapsedTimeBase+elapsedTime.elapsed();
        ui->lblTime->setText(tr("Time taken: %1:%2:%3%4%5")
                             .arg(time/3600000, 2, 10, QChar('0'))
                             .arg((time/60000)%60, 2, 10, QChar('0'))
                             .arg((time/1000)%60, 2, 10, QChar('0'))
                             .arg(QLocale().decimalPoint())
                             .arg(time%1000, 3, 10, QChar('0')));
    }
    ui->btnPause->setChecked(false);

    timer.stop();

    if(isCancelled) return;

    if(showOutput) {
        auto output = QString::fromUtf8(parpar.readAllStandardError()).trimmed();
        if(!output.isEmpty()) {
            ui->txtMessage->setPlainText(output);
            ui->txtMessage->show();

            this->setMinimumHeight(this->minimumHeight() + ui->txtMessage->minimumHeight() + this->layout()->spacing());
            this->setMaximumHeight(INT_MAX);
            this->resize(this->width(), this->layout()->sizeHint().height());

            this->setSizeGripEnabled(true);
        }
    }

    if(!success) deleteOutput();
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setFocus();

    if(ui->btnNotify->isChecked()) {
        if(QSystemTrayIcon::supportsMessages()) {
            sysTrayIcon.show();
            sysTrayIcon.showMessage(this->windowTitle(), ui->lblStatus->text(), success ? QSystemTrayIcon::Information : QSystemTrayIcon::Warning);
        } else {
            QApplication::beep();
        }
        QApplication::alert(this);
    }
}

void CreateProgress::deleteOutput()
{
    QString failed;
    QDir dir(outDir);
    for(const auto& name : outFiles) {
        if(dir.exists(name)) {
            if(!dir.remove(name))
                failed += QString("\r\n") + name;
        }
    }
    if(!failed.isEmpty()) {
        QMessageBox::critical(this, tr("Delete Output Files"), tr("Failed to delete the following incomplete file(s):%1")
                              .arg(failed));
    }
}

void CreateProgress::timerTick()
{
    quint64 time = (elapsedTimeBase+elapsedTime.elapsed() + 500) / 1000;
    ui->lblTime->setText(tr("Elapsed time: %1:%2:%3")
                         .arg(time/3600, 2, 10, QChar('0'))
                         .arg((time/60)%60, 2, 10, QChar('0'))
                         .arg(time%60, 2, 10, QChar('0')));
}

void CreateProgress::on_CreateProgress_rejected()
{
    if(parpar.state() == QProcess::Running) {
        isCancelled = true;
        // TODO: look at using .terminate() for non-Windows
        parpar.kill();
        ended(tr("Cancelled"), false);
        deleteOutput();
    }
}


void CreateProgress::on_btnBackground_clicked()
{
    bool isBackground = ui->btnBackground->isChecked();

#ifdef Q_OS_WINDOWS
    static DWORD normPrio = 0x7fff;
    HANDLE hPP = OpenProcess(PROCESS_SET_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, parpar.processId());
    if(hPP == NULL) {
        ui->btnBackground->setEnabled(false);
        ui->btnBackground->setChecked(!isBackground);
        return;
    }
    if(normPrio == 0x7fff) {
        normPrio = GetPriorityClass(hPP);
        if(!normPrio) {
            // failed to retrieve, disable option
            ui->btnBackground->setEnabled(false);
            ui->btnBackground->setChecked(false);
            CloseHandle(hPP);
            return;
        }
        if(normPrio == IDLE_PRIORITY_CLASS) {
            // already at low priority
            ui->btnBackground->setEnabled(false);
            ui->btnBackground->setChecked(true);
            CloseHandle(hPP);
            return;
        }
    }
    if(!SetPriorityClass(hPP, isBackground ? IDLE_PRIORITY_CLASS : normPrio)) {
        ui->btnBackground->setEnabled(!isBackground);
    }
    CloseHandle(hPP);
#elif defined(Q_OS_UNIX)
    static int normPrio = 0x7fff;
    if(normPrio == 0x7fff) {
        // get the normal priority level (assume we start there)
        errno = 0;
        normPrio = getpriority(PRIO_PROCESS, parpar.processId());
        if(normPrio == -1 && errno) {
            // failed to retrieve, disable option
            ui->btnBackground->setEnabled(false);
            ui->btnBackground->setChecked(false);
            return;
        }
        if(normPrio >= 19) {
            // already at low priority
            ui->btnBackground->setEnabled(false);
            ui->btnBackground->setChecked(true);
            return;
        }
    }

    if(setpriority(PRIO_PROCESS, parpar.processId(), isBackground ? 19 : normPrio)) {
        ui->btnBackground->setEnabled(!isBackground);
    }
#endif

    Settings::getInstance().setRunBackground(isBackground);
}


void CreateProgress::on_btnPause_clicked()
{
    bool isPaused = ui->btnPause->isChecked();

#ifdef Q_OS_WINDOWS
    static NtSuspendResumeProcess pfnNtSuspendProcess = nullptr;
    static NtSuspendResumeProcess pfnNtResumeProcess = nullptr;
    if(!pfnNtSuspendProcess) {
        HMODULE ntdll = GetModuleHandleA("ntdll");
        pfnNtSuspendProcess = (NtSuspendResumeProcess)GetProcAddress(ntdll, "NtSuspendProcess");
        pfnNtResumeProcess = (NtSuspendResumeProcess)GetProcAddress(ntdll, "NtResumeProcess");
    }
    if(!pfnNtSuspendProcess || !pfnNtResumeProcess) {
        ui->btnPause->setEnabled(false);
        ui->btnPause->setChecked(!isPaused);
        return;
    }
    HANDLE hPP = OpenProcess(PROCESS_ALL_ACCESS, FALSE, parpar.processId());
    if(hPP == NULL) {
        ui->btnPause->setEnabled(false);
        ui->btnPause->setChecked(!isPaused);
        return;
    }
#endif

    if(isPaused) {
#ifdef Q_OS_WINDOWS
        pfnNtSuspendProcess(hPP);
        CloseHandle(hPP);
#elif defined(Q_OS_UNIX)
        if(kill(parpar.processId(), SIGSTOP)) {
            ui->btnPause->setChecked(!isPaused);
            return;
        }
#endif

        this->setWindowTitle(tr("ParPar - Paused creating %1").arg(baseOutput));
        ui->lblStatus->setText(tr("Paused"));
        elapsedTimeBase += elapsedTime.elapsed();
        timer.stop();
    } else {
#ifdef Q_OS_WINDOWS
        pfnNtResumeProcess(hPP);
        CloseHandle(hPP);
#elif defined(Q_OS_UNIX)
        if(kill(parpar.processId(), SIGCONT)) {
            ui->btnPause->setChecked(!isPaused);
            return;
        }
#endif

        updateTitlePerc();
        ui->lblStatus->setText("");
        elapsedTime.start();
        timer.start();
        timerTick();
    }
    //ui->progressBar->setDisabled(isPaused);
    ui->lblTime->setDisabled(isPaused);
    WIN_PROGRESS(SetProgressState, isPaused ? TBPF_PAUSED : TBPF_NORMAL);
}

// TODO: also hide this when the window is activated
void CreateProgress::notificationClicked()
{
    sysTrayIcon.hide();
    this->activateWindow();
}

void CreateProgress::rerun()
{
    init();
    this->setWindowTitle(tr("ParPar - Creating %2")
                         .arg(baseOutput));

    ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    ui->progressBar->setEnabled(true);
    ui->lblTime->setEnabled(true);
    ui->progressBar->setTextVisible(true);

    elapsedTime.restart();
    parpar.start();
    timer.start();
    timerTick();
}

void CreateProgress::on_buttonBox_accepted()
{
    rerun();
}


void CreateProgress::on_btnNotify_clicked()
{
    Settings::getInstance().setRunNotification(ui->btnNotify->isChecked());
}

