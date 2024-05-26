#include "engine/vulkan/VktLoader.h"

#include "stb_image.h"
#include <iostream>

#include "engine/vulkan/VktCore.h"
#include "engine/vulkan/VktStructs.h"
#include "engine/vulkan/VktTypes.h"
#include <glm/gtx/quaternion.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>

#include "Logger.h"

Logger vktLoaderLogger("VulkanLoader");

/**
 * @brief Extracts VkFilter from fastgltf::Filter
 * @param filter fastglft::Filter
 * @return VkFilter
 */
VkFilter extractFilter(fastgltf::Filter filter){
    switch(filter){
        case fastgltf::Filter::Nearest:
        case fastgltf::Filter::NearestMipMapNearest:
        case fastgltf::Filter::NearestMipMapLinear:
            return VK_FILTER_NEAREST;

        case fastgltf::Filter::Linear:
        case fastgltf::Filter::LinearMipMapNearest:
        case fastgltf::Filter::LinearMipMapLinear:
        default:
            return VK_FILTER_LINEAR;
    }
}

/**
 * @brief Extracts VkSamplerMipmapMode from fastgltf::Filter
 * @param filter fastgltf::Filter
 * @return VkSamplerMipmapMode
 */
VkSamplerMipmapMode extractMipmapMode(fastgltf::Filter filter){
    switch(filter){
        case fastgltf::Filter::Nearest:
        case fastgltf::Filter::NearestMipMapNearest:
        case fastgltf::Filter::LinearMipMapNearest:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;

        case fastgltf::Filter::Linear:
        case fastgltf::Filter::NearestMipMapLinear:
        case fastgltf::Filter::LinearMipMapLinear:
        default:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
}

/**
 * @brief Allocates a fastgltf::Image from fastgltf::Asset
 * @param asset fastgltf::Asset
 * @param image fastgltf::Image
 * @return AllocatedImage or Nothing
 */
std::optional<VktTypes::AllocatedImage> loadImage(fastgltf::Asset& asset, fastgltf::Image& image){
    VktTypes::AllocatedImage newImage;
    int width, height, channels;

    std::visit(fastgltf::visitor{
        [](auto& arg){},
        [&](fastgltf::sources::URI& filePath){
            assert(filePath.fileByteOffset == 0);
            assert(filePath.uri.isLocalPath());

            const std::string path(filePath.uri.path().begin(), filePath.uri.path().end());
            unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
            if(data){
                VkExtent3D imageSize;
                imageSize.width = width;
                imageSize.height = height;
                imageSize.depth = 1;

                newImage = VktCore::createImage(data, imageSize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);

                stbi_image_free(data);
            }
        },
        [&](fastgltf::sources::Vector& vector){
            unsigned char* data = stbi_load_from_memory(vector.bytes.data(), static_cast<int>(vector.bytes.size()),
                                                        &width, &height, &channels, 4);
            if(data){
                VkExtent3D imageSize;
                imageSize.width = width;
                imageSize.height = height;
                imageSize.depth = 1;

                newImage = VktCore::createImage(data, imageSize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);

                stbi_image_free(data);
            }
        },
        [&](fastgltf::sources::BufferView& view){
            auto& bufferView = asset.bufferViews[view.bufferViewIndex];
            auto& buffer = asset.buffers[bufferView.bufferIndex];

            std::visit(fastgltf::visitor{
                [](auto& arg){},
                [&](fastgltf::sources::Vector& vector){
                    unsigned char* data = stbi_load_from_memory(vector.bytes.data() + bufferView.byteOffset,
                                                                static_cast<int>(bufferView.byteLength),
                                                                &width, &height, &channels, 4);
                    if(data) {
                        VkExtent3D imageSize;
                        imageSize.width = width;
                        imageSize.height = height;
                        imageSize.depth = 1;

                        newImage = VktCore::createImage(data, imageSize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);

                        stbi_image_free(data);
                    }
                }
            },buffer.data);
        }
    }, image.data);

    if(newImage.image == VK_NULL_HANDLE){
        return {};
    }else{
        return newImage;
    }
}

/**
 * @brief Determines the glTF file type and loads it.
 * @param path Path to glTF file
 * @return fastgltf::Asset or Nothing
 */
std::optional<fastgltf::Asset> loadFile(const std::filesystem::path& path){
    constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember |
                                 fastgltf::Options::AllowDouble |
                                 fastgltf::Options::LoadGLBBuffers |
                                 fastgltf::Options::LoadExternalBuffers;

    fastgltf::Parser parser{};

    fastgltf::GltfDataBuffer data;
    data.loadFromFile(path);

    auto type = fastgltf::determineGltfFileType(&data);
    if(type == fastgltf::GltfType::glTF){
        auto load = parser.loadGltf(&data, path.parent_path(), gltfOptions);
        if(load){
            vktLoaderLogger(Logger::DEBUG) << path << " Detected as JSON glTF file\n";
            return std::move(load.get());
        }else{
            vktLoaderLogger(Logger::ERROR) << "Failed to load glTF: " << path
                                           << " | error: " << fastgltf::to_underlying(load.error()) << '\n';
            return {};

        }
    }else if(type == fastgltf::GltfType::GLB){
        auto load = parser.loadGltfBinary(&data, path.parent_path(), gltfOptions);
        if(load){
            vktLoaderLogger(Logger::DEBUG) << path << " Detected as binary GLB file\n";
            return std::move(load.get());
        }else{
            vktLoaderLogger(Logger::ERROR) << "Failed to load glTF: " << path
                                           << " | error: " << fastgltf::to_underlying(load.error()) << '\n';
            return {};
        }
    }else{
        vktLoaderLogger(Logger::ERROR) << "Failed to determine glTF type" << '\n';
        return {};
    }
}

void loadSamplers(fastgltf::Asset& gltf, LoadedGLTF& file, const std::filesystem::path& path){
    for(fastgltf::Sampler& sampler : gltf.samplers) {
        VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};
        sampl.maxLod = VK_LOD_CLAMP_NONE;
        sampl.minLod = 0;

        sampl.magFilter = extractFilter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
        sampl.minFilter = extractFilter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

        sampl.mipmapMode = extractMipmapMode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

        VkSampler newSampler;
        vkCreateSampler(VktCore::device(), &sampl, nullptr, &newSampler);

        file.samplers.push_back(newSampler);
    }
}

