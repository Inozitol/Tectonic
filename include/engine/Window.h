#ifndef TECTONIC_WINDOW_H
#define TECTONIC_WINDOW_H

#include "extern/imgui/imgui_impl_glfw.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <functional>
#include <vulkan/vulkan.h>

#include "utils/utils.h"
#include "connector/Signal.h"
#include "Cursor.h"
#include "Keyboard.h"

/**
 * Application m_window abstraction. Implemented with GLFW m_window.
 */
class Window {
public:
    Window();
    explicit Window(const char* name);
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
     * @brief Emitted when m_window dimensions is resized.
     * Returns new width and height.
     */
    Signal<int32_t, int32_t> sig_widowDimensions;

    /**
     * @brief Emitted when m_window framebuffer is resized.
     * Returns new width and height.
     */
    Signal<int32_t, int32_t> sig_framebufferResize;

    /**
     * @brief Emitted when m_window should close.
     */
    Signal<> sig_shouldClose;

    /**
     * @brief Closes the m_window.
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
     * @brief Makes the m_window as the current context.
     */
    void makeCurrentContext();

    /**
     * @brief Gets the width and height of the m_window.
     * @return Width and height.
     */
    Utils::WindowDimension getSize();

    /**
     * @brief Gets the m_window aspect aspect.
     * @return Aspect aspect.
     */
    float getRatio();

    /**
     * Swaps the front and back buffers of the m_window.
     * Should be called after every frame.
     */
    void swapBuffers();

    /**
     * Returns true if the m_window should close.
     * @return True if m_window should close.
     */
    bool shouldClose();

    /**
     * @brief Disables the cursor inside the m_window.
     */
    void disableCursor();

    /**
     * @brief Enables the cursor inside the m_window.
     */
    void enableCursor();

    /**
     * @brief Toggles cursor being enabled/disabled.
     */
    void toggleCursor();

    /**
     * @brief Sets the m_window to shouldClose state.
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

    [[ nodiscard ]] VkSurfaceKHR createWindowSurface(VkInstance instance);
    void initImGuiVulkan();
    void clean();

    static constexpr const char* DEFAULT_WINDOW_NAME = "Tectonic";

private:
    void initSignals();

    GLFWwindow* m_window;
};

#endif //TECTONIC_WINDOW_H
