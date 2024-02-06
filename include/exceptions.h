#ifndef TECTONIC_EXCEPTIONS_H
#define TECTONIC_EXCEPTIONS_H

#include <stdexcept>
#include <utility>
#include <cstdarg>

#define TECTONIC_EXCEPTION(newException, baseException)                 \
class newException : public baseException {                             \
public:                                                                 \
    template<typename ...T>                                             \
    explicit newException(T... args) : baseException(args...){}         \
    newException() : baseException(){}                                  \
};

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

TECTONIC_EXCEPTION(engineException,         tectonicException)
TECTONIC_EXCEPTION(windowException,         engineException)
TECTONIC_EXCEPTION(keyboardException,       engineException)
TECTONIC_EXCEPTION(vulkanException,         engineException)

TECTONIC_EXCEPTION(cameraException,         tectonicException)
TECTONIC_EXCEPTION(textureException,        tectonicException)
TECTONIC_EXCEPTION(modelException,          tectonicException)
TECTONIC_EXCEPTION(shaderException,         tectonicException)
TECTONIC_EXCEPTION(shadowMapException,      tectonicException)
TECTONIC_EXCEPTION(sceneException,          tectonicException)
TECTONIC_EXCEPTION(modelLoaderException,    tectonicException)

#endif //TECTONIC_EXCEPTIONS_H
