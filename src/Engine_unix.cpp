#include "Engine.h"
#include "Settings.h"

#ifndef _WIN32

#include <cstdio>
#include <iostream>
#include <algorithm>
#include <string>
#include <unistd.h>

static FILE *engine = nullptr;

bool Engine::start()
{
    engine = popen("./stockfish", "r+");
    if (!engine)
    {
        std::cerr << "ERROR: Could not start Stockfish\n";
        return false;
    }
    return true;
}

bool Engine::init()
{
    send("uci");
    if (!waitFor("uciok"))
        return false;

    // Apply ELO if playing vs engine
    if (SETTINGS.vsEngine)
    {
        int realElo = SETTINGS.engineDepth;

        send("setoption name UCI_LimitStrength value true");
        send("setoption name UCI_Elo value " + std::to_string(realElo));
    }

    send("isready");
    if (!waitFor("readyok"))
        return false;

    return true;
}

void Engine::goMoveTime(int ms)
{
    send("go movetime " + std::to_string(ms));
}

void Engine::send(const std::string &cmd)
{
    if (!engine)
        return;
    fprintf(engine, "%s\n", cmd.c_str());
    fflush(engine);
}

bool Engine::read(std::string &out)
{
    if (!engine)
        return false;

    char buffer[4096];
    if (fgets(buffer, sizeof(buffer), engine))
    {
        out += buffer;
        return true;
    }

    return false;
}

bool Engine::waitFor(const std::string &token)
{
    std::string acc;

    for (int i = 0; i < 8000; ++i)
    {
        read(acc);
        if (acc.find(token) != std::string::npos)
            return true;

        usleep(1000);
    }

    return false;
}

std::string Engine::waitBestmove()
{
    std::string acc;

    for (int i = 0; i < 20000; ++i)
    {
        read(acc);

        size_t pos = acc.find("bestmove ");
        if (pos != std::string::npos)
        {
            size_t end = acc.find('\n', pos);
            if (end == std::string::npos)
                end = acc.size();

            return acc.substr(pos, end - pos);
        }

        usleep(1000);
    }

    return "";
}

std::string Engine::extractMove(const std::string &bestline)
{
    size_t pos = bestline.find("bestmove ");
    if (pos == std::string::npos)
        return "";

    pos += 9;
    return bestline.substr(pos, 4);
}

void Engine::stop()
{
    if (engine)
    {
        pclose(engine);
        engine = nullptr;
    }
}

#endif
