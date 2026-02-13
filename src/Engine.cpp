#include "Engine.h"

#include <iostream>
#include <algorithm>
#include <string>

HANDLE Engine::stdoutRead = NULL;
HANDLE Engine::stdinWrite = NULL;
PROCESS_INFORMATION Engine::pi{};

bool Engine::start()
{
    SECURITY_ATTRIBUTES sa{ sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    HANDLE stdinRead, stdoutWrite;

    // Create pipes
    CreatePipe(&stdoutRead, &stdoutWrite, &sa, 0);
    CreatePipe(&stdinRead, &stdinWrite, &sa, 0);

    // Prevent inheritance on our ends
    SetHandleInformation(stdoutRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(stdinWrite, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = stdinRead;
    si.hStdOutput = stdoutWrite;
    si.hStdError = stdoutWrite;

    if (!CreateProcessA(
        "stockfish.exe",
        NULL, NULL, NULL, TRUE, CREATE_NO_WINDOW,
        NULL, NULL, &si, &pi))
    {
        std::cerr << "ERROR: Could not start Stockfish.\n";
        return false;
    }

    // Close child's side of pipes
    CloseHandle(stdinRead);
    CloseHandle(stdoutWrite);

    return true;
}

bool Engine::init()
{
    send("uci");
    if (!waitFor("uciok"))
    {
        std::cerr << "WARNING: uciok not received\n";
        return false;
    }

    send("isready");
    if (!waitFor("readyok"))
    {
        std::cerr << "WARNING: readyok not received\n";
        return false;
    }

    return true;
}

void Engine::send(const std::string& cmd)
{
    DWORD written;
    std::string out = cmd + "\n";
    WriteFile(stdinWrite, out.c_str(), (DWORD)out.size(), &written, NULL);
}

bool Engine::read(std::string& out)
{
    DWORD bytesAvailable = 0;

    if (!PeekNamedPipe(stdoutRead, NULL, 0, NULL, &bytesAvailable, NULL))
        return false;

    if (bytesAvailable == 0)
        return false;

    CHAR buffer[4096];
    DWORD toRead = std::min<DWORD>(bytesAvailable, sizeof(buffer) - 1);
    DWORD read = 0;

    if (ReadFile(stdoutRead, buffer, toRead, &read, NULL) && read > 0)
    {
        buffer[read] = '\0';
        out += buffer;
        return true;
    }

    return false;
}

bool Engine::waitFor(const std::string& token)
{
    const int timeout_ms = 8000;

    std::string acc;
    DWORD start = GetTickCount64();

    while ((int)(GetTickCount64() - start) < timeout_ms)
    {
        read(acc);

        if (acc.find(token) != std::string::npos)
            return true;
    }

    return false;
}

std::string Engine::waitBestmove()
{
    const int timeout_ms = 20000;

    std::string acc;
    DWORD start = GetTickCount64();

    while ((int)(GetTickCount64() - start) < timeout_ms)
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

    pos += 9; // skip "bestmove "

    if (pos + 4 > bestline.size())
        return "";

    return bestline.substr(pos, 4);
}

void Engine::stop()
{
    CloseHandle(stdinWrite);
    CloseHandle(stdoutRead);

    TerminateProcess(pi.hProcess, 0);
}
