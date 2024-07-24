
// TODO handle for slot func template
#ifndef META_HANDLES_
#define META_HANDLES_
#define CONNECT(signal,slot) signal.connect(slot);
#define DISCONNECT(signal,slot) signal.disconnect(slot);
#endif

#ifndef TECTONIC_SIGNAL_H
#define TECTONIC_SIGNAL_H

#include <vector>
#include "Slot.h"

template <typename ...Args>
class Signal {
private:
    std::vector<Slot<Args...> *> m_connectedSlots;

public:
    Signal(const Signal&) = delete;
    Signal(Signal&&) = delete;
    Signal &operator=(const Signal&) = delete;

    Signal() = default;
    ~Signal() = default;

    void connect(Slot<Args...> &slot){
        m_connectedSlots.push_back(&slot);
    }

    void disconnect(Slot<Args...> &slot){
        for(auto it = m_connectedSlots.begin(); it != m_connectedSlots.end(); ){
            if(*it == slot)
                it = m_connectedSlots.erase();
            else
                it++;
        }
        m_connectedSlots.erase(&slot);
    }

    void disconnect(){
        m_connectedSlots.clear();
    }

    void emit(Args... parameters) {
        for(auto &slot: m_connectedSlots){
            slot->call(parameters...);
        }
    }
};


#endif //TECTONIC_SIGNAL_H
