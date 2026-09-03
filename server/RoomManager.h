#ifndef ROOM_MANAGER_H
#define ROOM_MANAGER_H

#include "Room.h"

#include <unordered_map>
#include <memory>
#include <mutex>
#include <string>

class RoomManager
{
private:

    std::unordered_map<
        std::string,
        std::shared_ptr<GameRoom>
    > rooms;

    std::mutex roomsMutex;

    std::string generateRoomId();

public:

    std::shared_ptr<GameRoom> createRoom(
        std::shared_ptr<
        websocket::stream<tcp::socket>
        > client
    );

    std::shared_ptr<GameRoom> findRoom(
        const std::string& id
    );

    void removeRoom(
        const std::string& id
    );

    size_t roomCount();
};

#endif