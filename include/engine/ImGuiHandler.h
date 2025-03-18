#pragma once
#include "imgui_impl_vulkan.h"

#include <functional>
#include <map>
#include <set>
#include <string>
#include <unordered_map>

namespace ImGuiHandler {
    using ImGuiProcedure = std::function<void()>;

    struct ImGuiMenuItem {
        ImGuiProcedure procedure = {};
        std::string name = {};
    };
    struct ImGuiMenu {
        std::map<std::string, ImGuiMenuItem> items = {};
        std::string name = {};
    };

    void initDockspace();
    void run();
    void runMenu();
    bool insertMenu(const ImGuiMenu& menu);
    bool toggleMenu(const char* menu, const char* item);
    void vkDebugCallback(VkResult err);

    void initMenus();

    inline std::unordered_map<std::string, ImGuiMenu> ImGuiMenus = {};
    inline std::set<std::pair<std::string, std::string>> ImGuiActiveMenuItems = {};
    inline ImGui_ImplVulkanH_Window mainWindow;
};
