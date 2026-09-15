#pragma once

enum class IPCCommand : int
{
    Feature1 = 1,
    Feature2 = 2
};

enum class IPCMessageType : int
{
    Feature = 1,
    Heartbeat = 2
};

struct IPCMessage
{
    IPCMessageType type;
    IPCCommand command;
    bool enabled;
};