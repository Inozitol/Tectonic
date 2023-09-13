#include "Logger.h"

#include <utility>
#include <iomanip>

std::string Logger::m_timeFormat =  "%H:%M:%S";
Logger::LogLevel Logger::m_logLevel = Logger::DEBUG;

Logger::Logger(std::string identifier)
: m_identity(std::move(identifier)), m_outStream(std::cout){}

void Logger::setOutputLevel(Logger::LogLevel level) {
    m_logLevel = level;
}

Logger &Logger::operator()(Logger::LogLevel level) {
    m_messageLevel = level;
    if(m_messageLevel >= m_logLevel){
        m_outStream << "[" << getTime() << "] [" << getLevel() << "] [" << m_identity << "] ";
    }
    return *this;
}

std::string Logger::getTime() {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, m_timeFormat.data());
    std::string s = oss.str();
    return oss.str();
}

std::string Logger::getLevel() {
    switch(m_messageLevel){
        case DEBUG:
            return "DEBUG";
        case INFO:
            return "INFO";
        case WARNING:
            return "WARNING";
        case ERROR:
            return "ERROR";
        default:
            return "";
    }
}

Logger& operator<<(Logger& l, const std::string& str) {
    if(l.m_messageLevel >= l.m_logLevel){
        l.m_outStream << str;
    }
    return l;
}

Logger& operator<<(Logger& l, const char* str) {
    if(l.m_messageLevel >= l.m_logLevel){
        l.m_outStream << str;
    }
    return l;
}

Logger& operator<<(Logger& l, char ch) {
    if(l.m_messageLevel >= l.m_logLevel){
        l.m_outStream << ch;
    }
    return l;
}

Logger& operator<<(Logger& l, uint32_t num) {
    if(l.m_messageLevel >= l.m_logLevel){
        l.m_outStream << '[' << num << ']';
    }
    return l;
}

Logger& operator<<(Logger& l, uint8_t num) {
    if(l.m_messageLevel >= l.m_logLevel){
        l.m_outStream << '[' << +num << ']';
    }
    return l;
}

Logger& operator<<(Logger& l, float num) {
    if(l.m_messageLevel >= l.m_logLevel){
        l.m_outStream << '[' << num << ']';
    }
    return l;
}

Logger& operator<<(Logger& l, int num) {
    if(l.m_messageLevel >= l.m_logLevel){
        l.m_outStream << '[' << num << ']';
    }
    return l;
}