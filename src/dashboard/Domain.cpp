#include "Domain.h"

Domain::Domain(std::uint64_t id, const QString& name, QObject *parent)
    : QObject{parent},
      m_id(id),
      m_name(name)
{}

Domain::~Domain()
{
    foreach(const auto &sensor, m_sensors)
        emit signal_sensor_removed(sensor->name());
}

void Domain::add_sensor(SensorPtr sensor)
{
    m_sensors[sensor->name()] = sensor;
    emit signal_sensor_added(sensor);

    if(m_housekeeping.isNull())
    {
        m_housekeeping = TimerPtr(new QTimer());
        m_housekeeping->setInterval(housekeeping_interval);
        connect(m_housekeeping.data(), &QTimer::timeout, this, &Domain::slot_housekeeping);
        m_housekeeping->start();
    }
}

void Domain::del_sensor(const QString& name)
{
    m_sensors.remove(name);
    if(m_sensors.isEmpty())
    {
        m_housekeeping->stop();
        m_housekeeping.clear();
    }

    emit signal_sensor_removed(name);
}

void Domain::update_sensor(QString name, SharedTypes::SensorState state, const QString& message)
{
    // Do we have this sensor already?
    assert(m_sensors.contains(name));

    bool notify = state > m_sensors[name]->state();
    m_sensors[name]->set_state(state, message);
    emit signal_sensor_updated(name, state, message, notify);
}

void Domain::slot_housekeeping()
{
    // Find all sensors that are Offline, and check their last_update
    // value.  Any that have been offline for more than N seconds get
    // deleted.  (Deletion is delayed this way so the user sees a brief
    // visual cue that a Sensor has gone offline instead of relying
    // only on the log.

    QStringList names_to_delete;
    auto now = QDateTime::currentDateTime();
    foreach(SensorPtr sensor, m_sensors)
    {
        if(sensor->state() == SharedTypes::SensorState::Offline)
        {
            auto last_update = sensor->last_update();
            if(last_update.msecsTo(now) > offline_timeout)
                names_to_delete.append(sensor->name());
        }
        else
        {
#ifndef TEST
            // Based on their update cadence, are any of the Sensors de facto offline?
            auto cadence = sensor->average_update();
            if(cadence)
            {
                // How long since the last update?
                auto delta = sensor->last_update().msecsTo(now);
                if(delta >= (cadence * m_offline_detection_multiplier))
                {
                    // Consider this one offline.
                    update_sensor(sensor->name(), SharedTypes::SensorState::Offline, tr("Domain detected offline sensor based on update cadence."));
                }
            }
#endif
        }
    }

    foreach(const QString& name, names_to_delete)
        del_sensor(name);
}
