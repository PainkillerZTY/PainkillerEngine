#pragma once

#include "Types.h"
#include <string>
#include <sstream>
#include <fstream>
#include <mutex>

namespace nebula {

enum class LogLevel : u8 {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
};

class Logger {
public:
    static Logger& Instance();

    void SetLevel(LogLevel level);
    void SetFileOutput(const std::string& path);
    
    void Log(LogLevel level, const char* file, i32 line, const std::string& message);
    
    template<typename... Args>
    void Trace(const char* file, i32 line, const std::string& fmt, Args&&... args) {
        if (m_level <= LogLevel::Trace)
            Log(LogLevel::Trace, file, line, Format(fmt, std::forward<Args>(args)...));
    }
    
    template<typename... Args>
    void Debug(const char* file, i32 line, const std::string& fmt, Args&&... args) {
        if (m_level <= LogLevel::Debug)
            Log(LogLevel::Debug, file, line, Format(fmt, std::forward<Args>(args)...));
    }
    
    template<typename... Args>
    void Info(const char* file, i32 line, const std::string& fmt, Args&&... args) {
        if (m_level <= LogLevel::Info)
            Log(LogLevel::Info, file, line, Format(fmt, std::forward<Args>(args)...));
    }
    
    template<typename... Args>
    void Warn(const char* file, i32 line, const std::string& fmt, Args&&... args) {
        if (m_level <= LogLevel::Warn)
            Log(LogLevel::Warn, file, line, Format(fmt, std::forward<Args>(args)...));
    }
    
    template<typename... Args>
    void Error(const char* file, i32 line, const std::string& fmt, Args&&... args) {
        if (m_level <= LogLevel::Error)
            Log(LogLevel::Error, file, line, Format(fmt, std::forward<Args>(args)...));
    }

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    std::string GetLevelPrefix(LogLevel level) const;
    
    template<typename T>
    void FormatArg(std::ostringstream& oss, T&& val) {
        oss << std::forward<T>(val);
    }
    
    std::string Format(const std::string& fmt) { return fmt; }
    
    template<typename T, typename... Args>
    std::string Format(const std::string& fmt, T&& first, Args&&... rest) {
        std::ostringstream oss;
        usize pos = 0;
        usize start = 0;
        
        while ((pos = fmt.find("{}", start)) != std::string::npos) {
            oss << fmt.substr(start, pos - start);
            FormatArg(oss, std::forward<T>(first));
            start = pos + 2;
            // Forward remaining args recursively
            return FormatRemaining(oss, fmt.substr(start), std::forward<Args>(rest)...);
        }
        
        oss << fmt.substr(start);
        return oss.str();
    }
    
    std::string FormatRemaining(std::ostringstream& oss, const std::string& fmt) {
        oss << fmt;
        return oss.str();
    }
    
    template<typename T, typename... Args>
    std::string FormatRemaining(std::ostringstream& oss, const std::string& fmt, T&& first, Args&&... rest) {
        usize pos = fmt.find("{}");
        if (pos != std::string::npos) {
            oss << fmt.substr(0, pos);
            FormatArg(oss, std::forward<T>(first));
            return FormatRemaining(oss, fmt.substr(pos + 2), std::forward<Args>(rest)...);
        }
        oss << fmt;
        return oss.str();
    }
    
    LogLevel m_level = LogLevel::Info;
    std::ofstream m_fileStream;
    std::mutex m_mutex;
};

} // namespace nebula

// ?? Convenience Macros ??
#define LOG_TRACE(...)   nebula::Logger::Instance().Trace(__FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...)   nebula::Logger::Instance().Debug(__FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)    nebula::Logger::Instance().Info(__FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)    nebula::Logger::Instance().Warn(__FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...)   nebula::Logger::Instance().Error(__FILE__, __LINE__, __VA_ARGS__)
