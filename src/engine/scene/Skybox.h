#pragma once
#include "engine/scene/model/Model.h"

struct Skybox {
    Skybox() = default;
    explicit Skybox(const char* path);
    ~Skybox();

    void load(const char* path);
    void clear();

    static void draw(VkCommandBuffer cmd, VkDescriptorSet sceneDescriptorSet);

    void writeColorSet() const;
    void writeIBLSets(DescriptorWriter& writer) const;

    constexpr static const char* CUBE_PATH = "meshes/cube.tecm";
    constexpr static uint32_t BRDF_SIZE = 512;
    static void initDescriptors();
    //static void clearDescriptors();
    static void initPipelines();
    static void clearPipelines();
    static void initCube();
    static void clearCube();
    static void initBRDF();
    static void clearBRDF();

    void generateIBLCubemaps();

    VktTypes::Resources::Image colorCubemap;
    VktTypes::Resources::Image IBLDiffuseCubemap;
    VktTypes::Resources::Image IBLSpecularCubemap;

    inline static Model cube;
    inline static VkDescriptorSet descriptorSet;
    inline static VktTypes::ModelPipeline colorPipeline;
    inline static VktTypes::ModelPipeline IBLDiffusePipeline;
    inline static VktTypes::ModelPipeline IBLSpecularPipeline;
    inline static VktTypes::ModelPipeline IBLBRDFPipeline;

    inline static VktTypes::Resources::Image BRDFImage; // TODO why static?
    inline static uint32_t skyboxCount = 0;

    inline static VktTypes::RigidRenderObject cubeRenderable;

    bool loaded = false;
    std::string path;
};