void loadImages(fastgltf::Asset& gltf, LoadedGLTF& file, const std::filesystem::path& path){
    for(fastgltf::Image& image : gltf.images){
        std::optional<VktTypes::AllocatedImage> img = loadImage(gltf, image);

        if(img.has_value()){
            file.images.push_back(*img);
            if(file.namedImages.contains(image.name.c_str())){
                vktLoaderLogger(Logger::WARNING) << path
                                                 << " Texture name collision at texture named [" << image.name.c_str() << "] and ID " << file.images.size()-1 << '\n';
            }
            file.namedImages[image.name.c_str()] = &file.images.back();
            vktLoaderLogger(Logger::DEBUG) << path << " Loaded texture with name [" << image.name.c_str() << "] at ID " << file.images.size()-1 << '\n';
        }else{
            file.images.push_back(VktCore::getInstance().m_errorCheckboardImage);
            vktLoaderLogger(Logger::ERROR) << "Failed to load texture [" << image.name.c_str() << "]\n";
        }
    }
}

void loadMaterials(fastgltf::Asset& gltf, LoadedGLTF& file, const std::filesystem::path& path) {
    uint32_t dataIndex = 0;
    VktCore::GLTFMetallicRoughness::MaterialConstants* sceneMaterialConstants = (VktCore::GLTFMetallicRoughness::MaterialConstants*)file.materialDataBuffer.info.pMappedData;

    for(fastgltf::Material& mat : gltf.materials){
        file.materials.push_back(std::make_unique<VktTypes::GLTFMaterial>());
        file.namedMaterials[mat.name.c_str()] = file.materials.back().get();
        VktTypes::GLTFMaterial* newMat = file.materials.back().get();

        VktCore::GLTFMetallicRoughness::MaterialConstants constants;
        constants.colorFactors.x = mat.pbrData.baseColorFactor[0];
        constants.colorFactors.y = mat.pbrData.baseColorFactor[1];
        constants.colorFactors.z = mat.pbrData.baseColorFactor[2];
        constants.colorFactors.w = mat.pbrData.baseColorFactor[3];

        constants.metalRoughFactors.x = mat.pbrData.metallicFactor;
        constants.metalRoughFactors.y = mat.pbrData.roughnessFactor;

        sceneMaterialConstants[dataIndex] = constants;

        VktTypes::MaterialPass passType = VktTypes::MaterialPass::OPAQUE;
        if(mat.alphaMode == fastgltf::AlphaMode::Blend){
            passType = VktTypes::MaterialPass::TRANSPARENT;
        }

        VktCore::GLTFMetallicRoughness::MaterialResources materialResources;
        materialResources.colorImage = VktCore::getInstance().m_errorCheckboardImage;
        materialResources.colorSampler = VktCore::getInstance().m_defaultSamplerLinear;
        materialResources.metalRoughImage = VktCore::getInstance().m_errorCheckboardImage;
        materialResources.metalRoughSampler = VktCore::getInstance().m_defaultSamplerLinear;

        materialResources.dataBuffer = file.materialDataBuffer.buffer;
        materialResources.dataBufferOffset = dataIndex * sizeof(VktCore::GLTFMetallicRoughness::MaterialConstants);

        if(mat.pbrData.baseColorTexture.has_value()){
            size_t img     = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
            size_t sampler = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();
            materialResources.colorImage = file.images[img];
            materialResources.colorSampler = file.samplers[sampler];
        }
        if(mat.pbrData.metallicRoughnessTexture.has_value()) {
            size_t img     = gltf.textures[mat.pbrData.metallicRoughnessTexture.value().textureIndex].imageIndex.value();
            size_t sampler = gltf.textures[mat.pbrData.metallicRoughnessTexture.value().textureIndex].samplerIndex.value();
            materialResources.metalRoughImage = file.images[img];
            materialResources.metalRoughSampler = file.samplers[sampler];
        }
        newMat->data = VktCore::getInstance().m_metalRoughMaterial.writeMaterial(VktCore::device(), passType, materialResources, file.descriptorPool);
        dataIndex++;

        vktLoaderLogger(Logger::DEBUG) << path << " Loaded material with name [" << mat.name.c_str() << "] at ID " << file.materials.size()-1 << '\n';
    }
}

