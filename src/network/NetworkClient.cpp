#include <boost/beast/websocket.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/asio.hpp>
#include <memory>

#include "NetworkClient.h"
#include "ProtocolUtils.h"

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
    ws = std::make_unique<websocket::stream<tcp::socket>>(ioContext);

    try
    {
        tcp::resolver resolver(ioContext);
        auto const results = resolver.resolve(address, std::to_string(port));

        net::connect(ws->next_layer(), results.begin(), results.end());

        ws->handshake(address + ":" + std::to_string(port), "/");

        connected = true;
        running = true;
        disconnected = false;
        ready = false;

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

                    if (!running)
                        break;

                    std::string msg =
                        beast::buffers_to_string(buffer.data());

                    std::cout << "[Network] Received: "
                        << msg << '\n';

                    NetworkPacket packet = parsePacket(msg);

                    switch (packet.type)
                    {
                    case MessageType::Start:
                    {
                        if (packet.payload == "WHITE")
                        {
                            myColor = PlayerColor::White;
                            myTurn = true;
                            ready = true;

                            std::cout << "[Network] You are WHITE\n";
                        }
                        else if (packet.payload == "BLACK")
                        {
                            myColor = PlayerColor::Black;
                            myTurn = false;
                            ready = true;

                            std::cout << "[Network] You are BLACK\n";
                        }

                        break;
                    }

                    case MessageType::Move:
                    {
                        MoveMessage move = deserialize(packet.payload);

                        {
                            std::lock_guard<std::mutex> lock(queueMutex);
                            incomingMoves.push(move);
                            myTurn = true;
                        }

                        std::cout << "[Network] Move queued\n";
                        break;
                    }

                    default:
                        std::cout << "[Network] Unknown packet: "
                            << msg << '\n';
                        break;
                    }
                }
                catch (const std::exception& e)
                {
                    std::cout << "[Network] Connection lost: "
                        << e.what() << '\n';

                    running = false;
                    connected = false;
                    disconnected = true;

                    break;
                }
            }
        });
}

void NetworkClient::disconnect()
{
    if (!running)
        return;

    running = false;

    beast::error_code ec;

    ws->next_layer().cancel(ec);

    if (receiveThread.joinable())
        receiveThread.join();

    connected = false;
    ready = false;
    myTurn = false;
    disconnected = false;
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

    std::string data = makePacket(
        MessageType::Move,
        serialize(msg));

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
        running = false;
        disconnected = true;
        return false;
    }
}

//bool NetworkClient::receiveMessage(std::string& msg)
//{
//    if (!connected)
//        return false;
//        
//    try
//    {
//        beast::flat_buffer buffer;
//
//        ws->read(buffer);
//
//        msg = beast::buffers_to_string(buffer.data());
//
//        std::cout << "[Network] Received: "
//            << msg << '\n';
//
//        return true;
//    }
//    catch (std::exception& e)
//    {
//        std::cout << "[Network] Receive failed: "
//            << e.what() << '\n';
//
//        connected = false;
//        running = false;
//        disconnected = true;
//        return false;
//    }
//}

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

bool NetworkClient::isReady() const
{
    return ready;
}

bool NetworkClient::hasDisconnected() const
{
    return disconnected;
}

void NetworkClient::clearDisconnectFlag()
{
    disconnected = false;
}