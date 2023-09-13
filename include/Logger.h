#ifndef TECTONIC_LOGGER_H
#define TECTONIC_LOGGER_H

#include <string>
#include <iostream>
#include <chrono>

class Logger {
public:
    explicit Logger(std::string  identifier);

    enum LogLevel : uint8_t{
        DEBUG = 0,
        INFO = 1,
        WARNING = 2,
        ERROR = 3,
    };

    Logger& operator()(LogLevel level);
    friend Logger& operator<<(Logger& l, const std::string& str);
    friend Logger& operator<<(Logger& l, const char* str);
    friend Logger& operator<<(Logger& l, char ch);
    friend Logger& operator<<(Logger& l, uint32_t num);
    friend Logger& operator<<(Logger& l, uint8_t num);
    friend Logger& operator<<(Logger& l, float num);
    friend Logger& operator<<(Logger& l, int num);

    static void setOutputLevel(LogLevel level);

private:

    std::string getLevel();
    std::string getTime();

    const std::string m_identity;
    LogLevel m_messageLevel;

    std::ostream& m_outStream;
    static std::string m_timeFormat;
    static LogLevel m_logLevel;

};

#endif //TECTONIC_LOGGER_H
