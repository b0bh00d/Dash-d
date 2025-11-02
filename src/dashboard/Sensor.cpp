#include "Sensor.h"

const QMap<SharedTypes::SensorState, QString> Sensor::StateImages{
    {SharedTypes::SensorState::Healthy, QStringLiteral(":/images/Healthy.png")},
    {SharedTypes::SensorState::Poor, QStringLiteral(":/images/Poor.png")},
    {SharedTypes::SensorState::Critical, QStringLiteral(":/images/Critical.png")},
    {SharedTypes::SensorState::Deceased, QStringLiteral(":/images/Deceased.png")},
};

Sensor::Sensor(const QString& name, QObject* parent)
    : QObject{parent},
      m_name(name)
{
}

void Sensor::set_state(SharedTypes::SensorState state)
{
    assert(state != SharedTypes::SensorState::Undefined);

    if(state != m_state)
    {
        m_state = state;
        emit signal_state_changed();
    }
}
