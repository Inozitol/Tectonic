#ifndef TECTONIC_VKTLOADER_H
#define TECTONIC_VKTLOADER_H

#include <unordered_map>
#include <filesystem>

#include "engine/vulkan/VktTypes.h"

struct GeoSurface{
    uint32_t startIndex;
    uint32_t count;
};

struct MeshAsset{
    std::string name;
    std::vector<GeoSurface> surfaces;
    VktTypes::GPUMeshBuffers meshBuffers;
};

class VktCore;

std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(VktCore* core, std::filesystem::path filePath);

#endif //TECTONIC_VKTLOADER_H
