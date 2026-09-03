#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "RoomManager.h"
#include "../src/network/ProtocolUtils.h"

#include <iostream>
#include <string>
#include <cstdlib>
#include <thread>
#include <mutex>
#include <memory>
#include <random>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;

using tcp = net::ip::tcp;

// ============================================================
// CLOSE PLAYER
// ============================================================

void closePlayer(
    std::shared_ptr<
    websocket::stream<tcp::socket>
    > player)
{
    if (!player)
        return;

    beast::error_code ec;

    player->close(
        websocket::close_code::normal,
        ec
    );

    if (ec)
    {
        std::cerr
            << "[SERVER] Error closing player: "
            << ec.message()
            << '\n';
    }
}

// ============================================================
// FORWARD MESSAGES
// ============================================================

void forwardMessages(
    std::shared_ptr<GameRoom> room,
    std::shared_ptr<websocket::stream<tcp::socket>> from,
    std::shared_ptr<websocket::stream<tcp::socket>> to,
    Turn playerTurn,
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


            // =================================================
            // PARSE PACKET
            // =================================================

            NetworkPacket packet =
                parsePacket(msg);

            if (packet.type != MessageType::Move)
            {
                std::cerr
                    << "[ROOM "
                    << room->id
                    << "] Ignoring non-MOVE packet from "
                    << playerName
                    << '\n';

                continue;
            }


            // =================================================
            // SERVER-AUTHORITATIVE TURN CHECK
            // =================================================

            {
                std::lock_guard<std::mutex>
                    lock(room->roomMutex);

                if (room->currentTurn != playerTurn)
                {
                    std::cerr
                        << "[ROOM "
                        << room->id
                        << "] REJECTED "
                        << playerName
                        << " move: not their turn\n";

                    continue;
                }

                // Switch turn only after accepting
                // the move from the correct player.

                room->currentTurn =
                    (room->currentTurn == Turn::White)
                    ? Turn::Black
                    : Turn::White;
            }


            // =================================================
            // FORWARD MOVE ONLY TO OPPONENT
            // =================================================

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

        try
        {
            std::lock_guard<std::mutex>
                lock(room->writeMutex);

            if (to)
            {
                to->write(
                    net::buffer(
                        makePacket(
                            MessageType::Disconnect,
                            ""
                        )
                    )
                );
            }
        }
        catch (const std::exception& sendError)
        {
            std::cerr
                << "[ROOM "
                << room->id
                << "] Could not notify opponent: "
                << sendError.what()
                << '\n';
        }

        closePlayer(to);
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

        try
        {
            std::lock_guard<std::mutex>
                lock(room->writeMutex);

            if (to)
            {
                to->write(
                    net::buffer(
                        makePacket(
                            MessageType::Disconnect,
                            ""
                        )
                    )
                );
            }
        }
        catch (const std::exception& sendError)
        {
            std::cerr
                << "[ROOM "
                << room->id
                << "] Could not notify opponent: "
                << sendError.what()
                << '\n';
        }

        closePlayer(to);
    }
}


// ============================================================
// START GAME
// ============================================================

void runGameRoom(
    std::shared_ptr<GameRoom> room,
    RoomManager& roomManager)
{
    {
        std::lock_guard<std::mutex>
            lock(room->roomMutex);

        // Game already started
        if (room->started)
            return;

        // Need two players
        if (!room->white ||
            !room->black)
        {
            return;
        }

        // ==================================================
        // RANDOMLY ASSIGN COLORS
        // ==================================================

        static thread_local std::mt19937 colorRng{
            std::random_device{}()
        };

        std::uniform_int_distribution<int>
            colorDist(0, 1);

        if (colorDist(colorRng) == 1)
        {
            // Swap the actual players.
            // The previous white player becomes black,
            // and the previous black player becomes white.
            std::swap(
                room->white,
                room->black
            );

            std::cerr
                << "[ROOM "
                << room->id
                << "] Random colors: "
                << "Player 1 = BLACK, "
                << "Player 2 = WHITE\n";
        }
        else
        {
            std::cerr
                << "[ROOM "
                << room->id
                << "] Random colors: "
                << "Player 1 = WHITE, "
                << "Player 2 = BLACK\n";
        }

        room->started = true;
    }


    std::cerr
        << "[ROOM "
        << room->id
        << "] Game Started!\n";


    // ========================================================
    // SEND COLORS
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
        Turn::White,
        "WHITE"
    );

    std::thread blackThread(
        forwardMessages,
        room,
        room->black,
        room->white,
        Turn::Black,
        "BLACK"
    );


    // Wait for both players
    whiteThread.join();
    blackThread.join();


    std::cerr
        << "[ROOM "
        << room->id
        << "] Game ended\n";


    // ========================================================
    // REMOVE ROOM
    // ========================================================

    roomManager.removeRoom(
        room->id
    );
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
                roomManager.createRoom(
                    client
                );

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

            /*
                IMPORTANT:

                We return here.

                The room stays alive inside
                RoomManager's unordered_map.

                The server immediately goes back
                to accepting other clients.

                Therefore multiple rooms can exist
                simultaneously.
            */

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


            // ------------------------------------------------
            // FIND ROOM
            // ------------------------------------------------

            auto room = roomManager.findRoom(roomId);

            // ------------------------------------------------
            // ROOM DOES NOT EXIST
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
            // ADD PLAYER TO ROOM
            // ------------------------------------------------

            {
                std::lock_guard<std::mutex>
                    lock(room->roomMutex);

                // Room already has two players
                if (room->black)
                {
                    std::cerr
                        << "[ROOM "
                        << room->id
                        << "] Room occupied - rejecting player\n";

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

                // Add second player
                room->black = client;

                // =================================================
                // RANDOM COLOR ASSIGNMENT
                // =================================================

                static thread_local
                    std::mt19937 colorRng{
                        std::random_device{}()
                };

                std::uniform_int_distribution<int>
                    colorDist(0, 1);

                if (colorDist(colorRng) == 1)
                {
                    std::swap(
                        room->white,
                        room->black
                    );

                    std::cerr
                        << "[ROOM "
                        << room->id
                        << "] Random colors: "
                        << "Player 1 = BLACK, "
                        << "Player 2 = WHITE\n";
                }
                else
                {
                    std::cerr
                        << "[ROOM "
                        << room->id
                        << "] Random colors: "
                        << "Player 1 = WHITE, "
                        << "Player 2 = BLACK\n";
                }


                startGame = true;
            }


            // ------------------------------------------------
            // TELL PLAYER 2 THEY JOINED
            // ------------------------------------------------

            {
                std::lock_guard<std::mutex>
                    lock(room->writeMutex);

                client->write(
                    net::buffer(
                        makePacket(
                            MessageType::RoomJoined,
                            room->id
                        )
                    )
                );
            }


            std::cerr
                << "[ROOM "
                << room->id
                << "] Player 2 joined\n";


            // ------------------------------------------------
            // START GAME
            // ------------------------------------------------

            if (startGame)
            {
                std::thread(
                    runGameRoom,
                    room,
                    std::ref(roomManager)
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


        // ====================================================
        // ONE ROOM MANAGER FOR ENTIRE SERVER
        // ====================================================

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


                // =================================================
                // KEEP YOUR EXISTING WEBSOCKET TYPE
                // =================================================

                auto client =
                    std::make_shared<
                    websocket::stream<
                    tcp::socket
                    >
                    >(std::move(socket));


                client->accept();


                std::cerr
                    << "[SERVER] WebSocket handshake complete\n";


                // =================================================
                // HANDLE CLIENT IN ITS OWN THREAD
                // =================================================

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