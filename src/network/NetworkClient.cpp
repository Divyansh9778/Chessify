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
    sslContext.set_verify_mode(ssl::verify_none);
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

    std::cout
        << "[Network] Connecting to "
        << address
        << ":"
        << port
        << '\n';

    try
    {
        // -----------------------------------------
        // DNS RESOLUTION
        // -----------------------------------------

        std::cout << "[Network] Resolving...\n";

        tcp::resolver resolver(ioContext);

        auto results =
            resolver.resolve(
                address,
                std::to_string(port)
            );

        std::cout
            << "[Network] DNS resolved\n";


        // -----------------------------------------
        // CREATE WEBSOCKET
        // -----------------------------------------

        ws = std::make_unique<WebSocketStream>(
            ioContext,
            sslContext
        );


        // -----------------------------------------
        // TCP CONNECTION
        // -----------------------------------------

        std::cout
            << "[Network] Connecting TCP...\n";

        beast::get_lowest_layer(*ws)
            .connect(results);

        std::cout
            << "[Network] TCP connected\n";


        // -----------------------------------------
        // SNI
        // -----------------------------------------

        std::cout
            << "[Network] Setting SNI...\n";

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

        std::cout
            << "[Network] SNI configured\n";


        // -----------------------------------------
        // CERTIFICATE VERIFICATION
        // -----------------------------------------

        //ws->next_layer().set_verify_callback(
        //    ssl::host_name_verification(address)
        //);


        // -----------------------------------------`
        // TLS HANDSHAKE
        // -----------------------------------------

        std::cout
            << "[Network] TLS handshake...\n";

        ws->next_layer().handshake(
            ssl::stream_base::client
        );

        std::cout
            << "[Network] TLS handshake successful\n";


        // -----------------------------------------
        // WEBSOCKET HANDSHAKE
        // -----------------------------------------

        std::cout
            << "[Network] WebSocket handshake...\n";

        ws->handshake(
            address,
            "/"
        );

        std::cout
            << "[Network] WebSocket handshake successful\n";


        // -----------------------------------------
        // CONNECTED
        // -----------------------------------------

        connected = true;
        running = true;
        disconnected = false;
        ready = false;
        myTurn = false;

        roomFull = false;
        roomJoined = false;

        startListening();

        std::cout
            << "[Network] Connected to Railway: "
            << address
            << ":"
            << port
            << '\n';

        return true;
    }
    catch (const std::exception& e)
    {
        std::cout
            << "\n========== NETWORK ERROR ==========\n"
            << e.what()
            << "\n===================================\n";

        connected = false;
        running = false;
        ready = false;
        myTurn = false;

        if (ws)
        {
            beast::error_code ec;

            // Close underlying TCP connection.
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
                    roomJoined = true;

                    std::cout
                        << "[Network] Joined room: "
                        << roomCode
                        << '\n';

                    break;
                }

                case MessageType::RoomFull:
                {
                    std::cout << "[Network] Room is full\n";

                    roomFull = true;
                    myTurn = false;
                    myColor = PlayerColor::None;

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

                    //myTurn = true;
                    std::cout << "[Network] Move queued\n";

                    break;
                }

                case MessageType::Disconnect:
                {
                    std::cout << "[Network] Opponent disconnected\n";

                    running = false;
                    connected = false;
                    disconnected = true;
                    ready = false;
                    myTurn = false;

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
    std::cout << "[Network] Disconnecting...\n";

    // Stop receive thread
    running = false;

    // Wake up ws->read()
    if (ws)
    {
        beast::get_lowest_layer(*ws).cancel();
    }

    // IMPORTANT:
    // Always join the receive thread if it is joinable.
    if (receiveThread.joinable())
    {
        if (receiveThread.get_id() != std::this_thread::get_id())
        {
            receiveThread.join();
        }
    }

    // Now it is safe to close the socket.
    if (ws)
    {
        beast::error_code ec;

        // SSL shutdown
        ws->next_layer().shutdown(ec);

        // Close underlying TCP socket
        beast::get_lowest_layer(*ws)
            .socket()
            .close(ec);

        ws.reset();
    }

    connected = false;
    ready = false;
    myTurn = false;

    roomFull = false;
    roomJoined = false;

    myColor = PlayerColor::None;

    roomCode.clear();

    disconnected = false;

    std::cout << "[Network] Disconnected\n";
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

    bool pending = !incomingMoves.empty();

    if (pending)
        std::cout << "[Network] hasPendingMessages() = TRUE\n";

    return pending;
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

void NetworkClient::setMyTurn(bool turn)
{
    myTurn = turn;
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

bool NetworkClient::consumeRoomFull()
{
    if (!roomFull)
        return false;

    roomFull = false;
    return true;
}

bool NetworkClient::consumeRoomJoined()
{
    if (!roomJoined)
        return false;

    roomJoined = false;
    return true;
}