#include <random>
// #include <iostream>

#include <QDir>
#include <QUrl>
#include <QSettings>
#include <QHostInfo>
#include <QStandardPaths>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include <QJsonDocument>
#include <QJsonObject>

#include "Collector.h"
#include "Logging.h"
#include "SharedTypes.h"

#define dumpvar(x) qDebug()<<#x<<'='<<x

Collector* collector{nullptr};      // (appears to be) required for custom log processing
QtMessageHandler originalHandler{nullptr};

Collector::Collector(int argc, char *argv[])
    : QCoreApplication(argc, argv)
{
    collector = this;

    m_start_time = QDateTime::currentDateTime();

    m_name = QHostInfo().localHostName();

    // Generate our settings file path/name
#ifdef QT_WIN
    m_settings_filename = QDir::toNativeSeparators(QString("%1/Dash-d/Settings.ini").arg(qgetenv("APPDATA").constData()));
#endif
#ifdef QT_LINUX
    auto paths = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);
    auto path = paths[0];
    m_settings_filename = QDir::toNativeSeparators(QString("%1/Dash-d.ini").arg(path));
#endif

    // Install a log message handler in case the user wants to capture
    // output to a file.
    originalHandler = qInstallMessageHandler(logHandler);
    Q_UNUSED(originalHandler)

    setApplicationName("Dash'd Collector");
    auto version = QString("%1.%2").arg(major).arg(minor);
    if(patch)
        version = QString("%1.%2").arg(version).arg(patch);
    setApplicationVersion(version);

    // Load persistent settings from a file before processing
    // commannd line options.

    load_settings();

    auto description = tr("\nUsing the following efaults:\n\t Port:\t%1\n\t IPv4:\t%2\n\t IPv6:\t%3\n\t  Log:\t%4\n\tQueue:\t%5")
        .arg(SharedTypes::MULTICAST_PORT)
        .arg(SharedTypes::MULTICAST_IPV4, SharedTypes::MULTICAST_IPV6, tr("<console>"),
#ifdef QT_LINUX
        "/tmp/dash-d"
#endif
        );
    // Process command-line options
    QCommandLineParser parser;
    parser.setApplicationDescription(description);
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption targetDirectoryOption(QStringList() << "q" << "queue-directory",
            QCoreApplication::translate("main", "Set the path to <directory> for queue entries."),
            QCoreApplication::translate("main", "DIR"));
#ifdef QT_LINUX
    targetDirectoryOption.setDefaultValue("/tmp/dash-d");
