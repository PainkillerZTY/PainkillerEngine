#include "Logger.h"
#include <iostream>
#include <iomanip>
#include <windows.h>

namespace nebula {

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    InitializeCriticalSection(&m_criticalSection);
}

Logger::~Logger() {
    DeleteCriticalSection(&m_criticalSection);
    if (m_fileStream.is_open()) {
        m_fileStream.close();
    }
}

void Logger::SetLevel(LogLevel level) {
    m_level = level;
}

void Logger::SetFileOutput(const std::string& path) {
    EnterCriticalSection(&m_criticalSection);
    if (m_fileStream.is_open()) {
        m_fileStream.close();
    }
    m_fileStream.open(path, std::ios::out | std::ios::trunc);
    LeaveCriticalSection(&m_criticalSection);
}

std::string Logger::GetLevelPrefix(LogLevel level) const {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
        default:              return "UNKNOWN";
    }
}

void Logger::Log(LogLevel level, const char* file, i32 line, const std::string& message) {
    EnterCriticalSection(&m_criticalSection);

    SYSTEMTIME st;
    GetLocalTime(&st);

    std::ostringstream oss;
    oss << "["
        << (st.wHour < 10 ? "0" : "") << st.wHour << ":"
        << (st.wMinute < 10 ? "0" : "") << st.wMinute << ":"
        << (st.wSecond < 10 ? "0" : "") << st.wSecond << "]"
        << "[" << GetLevelPrefix(level) << "]"
        << "(" << file << ":" << line << ") "
        << message;

    std::string formatted = oss.str();

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    switch (level) {
        case LogLevel::Error:
        case LogLevel::Fatal: SetConsoleTextAttribute(hConsole, FOREGROUND_RED); break;
        case LogLevel::Warn:  SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN); break;
        case LogLevel::Info:  SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN); break;
        case LogLevel::Debug: SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN); break;
        default:              SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); break;
    }

    std::cout << formatted << std::endl;
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

    if (m_fileStream.is_open()) {
        m_fileStream << formatted << std::endl;
        m_fileStream.flush();
    }

    LeaveCriticalSection(&m_criticalSection);

    if (level == LogLevel::Fatal) {
        std::abort();
    }
}

} // namespace nebula
