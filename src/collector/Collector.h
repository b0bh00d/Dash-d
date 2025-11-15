#pragma once

#include <QFile>
#include <QDateTime>
#include <QCoreApplication>
#include <QFileSystemWatcher>
#include <QSharedPointer>

#include "Sender.h"

//---------------------------------------------------------------------------
// Dash'd Collector
//
// The Collector gathers up Sensor data from the local machine (i.e., Domain)
// and sends it along to the multicast group.
//
// Requirements: apt install libqt5network5
//---------------------------------------------------------------------------

class Collector : public QCoreApplication
{
    Q_OBJECT

public:
    explicit Collector(int argc, char *argv[]);
    ~Collector();

    static void logHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    void        handle_log(QtMsgType type, const QString &msg) const;

    QString     queue_path() const { return m_queue_path; }
    void        setQueue_path(const QString &newQueue_path) { m_queue_path = newQueue_path; }

    QString     log_path() const { return m_log_path; }
    void        setLog_path(const QString &newLog_path) { m_log_path = newLog_path; }

private slots:
    void        slot_directory_event(const QString&);
    void        slot_file_event(const QString&);

private:    // typedefs and enums
    using WatcherPtr = QSharedPointer<QFileSystemWatcher>;
    using FilePtr = QSharedPointer<QFile>;
    using SensorDataList = QList<QVariant>; // stores sensor name and last modification timestamp
    using QueueMap = QMap<QString, SensorDataList>;

private:    // methods
    void        process_sensor_offline(const QString& file);
    bool        process_sensor_update(const QString& file, QDateTime last_modified);
    void        process_queue();
#if 0
    void        process_sensor_data();
#endif

    void        load_settings();
    void        save_settings();

private:    // data members
    std::uint64_t   m_id{0};
    QString     m_name;

    FilePtr     m_log;

    QString     m_queue_path;
    QueueMap    m_queue_cache;

    QString     m_log_path;

    WatcherPtr  m_watcher;
    SenderPtr   m_sender;

    QDateTime   m_start_time;

    QString     m_ip4_group;
    QString     m_ip6_group;
    uint16_t    m_port{20856};

    bool        m_clean_on_startup{false};
#if 0
    bool        m_ignore_older{false};
#endif

    QString     m_settings_filename;
};
