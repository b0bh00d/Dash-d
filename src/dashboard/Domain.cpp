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
        emit signal_sensor_removed(sensor->id());
    }
}

void Domain::add_sensor(SensorPtr sensor)
{
    m_sensors[sensor->id()] = sensor;
    emit signal_sensor_added(sensor);
}

void Domain::del_sensor(std::uint64_t id)
{
    m_sensors.remove(id);
    emit signal_sensor_removed(id);
}

void Domain::update_sensor(std::uint64_t id, SensorState state)
{
    bool notify = state > m_sensors[id]->state();
    m_sensors[id]->set_state(state);
    emit signal_sensor_updated(id, state, notify);
}
