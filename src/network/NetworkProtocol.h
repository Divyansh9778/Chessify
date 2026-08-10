#pragma once

enum class MessageType
{
    Unknown,

    Start,
    Move,

    Resign,

    DrawOffer,
    DrawAccept,
    DrawDecline,

    Chat,

    Disconnect
};