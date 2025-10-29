#pragma once

#include <QObject>

#include <QMap>
#include <QColor>
#include <QSharedPointer>

#include "Types.h"

// An Sensor is a monitored resource within a Domain.

class Sensor : public QObject
{
    Q_OBJECT

public:    // data members
    const static QMap<SensorState, QString> StateImages;

public:
    explicit Sensor(std::uint64_t id, const QString& name, QObject* parent = nullptr);

    std::uint64_t   id() const { return m_id; }
    QString         name() const { return m_name; }

    SensorState     state() const { return m_state; }
    void            set_state(SensorState state);

signals:
    void            signal_state_changed();
    void            signal_removed();

private:    // typedefs and enums

private:    // data members
    std::uint64_t   m_id;
    QString         m_name;
    SensorState     m_state;
};

using SensorPtr = QSharedPointer<Sensor>;