#endif
    parser.addOption(targetDirectoryOption);

    QCommandLineOption logFileOption(QStringList() << "l" << "log-directory",
            QCoreApplication::translate("main", "Specify the location to save log output."),
            QCoreApplication::translate("main", "DIR"));
    // If no log file is specified, outptut will go to the console.
    logFileOption.setDefaultValue("");
    parser.addOption(logFileOption);

    QCommandLineOption portOption(QStringList() << "P" << "port",
                                  QCoreApplication::translate("main", "Multicast port"),
                                  QCoreApplication::translate("main", "PORT"));
    portOption.setDefaultValue(QString::number(reinterpret_cast<uint16_t>(m_port)));
    parser.addOption(portOption);

    QCommandLineOption ip4Option(QStringList() << "ipv4",
            QCoreApplication::translate("main", "IPv4 address for Multicasting group."),
            QCoreApplication::translate("main", "ADDRESS"));
    ip4Option.setDefaultValue(m_ip4_group);
    parser.addOption(ip4Option);

    QCommandLineOption ip6Option(QStringList() << "ipv6",
            QCoreApplication::translate("main", "IPv6 address for Multicasting group."),
            QCoreApplication::translate("main", "ADDRESS"));
    ip4Option.setDefaultValue(m_ip6_group);
    parser.addOption(ip6Option);

    // QCommandLineOption cleanOption(QStringList() << "clean-on-startup",
    //         QCoreApplication::translate("main", "Clear all existing sensor-data files on startup."));
    // parser.addOption(cleanOption);

    QCommandLineOption detectOffline(QStringList() << "detect-offline",
            QCoreApplication::translate("main", "Heuristically attempt to detect that a Sensor has gone offline."));
    parser.addOption(detectOffline);

    QCommandLineOption updateOption(QStringList() << "update-settings",
            QCoreApplication::translate("main", "Update persistent settings with current command line options and exit."));
    parser.addOption(updateOption);

    parser.process(*this);

    m_detect_offline = parser.isSet(detectOffline);

    if(parser.isSet(updateOption))
    {
        m_queue_path = parser.value(targetDirectoryOption);
        m_log_path = parser.value(logFileOption);
        m_ip4_group = parser.value(ip4Option);
        m_ip6_group = parser.value(ip6Option);
        m_port = parser.value(portOption).toUShort();

        save_settings();

        exit(0);
    }

    // Initialization steps:
    // 1. Set up logging output (console or log file)
    // 2. Create a Watcher for the file system
    // 3. Create Sender instance for IPv4 or IPv6

    // ----- 1. Set up logging output (console or log file)

    // FYI: This needs to be initialized first, so logging macros
    // can properly redirect (if required)

    auto target_path = parser.value(logFileOption);
    if(!target_path.isEmpty())
    {
        QDir p(target_path);
        if(!p.exists())
        {
            // Ensure the full path exists
            if(!p.mkpath("."))
            {
                qCritical() << tr("Could not create directory ") << target_path << ".";
                qApp->exit(1);
                return;
            }
        }

        auto log_file_name = QString("%1/dash-d.log").arg(target_path);
        m_log = FilePtr(new QFile(log_file_name));
        if(!m_log->open(QIODevice::WriteOnly | QIODevice::Append))
        {
            qCritical() << tr("Could not create/open log file \"") << log_file_name<< "\".";
            qApp->exit(1);
            return;
        }
    }

    setLog_path(target_path);

    qInfo() << tr("─────────────────────────────┤ Start ├─────────────────────────────");
    qInfo() << applicationName() << " v" << applicationVersion();

    if(m_log.isNull())
        qInfo() << tr("Logging output to console.");
    else
        qInfo() << tr("Logging output to \"") << m_log->fileName() << "\".";

    // ----- 2. Create a Watcher for the file system
    target_path = parser.value(targetDirectoryOption);
    QDir p(target_path);
    if(!p.exists())
    {
        // Ensure the full path exists
        if(!p.mkpath("."))
        {
            qCritical() << tr("Could not create queue directory \"") << target_path << "\".";
            qApp->exit(1);
            return;
        }
    }

    setQueue_path(target_path);

    m_watcher = WatcherPtr(new QFileSystemWatcher());
    connect(m_watcher.data(), &QFileSystemWatcher::directoryChanged, this, &Collector::slot_directory_event);
    connect(m_watcher.data(), &QFileSystemWatcher::fileChanged, this, &Collector::slot_file_event);

    initialize_watcher();

    qInfo() << tr("Watching queue location \"") << target_path << "\".";

    // ----- 3. Create Sender instance for IPv4 or IPv6
    auto port = parser.value(portOption).toUShort();
    QString ip4group = parser.value(ip4Option);
    QString ip6group = parser.value(ip6Option);

    if(!ip4group.isEmpty() && !ip6group.isEmpty())
    {
        // This is an error

        qCritical() << tr("Only one of IPv4 or IPv6 may be specified.");
        qApp->exit(1);
        return;
    }
    else if(ip4group.isEmpty() && ip6group.isEmpty())
    {
        // This is an error

        qCritical() << tr("An address for either IPv4 or IPv6 must be specified.");
        qApp->exit(1);
        return;
    }

    // Create a Sender instance
    if(!ip4group.isEmpty())
    {
        // for IPv4
        m_sender = SenderPtr(new Sender(port, ip4group, QString()));
        qInfo() << tr("Sending sensor data to IPv4 multicast ") << qUtf8Printable(ip4group) << ":" << port << ".";
    }
    else
    {
        // for IPv6
        m_sender = SenderPtr(new Sender(port, QString(), ip6group));
        qInfo() << tr("Sending sensor data to IPv6 multicast ") << ip6group << ":" << port << ".";
    }

    if(m_detect_offline)
    {
        m_housekeeping = TimerPtr(new QTimer());
        m_housekeeping->setInterval(30000);
        connect(m_housekeeping.data(), &QTimer::timeout, this, &Collector::slot_housekeeping);
        m_housekeeping->start();
        qInfo() << tr("Detecting offline Sensors.");
    }
    else
        qInfo() << tr("Not detecting offline Sensors.");
}

