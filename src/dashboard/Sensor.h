#pragma once

#include <QObject>

#include <QMap>
#include <QColor>
#include <QDateTime>
#include <QSharedPointer>

#include "../SharedTypes.h"

//---------------------------------------------------------------------------
// Sensor
//
// A Sensor is concered with the state of an asset or resource.  In practice,
// a Sensor is a program of some type running on a computer system, reporting
// the state of an asset or resource on a regular basis.
//
//---------------------------------------------------------------------------

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
    qint64          average_update() const { return m_update_deltas / m_update_count; }

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
    qint64          m_update_count{0};  // number of updates received
    qint64          m_update_deltas{0}; // accumulated milliseconds between updates
    // (m_update_deltas / m_update_count) == average update delay (ms)
};

using SensorPtr = QSharedPointer<Sensor>;
