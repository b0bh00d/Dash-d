#include "SharedTypes.h"

SharedTypes::Type2TextMap SharedTypes::MsgType2Text = {
    { SharedTypes::MessageType::Sensor, "sensor" },
    { SharedTypes::MessageType::Offline, "offline" },
    { SharedTypes::MessageType::Warning, "warning" },
    { SharedTypes::MessageType::Error, "error" },
};

SharedTypes::Text2TypeMap SharedTypes::MsgText2Type = {
    { "sensor", SharedTypes::MessageType::Sensor },
    { "offline", SharedTypes::MessageType::Offline },
    { "warning", SharedTypes::MessageType::Warning },
    { "error", SharedTypes::MessageType::Error },
};

SharedTypes::State2TextMap SharedTypes::MsgState2Text = {
    { SharedTypes::SensorState::Undefined, "undefined" },
    { SharedTypes::SensorState::Healthy, "healthy" },
    { SharedTypes::SensorState::Poor, "poor" },
    { SharedTypes::SensorState::Critical, "critical" },
    { SharedTypes::SensorState::Deceased, "deceased" },
    { SharedTypes::SensorState::Offline, "offline" },
};

SharedTypes::Text2StateMap SharedTypes::MsgText2State = {
    { "undefined", SharedTypes::SensorState::Undefined},
    { "healthy", SharedTypes::SensorState::Healthy },
    { "poor", SharedTypes::SensorState::Poor },
    { "critical", SharedTypes::SensorState::Critical },
    { "deceased", SharedTypes::SensorState::Deceased },
    { "offline", SharedTypes::SensorState::Offline },
};

const char* SharedTypes::MULTICAST_IPV4{"239.255.77.15"};
const char* SharedTypes::MULTICAST_IPV6{"ff12::1870"};
