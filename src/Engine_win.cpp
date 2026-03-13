#include "Engine.h"
#include "Settings.h"

#ifdef _WIN32

#include <windows.h>
#include <iostream>
#include <algorithm>
#include <string>

static HANDLE stdoutRead = NULL;
static HANDLE stdinWrite = NULL;
static PROCESS_INFORMATION pi{};

bool Engine::start()
{
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE stdinRead = NULL, stdoutWrite = NULL;

    if (!CreatePipe(&stdoutRead, &stdoutWrite, &sa, 0))
        return false;

    if (!CreatePipe(&stdinRead, &stdinWrite, &sa, 0))
        return false;

    SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(stdinWrite, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = stdinRead;
    si.hStdOutput = stdoutWrite;
    si.hStdError = stdoutWrite;

    if (!CreateProcessA(
            NULL,
            (LPSTR) "stockfish.exe",
            NULL, NULL, TRUE,
            CREATE_NO_WINDOW,
            NULL, NULL,
            &si, &pi))
    {
        std::cerr << "ERROR: Could not start Stockfish.\n";
        return false;
    }

    CloseHandle(stdinRead);
    CloseHandle(stdoutWrite);

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
    DWORD written;
    std::string out = cmd + "\n";
    WriteFile(stdinWrite, out.c_str(), (DWORD)out.size(), &written, NULL);
}

bool Engine::read(std::string &out)
{
    DWORD bytesAvailable = 0;

    if (!PeekNamedPipe(stdoutRead, NULL, 0, NULL, &bytesAvailable, NULL))
        return false;

    if (bytesAvailable == 0)
        return false;

    char buffer[4096];
    DWORD toRead = std::min<DWORD>(bytesAvailable, sizeof(buffer) - 1);
    DWORD readBytes = 0;

    if (ReadFile(stdoutRead, buffer, toRead, &readBytes, NULL) && readBytes > 0)
    {
        buffer[readBytes] = '\0';
        out += buffer;
        return true;
    }

    return false;
}

bool Engine::waitFor(const std::string &token)
{
    std::string acc;
    ULONGLONG start = GetTickCount64();

    while (GetTickCount64() - start < 8000)
    {
        read(acc);
        if (acc.find(token) != std::string::npos)
            return true;
    }

    return false;
}

std::string Engine::waitBestmove()
{
    std::string acc;
    ULONGLONG start = GetTickCount64();

    while (GetTickCount64() - start < 20000)
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
    }

    return "";
}

std::string Engine::extractMove(const std::string& bestline)
{
    size_t pos = bestline.find("bestmove ");
    if (pos == std::string::npos)
        return "";

    pos += 9;

    // Extract until space or newline
    size_t end = bestline.find_first_of(" \n", pos);

    if (end == std::string::npos)
        end = bestline.size();

    return bestline.substr(pos, end - pos);
}

void Engine::stop()
{
    send("quit");
    WaitForSingleObject(pi.hProcess, 2000);

    if (stdinWrite) CloseHandle(stdinWrite);
    if (stdoutRead) CloseHandle(stdoutRead);
    if (pi.hProcess) CloseHandle(pi.hProcess);
    if (pi.hThread) CloseHandle(pi.hThread);
}

#endif
