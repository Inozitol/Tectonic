#include "engine/vulkan/VulkanLoader.h"

#include "stb_image.h"
#include <iostream>
#include "engine/vulkan/VulkanLoader.h"

#include "engine/vulkan/VulkanCore.h"
#include "engine/vulkan/VulkanStructs.h"
#include "engine/vulkan/VulkanTypes.h"
#include <glm/gtx/quaternion.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>

std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(VulkanCore* core, std::filesystem::path filePath){
    fastgltf::GltfDataBuffer data;
    data.loadFromFile(filePath);

    constexpr auto gltfOpt = fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;

    fastgltf::Asset gltf;
    fastgltf::Parser parser{};

    auto load = parser.loadGltfBinary(&data, filePath.parent_path(), gltfOpt);
    if(load){
        gltf = std::move(load.get());
    }else{
        fprintf(stderr, "Failed to load glTF %lu", fastgltf::to_underlying(load.error()));
        return {};
    }

    std::vector<std::shared_ptr<MeshAsset>> meshes;

    std::vector<uint32_t> indices;
    std::vector<VkTypes::Vertex> vertices;
    for(fastgltf::Mesh& mesh : gltf.meshes){
        MeshAsset newMesh;

        newMesh.name = mesh.name;

        indices.clear();
        vertices.clear();

        for(auto&& p : mesh.primitives){
            GeoSurface newSurface{};
            newSurface.startIndex = static_cast<uint32_t>(indices.size());
            newSurface.count = static_cast<uint32_t>(gltf.accessors[p.indicesAccessor.value()].count);

            size_t initialVtx = vertices.size();

            // Load indices
            {
                fastgltf::Accessor& indexAccessor = gltf.accessors[p.indicesAccessor.value()];
                indices.reserve(indices.size() + indexAccessor.count);

                fastgltf::iterateAccessor<std::uint32_t>(gltf, indexAccessor, [&](std::uint32_t idx){
                    indices.push_back(idx + initialVtx);
                });
            }

            // Load vertex positions
            {
                fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->second];
                vertices.resize(vertices.size() + posAccessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor, [&](glm::vec3 v, size_t index){
                    VkTypes::Vertex newVtx{};
                    newVtx.position = v;
                    newVtx.normal = {1.0f, 0.0f, 0.0f};
                    newVtx.color = glm::vec4{1.0f};
                    newVtx.uvX = 0.0f;
                    newVtx.uvY = 0.0f;
                    vertices[initialVtx + index] = newVtx;
                });
            }

            // Load normals
            {
                auto normals = p.findAttribute("NORMAL");
                if (normals != p.attributes.end()) {
                    fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[normals->second],[&](glm::vec3 v, size_t index) {
                                                                      vertices[initialVtx + index].normal = v;
                                                                  });
                }
            }

            // Load tex coords
            {
                auto uv = p.findAttribute("TEXCOORD_0");
                if (uv != p.attributes.end()) {
                    fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[uv->second],[&](glm::vec2 v, size_t index) {
                                                                      vertices[initialVtx + index].uvX = v.x;
                                                                      vertices[initialVtx + index].uvY = v.y;
                                                                  });
                }
            }

            // Load color
            auto colors = p.findAttribute("COLOR_0");
            if(colors != p.attributes.end()){
                fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[colors->second], [&](glm::vec4 v, size_t index){
                    vertices[initialVtx + index].color = v;
                });
            }
            newMesh.surfaces.push_back(newSurface);
        }

        constexpr bool overrideColors = true;
        if(overrideColors){
            for(VkTypes::Vertex& vtx : vertices){
                vtx.color = glm::vec4(vtx.normal, 1.0f);
            }
        }
        newMesh.meshBuffers = core->uploadMesh(indices, vertices);
        meshes.emplace_back(std::make_shared<MeshAsset>(std::move(newMesh)));
    }

    return meshes;
}