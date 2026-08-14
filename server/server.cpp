#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "../src/network/ProtocolUtils.h"

#include <iostream>
#include <string>
#include <cstdlib>
#include <thread>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;

using tcp = net::ip::tcp;


// ============================================================
// Forward messages from one player to the other
// ============================================================

void forwardMessages(
    websocket::stream<tcp::socket>& from,
    websocket::stream<tcp::socket>& to,
    const char* playerName)
{
    try
    {
        while (true)
        {
            beast::flat_buffer buffer;

            std::cerr
                << "[SERVER] Waiting for "
                << playerName
                << " move..."
                << std::endl;

            from.read(buffer);

            std::string msg =
                beast::buffers_to_string(buffer.data());

            std::cerr
                << "["
                << playerName
                << " -> SERVER] "
                << msg
                << std::endl;

            to.write(
                net::buffer(msg)
            );

            std::cerr
                << "[SERVER -> OPPONENT] "
                << playerName
                << " move forwarded"
                << std::endl;
        }
    }
    catch (const beast::system_error& e)
    {
        std::cerr
            << "[SERVER] "
            << playerName
            << " disconnected: "
            << e.what()
            << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "[SERVER] "
            << playerName
            << " error: "
            << e.what()
            << std::endl;
    }
}


// ============================================================
// MAIN
// ============================================================

int main()
{
    std::cerr
        << "[SERVER] Starting..."
        << std::endl;

    try
    {
        net::io_context io;


        // ====================================================
        // Railway provides PORT through environment variable
        // ====================================================

        const char* portEnv =
            std::getenv("PORT");

        int PORT =
            portEnv
            ? std::stoi(portEnv)
            : 9002;


        std::cerr
            << "[SERVER] Creating acceptor on port "
            << PORT
            << std::endl;


        // ====================================================
        // Listen on all network interfaces
        // ====================================================

        tcp::acceptor acceptor(
            io,
            tcp::endpoint(
                net::ip::make_address("0.0.0.0"),
                PORT
            )
        );


        std::cerr
            << "[SERVER] Listening on port "
            << PORT
            << std::endl;


        // ====================================================
        // PLAYER 1
        // ====================================================

        std::cerr
            << "[SERVER] Waiting for Player 1..."
            << std::endl;

        tcp::socket socket1(io);

        acceptor.accept(socket1);

        std::cerr
            << "[SERVER] Player 1 TCP connected"
            << std::endl;


        websocket::stream<tcp::socket>
            player1(
                std::move(socket1)
            );

        player1.accept();

        std::cerr
            << "[SERVER] Player 1 WebSocket ready"
            << std::endl;


        // ====================================================
        // PLAYER 2
        // ====================================================

        std::cerr
            << "[SERVER] Waiting for Player 2..."
            << std::endl;

        tcp::socket socket2(io);

        acceptor.accept(socket2);

        std::cerr
            << "[SERVER] Player 2 TCP connected"
            << std::endl;


        websocket::stream<tcp::socket>
            player2(
                std::move(socket2)
            );

        player2.accept();

        std::cerr
            << "[SERVER] Player 2 WebSocket ready"
            << std::endl;


        // ====================================================
        // START GAME
        // ====================================================

        std::cerr
            << "[SERVER] Game Started!"
            << std::endl;


        // Player 1 = White

        player1.write(
            net::buffer(
                makePacket(
                    MessageType::Start,
                    "WHITE"
                )
            )
        );


        // Player 2 = Black

        player2.write(
            net::buffer(
                makePacket(
                    MessageType::Start,
                    "BLACK"
                )
            )
        );


        std::cerr
            << "[SERVER] Assigned colors"
            << std::endl;


        // ====================================================
        // TWO-WAY MESSAGE FORWARDING
        //
        // Thread 1:
        //     White -> Black
        //
        // Thread 2:
        //     Black -> White
        // ====================================================

        std::thread whiteThread(
            forwardMessages,
            std::ref(player1),
            std::ref(player2),
            "WHITE"
        );


        std::thread blackThread(
            forwardMessages,
            std::ref(player2),
            std::ref(player1),
            "BLACK"
        );


        // ====================================================
        // Wait for both forwarding threads
        // ====================================================

        whiteThread.join();
        blackThread.join();


        std::cerr
            << "[SERVER] Server shutting down"
            << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "[SERVER] ERROR: "
            << e.what()
            << std::endl;

        return 1;
    }


    return 0;
}