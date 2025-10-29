#pragma once

#include <QMap>
#include <QObject>
#include <QString>
#include <QSharedPointer>

#include "Sensor.h"

// The Domain class represents a single domain with one or more Sensors.

class Domain : public QObject
{
    Q_OBJECT
public:
    explicit Domain(std::uint64_t id, const QString& name, QObject *parent = nullptr);
    ~Domain();

    std::uint64_t id() const { return m_id; }
    QString name() const { return m_name; }

    void    add_sensor(SensorPtr sensor);
    void    del_sensor(std::uint64_t id);
    void    update_sensor(std::uint64_t, SensorState state);

    int     sensor_count() const { return m_sensors.count(); }

signals:
    void        signal_sensor_added(SensorPtr sensor);
    void        signal_sensor_removed(std::uint64_t id);
    void        signal_sensor_updated(std::uint64_t id, SensorState state, bool notify);

private: // typedefs
    using SensorMap = QMap<std::uint64_t, SensorPtr>;

private:    // data members
    std::uint64_t   m_id;
    QString         m_name;

    SensorMap        m_sensors;
};

using DomainPtr = QSharedPointer<Domain>;
