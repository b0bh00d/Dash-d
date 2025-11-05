#pragma once

#include <QObject>

#include <QMap>
#include <QColor>
#include <QDateTime>
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
    const QString&  message() const { return m_message; }
    QDateTime       last_update() const { return m_last_update; }

    void            set_state(SharedTypes::SensorState state, const QString& message = QString());

signals:
    void            signal_state_changed();
    void            signal_removed();

private:    // typedefs and enums

private:    // data members
    QString         m_name;
    SharedTypes::SensorState     m_state;
    QString         m_message;

    QDateTime       m_last_update;
};

using SensorPtr = QSharedPointer<Sensor>;
