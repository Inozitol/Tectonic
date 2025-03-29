#include "Keyboard.h"

void Keyboard::addKeyGroup(std::string&& name, std::vector<int32_t>&& group, KeyboardGroupFlags flags) {
    if(keyGroups.contains(name)){
        throw keyboardException("Key group already exists");
    }
    keyGroups.try_emplace(name);
    keyGroups.at(name).keys = group;
    keyGroups.at(name).flags = flags;
    for(const auto& key : group) {
        if(!backwardsKeyGroup.contains(key)) {
            backwardsKeyGroup[key] = std::vector<KeyGroup*>();
        }
        backwardsKeyGroup.at(key).push_back(&keyGroups.at(name));
    }
}

void Keyboard::connectKeyGroup(const std::string &name, Slot<keyboardButtonInfo> &slot) {
    if(!keyGroups.contains(name)){
        throw keyboardException("Key group doesn't exists");
    }

    keyGroups.at(name).keySignals.buttonInfoSignal.connect(slot);
}

void Keyboard::connectKeyGroup(const std::string &name, Slot<> &slot) {
    if(!keyGroups.contains(name)){
        throw keyboardException("Key group doesn't exists");
    }

    keyGroups.at(name).keySignals.buttonEmptySignal.connect(slot);
}

void Keyboard::invokeKeyGroups(const keyboardButtonInfo& buttonInfo){
    if(backwardsKeyGroup.contains(buttonInfo.key)) {
        for(auto group : backwardsKeyGroup.at(buttonInfo.key)) {
            if(group->flags & KeyboardGroupFlags::STORE_HELD_KEYS) {
                switch(buttonInfo.action) {
                    case GLFW_PRESS:
                        group->heldKeys.insert(buttonInfo.key);
                    break;

                    case GLFW_RELEASE:
                        group->heldKeys.erase(buttonInfo.key);
                    break;

                    default:
                        break;
                }
            }
            group->keySignals.handleButtonInfo(buttonInfo, group->flags);
        }
    }
}

void Keyboard::createKeySignal(std::unordered_map<int32_t, KeySignals>& signals, const int32_t key) {
    signals.try_emplace(key);
}
