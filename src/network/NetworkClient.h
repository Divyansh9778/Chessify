#pragma once
#include "../MoveRecord.h"
#include "NetworkMessages.h"
#include "PlayerColor.h"
    
#include <queue>
#include <memory>

#include <thread>
#include <atomic>

#include <boost/asio.hpp>
#include <boost/beast/websocket.hpp>

namespace net = boost::asio;
namespace websocket = boost::beast::websocket;
using tcp = net::ip::tcp;

class NetworkClient
{
public:
    NetworkClient();
    ~NetworkClient();

    bool connect(const std::string &address, int port);
    void disconnect();

    bool isConnected() const;

    bool sendMove(const MoveRecord &move);
    //bool receiveMessage(std::string& msg);

    bool hasPendingMessages();
    MoveMessage getNextMove();

    bool isReady() const;

    void startListening();
    //void stopListening();

    PlayerColor getPlayerColor() const;
    bool isMyTurn() const;

    bool hasDisconnected() const;
    void clearDisconnectFlag();

private:
    net::io_context ioContext;
    std::unique_ptr<websocket::stream<tcp::socket>> ws;

    std::queue<MoveMessage> incomingMoves;
    std::mutex queueMutex;
    
    std::thread receiveThread;

    PlayerColor myColor = PlayerColor::None;

    std::atomic<bool> running = false;
    std::atomic<bool> ready = false;
    std::atomic<bool> myTurn = false;
    std::atomic<bool> connected = false;
    std::atomic<bool> disconnected = false;
};