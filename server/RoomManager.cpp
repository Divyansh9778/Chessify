#include "RoomManager.h"

#include <random>
#include <iostream>

// ============================================================
// GENERATE ROOM ID
// ============================================================

std::string RoomManager::generateRoomId()
{
    static const char chars[] =
        "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";

    static std::mt19937 rng{
        std::random_device{}()
    };

    std::uniform_int_distribution<int> dist(
        0,
        sizeof(chars) - 2
    );

    std::string id;

    do
    {
        id.clear();

        for (int i = 0; i < 6; ++i)
        {
            id += chars[dist(rng)];
        }

    } while (rooms.find(id) != rooms.end());

    return id;
}


// ============================================================
// CREATE ROOM
// ============================================================

std::shared_ptr<GameRoom>
RoomManager::createRoom(
    std::shared_ptr<
    websocket::stream<tcp::socket>
    > client)
{
    std::lock_guard<std::mutex>
        lock(roomsMutex);

    auto room =
        std::make_shared<GameRoom>();

    room->id =
        generateRoomId();

    room->white =
        client;

    rooms[room->id] =
        room;

    std::cerr
        << "[ROOM MANAGER] Created room: "
        << room->id
        << '\n';

    return room;
}


// ============================================================
// FIND ROOM
// ============================================================

std::shared_ptr<GameRoom>
RoomManager::findRoom(
    const std::string& id)
{
    std::lock_guard<std::mutex>
        lock(roomsMutex);

    auto it =
        rooms.find(id);

    if (it == rooms.end())
        return nullptr;

    return it->second;
}


// ============================================================
// REMOVE ROOM
// ============================================================

void RoomManager::removeRoom(
    const std::string& id)
{
    std::lock_guard<std::mutex>
        lock(roomsMutex);

    auto it =
        rooms.find(id);

    if (it != rooms.end())
    {
        rooms.erase(it);

        std::cerr
            << "[ROOM MANAGER] Removed room: "
            << id
            << '\n';
    }
}


// ============================================================
// ROOM COUNT
// ============================================================

size_t RoomManager::roomCount()
{
    std::lock_guard<std::mutex>
        lock(roomsMutex);

    return rooms.size();
}