#include "Keyboard.h"

void Keyboard::addKeyGroup(std::string&& name, std::vector<int32_t>&& group, KeyboardGroupFlags flags) {
    if(m_keyGroups.contains(name)){
        throw keyboardException("Key group already exists");
    }
    m_keyGroups.try_emplace(name);
    m_keyGroups.at(name).keys = group;
    m_keyGroups.at(name).flags = flags;
    for(const auto& key : group) {
        if(!m_backwardsKeyGroup.contains(key)) {
            m_backwardsKeyGroup[key] = std::vector<KeyGroup*>();
        }
        m_backwardsKeyGroup.at(key).push_back(&m_keyGroups.at(name));
    }
}

void Keyboard::connectKeyGroup(const std::string &name, Slot<keyboardButtonInfo> &slot) {
    if(!m_keyGroups.contains(name)){
        throw keyboardException("Key group doesn't exists");
    }

    m_keyGroups.at(name).keySignals.buttonInfoSignal.connect(slot);
}

void Keyboard::connectKeyGroup(const std::string &name, Slot<> &slot) {
    if(!m_keyGroups.contains(name)){
        throw keyboardException("Key group doesn't exists");
    }

    m_keyGroups.at(name).keySignals.buttonEmptySignal.connect(slot);
}

void Keyboard::invokeKeyGroups(const keyboardButtonInfo& buttonInfo){
    if(m_backwardsKeyGroup.contains(buttonInfo.key)) {
        for(auto group : m_backwardsKeyGroup.at(buttonInfo.key)) {
            group->keySignals.handleButtonInfo(buttonInfo, group->flags);
        }
    }
}

void Keyboard::createKeySignal(std::unordered_map<int32_t, KeySignals>& signals, const int32_t key) {
    signals.try_emplace(key);
}
