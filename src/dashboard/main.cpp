#include "Dialog.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    if (!QSystemTrayIcon::isSystemTrayAvailable())
    {
        QMessageBox::critical(nullptr, QObject::tr("Dash'd"), QObject::tr("I couldn't detect any system tray on this system."));
        return 1;
    }

    Dialog w;
    return a.exec();
}
