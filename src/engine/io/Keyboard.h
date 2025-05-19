#pragma once

#include <GLFW/glfw3.h>
#include <string>
#include <unordered_set>
#include <utils/Utils.h>

#include "utils/exceptions.h"
#include "connector/Slot.h"
#include "connector/Signal.h"

struct keyboardButtonInfo {
    int32_t key;
    int32_t scancode;
    int32_t action;
    int32_t mods;
};

struct Keyboard {
    enum class KeyboardGroupFlags : uint8_t {
        NONE = 0,
        EMIT_ON_RELEASE = 1 << 0,    /// Emits the group on GLFW_RELEASE
        EMIT_ON_REPEAT = 1 << 1,     /// Emits the group on GLFW_REPEAT
        STORE_HELD_KEYS = 1 << 2,    /// Stores which keys are currently held
    };

    void addKeyGroup(std::string&& name, std::vector<int32_t>&& group, KeyboardGroupFlags flags = KeyboardGroupFlags::NONE);
    void connectKeyGroup(const std::string &name, Slot<keyboardButtonInfo> &slot);
    void connectKeyGroup(const std::string &name, Slot<> &slot);

    struct KeySignals {
        mutable Signal<keyboardButtonInfo> buttonInfoSignal{};
        mutable Signal<> buttonEmptySignal{};

        void handleButtonInfo(const keyboardButtonInfo& buttonInfo, KeyboardGroupFlags flags) const {
            switch(buttonInfo.action) {
                case GLFW_PRESS:
                    buttonInfoSignal.emit(buttonInfo);
                    buttonEmptySignal.emit();
                break;

                case GLFW_REPEAT:
                    if(Utils::enumCheckBit(flags, KeyboardGroupFlags::EMIT_ON_REPEAT)) {
                        buttonInfoSignal.emit(buttonInfo);
                        buttonEmptySignal.emit();
                    }
                break;

                case GLFW_RELEASE:
                    if(Utils::enumCheckBit(flags, KeyboardGroupFlags::EMIT_ON_RELEASE)) {
                        buttonInfoSignal.emit(buttonInfo);
                        buttonEmptySignal.emit();
                    }
                break;

                default:
                    break;
            }
        }
    };

    struct KeyGroup {
        std::vector<int32_t> keys;
        KeyboardGroupFlags flags;
        KeySignals keySignals;
        std::unordered_set<int32_t> heldKeys;
    };

    void invokeKeyGroups(const keyboardButtonInfo& buttonInfo);
    static void createKeySignal(std::unordered_map<int32_t, KeySignals>& signals, int32_t key);

    static void signalRoutine(const keyboardButtonInfo& buttonInfo, KeyboardGroupFlags flags);

    std::unordered_map<std::string, KeyGroup> keyGroups;
    std::unordered_map<int32_t, std::vector<KeyGroup*>> backwardsKeyGroup;

    /** Emitted on any keyboard button action */
    Signal<keyboardButtonInfo> sig_updateButtonInfo;

    /** Emitted when any keyboard button is being pressed or held */
    Signal<int32_t> sig_updateKeyPressed;

    /** Catches incoming button signals from window callbacks */
    Slot<keyboardButtonInfo> slt_updateButtonInfo{[this](keyboardButtonInfo buttonInfo){
        sig_updateButtonInfo.emit(buttonInfo);
        invokeKeyGroups(buttonInfo);
    }};
};

INIT_ENUM_OP(Keyboard::KeyboardGroupFlags)