void loadMeshes(fastgltf::Asset& gltf, LoadedGLTF& file, const std::filesystem::path& path) {
    std::vector<uint32_t> indices;
    std::vector<VktTypes::Vertex> vertices;

    for(fastgltf::Mesh& mesh : gltf.meshes){
        file.meshes.push_back(std::make_unique<VktTypes::MeshAsset>());
        file.namedMeshes[mesh.name.c_str()] = file.meshes.back().get();
        VktTypes::MeshAsset* newMesh = file.meshes.back().get();

        indices.clear();
        vertices.clear();

        for(auto&& p : mesh.primitives){
            VktTypes::GeoSurface newSurface{};
            newSurface.startIndex = static_cast<uint32_t>(indices.size());
            newSurface.count = static_cast<uint32_t>(gltf.accessors[p.indicesAccessor.value()].count);

            size_t initialVtx = vertices.size();

            // Load indices
            {
                fastgltf::Accessor& indexAccessor = gltf.accessors[p.indicesAccessor.value()];
                indices.reserve(indices.size() + indexAccessor.count);

                fastgltf::iterateAccessor<std::uint32_t>(gltf, indexAccessor,
                                                         [&](std::uint32_t idx){
                                                             indices.push_back(idx + initialVtx);
                                                         });
            }

            // Load positions
            {
                fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->second];
                vertices.resize(vertices.size() + posAccessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
                                                              [&](glm::vec3 v, size_t index){
                                                                  VktTypes::Vertex vtx;
                                                                  vtx.position = v;
                                                                  vtx.color = {1.0f,1.0f,1.0f,1.0f};
                                                                  vertices[initialVtx + index] = vtx;
                                                              });
            }

            // Load normals
            {
                auto normals = p.findAttribute("NORMAL");
                if (normals != p.attributes.end()) {
                    fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[normals->second],
                                                                  [&](glm::vec3 v, size_t index) {
                                                                      vertices[initialVtx + index].normal = v;
                                                                  });
                }
            }

            // Load UV coords
            {
                auto uv = p.findAttribute("TEXCOORD_0");
                if (uv != p.attributes.end()) {
                    fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[uv->second],
                                                                  [&](glm::vec2 v, size_t index) {
                                                                      vertices[initialVtx + index].uvX = v.x;
                                                                      vertices[initialVtx + index].uvY = v.y;
                                                                  });
                }
            }

            // Load colors
            {
                auto colors = p.findAttribute("COLOR_0");
                if (colors != p.attributes.end()) {
                    fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[colors->second],
                                                                  [&](glm::vec4 v, size_t index) {
                                                                      vertices[initialVtx + index].color = v;
                                                                  });
                }
            }

            if(p.materialIndex.has_value()){
                newSurface.material = file.materials[p.materialIndex.value()].get();
            }else{
                newSurface.material = file.materials[0].get();
            }

            newMesh->surfaces.push_back(newSurface);
        }
        newMesh->meshBuffers = VktCore::uploadMesh(indices, vertices);

        vktLoaderLogger(Logger::DEBUG) << path << " Loaded mesh with name " << mesh.name.c_str() << " at ID " << file.meshes.size()-1 << '\n';
    }
}

