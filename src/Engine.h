#ifndef ENGINE_H
#define ENGINE_H

#include <string>

class Engine
{
public:
    static bool start();
    static bool init();
    static void stop();

    static void send(const std::string &cmd);
    static bool read(std::string &out);
    static bool waitFor(const std::string &token);

    void goMoveTime(int ms);

    static std::string waitBestmove();
    static std::string extractMove(const std::string &bestline);
};

#endif
