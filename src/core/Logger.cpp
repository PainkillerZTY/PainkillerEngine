#include "Logger.h"
#include <iostream>
#include <ctime>
#include <iomanip>

namespace nebula {

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    if (m_fileStream.is_open()) {
        m_fileStream.close();
    }
}

void Logger::SetLevel(LogLevel level) {
    m_level = level;
}

void Logger::SetFileOutput(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_fileStream.is_open()) {
        m_fileStream.close();
    }
    m_fileStream.open(path, std::ios::out | std::ios::trunc);
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
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Get timestamp
    auto now = std::time(nullptr);
    std::tm tm;
    localtime_s(&tm, &now);
    
    std::ostringstream oss;
    oss << "[" << std::put_time(&tm, "%H:%M:%S") << "]"
        << "[" << GetLevelPrefix(level) << "]"
        << "(" << file << ":" << line << ") "
        << message;
    
    std::string formatted = oss.str();
    
    // Console output with color
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    switch (level) {
        case LogLevel::Error:
        case LogLevel::Fatal:
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
            break;
        case LogLevel::Warn:
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN);
            break;
        case LogLevel::Info:
            SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
            break;
        case LogLevel::Debug:
            SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN);
            break;
        default:
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            break;
    }
    
    std::cout << formatted << std::endl;
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    
    // File output
    if (m_fileStream.is_open()) {
        m_fileStream << formatted << std::endl;
        m_fileStream.flush();
    }
    
    if (level == LogLevel::Fatal) {
        std::abort();
    }
}

} // namespace nebula
