#ifndef TECTONIC_CURSOR_H
#define TECTONIC_CURSOR_H

#include <GLFW/glfw3.h>
#include "connector/Slot.h"
#include "connector/Signal.h"

struct mouseButtonInfo{
    int32_t button;
    int32_t action;
    int32_t mods;
};

struct Cursor {

    bool isPressed;
    double xPos, yPos;
    mouseButtonInfo buttonInfo;

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

    Slot<mouseButtonInfo> slt_updateButtonInfo{[this](const mouseButtonInfo inButtonInfo){
        buttonInfo = inButtonInfo;
        sig_updateButtonInfo.emit(buttonInfo);
        switch(buttonInfo.action){
            case GLFW_PRESS:
                isPressed = true;
            sig_updatePressed.emit(true);
            sig_cursorPressedPos.emit(xPos, yPos);
            break;
            case GLFW_REPEAT:
                sig_updateHeld.emit();
            break;
            case GLFW_RELEASE:
                isPressed = false;
            sig_updatePressed.emit(false);
            break;
            default: break;
        }
    }};

    Slot<double, double> slt_updatePos{[this](double x, double y){
        xPos = x;
        yPos = y;
        sig_updatePos.emit(xPos, yPos);
    }};
};


#endif //TECTONIC_CURSOR_H