Collector::~Collector()
{
    // Tear down what we constructed above.
    // (technically speaking, the smart pointers will
    // clean up themselves, but I like the bookkeeping)

    qInfo() << tr("Shutting down.");

    if(m_housekeeping)
    {
        m_housekeeping->stop();
        m_housekeeping.clear();
    }

    if(m_log)
        m_log.clear();

    if(m_watcher)
        m_watcher.clear();

    if(m_sender)
        m_sender.clear();
}

// This is a callback used to intercept log message calls
// so we can inject custom handling.
void Collector::logHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString message = qFormatLogMessage(type, context, msg);
    collector->handle_log(type, message);

    // not sure there's a point in calling this
    if (originalHandler)
        originalHandler(type, context, msg);
}

// Redirect log messages to somewhere other than the console, if the user so desires.
void Collector::handle_log(QtMsgType type, const QString &msg) const
{
    Q_UNUSED(msg)

    static QMap<QtMsgType, QString> tags {
        { QtDebugMsg, "Debug" },
        { QtInfoMsg, "Info" },
        { QtWarningMsg, "Warning" },
        { QtCriticalMsg, "Critical" },
        { QtFatalMsg, "Fatal" }
    };

    auto tag = tags[type];

    if(m_log.isNull())
    {
        static QTextStream ts_stdout(stdout);
        static QTextStream ts_stderr(stderr);

        switch(type)
        {
            case QtDebugMsg:
            case QtInfoMsg:
                ts_stdout << "[" << tag << "] " << QDateTime::currentDateTime().toString() << ": " << msg;
                break;

            case QtWarningMsg:
            case QtCriticalMsg:
            case QtFatalMsg:
                ts_stderr << "[" << tag << "] " << QDateTime::currentDateTime().toString() << ": " << msg;
                break;
        }
    }
    else
    {
        QTextStream out(m_log.data());
        out << "[" << tag << "] " << QDateTime::currentDateTime().toString() << ": " << msg << "\n";
    }
}

void Collector::initialize_watcher()
{
    // Prime the QFileSystemWatcher on the queue path.
    // Locate every existing Sensor event file in the queue on startup, and add them
    // as watch points in the QFileSystemWatcher instance.  If we don't, we
    // won't get notified when they are updated.

    qInfo() << tr("Initializing file system watcher.");

    m_watcher->addPath(m_queue_path);

    QDir directory(m_queue_path);
    QStringList sensor_files = directory.entryList(QStringList() << "*.json",QDir::Files);
    QStringList messages;
    foreach(QString filename, sensor_files)
    {
        auto full_file_path = directory.absoluteFilePath(filename);
        messages.append(tr("Adding existing Sensor event file \"%1\".").arg(full_file_path));
        m_watcher->addPath(full_file_path);
    }

    int count = 0;
    foreach(const auto& msg, messages)
    {
        if(++count == messages.count())
            qInfo() << "└─ " << msg;
        else
            qInfo() << "├─ " << msg;
    }
}

void Collector::process_sensor_offline(const QString& file, const QString& msg)
{
    // This data file has gone missing since our last sweep.
    // Remove it from the cache, and notify the multicast group
    // that this sensor is offline.

    qWarning() << msg;

    auto sensor_name = m_queue_cache[file][0].toString();
    auto sensor_offline = QString("{ \"domain_id\" : \"%1\", \"domain_name\" : \"%2\","
                                " \"type\" : \"%3\","
                                " \"sensor_name\" : \"%4\" }")
        .arg(m_id)
        .arg(QUrl::toPercentEncoding(m_name),
             SharedTypes::MsgType2Text[SharedTypes::MessageType::Offline],
             QUrl::toPercentEncoding(sensor_name));

    // Send the domain error to the multicast group
    m_sender->send_datagram(sensor_offline.toUtf8());

    m_sensor_updates.remove(file);
}

