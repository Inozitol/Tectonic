#include "Window.h"

Window::Window(const std::string& name){
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);

    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    _window = glfwCreateWindow(mode->width, mode->height, name.c_str(), primary, nullptr);
    if(!_window){
        throw window_exception("Unable to create Window");
    }
}

Window::~Window() {
    glfwDestroyWindow(_window);
}

void Window::make_current_context() {
    glfwMakeContextCurrent(_window);
}

void Window::swap_buffers() {
    glfwSwapBuffers(_window);
}
bool Window::should_close() {
    return glfwWindowShouldClose(_window);
}

void Window::set_key_callback(void (*callback)(GLFWwindow *, int, int, int, int)) {
    glfwSetKeyCallback(_window, callback);
}

void Window::set_mouse_callback(void (*callback)(GLFWwindow *, double, double)) {
    glfwSetCursorPosCallback(_window, callback);
}

std::tuple<int, int> Window::get_size() {
    int width, height;
    glfwGetWindowSize(_window, &width, &height);
    return {width, height};
}

float Window::get_ratio(){
    int width, height;
    std::tie(width, height) = this->get_size();
    return (float)width/(float)height;
}

void Window::disable_cursor() {
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Window::enable_cursor() {
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void Window::set_cursor_pos(double x, double y) {
    glfwSetCursorPos(_window, x, y);
}


