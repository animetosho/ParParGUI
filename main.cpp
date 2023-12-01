#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QMessageBox>
#include "config.h"

int main(int argc, char *argv[])
{
    // consistency with Qt6
#if QT_VERSION < 0x060000
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
#endif
#if QT_VERSION >= 0x050E00
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    QCoreApplication::setApplicationName(PROJECT_NAME);
    QCoreApplication::setApplicationVersion(PROJECT_VERSION);

    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "parpargui_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    if(a.arguments().contains(
#ifdef Q_OS_WINDOWS
                "/h"
#else
                "-h"
#endif
                )) {
        QMessageBox::information(NULL, a.tr("ParPar Arguments Help"), a.tr("Currently, ParParGUI only handles file name arguments, which are loaded as source files on application launch."));
        return 0;
    }

    MainWindow w;
    w.show();
    return a.exec();
}
