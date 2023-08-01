#include "Window.h"

Window::Window(const std::string& name){
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);

    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    m_window = glfwCreateWindow(mode->width, mode->height, name.c_str(), primary, nullptr);
    if(!m_window){
        throw windowException("Unable to create Window");
    }
}

Window::~Window() {
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

void Window::setKeyCallback(void (*callback)(GLFWwindow *, int, int, int, int)) {
    glfwSetKeyCallback(m_window, callback);
}

void Window::setMouseCallback(void (*callback)(GLFWwindow *, double, double)) {
    glfwSetCursorPosCallback(m_window, callback);
}

std::pair<int32_t, int32_t> Window::getSize() {
    int width, height;
    glfwGetWindowSize(m_window, &width, &height);
    return {width, height};
}

float Window::getRatio(){
    int width, height;
    std::tie(width, height) = this->getSize();
    return (float)width/(float)height;
}

void Window::disableCursor() {
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Window::enableCursor() {
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void Window::setCursorPos(double x, double y) {
    glfwSetCursorPos(m_window, x, y);
}


