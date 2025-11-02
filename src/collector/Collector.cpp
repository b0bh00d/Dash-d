#include <random>

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
#include "../SharedTypes.h"

#define dumpvar(x) qDebug()<<#x<<'='<<x

Collector* collector{nullptr};
QtMessageHandler originalHandler{nullptr};

Collector::Collector(int argc, char *argv[])
    : QCoreApplication(argc, argv)
{
    collector = this; // this is required for custom log processing

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
    setApplicationVersion("1.0");

    // Load persistent settings from a file before processing
    // commannd line options.

    load_settings();

    // Process command-line options
    QCommandLineParser parser;
    parser.setApplicationDescription("Sensor data collector for Dash'd");
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
    // QString logPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    // logFileOption.setDefaultValue(logPath);
    logFileOption.setDefaultValue("");  // default to the console
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

    QCommandLineOption cleanOption(QStringList() << "clean-on-startup",
            QCoreApplication::translate("main", "Clear all existing sensor-data files on startup."));
    parser.addOption(cleanOption);

    QCommandLineOption updateOption(QStringList() << "update-settings",
            QCoreApplication::translate("main", "Update persistent settings with current command line options and exit."));
    parser.addOption(updateOption);

#if 0
    QCommandLineOption olderOption(QStringList() << "ignore-older",
            QCoreApplication::translate("main", "Ignore any existing sensor-data files on startup with timestamps older than our start time."));
    parser.addOption(olderOption);
#endif

    parser.process(*this);

#if 0
    m_ignore_older = parser.isSet(olderOption);
#endif

    if(parser.isSet(updateOption))
    {
        m_queue_path = parser.value(targetDirectoryOption);
        m_log_path = parser.value(logFileOption);
        m_ip4_group = parser.value(ip4Option);
        m_ip6_group = parser.value(ip6Option);
        m_port = parser.value(portOption).toUShort();
        m_clean_on_startup = parser.isSet(cleanOption);

        save_settings();

        qApp->exit(0);
        return;
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
                qCritical() << "Could not create directory " << target_path << ".";
                qApp->exit(1);
                return;
            }
        }

        auto log_file_name = QString("%1/dash-d.log").arg(target_path);
        m_log = FilePtr(new QFile(log_file_name));
        if(!m_log->open(QIODevice::WriteOnly | QIODevice::Append))
        {
            qCritical() << "Could not create/open log file \"" << log_file_name<< "\".";
            qApp->exit(1);
            return;
        }

        qInfo() << "Logging output to " << log_file_name << ".";
    }
    else
    {
        qInfo() << "Logging output to console.";
    }

    setLog_path(target_path);

    // ----- 2. Create a Watcher for the file system
    target_path = parser.value(targetDirectoryOption);
    QDir p(target_path);
    if(!p.exists())
    {
        // Ensure the full path exists
        if(!p.mkpath("."))
        {
            qCritical() << "Could not create queue directory \"" << target_path << "\".";
            qApp->exit(1);
            return;
        }
    }

    setQueue_path(target_path);

    m_watcher = WatcherPtr(new QFileSystemWatcher());
    m_watcher->addPath(target_path);
    connect(m_watcher.data(), &QFileSystemWatcher::directoryChanged, this, &Collector::slot_queue_event);

    qInfo() << "Watching queue location \"" << target_path << "\".";

    // ----- 3. Create Sender instance for IPv4 or IPv6
    auto port = parser.value(portOption).toUShort();
    QString ip4group = parser.value(ip4Option);
    QString ip6group = parser.value(ip6Option);

    if(!ip4group.isEmpty() && !ip6group.isEmpty())
    {
        // This is an error

        qCritical() << "Only one of IPv4 or IPv6 may be specified.";
        qApp->exit(1);
        return;
    }
    else if(ip4group.isEmpty() && ip6group.isEmpty())
    {
        // This is an error

        qCritical() << "An address for either IPv4 or IPv6 must be specified.";
        qApp->exit(1);
        return;
    }

    // Create a Sender instance
    if(!ip4group.isEmpty())
    {
        // for IPv4
        m_sender = SenderPtr(new Sender(port, ip4group, QString()));
        qInfo() << "Sending sensor data to IPv4 multicast " << qUtf8Printable(ip4group) << ":" << port << ".";
    }
    else
    {
        // for IPv6
        m_sender = SenderPtr(new Sender(port, QString(), ip6group));
        qInfo() << "Sending sensor data to IPv6 multicast " << ip6group << ":" << port << ".";
    }

    if(parser.isSet(cleanOption))
    {
        qInfo() << "Clear queue of older sensor data:";
        QDir directory(m_queue_path);
        QStringList sensor_files = directory.entryList(QStringList() << "*.json",QDir::Files);
        foreach(QString filename, sensor_files)
        {
            directory.remove(filename);
            qInfo() << "\t" << filename;
        }
    }
}

