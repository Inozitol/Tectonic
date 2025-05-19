find_package(Vulkan REQUIRED)

# Link libraries
add_library(vkbootstrap STATIC)
add_library(stb STATIC)
add_library(imgui STATIC)
add_library(pugixml STATIC)
add_library(vulkan-mini-libs STATIC)

# Includes submodules
add_subdirectory(fastgltf)

# VkBootstrap
target_sources(vkbootstrap PRIVATE vkbootstrap/VkBootstrap.h vkbootstrap/VkBootstrap.cpp)
target_include_directories(vkbootstrap PUBLIC vkbootstrap)
target_link_libraries(vkbootstrap PUBLIC Vulkan::Vulkan $<$<BOOL:UNIX>:${CMAKE_DL_LIBS}>)

# VMA
add_subdirectory(VulkanMemoryAllocator)

# STB
target_sources(stb PRIVATE stb/stb_image.cpp)
target_include_directories(stb INTERFACE stb)

# imgui
target_sources(imgui PRIVATE
        imgui/imconfig.h
        imgui/imgui.h
        imgui/imgui.cpp
        imgui/imgui_internal.h
        imgui/imgui_tables.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui_impl_vulkan.h
        imgui/imgui_impl_vulkan.cpp
        imgui/imgui_impl_glfw.h
        imgui/imgui_impl_glfw.cpp
        imgui/imnodes.cpp
        imgui/imnodes.h
        imgui/imnodes_internal.h)
target_include_directories(imgui INTERFACE imgui)

add_subdirectory(KTX-Software)

# pugixml
target_sources(pugixml PRIVATE pugixml/pugixml.cpp)
target_include_directories(pugixml INTERFACE pugixml)

# vulkan-mini-libs
set_target_properties(vulkan-mini-libs PROPERTIES LINKER_LANGUAGE CXX)
target_sources(vulkan-mini-libs PRIVATE
        vulkan-mini-libs-2/vk_value_serialization.h
        vulkan-mini-libs-2/vk_value_serialization.hpp)