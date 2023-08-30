#ifndef TECTONIC_WINDOW_H
#define TECTONIC_WINDOW_H

#include <GLFW/glfw3.h>
#include <functional>
#include "exceptions.h"

#include "meta/Signal.h"
#include "Cursor.h"
#include "Keyboard.h"

/**
 * Class representing the application window.
 */
class Window {
public:
    explicit Window(const std::string& name);
    ~Window();

    /**
     * @brief Emitted on every mouse button press.
     */
    Signal<mouseButtonInfo> sig_updateMouseButtonInfo;

    /**
     * @brief Emitted on every keyboard button press.
     */
    Signal<keyboardButtonInfo> sig_updateKeyboardButtonInfo;

    /**
     * @brief Emitted with every new mouse position.
     */
    Signal<double, double> sig_updateMousePos;

    /**
     * @brief Emitted when cursor becomes enabled/disabled.
     */
    Signal<bool> sig_cursorEnabled;

    /**
     * @brief Emitted when window dimensions are changed.
     * Returns width and height.
     */
    Signal<int32_t, int32_t> sig_widowDimensions;

    /**
     * @brief Closes the window.
     */
    Slot<> slt_setClose{[this](){
        close();
    }};

    /**
     * @brief Toggles between cursor being enabled/disabled.
     */
    Slot<> slt_toggleCursor{[this](){
        toggleCursor();
    }};

    /**
     * @brief Makes the window as the current context.
     */
    void makeCurrentContext();

    /**
     * @brief Gets the width and height of the window.
     * @return Width and height.
     */
    std::pair<int32_t, int32_t> getSize();

    /**
     * @brief Gets the window aspect aspect.
     * @return Aspect aspect.
     */
    float getRatio();

    /**
     * Swaps the front and back buffers of the window.
     * Should be called after every frame.
     */
    void swapBuffers();

    /**
     * Returns true if the window should close.
     * @return True if window should close.
     */
    bool shouldClose();

    /**
     * @brief Disables the cursor inside the window.
     */
    void disableCursor();

    /**
     * @brief Enables the cursor inside the window.
     */
    void enableCursor();

    /**
     * @brief Toggles cursor being enabled/disabled.
     */
    void toggleCursor();

    /**
     * @brief Sets the window to shouldClose state.
     */
    void close();

    /**
     * @brief Connects the appropriate signals and slots between this Window and provided Cursor.
     * @param cursor Cursor object.
     */
    void connectCursor(Cursor& cursor);

    /**
     * @brief Connects the appropriate signals and slots between this Window and provided Keyboard.
     * @param keyboard Keyboard object.
     */
    void connectKeyboard(Keyboard& keyboard);

    static Window* getContextFromWindow(GLFWwindow* window);

private:
    void initSignals();

    GLFWwindow* m_window;
};

#endif //TECTONIC_WINDOW_H
