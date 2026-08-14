#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "../src/network/ProtocolUtils.h"

#include <iostream>
#include <string>
#include <cstdlib>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <random>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;

using tcp = net::ip::tcp;

// ============================================================
// GAME ROOM
// ============================================================

struct GameRoom
{
    std::string id;

    std::shared_ptr<
        websocket::stream<tcp::socket>
    > white;

    std::shared_ptr<
        websocket::stream<tcp::socket>
    > black;

    // Protects white / black / started
    std::mutex roomMutex;

    // Protects writes to WebSocket streams
    std::mutex writeMutex;

    bool started = false;
};

// ============================================================
// ROOM MANAGER
// ============================================================

class RoomManager
{
private:
    std::unordered_map<
        std::string,
        std::shared_ptr<GameRoom>
    > rooms;

    std::mutex roomsMutex;

    std::mt19937 rng{
        std::random_device{}()
    };

    std::string generateRoomId()
    {
        static const char chars[] =
            "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";

        std::uniform_int_distribution<int> dist(
            0,
            sizeof(chars) - 2
        );

        std::string id;

        do
        {
            id.clear();

            for (int i = 0; i < 6; ++i)
                id += chars[dist(rng)];

        } while (rooms.find(id) != rooms.end());

        return id;
    }

public:

    std::shared_ptr<GameRoom> createRoom(
        std::shared_ptr<
        websocket::stream<tcp::socket>
        > client)
    {
        std::lock_guard<std::mutex>
            lock(roomsMutex);

        auto room =
            std::make_shared<GameRoom>();

        room->id = generateRoomId();

        room->white = client;

        rooms[room->id] = room;

        std::cerr
            << "[ROOM] Created "
            << room->id
            << '\n';

        return room;
    }

    std::shared_ptr<GameRoom> findRoom(
        const std::string& id)
    {
        std::lock_guard<std::mutex>
            lock(roomsMutex);

        auto it = rooms.find(id);

        if (it == rooms.end())
            return nullptr;

        return it->second;
    }

    void removeRoom(
        const std::string& id)
    {
        std::lock_guard<std::mutex>
            lock(roomsMutex);

        auto it = rooms.find(id);

        if (it != rooms.end())
        {
            rooms.erase(it);

            std::cerr
                << "[ROOM] Removed "
                << id
                << '\n';
        }
    }

    size_t roomCount()
    {
        std::lock_guard<std::mutex>
            lock(roomsMutex);

        return rooms.size();
    }
};

// ============================================================
// FORWARD MESSAGES
// ============================================================

void forwardMessages(
    std::shared_ptr<GameRoom> room,
    std::shared_ptr<
    websocket::stream<tcp::socket>
    > from,
    std::shared_ptr<
    websocket::stream<tcp::socket>
    > to,
    const char* playerName)
{
    try
    {
        while (true)
        {
            beast::flat_buffer buffer;

            from->read(buffer);

            std::string msg =
                beast::buffers_to_string(
                    buffer.data()
                );

            std::cerr
                << "[ROOM "
                << room->id
                << "] ["
                << playerName
                << "] "
                << msg
                << '\n';

            // ------------------------------------------------
            // Send to opponent
            // ------------------------------------------------

            {
                std::lock_guard<std::mutex>
                    lock(room->writeMutex);

                to->write(
                    net::buffer(msg)
                );
            }

            std::cerr
                << "[ROOM "
                << room->id
                << "] "
                << playerName
                << " -> opponent\n";
        }
    }
    catch (const beast::system_error& e)
    {
        std::cerr
            << "[ROOM "
            << room->id
            << "] "
            << playerName
            << " disconnected: "
            << e.what()
            << '\n';
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "[ROOM "
            << room->id
            << "] "
            << playerName
            << " error: "
            << e.what()
            << '\n';
    }
}

// ============================================================
// START GAME
// ============================================================

void runGameRoom(
    std::shared_ptr<GameRoom> room)
{
    {
        std::lock_guard<std::mutex>
            lock(room->roomMutex);

        if (room->started)
            return;

        if (!room->white ||
            !room->black)
        {
            return;
        }

        room->started = true;
    }

    std::cerr
        << "[ROOM "
        << room->id
        << "] Game Started!\n";

    // ========================================================
    // ASSIGN COLORS
    // ========================================================

    {
        std::lock_guard<std::mutex>
            lock(room->writeMutex);

        room->white->write(
            net::buffer(
                makePacket(
                    MessageType::Start,
                    "WHITE"
                )
            )
        );

        room->black->write(
            net::buffer(
                makePacket(
                    MessageType::Start,
                    "BLACK"
                )
            )
        );
    }

    std::cerr
        << "[ROOM "
        << room->id
        << "] Assigned colors\n";

    // ========================================================
    // TWO-WAY COMMUNICATION
    // ========================================================

    std::thread whiteThread(
        forwardMessages,
        room,
        room->white,
        room->black,
        "WHITE"
    );

    std::thread blackThread(
        forwardMessages,
        room,
        room->black,
        room->white,
        "BLACK"
    );

    whiteThread.join();
    blackThread.join();

    std::cerr
        << "[ROOM "
        << room->id
        << "] Game ended\n";
}

// ============================================================
// HANDLE ONE CLIENT
// ============================================================

