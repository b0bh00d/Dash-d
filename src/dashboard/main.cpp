#include "Dialog.h"

#include <QApplication>

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QApplication a(argc, argv);

    if (!QSystemTrayIcon::isSystemTrayAvailable())
    {
        QMessageBox::critical(nullptr, QObject::tr("Dash'd"), QObject::tr("I couldn't detect any system tray on this system."));
        return 1;
    }

    Dialog w;
    return a.exec();
}
