#pragma once

#include <string>
#include <utility>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <array>
#include <functional>

#include "exceptions.h"
#include "utils/Serial.h"

#include <filesystem>
#include "Transformation.h"
#include "ModelTypes.h"
#include "Logger.h"

/**
 * Used to load a model from file into the scene.
 * Handles loading of vertices, indices and textures.
 */
class Model {
public:
    Model() = default;
    void clear();

    explicit Model(const std::filesystem::path& path);
    void gatherDrawContext(VktTypes::DrawContext& ctx);
    void updateAnimationTime(float delta);
    void updateJoints();
    uint32_t currentAnimation() const;
    void setAnimation(uint32_t aID);
    std::string_view animationName(uint32_t aID);
    uint32_t animationCount();
    Transformation transformation;
private:

    struct Resources{
        SerialTypes::BinDataVec_t               data;

        std::vector<VktTypes::MeshAsset>        meshes;
        std::vector<VktTypes::AllocatedImage>   images;
        std::vector<VkSampler>                  samplers;
        std::vector<ModelTypes::Node>           nodes;
        std::vector<ModelTypes::GLTFMaterial>   materials;
        ModelTypes::Skin                        skin;
        std::vector<ModelTypes::Animation>      animations;
        uint32_t                                rootNode;

        DescriptorAllocatorDynamic              descriptorPool;
        VktTypes::AllocatedBuffer               materialBuffer;

        uint32_t                                activeModels = 0;
    };
    std::string                            m_modelPath;

    std::vector<VktTypes::MeshAsset>*      m_meshes = nullptr;
    std::vector<VktTypes::AllocatedImage>* m_images = nullptr;
    std::vector<VkSampler>*                m_samplers = nullptr;
    std::vector<ModelTypes::GLTFMaterial>* m_materials = nullptr;
    std::vector<ModelTypes::Node>          m_nodes;
    ModelTypes::Skin                       m_skin;
    std::vector<ModelTypes::Animation>     m_animations;
    uint32_t                               m_rootNode = ModelTypes::NULL_ID;

    uint32_t                               m_activeAnimation = 0;

    static void readMesh(VktTypes::MeshAsset& dst, SerialTypes::BinDataVec_t& src, std::size_t& offset);
    static void readImage(VktTypes::AllocatedImage& dst, SerialTypes::BinDataVec_t& src, std::size_t& offset);
    static void readSampler(VkSampler& dst, SerialTypes::BinDataVec_t& src, std::size_t& offset);
    static void readNode(ModelTypes::Node& dst, SerialTypes::BinDataVec_t& src, std::size_t& offset);
    static void readSkin(ModelTypes::Skin& dst, SerialTypes::BinDataVec_t& src, std::size_t& offset);
    static void readAnimation(ModelTypes::Animation& dst, SerialTypes::BinDataVec_t& src, std::size_t& offset);
    static void readAnimationSampler(ModelTypes::AnimationSampler& dst, SerialTypes::BinDataVec_t& src, std::size_t& offset);
    void readMaterial(ModelTypes::GLTFMaterial& dst,
                      SerialTypes::BinDataVec_t& src,
                      std::size_t& offset,
                      uint32_t mIndex,
                      Resources& resources);

    VktTypes::GPUJointsBuffers              m_jointsBuffer;

    void loadModelData(const std::filesystem::path& path);
    static std::unordered_map<std::string, Resources> m_loadedModels;

    static Logger m_logger;
};
