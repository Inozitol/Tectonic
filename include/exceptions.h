#ifndef TECTONIC_EXCEPTIONS_H
#define TECTONIC_EXCEPTIONS_H

#include <stdexcept>
#include <utility>
#include <cstdarg>

class tectonic_exception : public std::exception{
public:
    template<typename T>
    explicit tectonic_exception(T str){
        _msg.append(str);
    }
    template<typename T, typename ...Types>
    explicit tectonic_exception(T str, Types... rest){
        _msg.append(str);
        load_rest(rest...);
    }
    char* what(){
        return _msg.data();
    }

private:
    template<typename T>
    void load_rest(T str){
        _msg.append(str);
    }

    template<typename T, typename ...Types>
    void load_rest(T str, Types... rest){
        _msg.append(str);
        load_rest(rest...);
    }
    std::string _msg;
};

class window_exception : public tectonic_exception {
public:
    template<typename ...T>
    explicit window_exception(T... args) : tectonic_exception(args...){}
};

class texture_exception : public tectonic_exception {
public:
    template<typename ...T>
    explicit texture_exception(T... args) : tectonic_exception(args...) {};
};

class mesh_exception : public tectonic_exception {
public:
    template<typename ...T>
    explicit mesh_exception(T... args) : tectonic_exception(args...){}
};

class technique_exception : public tectonic_exception {
public:
    template<typename ...T>
    explicit technique_exception(T... args) : tectonic_exception(args...){}
};
#endif //TECTONIC_EXCEPTIONS_H
