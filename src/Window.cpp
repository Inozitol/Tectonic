#include "engine/Window.h"

Window::Window() : Window(DEFAULT_WINDOW_NAME){}

Window::Window(const char* name){
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);

    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    m_window = glfwCreateWindow(mode->width, mode->height, name, primary, nullptr);
    if(!m_window){
        throw windowException("Unable to create Window");
    }

    glfwSetWindowUserPointer(m_window, this);
    if(glfwRawMouseMotionSupported()) {
        glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
    initSignals();
}

Window::~Window() {
    clean();
}

void Window::clean() {
    glfwDestroyWindow(m_window);
}

void Window::makeCurrentContext() {
    glfwMakeContextCurrent(m_window);
}

void Window::swapBuffers() {
    glfwSwapBuffers(m_window);
}
bool Window::shouldClose() {
    return glfwWindowShouldClose(m_window);
}

Utils::WindowDimension Window::getSize() {
    int32_t winWidth, winHeight;
    glfwGetWindowSize(m_window, &winWidth, &winHeight);
    return {winWidth, winHeight};
}

float Window::getRatio(){
    return this->getSize().ratio();
}

void Window::disableCursor() {
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    sig_cursorEnabled.emit(false);
}

void Window::enableCursor() {
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    sig_cursorEnabled.emit(true);
}

void Window::toggleCursor() {
    int mode = glfwGetInputMode(m_window, GLFW_CURSOR);
    if(mode == GLFW_CURSOR_NORMAL){
        disableCursor();
    }else{
        enableCursor();
    }
}

void Window::initSignals() {
    glfwSetCursorPosCallback(m_window, [](GLFWwindow* win, double x, double y){
        Window* winContext = Window::getContextFromWindow(win);
        winContext->sig_updateMousePos.emit(x,y);
    });

    glfwSetMouseButtonCallback(m_window, [](GLFWwindow* win, int32_t button, int32_t action, int32_t mods){
        Window* winContext = Window::getContextFromWindow(win);
        winContext->sig_updateMouseButtonInfo.emit({button, action, mods});
    });

    glfwSetKeyCallback(m_window, [](GLFWwindow* win, int button, int scancode, int action, int mods){
        Window* winContext = Window::getContextFromWindow(win);
        winContext->sig_updateKeyboardButtonInfo.emit({button, scancode, action, mods});
    });

    glfwSetWindowSizeCallback(m_window, [](GLFWwindow* win, int width, int height){
        Window* winContext = Window::getContextFromWindow(win);
        winContext->sig_widowDimensions.emit(width, height);
    });

    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* win, int width, int height){
        Window* winContext = Window::getContextFromWindow(win);
        winContext->sig_framebufferResize.emit(width, height);
    });

    glfwSetWindowCloseCallback(m_window, [](GLFWwindow* win){
        Window* winContext = Window::getContextFromWindow(win);
        winContext->sig_shouldClose.emit();
    });
}

Window *Window::getContextFromWindow(GLFWwindow *window) {
    return static_cast<Window*>(glfwGetWindowUserPointer(window));
}

void Window::close() {
    glfwSetWindowShouldClose(m_window, GLFW_TRUE);
}

void Window::connectCursor(Cursor &cursor) {
    sig_updateMouseButtonInfo.connect(cursor.slt_updateButtonInfo);
    sig_updateMousePos.connect(cursor.slt_updatePos);
}

void Window::connectKeyboard(Keyboard &keyboard) {
    sig_updateKeyboardButtonInfo.connect(keyboard.slt_updateButtonInfo);
}

VkSurfaceKHR Window::createWindowSurface(VkInstance instance) {
    static bool surfaceCreated = false;
    if(surfaceCreated){
        throw windowException("Attempted to create VkSurfaceKHR on Window with previously created surface");
    }
    VkSurfaceKHR surface;
    if(glfwCreateWindowSurface(instance, m_window, nullptr, &surface) != VK_SUCCESS){
        throw windowException("Failed to create m_window KHR surface");
    }
    surfaceCreated = true;
    return surface;
}

void Window::initImGuiVulkan() {
    ImGui_ImplGlfw_InitForVulkan(m_window, true);
}

