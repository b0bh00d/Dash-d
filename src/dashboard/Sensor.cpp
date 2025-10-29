#include "Sensor.h"

const QMap<SensorState, QString> Sensor::StateImages{
    {SensorState::Healthy, QStringLiteral(":/images/Healthy.png")},
    {SensorState::Poor, QStringLiteral(":/images/Poor.png")},
    {SensorState::Critical, QStringLiteral(":/images/Critical.png")},
};

Sensor::Sensor(std::uint64_t id, const QString& name, QObject* parent)
    : QObject{parent},
      m_id(id),
      m_name(name)
{
}

void Sensor::set_state(SensorState state)
{
    assert(state != SensorState::Undefined);

    if(state != m_state)
    {
        m_state = state;
        emit signal_state_changed();
    }
}
