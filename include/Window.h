#ifndef TECTONIC_WINDOW_H
#define TECTONIC_WINDOW_H

#include <GLFW/glfw3.h>
#include <functional>
#include "exceptions.h"

class Window {
public:
    explicit Window(const std::string& name);
    ~Window();

    void make_current_context();
    std::tuple<int, int> get_size();
    void swap_buffers();
    bool should_close();
    void set_key_callback(void(*callback)(GLFWwindow *, int, int, int, int));
    void set_mouse_callback(void(*callback)(GLFWwindow *, double, double));
    float get_ratio();
    void disable_cursor();
    void enable_cursor();
    void set_cursor_pos(double x, double y);
private:
    GLFWwindow* _window;
};

#endif //TECTONIC_WINDOW_H
