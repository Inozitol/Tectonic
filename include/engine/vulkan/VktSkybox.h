#pragma once
#include "engine/model/Model.h"

class VktSkybox {
public:
    VktSkybox() = default;
    VktSkybox(const char* path);
    ~VktSkybox();

    void load(const char* path);
    void clear();

    static void draw(VkCommandBuffer cmd, VkDescriptorSet sceneDescriptorSet);

    void writeColorSet() const;
    void writeIBLSets(DescriptorWriter& writer) const;

    constexpr static const char* CUBE_PATH = "meshes/cube.tecm";
    constexpr static uint32_t BRDF_SIZE = 512;
private:
    static void initDescriptors();
    //static void clearDescriptors();
    static void initPipelines();
    static void clearPipelines();
    static void initCube();
    static void clearCube();
    static void initBRDF();
    static void clearBRDF();

    void generateIBLCubemaps();

    VktTypes::Resources::Image m_colorCubemap;
    VktTypes::Resources::Image m_IBLDiffuseCubemap;
    VktTypes::Resources::Image m_IBLSpecularCubemap;

    inline static Model m_cube;
    inline static VkDescriptorSet m_descriptorSet;
    inline static VktTypes::ModelPipeline m_colorPipeline;
    inline static VktTypes::ModelPipeline m_IBLDiffusePipeline;
    inline static VktTypes::ModelPipeline m_IBLSpecularPipeline;
    inline static VktTypes::ModelPipeline m_IBLBRDFPipeline;

    inline static VktTypes::Resources::Image m_BRDFImage;
    inline static uint32_t m_skyboxCount = 0;

    bool m_loaded = false;

    inline static Logger m_logger = Logger("VktSkybox");
};
