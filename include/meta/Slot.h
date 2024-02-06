#ifndef META_HANDLES_
#define META_HANDLES_
#define CONNECT(signal,slot) signal.connect(slot);
#define DISCONNECT(signal,slot) signal.disconnect(slot);
#endif

#ifndef TECTONIC_SLOT_H
#define TECTONIC_SLOT_H

#include <functional>

template <typename ...Args>
class Slot {
public:
    Slot(const Slot&) = delete;
    Slot(Slot&&) = delete;
    Slot &operator=(const Slot&) = delete;

    Slot() = default;
    explicit Slot(std::function<void(Args...)> callback): m_init(true), m_callback(callback){}
    ~Slot() = default;

    void operator()(Args... parameters){
        this->call(parameters...);
    }

    void call(Args... parameters){
        if(m_init)
            m_callback(parameters...);
    }

    void setSlotFunction(std::function<void(Args...)> callback){
        m_init = true;
        m_callback = callback;
    }

private:
    bool m_init = false;
    std::function<void(Args...)> m_callback;
};


#endif //TECTONIC_SLOT_H
