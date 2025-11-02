#include "Domain.h"

Domain::Domain(std::uint64_t id, const QString& name, QObject *parent)
    : QObject{parent},
      m_id(id),
      m_name(name)
{}

Domain::~Domain()
{
    foreach(const auto &sensor, m_sensors)
    {
        emit signal_sensor_removed(sensor->name());
    }
}

void Domain::add_sensor(SensorPtr sensor)
{
    m_sensors[sensor->name()] = sensor;
    emit signal_sensor_added(sensor);
}

void Domain::del_sensor(const QString& name)
{
    m_sensors.remove(name);
    emit signal_sensor_removed(name);
}

void Domain::update_sensor(QString name, SharedTypes::SensorState state)
{
    // Do we have this sensor already?
    assert(m_sensors.contains(name));

    bool notify = state > m_sensors[name]->state();
    m_sensors[name]->set_state(state);
    emit signal_sensor_updated(name, state, notify);
}
