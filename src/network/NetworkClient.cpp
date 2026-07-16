#include <boost/beast/websocket.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/asio.hpp>
#include <memory>

#include "NetworkClient.h"

#include <iostream>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

NetworkClient::NetworkClient()
{
    ws = std::make_unique<websocket::stream<tcp::socket>>(ioContext);
}

NetworkClient::~NetworkClient()
{
    disconnect();
}

bool NetworkClient::connect(const std::string& address, int port)
{
    try
    {
        tcp::resolver resolver(ioContext);
        auto const results = resolver.resolve(address, std::to_string(port));

        net::connect(ws->next_layer(), results.begin(), results.end());

        ws->handshake(address + ":" + std::to_string(port), "/");

        connected = true;
        running = true;
        startListening();

        std::cout << "[Network] Connected to "
            << address << ":" << port << std::endl;

        return true;
    }
    catch (std::exception& e)
    {
        std::cout << "[Network] Connection failed: "
            << e.what() << std::endl;

        connected = false;

        return false;
    }
}

void NetworkClient::startListening()
{
    receiveThread = std::thread([this]()
        {
            while (running)
            {
                try
                {
                    beast::flat_buffer buffer;

                    ws->read(buffer);

                    std::string msg =
                        beast::buffers_to_string(buffer.data());

                    std::cout << "[Network] Received: "
                        << msg << '\n';

                    if (msg == "WHITE")
                    {
                        myColor = PlayerColor::White;
                        myTurn = true;

                        std::cout << "[Network] You are WHITE\n";
                        continue;
                    }

                    if (msg == "BLACK")
                    {
                        myColor = PlayerColor::Black;
                        myTurn = false;

                        std::cout << "[Network] You are BLACK\n";
                        continue;
                    }

                    MoveMessage move = deserialize(msg);

                    {
                        std::lock_guard<std::mutex> lock(queueMutex);
                        incomingMoves.push(move);

                        myTurn = true;
                    }

                    std::cout << "[Network] Move queued\n";
                }
                catch (...)
                {
                    running = false;
                    connected = false;
                }
            }
        });
}

void NetworkClient::disconnect()
{
    if (!connected)
        return;

    running = false;

    if (receiveThread.joinable())
        receiveThread.join();

    beast::error_code ec;
    ws->close(websocket::close_code::normal, ec);

    connected = false;
}

bool NetworkClient::isConnected() const
{
    return connected;
}

bool NetworkClient::sendMove(const MoveRecord &move)
{
    if (!connected)
        return false;

    MoveMessage msg;

    msg.fromRow = move.fromRow;
    msg.fromCol = move.fromCol;
    msg.toRow = move.toRow;
    msg.toCol = move.toCol;
    msg.promotion = move.promotionPiece;

    std::string data = serialize(msg);

    try
    {
        ws->write(net::buffer(data));
        myTurn = false;

        std::cout << "[Network] Sent: " << data << '\n';

        return true;
    }
    catch (std::exception& e)
    {
        std::cout << "[Network] Send failed: "
            << e.what() << '\n';

        connected = false;
        return false;
    }
}

bool NetworkClient::receiveMessage(std::string& msg)
{
    if (!connected)
        return false;

    try
    {
        beast::flat_buffer buffer;

        ws->read(buffer);

        msg = beast::buffers_to_string(buffer.data());

        std::cout << "[Network] Received: "
            << msg << '\n';

        return true;
    }
    catch (std::exception& e)
    {
        std::cout << "[Network] Receive failed: "
            << e.what() << '\n';

        connected = false;

        return false;
    }
}

bool NetworkClient::hasPendingMessages()
{
    std::lock_guard<std::mutex> lock(queueMutex);
    return !incomingMoves.empty();
}

MoveMessage NetworkClient::getNextMove()
{
    std::lock_guard<std::mutex> lock(queueMutex);

    MoveMessage move = incomingMoves.front();
    incomingMoves.pop();

    return move;
}

PlayerColor NetworkClient::getPlayerColor() const
{
    return myColor;
}

bool NetworkClient::isMyTurn() const
{
    return myTurn;
}