#pragma once

#include <iostream>
#include <cstdint>

#define LOG_DEBUG "Debug"
#define LOG_INFO "Info"
#define LOG_WARNING "Warning"
#define LOG_ERROR "Error"

// Implementation from https://stackoverflow.com/questions/19415845/a-better-log-macro-using-template-metaprogramming

#define LOG(lvl, msg) (Log(lvl, __FILE__, __LINE__, LogData<None>() << msg))

#ifndef NOINLINE_ATTRIBUTE
  #ifdef __ICC
    #define NOINLINE_ATTRIBUTE __attribute__(( noinline ))
  #else
    #define NOINLINE_ATTRIBUTE
  #endif // __ICC
#endif // NOINLINE_ATTRIBUTE

template<typename L>
struct LogData {
  L list;
};

struct None {};

template<typename Begin, typename Value>
constexpr LogData<std::pair<Begin&&, Value&&>> operator<<(LogData<Begin>&& begin,
                                                          Value&& value) noexcept{
  return {{ std::forward<Begin>(begin.list), std::forward<Value>(value) }};
}

template<typename Begin, size_t n>
constexpr LogData<std::pair<Begin&&, const char*>> operator<<(LogData<Begin>&& begin,
                                                              const char (&value)[n]) noexcept{
  return {{ std::forward<Begin>(begin.list), value }};
}

typedef std::ostream& (*PfnManipulator)(std::ostream&);

template<typename Begin>
constexpr LogData<std::pair<Begin&&, PfnManipulator>> operator<<(LogData<Begin>&& begin,
                                                                 PfnManipulator value) noexcept{
  return {{ std::forward<Begin>(begin.list), value }};
}

template <typename Begin, typename Last>
void output(std::ostream& os, std::pair<Begin, Last>&& data){
  output(os, std::move(data.first));
  os << data.second;
}

inline void output(std::ostream&, None)
{ }

template<typename L>
void Log(const char* lvl, const char* file, uint32_t line, LogData<L>&& data) {
  std::cout << '[' << lvl << "] " << file << ":" << line << ": ";
  output(std::cout, std::move(data.list));
  std::cout << std::endl;
}
