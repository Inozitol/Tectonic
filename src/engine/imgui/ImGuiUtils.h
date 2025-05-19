#pragma once
#include <functional>

#define RAII_STRUCT(NAME, DESTR_OPER) \
struct NAME { \
~NAME() { if(ok){DESTR_OPER;} } \
bool ok = false; \
explicit operator bool() const { return ok; } \
};

#define IMGUI_WINDOW(x) \
    auto window_##x = ImGuiTec::Begin(#x); \
    if(!window_##x) return;

#define IMGUI_CHAR_BUFFER_SIZE 256

inline char IMGUI_CHAR_BUFFER[IMGUI_CHAR_BUFFER_SIZE];

namespace ImGuiTec {
    struct Context {
        std::function<void(void *)> showFunc;
    };

    struct Empheral : Context {
        bool shouldDelete = false;
        bool shouldRename = false;
    };

    inline void Text(const char *name, uint8_t n, const char *postfix = nullptr) {
        if(postfix) ImGui::Text("%s: %u %s", name, n, postfix);
        else ImGui::Text("%s: %u", name, n);
    }

    inline void Text(const char *name, uint16_t n, const char *postfix = nullptr) {
        if(postfix) ImGui::Text("%s: %u %s", name, n, postfix);
        else ImGui::Text("%s: %u", name, n);
    }

    inline void Text(const char *name, uint32_t n, const char *postfix = nullptr) {
        if(postfix) ImGui::Text("%s: %u %s", name, n, postfix);
        else ImGui::Text("%s: %u", name, n);
    }

    inline void Text(const char *name, uint64_t n, const char *postfix = nullptr) {
        if(postfix) ImGui::Text("%s: %lu %s", name, n, postfix);
        else ImGui::Text("%s: %lu", name, n);
    }

    inline void Text(const char *name, bool b, const char *postfix = nullptr) {
        if(postfix) ImGui::Text("%s: %s %s", name, b ? "True" : "False", postfix);
        else ImGui::Text("%s: %s", name, b ? "True" : "False");
    }

    inline void Text(const char *name, float f, const char *postfix = nullptr) {
        if(postfix) ImGui::Text("%s: [%f] %s", name, f, postfix);
        else ImGui::Text("%s: [%f]", name, f);
    }

    inline void Text(const char *name, const glm::vec1 &vec, const char *postfix = nullptr) { Text(name, vec[0], postfix); }

    inline void Text(const char *name, const glm::vec2 &vec, const char *postfix = nullptr) {
        if(postfix) ImGui::Text("%s: [%f, %f] %s", name, vec[0], vec[1], postfix);
        else ImGui::Text("%s: [%f, %f]", name, vec[0], vec[1]);
    }

    inline void Text(const char *name, const glm::vec3 &vec, const char *postfix = nullptr) {
        if(postfix) ImGui::Text("%s: [%f, %f, %f] %s", name, vec[0], vec[1], vec[2], postfix);
        else ImGui::Text("%s: [%f, %f, %f]", name, vec[0], vec[1], vec[2]);
    }

    inline void Text(const char *name, const glm::vec4 &vec, const char *postfix = nullptr) {
        if(postfix) ImGui::Text("%s: [%f, %f, %f, %f] %s", name, vec[0], vec[1], vec[2], vec[3], postfix);
        else ImGui::Text("%s: [%f, %f, %f, %f]", name, vec[0], vec[1], vec[2], vec[3]);
    }

    inline void Text(const char *name, const glm::quat &quat, const char *postfix = nullptr) {
        Text(name, glm::vec4{quat.w, quat.x, quat.y, quat.z}, postfix);
    }

    inline void Text(const char *name, const char* str, const char *postfix = nullptr) {
        if(postfix) ImGui::Text("%s: [%s] %s", name, str, postfix);
        else ImGui::Text("%s: [%s]", name, str);
    }

    struct BeginRAII {
        ~BeginRAII() { if(ok) { ImGui::End(); } }
        bool ok = false;
        explicit operator bool() const { return ok; }
    };

    inline BeginRAII Begin(const char *name, bool *p_open = nullptr, const ImGuiWindowFlags flags = 0) {
        if(!ImGui::Begin(name, p_open, flags)) {
            ImGui::End();
            return {.ok = false};
        }
        return {.ok = true};
    }
}