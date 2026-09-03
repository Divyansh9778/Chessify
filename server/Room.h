#ifndef ROOM_H
#define ROOM_H

#include <string>
#include <memory>
#include <mutex>

#include <boost/asio.hpp>
#include <boost/beast/websocket.hpp>

namespace net = boost::asio;
namespace websocket = boost::beast::websocket;

using tcp = net::ip::tcp;

enum class Turn
{
    White,
    Black
};

struct GameRoom
{
    std::string id;

    std::shared_ptr<
        websocket::stream<tcp::socket>
    > white;

    std::shared_ptr<
        websocket::stream<tcp::socket>
    > black;

    std::mutex roomMutex;
    std::mutex writeMutex;

    bool started = false;

    Turn currentTurn = Turn::White;
};

#endif