#include "ImGuiHandler.h"

#include "engine/GlobalMemory.h"
#include "engine/scene/World.h"
#include "extern/imgui/imgui.h"
#include "extern/imgui/imgui_impl_glfw.h"
#include "extern/imgui/imgui_impl_vulkan.h"

#include <ranges>

void ImGuiHandler::initDockspace() { ImGuiID dockspaceID = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode); }

void ImGuiHandler::run() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
    runMenu();
    for(const auto &[menu, item]: ImGuiActiveMenuItems) { ImGuiMenus.at(menu).items.at(item).procedure(); }
    ImGui::Render();
}

void ImGuiHandler::runMenu() {
    if(ImGui::BeginMainMenuBar()) {
        for(const auto &[menuName, menu]: ImGuiMenus) {
            if(ImGui::BeginMenu(menuName.c_str())) {
                for(const auto &itemName: std::views::keys(menu.items)) { if(ImGui::MenuItem(itemName.c_str())) { toggleMenu(menuName.c_str(), itemName.c_str()); } }
                ImGui::EndMenu();
            }
        }
        ImGui::EndMainMenuBar();
    }
}

bool ImGuiHandler::insertMenu(const ImGuiMenu &menu) {
    if(ImGuiMenus.contains(menu.name)) return false;
    ImGuiMenus.emplace(menu.name, menu);
    for(const auto &itemName: std::views::keys(ImGuiMenus.at(menu.name).items)) { toggleMenu(menu.name.c_str(), itemName.c_str()); }
    return true;
}

bool ImGuiHandler::toggleMenu(const char *menu, const char *item) {
    if(!ImGuiMenus.contains(menu)) return false;
    const auto &m = ImGuiMenus.at(menu);
    if(!m.items.contains(item)) return false;

    const std::pair<std::string, std::string> key = {menu, item};
    if(ImGuiActiveMenuItems.contains(key)) ImGuiActiveMenuItems.erase(key);
    ImGuiActiveMenuItems.insert(key);
    return true;
}

ImGuiHandler::ImGuiMenu &ImGuiHandler::getMenu(const char *menu) { return ImGuiMenus.at(menu); }

void ImGuiHandler::vkDebugCallback(VkResult err) {
    //fprintf(stderr, "ImGui Vulkan ERR: %s\n", string_VkResult(err));
}

void ImGuiHandler::initMenus() {

    ImGuiMenu coreMenu;
    coreMenu.name = "GPU";

    ImGuiMenuItem sceneItem;
    sceneItem.name = "Scene";
    sceneItem.procedure = [] {
        IMGUI_WINDOW(Scene)
        ImGui::InputFloat3("Ambient color", reinterpret_cast<float *>(&TecCorePtr->world->sceneData.ambientColor));
        ImGui::InputFloat3("Sunlight direction", reinterpret_cast<float *>(&TecCorePtr->world->sceneData.sunlightDirection));
        ImGui::InputFloat3("Sunlight color", reinterpret_cast<float *>(&TecCorePtr->world->sceneData.sunlightColor));
        ImGui::InputFloat3("Camera position", reinterpret_cast<float *>(&PlayerPtr->camera.position));
        ImGui::InputFloat3("Camera direction", reinterpret_cast<float *>(&TecCorePtr->world->sceneData.cameraDirection));
        ImGui::InputFloat("Time", &TecCorePtr->world->sceneData.time);
    };
    coreMenu.items.emplace(sceneItem.name, sceneItem);

    ImGuiMenuItem perfItem;
    perfItem.name = "Performance";
    perfItem.procedure = [] {
        IMGUI_WINDOW(Performance)
        uint32_t frames = VktCorePtr->perfStats.frametime > 0.0 ? static_cast<uint32_t>(static_cast<float>(1000) / VktCorePtr->perfStats.frametime) : 0;
        ImGuiTec::Text("Frames", frames);
        ImGuiTec::Text("Frametime", VktCorePtr->perfStats.frametime, "ms");
        ImGuiTec::Text("Drawtime", VktCorePtr->perfStats.meshDrawTime, "ms");
        ImGuiTec::Text("Update time", VktCorePtr->perfStats.sceneUpdateTime, "ms");
        ImGuiTec::Text("Triangles", VktCorePtr->perfStats.trigDrawCount);
        ImGuiTec::Text("Draws", VktCorePtr->perfStats.drawCallCount);
    };
    coreMenu.items.emplace(perfItem.name, perfItem);

    ImGuiMenuItem modelsItem;
    modelsItem.name = "Models";
    modelsItem.procedure = [] {
        IMGUI_WINDOW(Models)

        for(auto [id,obj]: TecCorePtr->world->skinnedObjects.objects) {
            if(ImGui::TreeNode(obj.name.c_str())) {
                ImGuiTec::Text("ID", id);
                ImGuiTec::Text("Name", obj.name.c_str());
                if(ImGui::TreeNode("Transformation")) {

                    // Change position
                    glm::vec3 pos = obj.model.transformation.getTranslation();
                    if(ImGui::DragFloat3("Pos", reinterpret_cast<float *>(&(pos)), 0.1)) { obj.model.transformation.setTranslation(pos.x, pos.y, pos.z); }

                    // Change rotation
                    glm::vec3 rotation = obj.model.transformation.getRotation();
                    if(ImGui::DragFloat3("Rotation", reinterpret_cast<float *>(&(rotation)), 0.1)) { obj.model.transformation.setRotation(rotation.x, rotation.y, rotation.z); }

                    // Change scale
                    float scale = obj.model.transformation.getScale();
                    if(ImGui::DragFloat("Scale", &scale, 0.1, 0.0)) { obj.model.transformation.setScale(scale); }
                    ImGui::TreePop();
                }

                if(obj.model.isSkinned() && ImGui::TreeNode("Animation")) {
                    static std::size_t currentAnimation = obj.model.currentAnimation();
                    if(ImGui::BeginListBox("##animation_list", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing()))) {
                        uint32_t animCount = obj.model.animationCount();
                        for(std::size_t animID = 0; animID < animCount; animID++) {
                            const bool isActive = (currentAnimation == animID);
                            if(ImGui::Selectable(obj.model.animationName(animID).data(), isActive)) {
                                obj.model.setAnimation(animID);
                                currentAnimation = animID;
                            }
                            if(isActive) { ImGui::SetItemDefaultFocus(); }
                        }
                        ImGui::EndListBox();
                    }
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }
        }
    };
    coreMenu.items.emplace(modelsItem.name, modelsItem);

    ImGuiMenuItem debugItem;
    debugItem.name = "Debug";
    debugItem.procedure = [] {
        IMGUI_WINDOW(Debug)
        ImGui::Checkbox("Debug normals", &VktCorePtr->debugConfig.enableDebugNormals);
        ImGui::Checkbox("Debug vectors", &VktCorePtr->debugConfig.enableDebugVectors);
        ImGui::Checkbox("Debug picking", &VktCorePtr->debugConfig.enablePicking);

    };
    coreMenu.items.emplace(debugItem.name, debugItem);

    insertMenu(coreMenu);
}