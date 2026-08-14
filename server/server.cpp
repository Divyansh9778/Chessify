#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "../src/network/ProtocolUtils.h"

#include <iostream>
#include <string>
#include <cstdlib>
#include <thread>
#include <mutex>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;

using tcp = net::ip::tcp;

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

            from.read(buffer);

            std::string msg =
                beast::buffers_to_string(buffer.data());

            std::cout
                << "[" << playerName << "] "
                << msg << '\n';

            to.write(net::buffer(msg));

            std::cout
                << "[SERVER] Forwarded "
                << playerName
                << " -> opponent\n";
        }
    }
    catch (const beast::system_error& e)
    {
        std::cout
            << "[SERVER] "
            << playerName
            << " disconnected: "
            << e.what()
            << '\n';
    }
    catch (const std::exception& e)
    {
        std::cout
            << "[SERVER] "
            << playerName
            << " error: "
            << e.what()
            << '\n';
    }
}

int main()
{
    std::cerr << "[SERVER] Starting...\n";

    try
    {
        net::io_context io;

        const char* portEnv =
            std::getenv("PORT");

        int PORT =
            portEnv
            ? std::stoi(portEnv)
            : 9002;

        std::cerr
            << "[SERVER] Creating acceptor on port "
            << PORT
            << '\n';

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
            << '\n';

        // ==========================================
        // PLAYER 1
        // ==========================================

        std::cerr
            << "[SERVER] Waiting for Player 1...\n";

        tcp::socket socket1(io);

        acceptor.accept(socket1);

        std::cerr
            << "[SERVER] Player 1 TCP connected\n";

        websocket::stream<tcp::socket>
            player1(std::move(socket1));

        player1.accept();

        std::cerr
            << "[SERVER] Player 1 WebSocket ready\n";


        // ==========================================
        // PLAYER 2
        // ==========================================

        std::cerr
            << "[SERVER] Waiting for Player 2...\n";

        tcp::socket socket2(io);

        acceptor.accept(socket2);

        std::cerr
            << "[SERVER] Player 2 TCP connected\n";

        websocket::stream<tcp::socket>
            player2(std::move(socket2));

        player2.accept();

        std::cerr
            << "[SERVER] Player 2 WebSocket ready\n";


        // ==========================================
        // START GAME
        // ==========================================

        std::cerr
            << "[SERVER] Game Started!\n";

        player1.write(
            net::buffer(
                makePacket(
                    MessageType::Start,
                    "WHITE"
                )
            )
        );

        player2.write(
            net::buffer(
                makePacket(
                    MessageType::Start,
                    "BLACK"
                )
            )
        );

        std::cerr
            << "[SERVER] Assigned colors\n";


        // ==========================================
        // TWO-WAY MESSAGE FORWARDING
        // ==========================================

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

        whiteThread.join();
        blackThread.join();

        std::cerr
            << "[SERVER] Server shutting down\n";
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "[SERVER] ERROR: "
            << e.what()
            << '\n';

        return 1;
    }

    return 0;
}