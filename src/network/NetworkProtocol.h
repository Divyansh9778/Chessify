#pragma once

enum class MessageType
{
    Unknown,

    // Game
    Start,
    Move,

    // Room management
    CreateRoom,
    JoinRoom,
    RoomCreated,
    RoomJoined,
    RoomFull,
    RoomNotFound,

    // Game actions
    Resign,

    DrawOffer,
    DrawAccept,
    DrawDecline,

    Chat,

    Disconnect
};