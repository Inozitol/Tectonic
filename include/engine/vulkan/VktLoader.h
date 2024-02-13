#ifndef TECTONIC_VKTLOADER_H
#define TECTONIC_VKTLOADER_H

#include <unordered_map>
#include <filesystem>

#include "engine/vulkan/VktTypes.h"
#include "Logger.h"

struct GLTFMaterial{
    explicit GLTFMaterial(VktTypes::MaterialInstance data): data(data){}
    GLTFMaterial() = default;
    VktTypes::MaterialInstance data;
};

struct GeoSurface{
    uint32_t startIndex;
    uint32_t count;
    std::shared_ptr<GLTFMaterial> material;
};

struct MeshAsset{
    std::string name;
    std::vector<GeoSurface> surfaces;
    VktTypes::GPUMeshBuffers meshBuffers;
};

struct LoadedGLTF : public VktTypes::IRenderable{
    std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
    std::unordered_map<std::string, std::shared_ptr<VktTypes::Node>> nodes;
    std::unordered_map<std::string, VktTypes::AllocatedImage> images;
    std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> materials;

    std::vector<std::shared_ptr<VktTypes::Node>> topNodes;
    std::vector<VkSampler> samplers;
    DescriptorAllocatorDynamic descriptorPool;
    VktTypes::AllocatedBuffer materialDataBuffer;

    ~LoadedGLTF(){ clean(); }
    virtual void draw(const glm::mat4& topMatrix, VktTypes::DrawContext& ctx);
private:
    void clean();
};

std::optional<std::shared_ptr<LoadedGLTF>> loadGltfMeshes(const std::filesystem::path& filePath);

#endif //TECTONIC_VKTLOADER_H
