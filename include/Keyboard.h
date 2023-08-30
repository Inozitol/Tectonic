#ifndef TECTONIC_KEYBOARD_H
#define TECTONIC_KEYBOARD_H

#include <GLFW/glfw3.h>
#include <string>
#include "exceptions.h"
#include "meta/Slot.h"
#include "meta/Signal.h"

struct keyboardButtonInfo {
    int32_t key;
    int32_t scancode;
    int32_t action;
    int32_t mods;
};

class Keyboard {
public:

    /// Emitted on any keyboard button action
    Signal<keyboardButtonInfo> sig_updateButtonInfo;

    /// Emitted when any keyboard button is being pressed or held
    Signal<int32_t> sig_updateKeyPressed;

    Slot<keyboardButtonInfo> slt_updateButtonInfo{[this](keyboardButtonInfo buttonInfo){
        m_buttonInfo = buttonInfo;
        sig_updateButtonInfo.emit(m_buttonInfo);
        switch (m_buttonInfo.action){
            case GLFW_PRESS:
            case GLFW_REPEAT:
                sig_updateKeyPressed.emit(m_buttonInfo.key);
                invokeKeyGroups(buttonInfo.key);
                break;
        }
    }};

    void addKeyGroup(std::string&& name, std::vector<int32_t>&& group);
    void connectKeyGroup(const std::string &name, Slot<int32_t> &slot);
    void connectKeyGroup(const std::string &name, Slot<> &slot);

private:

    void invokeKeyGroups(int32_t buttonInfo);
    void createKeySignal(int32_t key);

    keyboardButtonInfo m_buttonInfo;
    std::unordered_map<std::string, std::vector<int32_t>> m_keyGroups;
    std::unordered_map<int32_t, std::pair<Signal<int32_t>, Signal<>>> m_keySignals;
};


#endif //TECTONIC_KEYBOARD_H