Collector::~Collector()
{
    // Tear down what we constructed above.
    // (technically speaking, the smart pointers will
    // clean up themselves, but I like the bookkeeping)

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

QString Collector::queue_path() const
{
    return m_queue_path;
}

void Collector::setQueue_path(const QString &newQueue_path)
{
    m_queue_path = newQueue_path;
}

QString Collector::log_path() const
{
    return m_log_path;
}

void Collector::setLog_path(const QString &newLog_path)
{
    m_log_path = newLog_path;
}

void Collector::slot_queue_event(const QString& file)
{
    Q_UNUSED(file)

    process_queue();
}

// Scan the Collector queue directory, and process any files that have the .json
// extension.  These will be Sensor data files.
void Collector::process_queue()
{
    // The Collector queue works like this:
    //
    // Sensors (which are any process in any language that monitor a system resource) will
    // create a file in the queue folder for each resource they are monitoring.  The file
    // name is unimportant to Collector; the extension must be ".json" in order to be regarded.
    //
    // This file-per-resource-per-domain is persistent for the runtime of Collector.  The Sensor
    // process will update the Resource file, at an interval of its choosing, and Collector will
    // monitor the timestamp of the file.  When the timestamp changes, Collector will re-load the
    // file contents and send it on to the multicast group.  The 'sensor_name' attribute
    // within the JSON file should not be changed within the same file.  If a Sensor process
    // must change the sensor name, it should first remove the existing sensor data file, and
    // then create a new one with the updated name.
    //
    // If an existing file disappears (perhaps the Sensor process goes offline), Collector will
    // remove it from its database, and notify the multicast group that the resource is no
    // longer being monitored.
    //
    // Collector will clear the queue folder on start.  This is to remove any lingering data and
    // start reporting fresh.  Sensors should be coded for this behavior, if necessary (i.e.,
    // a resource file could suddenly disappear, so should not be held open).

    QDir directory(m_queue_path);
    QStringList sensor_files = directory.entryList(QStringList() << "*.json",QDir::Files);

    // Step 1: Process new or existing Sensor data files
    foreach(QString filename, sensor_files)
    {
        auto full_file_path = directory.absoluteFilePath(filename);
        QString process_file; // populated with a filename if this file is eligible for processing
        const QFileInfo info(full_file_path);

        if(m_queue_cache.contains(full_file_path))
        {
            auto last_modified = m_queue_cache[full_file_path][1].toDateTime();

            // Has this file been updated?
            if(info.lastModified() > last_modified)
            {
                // Yes, reload its data and inform the multicast group
                process_file = full_file_path;
            }
        }
        else    // This is a new entry in the queue
        {
            process_file = full_file_path;
        }

        if(!process_file.isEmpty())
        {
            QFile sensor_file(process_file);
            if(sensor_file.open(QIODevice::ReadOnly))
            {
                auto data = sensor_file.readAll();
                sensor_file.close();

                auto doc = QJsonDocument::fromJson(data);
                if(!doc.isNull())
                {
                    QJsonObject object = doc.object();

                    if(object.contains("sensor_name") && object.contains("sensor_state"))
                    {
                        auto sensor_name = object["sensor_name"].toString();
                        auto sensor_state= object["sensor_state"].toString().toLower();

                        if(SharedTypes::MsgText2State.contains(sensor_state))
                        {
                            auto sensor_data = QString("{ \"domain_id\" : \"%1\", \"domain_name\" : \"%2\", "
                                                       " \"type\" : \"%3\", "
                                                       " \"sensor_name\" : \"%4\", \"sensor_state\" : \"%5\" }")
                                .arg(m_id)
                                .arg(QUrl::toPercentEncoding(m_name),
                                    SharedTypes::MsgType2Text[SharedTypes::MessageType::Sensor],
                                    QUrl::toPercentEncoding(sensor_name), sensor_state);

                            // Send the sensor data to the multicast group
                            m_sender->send_datagram(sensor_data.toUtf8());

                            // Add/update the cache info
                            m_queue_cache[process_file] = SensorDataList();
                            m_queue_cache[process_file].append(sensor_name);
                            m_queue_cache[process_file].append(info.lastModified());
                        }
                        else
                        {
                            qWarning() << "Sensor \"" << sensor_name << "\" used invalid state value: \"" << sensor_state << "\".";
                        }
                    }
                }
                else
                {
                    // The JSON doc did not load--it could be empty, or it could be partial.
                    // We'll leave it alone for now, and try to process it on the never event.
                    qWarning() << "Failed to load Sensor data file: \"" << process_file << "\".";
                }
            }
        }
    }

    // Step 2: Identify missing files in the cache
    auto keys = m_queue_cache.keys();
    foreach(QString key, keys)
    {
        if(!QFile::exists(key))
        {
            // This data file has gone missing since our last sweep.
            // Remove it from the cache, and notify the multicast group
            // that this sensor is offline.

            auto warning_msg = QStringLiteral("Sensor data file \"%1\" has been removed.").arg(key);
            qWarning() << warning_msg;

            auto sensor_name = m_queue_cache[key][0].toString();
            auto sensor_offline = QString("{ \"domain_id\" : \"%1\", \"domain_name\" : \"%2\","
                                        " \"type\" : \"%3\","
                                        " \"sensor_name\" : \"%4\" }")
                .arg(m_id)
                .arg(QUrl::toPercentEncoding(m_name),
                     SharedTypes::MsgType2Text[SharedTypes::MessageType::Offline],
                     QUrl::toPercentEncoding(sensor_name));

            // Send the domain error to the multicast group
            m_sender->send_datagram(sensor_offline.toUtf8());

            m_queue_cache.remove(key);
        }
    }
}

// This is the original queue-processing implementation, and is here
// for reference.
#if 0
void Collector::process_sensor_data()
{
    QMap<QString, QString> reduction;
    QMap<QString, QJsonObject> sensor_data;

    QDir directory(m_queue_path);
    QStringList sensor_files = directory.entryList(QStringList() << "*.json",QDir::Files);

    // Step 1: get the latest sensor data from anything in the queue folder
    foreach(QString filename, sensor_files)
    {
        const QFileInfo info(filename);

        if(m_ignore_older)
        {
            if(info.lastModified() < m_start_time)
                continue;
        }

        QFile sensor_file(filename);
        if(sensor_file.open(QIODevice::ReadOnly))
        {
            auto data = sensor_file.readAll();
            sensor_file.close();

            auto doc = QJsonDocument::fromJson(data);
            if(!doc.isNull())
            {
                QJsonObject object = doc.object();

                if(object.contains("sensor_name") && object.contains("sensor_state"))
                {
                    sensor_data[filename] = object;

                    auto sensor_name = object["sensor_name"].toString();

                    // Are there multiple files in the queue from the same sensor?
                    if(reduction.contains(sensor_name))
                    {
                        // Compare timestamps to see which one is newer
                        const QFileInfo info2(reduction[sensor_name]);
                        if(info2.lastModified() < info.lastModified())
                        {
                            // Remove the older file
                            directory.remove(reduction[sensor_name]);
                            // Replace the older data with this newer one
                            reduction[sensor_name] = filename;
                        }
                    }
                    else
                        reduction[sensor_name] = /* this will be the key into the sensor_data{} map */ filename;
                }
            }
        }
    }

    // Step 2: construct the sensor data packet and send it to the multicast group
    foreach(QString key, reduction)
    {
        const auto filename = reduction[key];
        auto object = sensor_data[filename];

        // 'key' is the sensor name; no need to grab it again
        auto sensor_state = object["sensor_state"].toString();

        auto sensor_data = QString("{ \"domain_id\" : \"%1\", \"domain_name\" : \"%2\","
                                   " \"type\" : \"sensor\", "
                                    " \"sensor_name\" : \"%3\", \"sensor_state\" : \"%4\" }")
            .arg(m_id)
            .arg(m_name, key, sensor_state);

        // Send the sensor data to the multicast group
        m_sender->send_datagram(sensor_data.toUtf8());

        // Remove the data file
        if(!directory.remove(filename))
        {
            // This is a problem. :(

            auto error_msg = QStringLiteral("Failed to remove sensor data file.");
            qCritical() << error_msg;

            auto domain_error = QString("{ \"domain_id\" : \"%1\", \"domain_name\" : \"%2\","
                                        " \"type\" : \"error\", "
                                        " \"domain_error\" : \"%3\" }")
                .arg(m_id)
                .arg(m_name, error_msg);

            // Send the domain error to the multicast group
            m_sender->send_datagram(domain_error.toUtf8());

            // And go offline until somebody cleans up the mess.
            qApp->quit();

            break;
        }
    }
}
#endif

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
        {
            std::random_device rd;
            std::mt19937 rd_mt(rd());
            std::uniform_int_distribution<> byte(0, 255);

            auto b1{byte(rd_mt)};
            auto b2{byte(rd_mt)};
            auto b3{byte(rd_mt)};

            m_ip4_group = QString("239.%1.%2.%3").arg(b1).arg(b2).arg(b3);
        }

        save_settings();
    }

    QSettings settings(m_settings_filename, QSettings::IniFormat);

    settings.beginGroup("Collector");
        m_id = settings.value("id", 0).toULongLong();
        m_ip4_group = settings.value("ip4group", "").toString();
        m_ip6_group = settings.value("ip6group", "").toString();
        m_port = settings.value("port", 0).toString().toUShort();
        m_queue_path = settings.value("queue-folder", "").toString();
        m_log_path = settings.value("log-folder", "").toString();
        m_clean_on_startup = settings.value("clean-on-startup", true).toBool();
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
        settings.setValue("clean-on-startup", m_clean_on_startup);
    settings.endGroup();
}
