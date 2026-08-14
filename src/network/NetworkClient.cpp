#include <boost/beast/websocket.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/core.hpp>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <memory>
#include <iostream>

#include "NetworkClient.h"
#include "ProtocolUtils.h"

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;

using tcp = net::ip::tcp;

NetworkClient::NetworkClient()
    : sslContext(ssl::context::tls_client)
{
    sslContext.set_default_verify_paths();
    sslContext.set_verify_mode(ssl::verify_peer);
}

NetworkClient::~NetworkClient()
{
    disconnect();
}

bool NetworkClient::connect(
    const std::string& address,
    int port)
{
    if (connected)
        return true;

    if (receiveThread.joinable())
        disconnect();

    try
    {
        tcp::resolver resolver(ioContext);

        auto results =
            resolver.resolve(
                address,
                std::to_string(port)
            );

        ws = std::make_unique<WebSocketStream>(
            ioContext,
            sslContext
        );

        // TCP connection
        beast::get_lowest_layer(*ws).connect(results);

        // SNI
        if (!SSL_set_tlsext_host_name(
                ws->next_layer().native_handle(),
                address.c_str()))
        {
            throw beast::system_error(
                beast::error_code(
                    static_cast<int>(
                        ::ERR_get_error()
                        ),
                    net::error::get_ssl_category()
                )
            );
        }

        // Verify certificate hostname
        ws->next_layer().set_verify_callback(
            ssl::host_name_verification(address)
        );

        // TLS handshake
        ws->next_layer().handshake(
            ssl::stream_base::client
        );

        // WebSocket handshake
        ws->handshake(
            address,
            "/"
        );

        connected = true;
        running = true;
        disconnected = false;
        ready = false;

        startListening();

        std::cout
            << "[Network] Connected to Railway: "
            << address
            << ":"
            << port
            << std::endl;

        return true;
    }
    catch (const std::exception& e)
    {
        std::cout
            << "[Network] Connection failed: "
            << e.what()
            << std::endl;

        connected = false;
        running = false;
        ready = false;
        myTurn = false;

        if (ws)
        {
            beast::error_code ec;

            beast::get_lowest_layer(*ws)
                .socket()
                .close(ec);

            ws.reset();
        }

        return false;
    }
}

bool NetworkClient::createRoom()
{
    if (!connected || !ws)
        return false;

    try
    {
        std::string data =
            makePacket(MessageType::CreateRoom);

        ws->write(
            net::buffer(data)
        );

        std::cout
            << "[Network] Requested room creation\n";

        return true;
    }
    catch (const std::exception& e)
    {
        std::cout
            << "[Network] Create room failed: "
            << e.what()
            << '\n';

        connected = false;
        running = false;
        disconnected = true;

        return false;
    }
}

bool NetworkClient::joinRoom(
    const std::string& code)
{
    if (!connected || !ws)
        return false;

    if (code.empty())
        return false;

    try
    {
        std::string data =
            makePacket(
                MessageType::JoinRoom,
                code
            );

        ws->write(
            net::buffer(data)
        );

        std::cout
            << "[Network] Joining room: "
            << code
            << '\n';

        return true;
    }
    catch (const std::exception& e)
    {
        std::cout
            << "[Network] Join room failed: "
            << e.what()
            << '\n';

        connected = false;
        running = false;
        disconnected = true;

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
                    beast::buffers_to_string(
                        buffer.data()
                    );

                std::cout
                    << "[Network] Received: "
                    << msg
                    << '\n';

                NetworkPacket packet =
                    parsePacket(msg);

                switch (packet.type)
                {
                case MessageType::RoomCreated:
                {
                    roomCode = packet.payload;

                    std::cout
                        << "[Network] Room created: "
                        << roomCode
                        << '\n';

                    break;
                }

                case MessageType::RoomJoined:
                {
                    roomCode = packet.payload;

                    std::cout
                        << "[Network] Joined room: "
                        << roomCode
                        << '\n';

                    break;
                }

                case MessageType::RoomFull:
                {
                    std::cout
                        << "[Network] Room is full\n";

                    break;
                }

                case MessageType::RoomNotFound:
                {
                    std::cout
                        << "[Network] Room not found\n";

                    break;
                }
                case MessageType::Start:
                {
                    if (packet.payload == "WHITE")
                    {
                        myColor =
                            PlayerColor::White;

                        myTurn = true;
                        ready = true;

                        std::cout
                            << "[Network] You are WHITE\n";
                    }
                    else if (
                        packet.payload == "BLACK")
                    {
                        myColor =
                            PlayerColor::Black;

                        myTurn = false;
                        ready = true;

                        std::cout
                            << "[Network] You are BLACK\n";
                    }

                    break;
                }

                case MessageType::Move:
                {
                    MoveMessage move =
                        deserialize(packet.payload);

                    {
                        std::lock_guard<std::mutex>
                            lock(queueMutex);

                        incomingMoves.push(move);
                    }

                    myTurn = true;
                    std::cout << "[Network] Move queued\n";

                    break;
                }

                default:

                    std::cout
                        << "[Network] Unknown packet: "
                        << msg
                        << '\n';

                    break;
                }
            }
            catch (const std::exception& e)
            {
                // If disconnect() intentionally stopped the
                // thread, don't treat it as an opponent disconnect.
                if (!running)
                    break;

                std::cout
                    << "[Network] Connection lost: "
                    << e.what()
                    << '\n';

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
    if (!running && !connected)
        return;

    // Tell receive thread this is intentional.
    running = false;

    if (ws)
    {
        beast::error_code ec;

        // Cancel the underlying TCP operation so
        // ws->read() wakes up.
        beast::get_lowest_layer(*ws).cancel();
    }

    if (receiveThread.joinable())
        receiveThread.join();

    if (ws)
    {
        beast::error_code ec;

        ws->next_layer().shutdown(ec);

        beast::get_lowest_layer(*ws)
            .socket()
            .close(ec);

        ws.reset();
    }

    connected = false;
    ready = false;
    myTurn = false;
    roomCode.clear();

    disconnected = false;
}

bool NetworkClient::isConnected() const
{
    return connected;
}

bool NetworkClient::sendMove(
    const MoveRecord& move)
{
    if (!connected || !ws)
        return false;

    MoveMessage msg;

    msg.fromRow = move.fromRow;
    msg.fromCol = move.fromCol;
    msg.toRow = move.toRow;
    msg.toCol = move.toCol;
    msg.promotion = move.promotionPiece;

    std::string data =
        makePacket(
            MessageType::Move,
            serialize(msg)
        );

    try
    {
        ws->write(
            net::buffer(data)
        );

        myTurn = false;

        std::cout
            << "[Network] Sent: "
            << data
            << '\n';

        return true;
    }
    catch (const std::exception& e)
    {
        std::cout
            << "[Network] Send failed: "
            << e.what()
            << '\n';

        connected = false;
        running = false;
        disconnected = true;

        return false;
    }
}

bool NetworkClient::hasPendingMessages()
{
    std::lock_guard<std::mutex>
        lock(queueMutex);

    return !incomingMoves.empty();
}

MoveMessage NetworkClient::getNextMove()
{
    std::lock_guard<std::mutex>
        lock(queueMutex);

    if (incomingMoves.empty())
        return MoveMessage{};

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

std::string NetworkClient::getRoomCode() const
{
    return roomCode;
}