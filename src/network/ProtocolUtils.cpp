#include "ProtocolUtils.h"

#include <sstream>

NetworkPacket parsePacket(const std::string& message)
{
    NetworkPacket packet;

    std::istringstream in(message);

    std::string command;
    in >> command;

    if (command == "START")
        packet.type = MessageType::Start;

    else if (command == "MOVE")
        packet.type = MessageType::Move;

    else if (command == "CREATE_ROOM")
        packet.type = MessageType::CreateRoom;

    else if (command == "JOIN_ROOM")
        packet.type = MessageType::JoinRoom;

    else if (command == "ROOM_CREATED")
        packet.type = MessageType::RoomCreated;

    else if (command == "ROOM_JOINED")
        packet.type = MessageType::RoomJoined;

    else if (command == "ROOM_FULL")
        packet.type = MessageType::RoomFull;

    else if (command == "ROOM_NOT_FOUND")
        packet.type = MessageType::RoomNotFound;

    else if (command == "RESIGN")
        packet.type = MessageType::Resign;

    else if (command == "DRAW_OFFER")
        packet.type = MessageType::DrawOffer;

    else if (command == "DRAW_ACCEPT")
        packet.type = MessageType::DrawAccept;

    else if (command == "DRAW_DECLINE")
        packet.type = MessageType::DrawDecline;

    else if (command == "CHAT")
        packet.type = MessageType::Chat;

    else if (command == "DISCONNECT")
        packet.type = MessageType::Disconnect;

    std::getline(in, packet.payload);

    if (!packet.payload.empty() && packet.payload[0] == ' ')
        packet.payload.erase(0, 1);

    return packet;
}

std::string makePacket(
    MessageType type,
    const std::string& payload)
{
    std::string prefix;

    switch (type)
    {
    case MessageType::Start:
        prefix = "START";
        break;

    case MessageType::Move:
        prefix = "MOVE";
        break;

    case MessageType::CreateRoom:
        prefix = "CREATE_ROOM";
        break;

    case MessageType::JoinRoom:
        prefix = "JOIN_ROOM";
        break;

    case MessageType::RoomCreated:
        prefix = "ROOM_CREATED";
        break;

    case MessageType::RoomJoined:
        prefix = "ROOM_JOINED";
        break;

    case MessageType::RoomFull:
        prefix = "ROOM_FULL";
        break;

    case MessageType::RoomNotFound:
        prefix = "ROOM_NOT_FOUND";
        break;

    case MessageType::Resign:
        prefix = "RESIGN";
        break;

    case MessageType::DrawOffer:
        prefix = "DRAW_OFFER";
        break;

    case MessageType::DrawAccept:
        prefix = "DRAW_ACCEPT";
        break;

    case MessageType::DrawDecline:
        prefix = "DRAW_DECLINE";
        break;

    case MessageType::Chat:
        prefix = "CHAT";
        break;

    case MessageType::Disconnect:
        prefix = "DISCONNECT";
        break;

    default:
        prefix = "UNKNOWN";
        break;
    }

    if (payload.empty())
        return prefix;

    return prefix + " " + payload;
}