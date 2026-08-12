#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "../src/network/ProtocolUtils.h"

#include <iostream>
#include <string>
#include <cstdlib>
#include <thread>
#include <atomic>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;

using tcp = net::ip::tcp;

using WebSocket =
websocket::stream<tcp::socket>;

void closeWebSocket(WebSocket& ws)
{
    beast::error_code ec;

    ws.close(
        websocket::close_code::normal,
        ec
    );

    if (ec)
    {
        beast::get_lowest_layer(ws)
            .close(ec);
    }
}

void runGame(
    WebSocket& player1,
    WebSocket& player2)
{
    std::atomic<bool> gameOver = false;

    std::cout << "[SERVER] Game Started!\n";

    // Assign colors
    try
    {
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

        std::cout << "[SERVER] Assigned colors\n";
    }
    catch (const std::exception& e)
    {
        std::cout
            << "[SERVER] Failed to assign colors: "
            << e.what()
            << '\n';

        return;
    }

    // --------------------------------------------------
    // WHITE -> BLACK
    // --------------------------------------------------

    std::thread whiteThread(
        [&]()
        {
            try
            {
                while (!gameOver)
                {
                    beast::flat_buffer buffer;

                    player1.read(buffer);

                    if (gameOver)
                        break;

                    std::string msg =
                        beast::buffers_to_string(
                            buffer.data()
                        );

                    std::cout
                        << "[WHITE] "
                        << msg
                        << '\n';

                    player2.write(
                        net::buffer(msg)
                    );

                    std::cout
                        << "[SERVER] Forwarded to Black\n";
                }
            }
            catch (const beast::system_error& e)
            {
                if (!gameOver)
                {
                    std::cout
                        << "[SERVER] White disconnected: "
                        << e.what()
                        << '\n';

                    gameOver = true;
                }
            }
            catch (const std::exception& e)
            {
                if (!gameOver)
                {
                    std::cout
                        << "[SERVER] White error: "
                        << e.what()
                        << '\n';

                    gameOver = true;
                }
            }
        }
    );

    // --------------------------------------------------
    // BLACK -> WHITE
    // --------------------------------------------------

    std::thread blackThread(
        [&]()
        {
            try
            {
                while (!gameOver)
                {
                    beast::flat_buffer buffer;

                    player2.read(buffer);

                    if (gameOver)
                        break;

                    std::string msg =
                        beast::buffers_to_string(
                            buffer.data()
                        );

                    std::cout
                        << "[BLACK] "
                        << msg
                        << '\n';

                    player1.write(
                        net::buffer(msg)
                    );

                    std::cout
                        << "[SERVER] Forwarded to White\n";
                }
            }
            catch (const beast::system_error& e)
            {
                if (!gameOver)
                {
                    std::cout
                        << "[SERVER] Black disconnected: "
                        << e.what()
                        << '\n';

                    gameOver = true;
                }
            }
            catch (const std::exception& e)
            {
                if (!gameOver)
                {
                    std::cout
                        << "[SERVER] Black error: "
                        << e.what()
                        << '\n';

                    gameOver = true;
                }
            }
        }
    );

    // Wait until one player disconnects.
    while (!gameOver)
    {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(100)
        );
    }

    std::cout
        << "[SERVER] Game ended.\n";

    // Closing the sockets wakes the other
    // player's blocking read().
    {
        beast::error_code ec;

        beast::get_lowest_layer(player1)
            .shutdown(
                tcp::socket::shutdown_both,
                ec
            );

        beast::get_lowest_layer(player1)
            .close(ec);
    }

    {
        beast::error_code ec;

        beast::get_lowest_layer(player2)
            .shutdown(
                tcp::socket::shutdown_both,
                ec
            );

        beast::get_lowest_layer(player2)
            .close(ec);
    }

    if (whiteThread.joinable())
        whiteThread.join();

    if (blackThread.joinable())
        blackThread.join();

    std::cout
        << "[SERVER] Players released.\n";
}


int main()
{
    try
    {
        net::io_context io;

        std::cout
            << "[SERVER] Starting...\n";

        // Railway supplies PORT.
        const char* portEnv =
            std::getenv("PORT");

        int PORT =
            portEnv
            ? std::stoi(portEnv)
            : 9002;

        std::cout
            << "[SERVER] Creating acceptor on port "
            << PORT
            << '\n';

        tcp::acceptor acceptor(
            io,
            tcp::endpoint(
                tcp::v4(),
                PORT
            )
        );

        std::cout
            << "[SERVER] Listening on port "
            << PORT
            << '\n';

        // --------------------------------------------------
        // SERVER LIFETIME LOOP
        // --------------------------------------------------

        while (true)
        {
            std::cout
                << "\n=================================\n"
                << "[SERVER] Waiting for Player 1...\n"
                << "=================================\n";

            WebSocket player1(io);

            try
            {
                acceptor.accept(
                    player1.next_layer()
                );

                std::cout
                    << "[SERVER] Player 1 TCP connected\n";

                player1.accept();

                std::cout
                    << "[SERVER] Player 1 WebSocket ready\n";
            }
            catch (const std::exception& e)
            {
                std::cout
                    << "[SERVER] Player 1 connection failed: "
                    << e.what()
                    << '\n';

                continue;
            }

            std::cout
                << "\n[SERVER] Waiting for Player 2...\n";

            WebSocket player2(io);

            try
            {
                acceptor.accept(
                    player2.next_layer()
                );

                std::cout
                    << "[SERVER] Player 2 TCP connected\n";

                player2.accept();

                std::cout
                    << "[SERVER] Player 2 WebSocket ready\n";
            }
            catch (const std::exception& e)
            {
                std::cout
                    << "[SERVER] Player 2 connection failed: "
                    << e.what()
                    << '\n';

                beast::error_code ec;

                player1.close(
                    websocket::close_code::normal,
                    ec
                );

                continue;
            }

            // Both players connected.
            runGame(
                player1,
                player2
            );

            std::cout
                << "[SERVER] Game session finished.\n";

            std::cout
                << "[SERVER] Ready for another game.\n";
        }
    }
    catch (const std::exception& e)
    {
        std::cout
            << "[SERVER] FATAL ERROR: "
            << e.what()
            << '\n';

        return 1;
    }

    return 0;
}