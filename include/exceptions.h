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

    tectonicException() = default;

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
    windowException() : tectonicException(){}
};

class cameraException : public tectonicException {
public:
    template<typename ...T>
    explicit cameraException(T... args) : tectonicException(args...){}
    cameraException() : tectonicException(){}
};

class textureException : public tectonicException {
public:
    template<typename ...T>
    explicit textureException(T... args) : tectonicException(args...) {};
    textureException() : tectonicException(){}
};

class modelException : public tectonicException {
public:
    template<typename ...T>
    explicit modelException(T... args) : tectonicException(args...){}
    modelException() : tectonicException(){}
};

class shaderException : public tectonicException {
public:
    template<typename ...T>
    explicit shaderException(T... args) : tectonicException(args...){}
    shaderException() : tectonicException(){}
};

class shadowMapException : public tectonicException {
public:
    template<typename ...T>
    explicit shadowMapException(T... args) : tectonicException(args...){}
    shadowMapException() : tectonicException(){}
};

class sceneException : public tectonicException {
public:
    template<typename ...T>
    explicit sceneException(T... args) : tectonicException(args...){}
    sceneException() : tectonicException(){}
};

class keyboardException : public tectonicException {
public:
    template<typename ...T>
    explicit keyboardException(T... args) : tectonicException(args...){}
    keyboardException() : tectonicException(){}
};

class rendererException : public tectonicException {
public:
    template<typename ...T>
    explicit rendererException(T... args) : tectonicException(args...){}
    rendererException() : tectonicException(){}
};

class modelLoaderException : public tectonicException {
public:
    template<typename ...T>
    explicit modelLoaderException(T... args) : tectonicException(args...){}
    modelLoaderException() : tectonicException(){}
};
#endif //TECTONIC_EXCEPTIONS_H
