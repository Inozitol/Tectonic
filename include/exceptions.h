#ifndef TECTONIC_EXCEPTIONS_H
#define TECTONIC_EXCEPTIONS_H

#include <stdexcept>
#include <utility>

class tectonic_exception : public std::exception{
public:
    explicit tectonic_exception(std::string  msg): _msg(std::move(msg)) {}
    char* what(){
        return _msg.data();
    }
private:
    std::string _msg;
};

class window_exception : public tectonic_exception{
public:
    explicit window_exception(const std::string& msg) : tectonic_exception(msg){}
};

class texture_exception : public tectonic_exception{
public:
    explicit texture_exception(const std::string& msg) : tectonic_exception(msg){}
};

class mesh_exception : public tectonic_exception{
public:
    explicit mesh_exception(const std::string& msg) : tectonic_exception(msg){}
};

class technique_exception : public tectonic_exception{
public:
    explicit technique_exception(const std::string& msg) : tectonic_exception(msg){}
};
#endif //TECTONIC_EXCEPTIONS_H
