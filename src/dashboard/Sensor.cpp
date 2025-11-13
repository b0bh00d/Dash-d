#include "Sensor.h"

const QMap<SharedTypes::SensorState, QString> Sensor::StateImages{
    {SharedTypes::SensorState::Healthy, QStringLiteral(":/images/Healthy.png")},
    {SharedTypes::SensorState::Poor, QStringLiteral(":/images/Poor.png")},
    {SharedTypes::SensorState::Critical, QStringLiteral(":/images/Critical.png")},
    {SharedTypes::SensorState::Deceased, QStringLiteral(":/images/Deceased.png")},
    {SharedTypes::SensorState::Offline, QStringLiteral(":/images/Offline.png")},
};

Sensor::Sensor(const QString& name, QObject* parent)
    : QObject{parent},
      m_name(name)
{
}

void Sensor::set_state(SharedTypes::SensorState state, const QString& message)
{
    assert(state != SharedTypes::SensorState::Undefined);

    m_message = message;

    if(state != m_state)
    {
        m_state = state;

        ++m_update_count;

        auto now = QDateTime::currentDateTime();
        if(!m_last_update.isNull())
        {
            // Update our totals
            m_update_deltas += m_last_update.msecsTo(now);
        }

        m_last_update = now;
        emit signal_state_changed();
    }
}
