#pragma once

#include <glm/mat4x4.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/ext/quaternion_float.hpp>

#include <vulkan/vulkan_core.h>

#include <array>
#include <string>

#include "utils/SerialTypes.h"
#include "engine/vulkan/VktTypes.h"


namespace ModelTypes {

    constexpr uint32_t NULL_ID = UINT32_MAX;

    using ImageID_t         = uint32_t;
    using ImageSamplerID_t  = uint32_t;
    using MeshID_t          = uint32_t;
    using MaterialID_t      = uint32_t;
    using NodeID_t          = uint32_t;
    using SkinID_t          = uint32_t;
    using AnimationID_t     = uint32_t;
    using AnimSamplerID_t   = uint32_t;
    using AnimChannelID_t   = uint32_t;

    struct MaterialResources {
        ImageID_t colorImage                = NULL_ID;
        ImageSamplerID_t colorSampler       = NULL_ID;
        ImageID_t metalRoughImage           = NULL_ID;
        ImageSamplerID_t metalRoughSampler  = NULL_ID;
    };

    struct GLTFMaterial {
        VktTypes::MaterialInstance data;
    };

    struct Node {
        SerialTypes::Span<uint32_t,char,false> name;

        NodeID_t parent = NULL_ID;
        SerialTypes::Span<uint32_t,NodeID_t,false> children;

        MeshID_t mesh = NULL_ID;

        glm::mat4 localTransform = glm::identity<glm::mat4>();
        glm::mat4 worldTransform = glm::identity<glm::mat4>();
        glm::mat4 animationTransform = glm::identity<glm::mat4>();

        glm::vec3 translation{0.0f, 0.0f, 0.0f};
        glm::vec3 scale{1.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};

        SkinID_t skin = NULL_ID;
    };

    struct AnimationSampler {
        enum class Interpolation : uint8_t {
            LINEAR,
            STEP,
            CUBICSPLINE
        };

        Interpolation interpolation;
        SerialTypes::Span<uint32_t,float,false> inputs;
        SerialTypes::Span<uint32_t,glm::vec4,false> outputsVec4;
        mutable uint32_t lastInput = 0;
    };

    struct AnimationChannel {
        NodeID_t node = NULL_ID;

        AnimSamplerID_t scaleSampler        = NULL_ID;
        AnimSamplerID_t rotationSampler     = NULL_ID;
        AnimSamplerID_t translationSampler  = NULL_ID;
    };

    struct Animation {
        SerialTypes::Span<uint32_t,char, false> name;
        float start = std::numeric_limits<float>::max();
        float end = std::numeric_limits<float>::min();
        std::vector<AnimationSampler> samplers;
        SerialTypes::Span<uint32_t,AnimationChannel,false> channels;

        /** Each pair has ID of the node and ID of animation channel for that node */
        SerialTypes::Span<uint32_t,std::pair<uint32_t, uint32_t>,false> animatedNodes;
        float currentTime;
    };

    struct Skin {
        SerialTypes::Span<uint32_t,char,false> name;
        uint32_t skeletonRoot = NULL_ID;
        SerialTypes::Span<uint32_t,NodeID_t,false> skinNodes;
        SerialTypes::Span<uint32_t,glm::mat4,false> inverseBindMatrices;
        SerialTypes::Span<uint32_t,NodeID_t,false> joints;
    };
}