bool Collector::process_sensor_update(const QString& file, QDateTime last_modified)
{
    bool result = false;

    QFile sensor_file(file);
    if(sensor_file.open(QIODevice::ReadOnly))
    {
        auto data = sensor_file.readAll();
        sensor_file.close();

        QJsonParseError error;
        auto doc = QJsonDocument::fromJson(data, &error);
        if(!doc.isNull())
        {
            QJsonObject object = doc.object();

            if(object.contains("sensor_name") && object.contains("sensor_state"))
            {
                auto sensor_name = object["sensor_name"].toString();
                auto sensor_state= object["sensor_state"].toString().toLower();
                QString sensor_message;
                if(object.contains("sensor_message"))
                    sensor_message = object["sensor_message"].toString();

                if(SharedTypes::MsgText2State.contains(sensor_state))
                {
                    auto sensor_data = QString("{ \"domain_id\" : \"%1\", \"domain_name\" : \"%2\", "
                                               " \"type\" : \"%3\", "
                                               " \"sensor_name\" : \"%4\", \"sensor_state\" : \"%5\", "
                                               " \"sensor_message\" : \"%6\" }")
                        .arg(m_id)
                        .arg(QUrl::toPercentEncoding(m_name),
                            SharedTypes::MsgType2Text[SharedTypes::MessageType::Sensor],
                            QUrl::toPercentEncoding(sensor_name), sensor_state,
                            QUrl::toPercentEncoding(sensor_message));

                    // Send the sensor data to the multicast group
                    m_sender->send_datagram(sensor_data.toUtf8());

                    if(m_queue_cache.contains(file))
                    {
                        auto delta = m_queue_cache[file][1].toDateTime().msecsTo(last_modified);
                        m_queue_cache[file][1] = last_modified;

                        m_sensor_updates[file][0] += 1;
                        m_sensor_updates[file][1] += delta;
                    }
                    else
                    {
                        // Add/update the cache info
                        m_queue_cache[file] = SensorDataList();
                        m_queue_cache[file].append(sensor_name);
                        m_queue_cache[file].append(last_modified);

                        m_sensor_updates[file] = UpdateDataList();
                        m_sensor_updates[file].append(0);
                        m_sensor_updates[file].append(0);
                    }

                    result = true;
                }
                else
                {
                    qWarning() << tr("Sensor \"") << sensor_name << tr("\" used invalid state value: \"") << sensor_state << "\".";
                }
            }
        }
        else
        {
            // The JSON doc did not load--it could be empty, or it could be partial.
            // We'll leave it alone for now, and try to process it on the next event.
            qWarning() << tr("Failed to load Sensor data file: \"") << file << "\": " << error.errorString();
        }
    }

    return result;
}

void Collector::slot_directory_event(const QString& dir)
{
    Q_UNUSED(dir)

    // Perform a delta on the folder and see if there are any new or removed files

    QDir directory(m_queue_path);
    QStringList sensor_files = directory.entryList(QStringList() << "*.json",QDir::Files);

    // Step 1: Process new Sensor data files
    foreach(QString filename, sensor_files)
    {
        auto full_file_path = directory.absoluteFilePath(filename);
        QString new_file; // populated with a filename if this file is eligible for processing
        const QFileInfo info(full_file_path);

        if(info.lastModified() > m_start_time)
        {
            if(!m_queue_cache.contains(full_file_path))
            {
                // This is a new entry in the queue
                new_file = full_file_path;
            }
        }
        else
            qInfo() << tr("Ignoring outdated Sensor file '") << filename << "'";

        if(!new_file.isEmpty())
        {
            qInfo() << tr("Processing Sensor add: \"") << new_file << "\"";
            if(process_sensor_update(new_file, info.lastModified()))
            {
                // The file was added to our cache, update our
                // file system watcher to tell us about events

                m_watcher->addPath(new_file);
            }
        }
    }

    // Step 2: Identify missing files in the cache
    auto keys = m_queue_cache.keys();
    foreach(QString key, keys)
    {
        if(!QFile::exists(key))
        {
            qInfo() << tr("Processing Sensor offline: \"") << key << "\"";
            auto msg = tr("Sensor data file \"%1\" has been removed.").arg(key);
            process_sensor_offline(key, msg);

            m_watcher->removePath(key);
            m_queue_cache.remove(key);
        }
    }
}

