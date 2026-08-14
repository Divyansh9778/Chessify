#pragma once

#include "NetworkProtocol.h"

#include <string>

struct NetworkPacket
{
    MessageType type = MessageType::Unknown;
    std::string payload;
};