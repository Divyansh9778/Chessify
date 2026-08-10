#pragma once

#include "NetworkPacket.h"

#include <string>

NetworkPacket parsePacket(const std::string& msg);

std::string makePacket(
    MessageType type,
    const std::string& payload = "");