void loadNodes(fastgltf::Asset& gltf, LoadedGLTF& file, const std::filesystem::path& path) {
    for(fastgltf::Node& node : gltf.nodes){
        file.nodes.push_back(std::make_unique<VktTypes::Node>());
        file.namedNodes[node.name.c_str()] = file.nodes.back().get();
        VktTypes::Node* newNode = file.nodes.back().get();
        newNode->name = node.name;

        if(node.meshIndex.has_value()){
            newNode->mesh = file.meshes[*node.meshIndex].get();
        }

        std::visit(fastgltf::visitor{
                [&](fastgltf::Node::TransformMatrix matrix){
                    memcpy(&newNode->localTransform, matrix.data(), sizeof(matrix));
                },
                [&](fastgltf::TRS transform){
                    glm::vec3 tl(transform.translation[0], transform.translation[1], transform.translation[2]);
                    glm::quat rot(transform.rotation[3], transform.rotation[0], transform.rotation[1], transform.rotation[2]);
                    glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);

                    glm::mat4 tm = glm::translate(glm::identity<glm::mat4>(), tl);
                    glm::mat4 rm = glm::toMat4(rot);
                    glm::mat4 sm = glm::scale(glm::identity<glm::mat4>(), sc);

                    newNode->localTransform = tm * rm * sm;
                }},node.transform);

        vktLoaderLogger(Logger::DEBUG) << path << " Loaded node with name " << node.name.c_str() << " at ID " << file.nodes.size()-1 << '\n';
    }
}

