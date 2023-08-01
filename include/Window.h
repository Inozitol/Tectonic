#ifndef TECTONIC_WINDOW_H
#define TECTONIC_WINDOW_H

#include <GLFW/glfw3.h>
#include <functional>
#include "exceptions.h"

/**
 * Class representing the application window.
 */
class Window {
public:
    explicit Window(const std::string& name);
    ~Window();

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
     * @brief Sets the keyboard callback function.
     * @param callback Callback function.
     */
    void setKeyCallback(void(*callback)(GLFWwindow *, int, int, int, int));

    /**
     * @brief Sets the mouse callback function.
     * @param callback Callback function.
     */
    void setMouseCallback(void(*callback)(GLFWwindow *, double, double));

    /**
     * @brief Disables the cursor inside the window.
     */
    void disableCursor();

    /**
     * @brief Enables the cursor inside the window.
     */
    void enableCursor();

    /**
     * Sets the cursor local_position inside the window.
     * @param x X coordinate.
     * @param y Y coordinate.
     */
    void setCursorPos(double x, double y);
private:
    GLFWwindow* m_window;
};

#endif //TECTONIC_WINDOW_H
