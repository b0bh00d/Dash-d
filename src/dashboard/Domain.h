#pragma once

#include <QMap>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QSharedPointer>

#include "Sensor.h"

constexpr int offline_timeout =       10 /* seconds */ * 1000 /* to milliseconds */;
constexpr int housekeeping_interval = 5                * 1000;

//---------------------------------------------------------------------------
// Domain
//
// A Domain contains one or more Sensors.  In practice, a Domain is a
// computer system with assets and resources to be monitored.
//
//---------------------------------------------------------------------------

class Domain : public QObject
{
    Q_OBJECT

public:
    explicit Domain(std::uint64_t id, const QString& name, QObject *parent = nullptr);
    ~Domain();

    std::uint64_t id() const { return m_id; }
    QString     name() const { return m_name; }

    bool        has_sensor(const QString& name) const { return m_sensors.contains(name); }
    void        add_sensor(SensorPtr sensor);
    void        del_sensor(const QString& name);
    void        update_sensor(QString name, SharedTypes::SensorState state, const QDateTime& update, const QString& message = QString());

    int         sensor_count() const { return m_sensors.count(); }

signals:
    void        signal_sensor_added(SensorPtr sensor, Domain* domain);
    void        signal_sensor_removed(SensorPtr sensor);
    void        signal_sensor_updated(SensorPtr sensor, const QString& message, bool notify);

private slots:
    void        slot_housekeeping();

private:    // typedefs and enums
    using SensorMap = QMap<QString, SensorPtr>;
    using TimerPtr = QSharedPointer<QTimer>;

private:    // data members
    std::uint64_t   m_id;
    QString     m_name;

    SensorMap   m_sensors;

    TimerPtr    m_housekeeping{nullptr};
};

using DomainPtr = QSharedPointer<Domain>;
