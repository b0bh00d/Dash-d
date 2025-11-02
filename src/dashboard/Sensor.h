#pragma once

#include <QObject>

#include <QMap>
#include <QColor>
#include <QSharedPointer>

#include "../SharedTypes.h"

// An Sensor is a monitored resource within a Domain.

class Sensor : public QObject
{
    Q_OBJECT

public:    // data members
    const static QMap<SharedTypes::SensorState, QString> StateImages;

public:
    explicit Sensor(const QString& name, QObject* parent = nullptr);

    const QString&  name() const { return m_name; }

    SharedTypes::SensorState     state() const { return m_state; }
    void            set_state(SharedTypes::SensorState state);

signals:
    void            signal_state_changed();
    void            signal_removed();

private:    // typedefs and enums

private:    // data members
    QString         m_name;
    SharedTypes::SensorState     m_state;
};

using SensorPtr = QSharedPointer<Sensor>;