void handleClient(
    std::shared_ptr<
    websocket::stream<tcp::socket>
    > client,
    RoomManager& roomManager)
{
    try
    {
        std::cerr
            << "[SERVER] WebSocket client ready\n";

        // ====================================================
        // FIRST MESSAGE
        // ====================================================

        beast::flat_buffer buffer;

        client->read(buffer);

        std::string msg =
            beast::buffers_to_string(
                buffer.data()
            );

        std::cerr
            << "[SERVER] Initial message: "
            << msg
            << '\n';

        NetworkPacket packet =
            parsePacket(msg);

        // ====================================================
        // CREATE ROOM
        // ====================================================

        if (packet.type ==
            MessageType::CreateRoom)
        {
            auto room =
                roomManager.createRoom(client);

            client->write(
                net::buffer(
                    makePacket(
                        MessageType::RoomCreated,
                        room->id
                    )
                )
            );

            std::cerr
                << "[ROOM "
                << room->id
                << "] Player 1 waiting\n";

            // ------------------------------------------------
            // IMPORTANT:
            //
            // We DO NOT wait here.
            //
            // handleClient() returns.
            //
            // The main accept loop can immediately accept
            // another client.
            // ------------------------------------------------

            return;
        }

        // ====================================================
        // JOIN ROOM
        // ====================================================

        if (packet.type ==
            MessageType::JoinRoom)
        {
            std::string roomId =
                packet.payload;

            auto room =
                roomManager.findRoom(roomId);

            // ------------------------------------------------
            // Room doesn't exist
            // ------------------------------------------------

            if (!room)
            {
                std::cerr
                    << "[SERVER] Room not found: "
                    << roomId
                    << '\n';

                client->write(
                    net::buffer(
                        makePacket(
                            MessageType::RoomNotFound
                        )
                    )
                );

                client->close(
                    websocket::close_code::normal
                );

                return;
            }

            bool startGame = false;

            // ------------------------------------------------
            // Add player to room
            // ------------------------------------------------

            {
                std::lock_guard<std::mutex>
                    lock(room->roomMutex);

                if (room->black)
                {
                    std::cerr
                        << "[ROOM "
                        << room->id
                        << "] Room full\n";

                    client->write(
                        net::buffer(
                            makePacket(
                                MessageType::RoomFull
                            )
                        )
                    );

                    client->close(
                        websocket::close_code::normal
                    );

                    return;
                }

                room->black = client;

                startGame = true;
            }

            // ------------------------------------------------
            // Tell Player 2 they joined
            // ------------------------------------------------

            client->write(
                net::buffer(
                    makePacket(
                        MessageType::RoomJoined,
                        room->id
                    )
                )
            );

            std::cerr
                << "[ROOM "
                << room->id
                << "] Player 2 joined\n";

            // ------------------------------------------------
            // Start game
            // ------------------------------------------------

            if (startGame)
            {
                std::thread(
                    runGameRoom,
                    room
                ).detach();
            }

            return;
        }

        // ====================================================
        // INVALID INITIAL MESSAGE
        // ====================================================

        std::cerr
            << "[SERVER] Invalid initial packet\n";

        client->write(
            net::buffer(
                makePacket(
                    MessageType::Unknown
                )
            )
        );

        client->close(
            websocket::close_code::protocol_error
        );
    }
    catch (const beast::system_error& e)
    {
        std::cerr
            << "[SERVER] Client error: "
            << e.what()
            << '\n';
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "[SERVER] Client exception: "
            << e.what()
            << '\n';
    }
}

// ============================================================
// MAIN
// ============================================================

int main()
{
    std::cerr
        << "[SERVER] Starting...\n";

    try
    {
        net::io_context io;

        // ====================================================
        // RAILWAY PORT
        // ====================================================

        const char* portEnv =
            std::getenv("PORT");

        int PORT =
            portEnv
            ? std::stoi(portEnv)
            : 9002;

        // ====================================================
        // ACCEPTOR
        // ====================================================

        tcp::acceptor acceptor(
            io,
            tcp::endpoint(
                net::ip::make_address(
                    "0.0.0.0"
                ),
                PORT
            )
        );

        std::cerr
            << "[SERVER] Listening on port "
            << PORT
            << '\n';

        RoomManager roomManager;

        // ====================================================
        // CONTINUOUS ACCEPT LOOP
        // ====================================================

        while (true)
        {
            try
            {
                std::cerr
                    << "[SERVER] Waiting for client...\n";

                tcp::socket socket(io);

                acceptor.accept(socket);

                std::cerr
                    << "[SERVER] TCP client connected\n";

                auto client =
                    std::make_shared<
                    websocket::stream<tcp::socket>
                    >(std::move(socket));

                client->accept();

                std::cerr
                    << "[SERVER] WebSocket handshake complete\n";

                // ------------------------------------------------
                // IMPORTANT:
                //
                // Handle this client in its own thread.
                //
                // The main server thread immediately goes back
                // to accept().
                // ------------------------------------------------

                std::thread(
                    handleClient,
                    client,
                    std::ref(roomManager)
                ).detach();
            }
            catch (const std::exception& e)
            {
                std::cerr
                    << "[SERVER] Accept error: "
                    << e.what()
                    << '\n';
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "[SERVER] FATAL ERROR: "
            << e.what()
            << '\n';

        return 1;
    }

    return 0;
}