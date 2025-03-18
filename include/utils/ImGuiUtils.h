#pragma once
#include <functional>

#define IMGUI_CHAR_BUFFER_SIZE 256

inline char IMGUI_CHAR_BUFFER[IMGUI_CHAR_BUFFER_SIZE];

struct ImGuiTecContext {
    std::function<void(void*)> showFunc;
};

struct ImGuiTecEmpheral : ImGuiTecContext {
    bool shouldDelete = false;
    bool shouldRename = false;
};