void Collector::slot_file_event(const QString& file)
{
    const QFileInfo info(file);

    if(m_queue_cache.contains(file))
    {
        // Avoid bounce.
        const auto now = QDateTime::currentDateTime();
        if(abs(m_queue_cache[file][1].toDateTime().msecsTo(now)) < 1000)
        {
            // qInfo() << tr("└─ Sensor event \"") << file << tr("\" is updating too fast; ignoring.");
            return;
        }
    }

    qInfo() << tr("Processing Sensor event: \"") << file << "\"";
    process_sensor_update(file, info.lastModified());
}

void Collector::slot_housekeeping()
{
    auto now = QDateTime::currentDateTime();

    auto keys = m_queue_cache.keys();
    foreach(QString key, keys)
    {
        // Calculate the current average update cadence
        auto average_cadence = m_sensor_updates[key][1] / m_sensor_updates[key][0];
        if(average_cadence)
        {
            // How long since the last update?
            auto delta = m_queue_cache[key][1].toDateTime().msecsTo(now);
            if(delta >= (average_cadence * m_offline_detection_multiplier))
            {
                // Consider this one offline.
                qInfo() << tr("Processing Sensor offline: \"") << key << "\" (avg: " << average_cadence << ", delta: " << delta << ")";
                process_sensor_offline(key, tr("Sensor update overdue; flagging offline."));
            }
        }
    }
}

void Collector::load_settings()
{
    if(!QFile::exists(m_settings_filename))
    {
        // perform one-time data initialization

        // Generate our persistent domain id
        {
            std::random_device rd;
            std::mt19937 rd_mt(rd());
            std::uniform_int_distribution<unsigned long long> r(
                    std::numeric_limits<std::uint64_t>::min(),
                    std::numeric_limits<std::uint64_t>::max()
                );
            m_id = {r(rd_mt)};
        }

        // Generate a random IPv4 multicast address
        // {
        //     std::random_device rd;
        //     std::mt19937 rd_mt(rd());
        //     std::uniform_int_distribution<> byte(0, 255);

        //     auto b1{byte(rd_mt)};
        //     auto b2{byte(rd_mt)};
        //     auto b3{byte(rd_mt)};

        //     m_ip4_group = QString("239.%1.%2.%3").arg(b1).arg(b2).arg(b3);
        // }

        save_settings();
    }

    QSettings settings(m_settings_filename, QSettings::IniFormat);

    settings.beginGroup("Collector");
        m_id = settings.value("id", 0).toULongLong();
        m_ip4_group = settings.value("ip4group", SharedTypes::MULTICAST_IPV4).toString();
        m_ip6_group = settings.value("ip6group", SharedTypes::MULTICAST_IPV6).toString();
        m_port = settings.value("port", SharedTypes::MULTICAST_PORT).toString().toUShort();
        m_queue_path = settings.value("queue-folder", "").toString();
        m_log_path = settings.value("log-folder", "").toString();
        // m_clean_on_startup = settings.value("clean-on-startup", true).toBool();
    settings.endGroup();
}

void Collector::save_settings()
{
    QSettings settings(m_settings_filename, QSettings::IniFormat);

    settings.clear();

    settings.beginGroup("Collector");
        settings.setValue("id", QString::number(m_id));
        settings.setValue("ip4group", m_ip4_group);
        settings.setValue("ip6group", m_ip6_group);
        settings.setValue("port", m_port);
        settings.setValue("queue-folder", m_queue_path);
        settings.setValue("log-folder", m_log_path);
        // settings.setValue("clean-on-startup", m_clean_on_startup);
    settings.endGroup();
}
