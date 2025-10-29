#pragma once

#include <cstdint>

// Packet defines the data format that monitoring events will use
// on the multicast group.

constexpr int magic_number{('D' << 24) | ('A' << 16) | ('S' << 8) | 'H'};

namespace Packet
{
    enum class Action : uint32_t
    {
        None,
        Event,
    };

    struct Data
    {
        int magic{magic_number}; // uniquely identifies this data as belonging to Dash'd
        int type{static_cast<uint32_t>(Action::None)};
        int payload_size{0};
        uint8_t payload[1];
    };
};