void connectNodes(fastgltf::Asset& gltf, LoadedGLTF& file, const std::filesystem::path& path){
    // Connect nodes to create trees
    for(int i = 0; i < gltf.nodes.size(); i++){
        fastgltf::Node& node = gltf.nodes[i];
        VktTypes::Node* sceneNode = file.nodes[i].get();

        for(auto& c : node.children){
            sceneNode->children.push_back(file.nodes[c].get());
            file.nodes[c]->parent = sceneNode;

            vktLoaderLogger(Logger::DEBUG) << path
                                           << " Node with name " << file.nodes[c]->name
                                           << " at ID " << c
                                           << " connected to parent with name " << sceneNode->name
                                           << " at ID " << i << '\n';
        }
    }
}

bool hasCycle(const VktTypes::Node* node, std::vector<const VktTypes::Node*> visited){
    if(std::find(visited.begin(),visited.end(), node) != visited.end()){
        return true;
    }
    visited.push_back(node);
    for(auto& c : node->children) {
        if(hasCycle(c, visited)){
            return true;
        }
    }
    visited.pop_back();
    return false;
}

void createTrees(LoadedGLTF& file, const std::filesystem::path& path) {
    // Store root nodes as reference
    for(std::size_t i = 0; i < file.nodes.size(); i++){
        VktTypes::Node* node = file.nodes[i].get();
        if(node->parent == nullptr){
            file.topNodes.push_back(node);
            if(hasCycle(node, {})){
                vktLoaderLogger(Logger::ERROR) << path << " Root node with name " << node->name << " at ID " << i << " contains a cycle" << '\n';
            }
            VktTypes::Node::refreshTransform(*node, glm::identity<glm::mat4>());
            vktLoaderLogger(Logger::DEBUG) << path << " Node with name " << node->name << " at ID " << i << " used as a root node" << '\n';
        }
    }
}


/**
 * @brief Loads a glTF file.
 * @param filePath Path to glTF file
 * @return LoadedGLTF or Nothing
 */
std::optional<std::shared_ptr<LoadedGLTF>> loadGltfMeshes(const std::filesystem::path& filePath){
    vktLoaderLogger(Logger::INFO) << "Loading GLTF: " << filePath << '\n';
    std::shared_ptr<LoadedGLTF> scene = std::make_shared<LoadedGLTF>();
    LoadedGLTF& file = *scene;

    std::optional<fastgltf::Asset> loadedGLTF = loadFile(filePath);
    if(!loadedGLTF.has_value()) return {};

    fastgltf::Asset gltf = std::move(loadedGLTF.value());

    std::vector<DescriptorAllocatorDynamic::PoolSizeRatio> sizes = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}
    };

    file.descriptorPool.initPool(VktCore::device(), gltf.materials.size(), sizes);

    loadSamplers(gltf,file,filePath);
    loadImages(gltf,file,filePath);

    file.materialDataBuffer = VktCore::createBuffer(sizeof(VktCore::GLTFMetallicRoughness::MaterialConstants) * gltf.materials.size(),
                                                                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    loadMaterials(gltf,file,filePath);
    loadMeshes(gltf,file,filePath);
    loadNodes(gltf,file,filePath);

    connectNodes(gltf,file,filePath);
    createTrees(file,filePath);

    return scene;
}

void LoadedGLTF::gatherContext(const glm::mat4 &topMatrix, VktTypes::DrawContext &ctx) {
    for(auto& n : topNodes){
        VktTypes::Node::gatherContext(*n, topMatrix, ctx);
    }
}

void LoadedGLTF::clean() {
    VkDevice device = VktCore::device();

    descriptorPool.destroyPool(device);
    VktCore::destroyBuffer(materialDataBuffer);

    for(auto& m : meshes){
        VktCore::destroyBuffer(m->meshBuffers.indexBuffer);
        VktCore::destroyBuffer(m->meshBuffers.vertexBuffer);
    }

    for(auto& i : images){
        if(i.image == VktCore::getInstance().m_errorCheckboardImage.image){
            continue;
        }
        // TODO image destroyed elsewhere, move it here
        //VktCore::destroyImage(v);
    }

    for(auto& sampler : samplers){
        vkDestroySampler(device, sampler, nullptr);
    }
}