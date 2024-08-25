#pragma once

#include <array>
#include <functional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "exceptions.h"
#include "utils/Serial.h"

#include "Logger.h"
#include "ModelTypes.h"
#include "Transformation.h"
#include <filesystem>

/**
 * Used to load a model from file into the scene.
 * Handles loading of vertices, indices and textures.
 */
class Model {
public:
    Model() = default;
    ~Model();
    Model& operator=(Model const&);

    void clear();

    explicit Model(const std::filesystem::path &path);

    void gatherDrawContext(VktTypes::DrawContext &ctx);

    void setAnimation(uint32_t aID);
    uint32_t currentAnimation() const;
    std::string_view animationName(uint32_t aID) const;
    uint32_t animationCount() const;

    void updateAnimationTime();
    void updateJoints();
    void uploadJointsMatrices();

    std::vector<ModelTypes::Node>& nodes();
    ModelTypes::Skin& skin();

    bool isSkinned() const;
    bool isLoaded() const;
    const std::string& path() const;

    Transformation transformation;

private:
    struct Resources {
        SerialTypes::BinDataVec_t data;
        bool isSkinned;

        std::vector<VktTypes::MeshAsset> meshes;
        std::vector<VktTypes::Resources::Image> images;
        std::vector<VkSampler> samplers;
        std::vector<ModelTypes::Node> nodes;
        std::vector<ModelTypes::GLTFMaterial> materials;
        ModelTypes::Skin skin;
        std::vector<ModelTypes::Animation> animations;
        uint32_t rootNode;

        std::vector<uint32_t> meshNodes;
        std::vector<uint32_t> skinNodes;

        DescriptorAllocatorDynamic descriptorPool;
        VktTypes::Resources::Buffer materialBuffer;
        VktTypes::ModelPipeline *pipeline;

        uint32_t activeModels = 0;
    };
    std::string m_modelPath;
    bool m_isLoaded = false;

    std::vector<VktTypes::MeshAsset> *m_meshes = nullptr;
    std::vector<VktTypes::Resources::Image> *m_images = nullptr;
    std::vector<VkSampler> *m_samplers = nullptr;
    std::vector<ModelTypes::GLTFMaterial> *m_materials = nullptr;
    std::vector<ModelTypes::Node> m_nodes;
    ModelTypes::Skin m_skin;
    std::vector<ModelTypes::Animation> m_animations;
    uint32_t m_rootNode = ModelTypes::NULL_ID;
    bool m_isSkinned = false;

    uint32_t m_activeAnimation = ModelTypes::NULL_ID;

    static void readMesh(VktTypes::MeshAsset &dst, SerialTypes::BinDataVec_t &src, std::size_t &offset);
    static void readSkinnedMesh(VktTypes::MeshAsset &dst, SerialTypes::BinDataVec_t &src, std::size_t &offset);
    static void readImage(VktTypes::Resources::Image &dst, SerialTypes::BinDataVec_t &src, std::size_t &offset);
    static void readSampler(VkSampler &dst, SerialTypes::BinDataVec_t &src, std::size_t &offset);
    static void readNode(ModelTypes::Node &dst, SerialTypes::BinDataVec_t &src, std::size_t &offset);
    static void readSkin(ModelTypes::Skin &dst, SerialTypes::BinDataVec_t &src, std::size_t &offset);
    static void readAnimation(ModelTypes::Animation &dst, SerialTypes::BinDataVec_t &src, std::size_t &offset);
    static void readAnimationSampler(ModelTypes::AnimationSampler &dst, SerialTypes::BinDataVec_t &src, std::size_t &offset);
    void readMaterial(ModelTypes::GLTFMaterial &dst,
                      SerialTypes::BinDataVec_t &src,
                      std::size_t &offset,
                      uint32_t mIndex,
                      Resources &resources);

    VktTypes::GPU::JointsBuffers m_jointsBuffer;

    void loadModelData(const std::filesystem::path &path);
    static std::unordered_map<std::string, Resources> m_loadedModels;

    static Logger m_logger;
};
