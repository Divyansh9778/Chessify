#ifndef SETTINGS_H
#define SETTINGS_H

struct GameSettings
{
    bool vsEngine = false;
    int engineDepth = 12;
};

extern GameSettings SETTINGS;

#endif