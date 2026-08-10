#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "../src/network/ProtocolUtils.h"

#include <iostream>
#include <string>
#include <cstdlib>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

int main()
{
    try
    {
        net::io_context io;
        std::cout << "[SERVER] Starting...\n";

        const char* portEnv = std::getenv("PORT");
        int PORT = portEnv ? std::stoi(portEnv) : 9002;
        std::cerr << "[SERVER] Creating acceptor on port " << PORT << std::endl;

        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), PORT));
        std::cerr << "[SERVER] Listening on port "<< PORT << std::endl;

        std::cout << "[SERVER] Waiting for Player 1...\n";

        tcp::socket socket1(io);
        acceptor.accept(socket1);

        std::cout << "[SERVER] Player 1 TCP connected\n";

        websocket::stream<tcp::socket> player1(std::move(socket1));
        player1.accept();
        std::cout << "[SERVER] Player 1 WebSocket ready\n\n";

        std::cout << "[SERVER] Waiting for Player 2...\n";

        tcp::socket socket2(io);
        acceptor.accept(socket2);

        std::cout << "[SERVER] Player 2 TCP connected\n";

        websocket::stream<tcp::socket> player2(std::move(socket2));
        player2.accept();
        std::cout << "[SERVER] Player 2 WebSocket ready\n\n";

        std::cout << "[SERVER] Game Started!\n";

        player1.write(net::buffer(
            makePacket(MessageType::Start, "WHITE")));

        player2.write(net::buffer(
            makePacket(MessageType::Start, "BLACK")));

        std::cout << "[SERVER] Assigned colors\n";

        while (true)
        {
            try
            {
                // ---------- White -> Black ----------
                beast::flat_buffer buffer1;

                player1.read(buffer1);

                std::string msg1 = beast::buffers_to_string(buffer1.data());

                std::cout << "[WHITE] " << msg1 << '\n';

                player2.write(net::buffer(msg1));

                std::cout << "[SERVER] Forwarded to Black\n";

                // ---------- Black -> White ----------
                beast::flat_buffer buffer2;

                player2.read(buffer2);

                std::string msg2 = beast::buffers_to_string(buffer2.data());

                std::cout << "[BLACK] " << msg2 << '\n';

                player1.write(net::buffer(msg2));

                std::cout << "[SERVER] Forwarded to White\n";
            }
            catch (const beast::system_error&)
            {
                std::cout << "[SERVER] A player disconnected.\n";
                break;
            }
        }

        std::cout << "[SERVER] Server shutting down\n";
    }
    catch (const std::exception& e)
    {
        std::cout << "[SERVER] ERROR: " << e.what() << '\n';
    }
}