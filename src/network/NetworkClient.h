#pragma once

#include "../MoveRecord.h"
#include "NetworkMessages.h"
#include "PlayerColor.h"

#include <queue>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <string>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>

namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = net::ip::tcp;

class NetworkClient
{
public:
    NetworkClient();
    ~NetworkClient();

    bool connect(const std::string& address, int port);
    void disconnect();

    bool isConnected() const;

    // Room management
    bool createRoom();
    bool joinRoom(const std::string& roomCode);
    std::string getRoomCode() const;

    bool sendMove(const MoveRecord& move);

    bool hasPendingMessages();
    MoveMessage getNextMove();

    bool isReady() const;

    bool roomFull = false;
    bool roomJoined = false;

    bool consumeRoomFull();
    bool consumeRoomJoined();

    void startListening();

    PlayerColor getPlayerColor() const;
    bool isMyTurn() const;
    void setMyTurn(bool turn);

    bool hasDisconnected() const;
    void clearDisconnectFlag();

private:
    net::io_context ioContext;

    ssl::context sslContext{
        ssl::context::tls_client
    };

    using WebSocketStream =
        websocket::stream<
        beast::ssl_stream<
        beast::tcp_stream
        >
        >;

    std::unique_ptr<WebSocketStream> ws;

    std::queue<MoveMessage> incomingMoves;
    std::mutex queueMutex;

    std::atomic<bool> running = false;
    std::atomic<bool> ready = false;
    std::atomic<bool> myTurn = false;
    std::atomic<bool> connected = false;
    std::atomic<bool> disconnected = false;

    std::thread receiveThread;

    PlayerColor myColor = PlayerColor::None;

    std::string roomCode;
};