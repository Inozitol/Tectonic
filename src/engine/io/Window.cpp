#include "engine/io/Window.h"

Window::Window() : Window(DEFAULT_WINDOW_NAME){}

Window::Window(const char* name){
    initGLFW();
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);

    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);

    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    glfwWindow = glfwCreateWindow(mode->width-75, mode->height-75, name, nullptr, nullptr);
    if(!glfwWindow){
        throw windowException("Unable to create Window");
    }

    glfwSetWindowUserPointer(glfwWindow, this);
    if(glfwRawMouseMotionSupported()) {
        glfwSetInputMode(glfwWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
    initSignals();
}

void glfwErrorCallback(int, const char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
}

void Window::initGLFW() {
    glfwSetErrorCallback(glfwErrorCallback);

    if(!glfwInit()) {
        throw engineException("Engine couldn't initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_CENTER_CURSOR, GLFW_FALSE);
}


Window::~Window() {
    clean();
}

void Window::clean() {
    glfwDestroyWindow(glfwWindow);
}

void Window::makeCurrentContext() {
    glfwMakeContextCurrent(glfwWindow);
}

void Window::swapBuffers() {
    glfwSwapBuffers(glfwWindow);
}
bool Window::shouldClose() {
    return glfwWindowShouldClose(glfwWindow);
}

Utils::WindowDimension Window::getSize() {
    int32_t winWidth, winHeight;
    glfwGetWindowSize(glfwWindow, &winWidth, &winHeight);
    return {winWidth, winHeight};
}

float Window::getRatio(){
    return this->getSize().ratio();
}

void Window::disableCursor() {
    glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    sig_cursorEnabled.emit(false);
}

void Window::enableCursor() {
    glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    sig_cursorEnabled.emit(true);
}

void Window::toggleCursor() {
    int mode = glfwGetInputMode(glfwWindow, GLFW_CURSOR);
    if(mode == GLFW_CURSOR_NORMAL){
        disableCursor();
    }else{
        enableCursor();
    }
}

void Window::initSignals() {
    glfwSetCursorPosCallback(glfwWindow, [](GLFWwindow* win, double x, double y){
        Window* winContext = Window::getContextFromWindow(win);
        winContext->sig_updateMousePos.emit(x,y);
    });

    glfwSetMouseButtonCallback(glfwWindow, [](GLFWwindow* win, int32_t button, int32_t action, int32_t mods){
        Window* winContext = Window::getContextFromWindow(win);
        winContext->sig_updateMouseButtonInfo.emit({button, action, mods});
    });

    glfwSetKeyCallback(glfwWindow, [](GLFWwindow* win, int button, int scancode, int action, int mods){
        Window* winContext = Window::getContextFromWindow(win);
        winContext->sig_updateKeyboardButtonInfo.emit({button, scancode, action, mods});
    });

    glfwSetWindowSizeCallback(glfwWindow, [](GLFWwindow* win, int width, int height){
        Window* winContext = Window::getContextFromWindow(win);
        winContext->sig_widowDimensions.emit(width, height);
    });

    glfwSetFramebufferSizeCallback(glfwWindow, [](GLFWwindow* win, int width, int height){
        Window* winContext = Window::getContextFromWindow(win);
        winContext->sig_framebufferResize.emit(width, height);
    });

    glfwSetWindowCloseCallback(glfwWindow, [](GLFWwindow* win){
        Window* winContext = Window::getContextFromWindow(win);
        winContext->sig_shouldClose.emit();
    });
}

Window *Window::getContextFromWindow(GLFWwindow *window) {
    return static_cast<Window*>(glfwGetWindowUserPointer(window));
}

void Window::close() {
    glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE);
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
    if(glfwCreateWindowSurface(instance, glfwWindow, nullptr, &surface) != VK_SUCCESS){
        throw windowException("Failed to create glfwWindow KHR surface");
    }
    surfaceCreated = true;
    return surface;
}

void Window::initImGuiVulkan() {
    ImGui_ImplGlfw_InitForVulkan(glfwWindow, true);
}

