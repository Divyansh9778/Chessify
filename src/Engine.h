#ifndef ENGINE_H
#define ENGINE_H

#include <string>
#include <windows.h>

class Engine
{
public:
    static bool start();
    static bool init();
    static void stop();

    static void send(const std::string& cmd);
    static bool read(std::string& out);
    static bool waitFor(const std::string& token);

    static std::string waitBestmove();
    static std::string extractMove(const std::string& bestline);

private:
    static HANDLE stdoutRead;
    static HANDLE stdinWrite;
    static PROCESS_INFORMATION pi;
};

#endif
