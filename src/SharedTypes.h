#pragma once

#include <QMap>

constexpr float version = 1.0;

class SharedTypes
{
public:
    SharedTypes() = default;
    ~SharedTypes() = default;

public:    // typedefs and enums
    enum class MessageType {
        Sensor,     // Sensor data
        Offline,    // Informational: Sensor has gone offline
        Warning,
        Error
    };

    using Type2TextMap = QMap<SharedTypes::MessageType, QString>;
    using Text2TypeMap = QMap<QString, SharedTypes::MessageType>;

    static Type2TextMap MsgType2Text;
    static Text2TypeMap MsgText2Type;

    enum class SensorState {
        Undefined,
        Healthy,
        Poor,
        Critical,
        Deceased
    };

    using State2TextMap = QMap<SharedTypes::SensorState, QString>;
    using Text2StateMap = QMap<QString, SharedTypes::SensorState>;

    static State2TextMap MsgState2Text;
    static Text2StateMap MsgText2State;
};
