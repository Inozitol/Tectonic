find_package(Vulkan REQUIRED)

add_library(vkbootstrap STATIC)
add_library(vma INTERFACE)
add_library(stb STATIC)
add_library(imgui STATIC)

add_subdirectory(fastgltf)

target_sources(vkbootstrap PRIVATE vkbootstrap/VkBootstrap.h vkbootstrap/VkBootstrap.cpp)
target_include_directories(vkbootstrap PUBLIC vkbootstrap)
target_link_libraries(vkbootstrap PUBLIC Vulkan::Vulkan $<$<BOOL:UNIX>:${CMAKE_DL_LIBS}>)

target_include_directories(vma INTERFACE vma)

target_sources(stb PRIVATE stb/stb_image.cpp)
target_include_directories(stb INTERFACE stb)

target_sources(imgui PRIVATE
        imgui/imconfig.h
        imgui/imgui.h
        imgui/imgui.cpp
        imgui/imgui_internal.h
        imgui/imgui_tables.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_widgets.cpp
        imgui/imgui_impl_vulkan.h
        imgui/imgui_impl_vulkan.cpp)
target_include_directories(imgui INTERFACE imgui)