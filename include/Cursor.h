#ifndef TECTONIC_CURSOR_H
#define TECTONIC_CURSOR_H

#include <GLFW/glfw3.h>
#include "meta/Slot.h"
#include "meta/Signal.h"

struct mouseButtonInfo{
    int32_t button;
    int32_t action;
    int32_t mods;
};

class Cursor {
public:
    /// Emitted when mouse button got pressed or released
    Signal<bool> sig_updatePressed;

    /// Emitted when mouse button is held
    Signal<> sig_updateHeld;

    /// Emitted on mouse button action
    Signal<mouseButtonInfo> sig_updateButtonInfo;

    /// Emitted with every new mouse position
    Signal<double, double> sig_updatePos;

    /// Emitted when cursor is pressed sending position of cursor
    Signal<double, double> sig_cursorPressedPos;

    Slot<mouseButtonInfo> slt_updateButtonInfo{[this](mouseButtonInfo buttonInfo){
        m_buttonInfo = buttonInfo;
        sig_updateButtonInfo.emit(m_buttonInfo);
        switch(m_buttonInfo.action){
            case GLFW_PRESS:
                m_isPressed = true;
                sig_updatePressed.emit(true);
                sig_cursorPressedPos.emit(m_xPos, m_yPos);
            break;
            case GLFW_REPEAT:
                sig_updateHeld.emit();
                break;
            case GLFW_RELEASE:
                m_isPressed = false;
                sig_updatePressed.emit(false);
            break;
        }
    }};

    Slot<double, double> slt_updatePos{[this](double x, double y){
        m_xPos = x;
        m_yPos = y;
        sig_updatePos.emit(m_xPos, m_yPos);
    }};

private:
    bool m_isPressed;
    double m_xPos, m_yPos;
    mouseButtonInfo m_buttonInfo;
};


#endif //TECTONIC_CURSOR_H