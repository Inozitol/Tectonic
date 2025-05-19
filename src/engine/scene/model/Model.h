#pragma once

#include <array>
#include <functional>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "utils/exceptions.h"
#include "utils/Serial.h"

#include "utils/Logger.h"
#include "ModelTypes.h"
#include "math/Transformation.h"
#include <filesystem>
#include <engine/camera/Camera.h>

#include "geometry/AABB.h"
#include "geometry/OBB.h"

#include <utils/Utils.h>

/**
 * Used to load a model from file into the scene.
 * Handles loading of vertices, indices and textures.
 */
struct Model {
    enum class Flags : uint8_t {
        NONE = 0,
        SKINNED = 1 << 0
    };
    INIT_ENUM_FR_OP(Flags);

    Model();
    ~Model();
    explicit Model(const std::filesystem::path& path);
    Model(const Model& other);
    Model& operator=(Model const&);

    void clear() const;

    void initSignals();

    template<typename RenderType>
    void updateDrawContextFromOffset(VktTypes::DrawContext<RenderType>& ctx, std::size_t offset) const
    requires VktConstraints::IsRenderableTransformable<RenderType>;

    template<typename RenderType>
    void getDrawContext(VktTypes::DrawContext<RenderType>& ctx) const
    requires VktConstraints::IsRenderableTransformable<RenderType>;

    void setAnimation(uint32_t aID);
    uint32_t currentAnimation() const;
    std::string_view animationName(uint32_t aID) const;
    uint32_t animationCount() const;

    void updateAnimationTime();
    void updateJoints();
    void uploadJointsMatrices();

    bool isSkinned() const;
    bool isLoaded() const;

    std::size_t renderables() const;

    Transformation transformation{};

    struct Resources {
        SerialTypes::BinDataVec_t data;
        bool isSkinned;

        std::vector<VktTypes::MeshAsset> meshes;
        std::vector<VktTypes::Resources::Image> images;
        std::vector<VkSampler> samplers;
        std::vector<ModelTypes::Node> nodes;
        std::vector<ModelTypes::GLTFMaterial> materials;
        OBB obb;
        ModelTypes::Skin skin;
        std::vector<ModelTypes::Animation> animations;
        uint32_t rootNode;

        std::vector<uint32_t> meshNodes;
        std::vector<uint32_t> skinNodes;

        DescriptorAllocatorDynamic descriptorPool;
        VktTypes::Resources::Buffer materialBuffer;
        VktTypes::ModelPipeline* pipeline;

        uint32_t activeModels = 0;
    };

    std::string path;
    std::vector<VktTypes::MeshAsset>* meshes = nullptr;
    std::vector<VktTypes::Resources::Image>* images = nullptr;
    std::vector<VkSampler>* samplers = nullptr;
    std::vector<ModelTypes::GLTFMaterial>* materials = nullptr;
    std::vector<ModelTypes::Node> nodes{};
    OBB obb;
    glm::vec3 obbOriginalHalfLengths = {0.0f,0.0f,0.0f};
    ModelTypes::Skin skin;
    std::vector<ModelTypes::Animation> animations;
    uint32_t rootNode = ModelTypes::NULL_ID;

    Flags flags = Flags::NONE;

    uint32_t activeAnimation = ModelTypes::NULL_ID;

    uint32_t id = ModelTypes::NULL_ID;

    static void readMesh(VktTypes::MeshAsset& dst, SerialTypes::BinDataVec_t& src, std::size_t& offset);
    static void readSkinnedMesh(VktTypes::MeshAsset& dst, SerialTypes::BinDataVec_t& src, std::size_t& offset);
    static void readImage(VktTypes::Resources::Image& dst, SerialTypes::BinDataVec_t& src, std::size_t& offset);
    static void readSampler(VkSampler& dst, SerialTypes::BinDataVec_t& src, std::size_t& offset);
    static void readNode(ModelTypes::Node& dst, SerialTypes::BinDataVec_t& src, std::size_t& offset);
    static void readSkin(ModelTypes::Skin& dst, SerialTypes::BinDataVec_t& src, std::size_t& offset);
    static void readAnimation(ModelTypes::Animation& dst, SerialTypes::BinDataVec_t& src, std::size_t& offset);
    static void readAnimationSampler(ModelTypes::AnimationSampler& dst, SerialTypes::BinDataVec_t& src, std::size_t& offset);
    static void readMaterial(ModelTypes::GLTFMaterial& dst,
                             SerialTypes::BinDataVec_t& src,
                             std::size_t& offset,
                             uint32_t mIndex,
                             Resources& resources);
    static void readOBB(OBB& dst, SerialTypes::BinDataVec_t& src, std::size_t& offset);

    VktTypes::GPU::JointsBuffers jointsBuffer;

    static void loadModelData(const std::filesystem::path& path);
    static std::unordered_map<std::string, Resources> loadedModels;

    Signal<uint32_t> sig_change;

    Slot<glm::vec3> slt_translated{
        [this](const glm::vec3 pos) {
            obb.center = pos;
            sig_change.emit(id);
        }
    };

    Slot<glm::vec3> slt_rotation{
        [this](const glm::vec3 rot) {
            obb.rotation = glm::angleAxis(glm::radians(rot.x), Axis::POS_X);
            obb.rotation *= glm::angleAxis(glm::radians(rot.y), Axis::POS_Y);
            obb.rotation *= glm::angleAxis(glm::radians(rot.z), Axis::POS_Z);

            sig_change.emit(id);
        }
    };

    Slot<float> slt_scale{
        [this](const float scale) {
            obb.halfLengths = obbOriginalHalfLengths * scale;

            sig_change.emit(id);
        }
    };

private:
    bool m_isLoaded = false;
};
