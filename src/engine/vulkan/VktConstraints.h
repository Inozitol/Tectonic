#pragma once
#include <glm/fwd.hpp>

namespace VktConstraints {
    template<typename Renderable>
    concept IsRenderable = requires(Renderable) {
        {Renderable{}.indexCount} -> std::convertible_to<uint32_t>;
        {Renderable{}.firstIndex} -> std::convertible_to<uint32_t>;
        {Renderable{}.vertexOffset} -> std::convertible_to<int32_t>;
        {Renderable{}.indexBuffer} -> std::convertible_to<VkBuffer>;
        {Renderable{}.vertexBufferAddress} -> std::convertible_to<VkDeviceAddress>;
    };

    template<typename Renderable>
    concept IsRenderableTransformable = requires(Renderable) {
        requires IsRenderable<Renderable>;
        {Renderable{}.transform} -> std::convertible_to<glm::mat4>;
    };

    template<typename Renderable>
    concept IsRenderableSkinned = requires(Renderable) {
        requires IsRenderable<Renderable>;
        {Renderable{}.jointsBufferAddress} -> std::convertible_to<VkDeviceAddress>;
    };

}