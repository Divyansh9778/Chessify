#ifndef SETTINGS_H
#define SETTINGS_H

#include "Constants.h"

struct GameSettings
{
    bool vsEngine = false;
    int engineDepth = 12;

    PlayerColorChoice playerColor =
        PlayerColorChoice::WHITE;
};

extern GameSettings SETTINGS;

#endif