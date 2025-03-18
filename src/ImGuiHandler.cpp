#include "engine/ImGuiHandler.h"

#include "engine/GlobalMemory.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "World.h"

#include <ranges>

void ImGuiHandler::initDockspace() {
    ImGuiID dockspaceID = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
}

void ImGuiHandler::run() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
    runMenu();
    for(const auto &[menu, item]: ImGuiActiveMenuItems) {
        ImGuiMenus.at(menu).items.at(item).procedure();
    }
    ImGui::Render();
}

void ImGuiHandler::runMenu() {
    if(ImGui::BeginMainMenuBar()) {
        for(const auto &[menuName, menu]: ImGuiMenus) {
            if(ImGui::BeginMenu(menuName.c_str())) {
                for(const auto &itemName: std::views::keys(menu.items)) {
                    if(ImGui::MenuItem(itemName.c_str())) {
                        toggleMenu(menuName.c_str(), itemName.c_str());
                    }
                }
                ImGui::EndMenu();
            }
        }
        ImGui::EndMainMenuBar();
    }
}

bool ImGuiHandler::insertMenu(const ImGuiMenu &menu) {
    if(ImGuiMenus.contains(menu.name)) return false;
    ImGuiMenus.emplace(menu.name, menu);
    for(const auto &itemName: std::views::keys(ImGuiMenus.at(menu.name).items)) {
        toggleMenu(menu.name.c_str(), itemName.c_str());
    }
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

void ImGuiHandler::vkDebugCallback(VkResult err) {
    //fprintf(stderr, "ImGui Vulkan ERR: %s\n", string_VkResult(err));
}

void ImGuiHandler::initMenus() {
    ImGuiHandler::ImGuiMenu coreMenu;
    coreMenu.name = "VktCore";

    ImGuiHandler::ImGuiMenuItem sceneItem;
    sceneItem.name = "Scene";
    sceneItem.procedure = [] {
        if(ImGui::Begin("Scene")) {
            ImGui::InputFloat3("Ambient color: ", reinterpret_cast<float*>(&TecCorePtr->world->sceneData.ambientColor));
            ImGui::InputFloat3("Sunlight direction: ", reinterpret_cast<float*>(&TecCorePtr->world->sceneData.sunlightDirection));
            ImGui::InputFloat3("Sunlight color: ", reinterpret_cast<float*>(&TecCorePtr->world->sceneData.sunlightColor));
            ImGui::InputFloat3("Camera position: ", reinterpret_cast<float*>(&TecCorePtr->gameCamera->position));
            ImGui::InputFloat3("Camera direction: ", reinterpret_cast<float*>(&TecCorePtr->world->sceneData.cameraDirection));
            ImGui::InputFloat("Time: ", &TecCorePtr->world->sceneData.time);
            ImGui::End();
        }
    };
    coreMenu.items.emplace(sceneItem.name, sceneItem);

    ImGuiHandler::ImGuiMenuItem perfItem;
    perfItem.name = "Performance";
    perfItem.procedure = [] {
        if(ImGui::Begin("Performance")) {
            uint32_t frames = VktCorePtr->stats.frametime > 0.0 ? static_cast<uint32_t>(static_cast<float>(1000) / VktCorePtr->stats.frametime) : 0;
            ImGui::Text("Frames %u", frames);
            ImGui::Text("Frametime %f ms", VktCorePtr->stats.frametime);
            ImGui::Text("Drawtime %f ms", VktCorePtr->stats.meshDrawTime);
            ImGui::Text("Update time %f ms", VktCorePtr->stats.sceneUpdateTime);
            ImGui::Text("Triangles %u", VktCorePtr->stats.trigDrawCount);
            ImGui::Text("Draws: %u", VktCorePtr->stats.drawCallCount);
            ImGui::End();
        }
    };
    coreMenu.items.emplace(perfItem.name, perfItem);

    ImGuiHandler::ImGuiMenuItem modelsItem;
    modelsItem.name = "Models";
    modelsItem.procedure = [] {
        if(ImGui::Begin("Models")) {
            for(auto &[mID, object]: VktCorePtr->loadedObjects) {
                if(ImGui::TreeNode(object.name.c_str())) {
                    ImGui::Text("ID: %u", mID);
                    ImGui::Text("Name: %s", object.name.c_str());
                    if(ImGui::TreeNode("Transformation")) {

                        // Change position
                        glm::vec3 pos = object.model->transformation.getTranslation();
                        if(ImGui::DragFloat3("Pos", (float *) &(pos))) {
                            object.model->transformation.setTranslation(pos.x, pos.y, pos.z);
                        }

                        // Change rotation
                        glm::vec3 rotation = object.model->transformation.getRotation();
                        if(ImGui::DragFloat3("Rotation", (float *) &(rotation))) {
                            object.model->transformation.setRotation(rotation.x, rotation.y, rotation.z);
                        }

                        // Change scale
                        float scale = object.model->transformation.getScale();
                        if(ImGui::DragFloat("Scale", &scale)) {
                            object.model->transformation.setScale(scale);
                        }
                        ImGui::TreePop();
                    }

                    if(object.model->isSkinned() && ImGui::TreeNode("Animation")) {
                        static std::size_t currentAnimation = object.model->currentAnimation();
                        if(ImGui::BeginListBox("##animation_list", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing()))) {
                            uint32_t animCount = object.model->animationCount();
                            for(std::size_t animID = 0; animID < animCount; animID++) {
                                const bool isActive = (currentAnimation == animID);
                                if(ImGui::Selectable(object.model->animationName(animID).data(), isActive)) {
                                    object.model->setAnimation(animID);
                                    currentAnimation = animID;
                                }
                                if(isActive) {
                                    ImGui::SetItemDefaultFocus();
                                }
                            }
                            ImGui::EndListBox();
                        }
                        ImGui::TreePop();
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::End();
        }
    };
    coreMenu.items.emplace(modelsItem.name, modelsItem);

    ImGuiHandler::ImGuiMenuItem debugItem;
    debugItem.name = "Debug";
    debugItem.procedure = [] {
        if(ImGui::Begin("Debug")) {
            ImGui::Checkbox("Debug normals", &VktCorePtr->debugConf.enableDebugNormals);
            ImGui::Checkbox("Debug vectors", &VktCorePtr->debugConf.enableDebugVectors);
            ImGui::End();
        }
    };
    coreMenu.items.emplace(debugItem.name, debugItem);

    ImGuiHandler::insertMenu(coreMenu);
}