#ifndef TECTONIC_EXCEPTIONS_H
#define TECTONIC_EXCEPTIONS_H

#include <stdexcept>
#include <utility>
#include <cstdarg>

class tectonicException : public std::exception{
public:
    template<typename T>
    explicit tectonicException(T str){
        _msg.append(str);
    }
    template<typename T, typename ...Types>
    explicit tectonicException(T str, Types... rest){
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

class windowException : public tectonicException {
public:
    template<typename ...T>
    explicit windowException(T... args) : tectonicException(args...){}
};

class cameraException : public tectonicException {
public:
    template<typename ...T>
    explicit cameraException(T... args) : tectonicException(args...){}
};

class textureException : public tectonicException {
public:
    template<typename ...T>
    explicit textureException(T... args) : tectonicException(args...) {};
};

class meshException : public tectonicException {
public:
    template<typename ...T>
    explicit meshException(T... args) : tectonicException(args...){}
};

class shaderException : public tectonicException {
public:
    template<typename ...T>
    explicit shaderException(T... args) : tectonicException(args...){}
};

class shadowMapException : public tectonicException {
public:
    template<typename ...T>
    explicit shadowMapException(T... args) : tectonicException(args...){}
};

class sceneException : public tectonicException {
public:
    template<typename ...T>
    explicit sceneException(T... args) : tectonicException(args...){}
};
#endif //TECTONIC_EXCEPTIONS_H
