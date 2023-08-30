#include "Keyboard.h"

void Keyboard::addKeyGroup(std::string&& name, std::vector<int32_t>&& group) {
    if(m_keyGroups.contains(name)){
        throw keyboardException("Key group already exists");
    }
    m_keyGroups.insert({name, group});
}

void Keyboard::connectKeyGroup(const std::string &name, Slot<int32_t> &slot) {
    if(!m_keyGroups.contains(name)){
        throw keyboardException("Key group doesn't exists");
    }

    for(auto& key : m_keyGroups.at(name)){
        if(!m_keySignals.contains(key))
            createKeySignal(key);
        m_keySignals.at(key).first.connect(slot);
    }
}

void Keyboard::connectKeyGroup(const std::string &name, Slot<> &slot) {
    if(!m_keyGroups.contains(name)){
        throw keyboardException("Key group doesn't exists");
    }

    for(auto& key : m_keyGroups.at(name)){
        if(!m_keySignals.contains(key))
            createKeySignal(key);
        m_keySignals.at(key).second.connect(slot);
    }
}

void Keyboard::invokeKeyGroups(int32_t key) {
    if(!m_keySignals.contains(key)) {
        createKeySignal(key);
        return;
    }
    m_keySignals.at(key).first.emit(key);
    m_keySignals.at(key).second.emit();
}

void Keyboard::createKeySignal(int32_t key) {
    m_keySignals.emplace(std::piecewise_construct,
                         std::forward_as_tuple(key),
                         std::forward_as_tuple());
}
