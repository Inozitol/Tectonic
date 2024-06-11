#include "engine/model/gltf2tec.h"

#include "stb_image.h"
#include <iostream>

#include <glm/gtx/quaternion.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <set>
#include <queue>
#include <algorithm>

#include "Logger.h"
#include "utils/utils.h"

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
 * @return Image or Nothing
 */
std::optional<SerialTypes::Model::Image> loadImage(fastgltf::Asset& asset, fastgltf::Image& image){
    SerialTypes::Model::Image newImage;
    newImage.name = image.name.c_str();

    int width, height, channels;
    std::visit(fastgltf::visitor{
        [](auto& arg){},
        [&newImage, &width, &height, &channels](fastgltf::sources::URI& filePath){
            assert(filePath.fileByteOffset == 0);
            assert(filePath.uri.isLocalPath());

            const std::string path(filePath.uri.path().begin(), filePath.uri.path().end());
            unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
            if(data){
                newImage.extent.width = width;
                newImage.extent.height = height;
                newImage.extent.depth = 1;
                std::size_t imgSize = width * height * 4;
                newImage.data = SerialTypes::Span<uint32_t,std::byte,true>(reinterpret_cast<std::byte*>(data),imgSize);
            }
        },
        [&newImage, &width, &height, &channels](fastgltf::sources::Vector& vector){
            unsigned char* data = stbi_load_from_memory(vector.bytes.data(), static_cast<int>(vector.bytes.size()),
                                                        &width, &height, &channels, 4);
            if(data){
                newImage.extent.width = width;
                newImage.extent.height = height;
                newImage.extent.depth = 1;
                std::size_t imgSize = width * height * 4;
                newImage.data = SerialTypes::Span<uint32_t,std::byte,true>(reinterpret_cast<std::byte*>(data),imgSize);
            }
        },
        [&newImage, &width, &height, &channels, &asset](fastgltf::sources::BufferView& view){
            auto& bufferView = asset.bufferViews[view.bufferViewIndex];
            auto& buffer = asset.buffers[bufferView.bufferIndex];

            std::visit(fastgltf::visitor{
                [](auto& arg){},
                [&newImage, &width, &height, &channels, &bufferView](fastgltf::sources::Vector& vector){
                    unsigned char* data = stbi_load_from_memory(vector.bytes.data() + bufferView.byteOffset,
                                                                static_cast<int>(bufferView.byteLength),
                                                                &width, &height, &channels, 4);
                    if(data) {
                        newImage.extent.width = width;
                        newImage.extent.height = height;
                        newImage.extent.depth = 1;
                        std::size_t imgSize = width * height * 4;
                        newImage.data = SerialTypes::Span<uint32_t,std::byte,true>(reinterpret_cast<std::byte*>(data),imgSize);
                    }
                }
            },buffer.data);
        }
    }, image.data);

    if(newImage.data.empty()){
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

void loadSamplers(fastgltf::Asset& gltf, gltf2tec::GLTFResources& file, const std::filesystem::path& path){
    for(fastgltf::Sampler& sampler : gltf.samplers) {
        VkSamplerCreateInfo samplerCreateInfo = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};
        samplerCreateInfo.maxLod = VK_LOD_CLAMP_NONE;
        samplerCreateInfo.minLod = 0;

        samplerCreateInfo.magFilter = extractFilter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
        samplerCreateInfo.minFilter = extractFilter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

        samplerCreateInfo.mipmapMode = extractMipmapMode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

        file.samplers.push_back(samplerCreateInfo);
        vktLoaderLogger(Logger::DEBUG) << path << " Loaded sampler\n";
    }
}

void loadImages(fastgltf::Asset& gltf, gltf2tec::GLTFResources& file, const std::filesystem::path& path){
    for(fastgltf::Image& image : gltf.images){

        std::optional<SerialTypes::Model::Image> img = loadImage(gltf, image);

        if(img.has_value()){
            file.images.push_back(std::make_unique<SerialTypes::Model::Image>(*img));
            vktLoaderLogger(Logger::DEBUG) << path << " Loaded texture with name [" << image.name.c_str() << "] at ID " << file.images.size()-1 << '\n';
        }else{
            file.images.push_back(nullptr);
            vktLoaderLogger(Logger::ERROR) << "Failed to load texture [" << image.name.c_str() << "]\n";
        }
    }
}

void loadMaterials(fastgltf::Asset& gltf, gltf2tec::GLTFResources& file, const std::filesystem::path& path) {
    uint32_t dataIndex = 0;

    for(fastgltf::Material& mat : gltf.materials){
        file.materials.push_back(std::make_unique<SerialTypes::Model::GLTFMaterial>());
        SerialTypes::Model::GLTFMaterial* newMat = file.materials.back().get();

        VktTypes::GLTFMetallicRoughness::MaterialConstants& materialConstants = newMat->constants;
        materialConstants.colorFactors.x = mat.pbrData.baseColorFactor[0];
        materialConstants.colorFactors.y = mat.pbrData.baseColorFactor[1];
        materialConstants.colorFactors.z = mat.pbrData.baseColorFactor[2];
        materialConstants.colorFactors.w = mat.pbrData.baseColorFactor[3];

        materialConstants.metalRoughFactors.x = mat.pbrData.metallicFactor;
        materialConstants.metalRoughFactors.y = mat.pbrData.roughnessFactor;

        VktTypes::MaterialPass passType = VktTypes::MaterialPass::OPAQUE;
        if(mat.alphaMode == fastgltf::AlphaMode::Blend){
            passType = VktTypes::MaterialPass::TRANSPARENT;
        }

        SerialTypes::Model::MaterialResources materialResources;

        materialResources.colorImage        = SerialTypes::Model::NULL_ID;
        materialResources.colorSampler      = SerialTypes::Model::NULL_ID;
        materialResources.metalRoughImage   = SerialTypes::Model::NULL_ID;
        materialResources.metalRoughSampler = SerialTypes::Model::NULL_ID;

        if(mat.pbrData.baseColorTexture.has_value()){
            if(gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.has_value()) {
                materialResources.colorImage = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
            }
            if(gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.has_value()){
                materialResources.colorSampler = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();
            }
        }
        if(mat.pbrData.metallicRoughnessTexture.has_value()) {
            if(gltf.textures[mat.pbrData.metallicRoughnessTexture.value().textureIndex].imageIndex.has_value()) {
                materialResources.metalRoughImage = gltf.textures[mat.pbrData.metallicRoughnessTexture.value().textureIndex].imageIndex.value();
            }
            if(gltf.textures[mat.pbrData.metallicRoughnessTexture.value().textureIndex].samplerIndex.has_value()){
                materialResources.metalRoughSampler = gltf.textures[mat.pbrData.metallicRoughnessTexture.value().textureIndex].samplerIndex.value();
            }
        }
        newMat->resources = materialResources;
        newMat->constants = materialConstants;
        newMat->pass = passType;
        dataIndex++;

        vktLoaderLogger(Logger::DEBUG) << path << " Loaded material [" << mat.name.c_str() << "] at ID " << file.materials.size()-1 << '\n';
    }
}

void loadMeshes(fastgltf::Asset& gltf, gltf2tec::GLTFResources& file, const std::filesystem::path& path) {
    for(fastgltf::Mesh& mesh : gltf.meshes){
        std::get<gltf2tec::GLTFResources::StaticMeshVec_t>(file.meshes).push_back(std::make_unique<SerialTypes::Model::MeshAsset<VktTypes::Static>>());
        SerialTypes::Model::MeshAsset<VktTypes::Static>* newMesh = std::get<gltf2tec::GLTFResources::StaticMeshVec_t>(file.meshes).back().get();
        std::size_t initialVtx = 0;

        newMesh->surfaces.resize(mesh.primitives.size());
        for(std::size_t surfID = 0; surfID < mesh.primitives.size(); surfID++){
            const fastgltf::Primitive* p = &mesh.primitives[surfID];
            SerialTypes::Model::MeshSurface* newSurface = &newMesh->surfaces[surfID];
            std::size_t indicesOffset = newMesh->indices.size();
            std::size_t verticesOffset = newMesh->vertices.size();
            initialVtx = newMesh->vertices.size();
            newSurface->startIndex = indicesOffset;
            newSurface->count = gltf.accessors[p->indicesAccessor.value()].count;

            // Load indices
            {
                fastgltf::Accessor& indexAccessor = gltf.accessors[p->indicesAccessor.value()];
                newMesh->indices.resize(newMesh->indices.size() + indexAccessor.count);

                fastgltf::iterateAccessorWithIndex<uint32_t>(gltf, indexAccessor,
                                                         [&](uint32_t idx, std::size_t index){
                                                             newMesh->indices[indicesOffset + index] = initialVtx + idx;
                                                         });
            }

            // Load positions
            {
                fastgltf::Accessor& posAccessor = gltf.accessors[p->findAttribute("POSITION")->second];
                newMesh->vertices.resize(newMesh->vertices.size() + posAccessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
                                                              [&](glm::vec3 v, size_t index){
                                                                  VktTypes::Vertex<VktTypes::Static> vtx;
                                                                  vtx.position = v;
                                                                  newMesh->vertices[verticesOffset + index] = vtx;
                                                              });
            }

            // Load normals
            {
                auto normals = p->findAttribute("NORMAL");
                if (normals != p->attributes.end()) {
                    fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[normals->second],
                                                                  [&](glm::vec3 v, size_t index) {
                                                                      newMesh->vertices[verticesOffset + index].normal = v;
                                                                  });
                }
            }

            // Load UV coords
            {
                auto uv = p->findAttribute("TEXCOORD_0");
                if (uv != p->attributes.end()) {
                    fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[uv->second],
                                                                  [&](glm::vec2 v, size_t index) {
                                                                      newMesh->vertices[verticesOffset + index].uvX = v.x;
                                                                      newMesh->vertices[verticesOffset + index].uvY = v.y;
                                                                  });
                }
            }

            // Load colors
            {
                auto colors = p->findAttribute("COLOR_0");
                if (colors != p->attributes.end()) {
                    fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[colors->second],
                                                                  [&](glm::vec4 v, size_t index) {
                                                                      newMesh->vertices[verticesOffset + index].color = v;
                                                                  });
                }
            }

            if(p->materialIndex.has_value()){
                newSurface->materialIndex = p->materialIndex.value();
            }else{
                newSurface->materialIndex = SerialTypes::Model::NULL_ID;
            }

            vktLoaderLogger(Logger::DEBUG) << path << " Loaded mesh surface with " << newMesh->indices.size()-indicesOffset << " indices and " << newMesh->vertices.size()-verticesOffset << " vertices\n";
        }

        vktLoaderLogger(Logger::DEBUG) << path << " Loaded mesh with name " << mesh.name.c_str() << " at ID " << std::get<gltf2tec::GLTFResources::StaticMeshVec_t>(file.meshes).size()-1 << '\n';
    }
}

void loadSkinnedMeshes(fastgltf::Asset& gltf, gltf2tec::GLTFResources& file, const std::filesystem::path& path) {
    for(fastgltf::Mesh& mesh : gltf.meshes){
        std::get<gltf2tec::GLTFResources::SkinnedMeshVec_t>(file.meshes).push_back(std::make_unique<SerialTypes::Model::MeshAsset<VktTypes::Skinned>>());
        SerialTypes::Model::MeshAsset<VktTypes::Skinned>* newMesh = std::get<gltf2tec::GLTFResources::SkinnedMeshVec_t>(file.meshes).back().get();
        std::size_t initialVtx = 0;

        newMesh->surfaces.resize(mesh.primitives.size());
        for(std::size_t surfID = 0; surfID < mesh.primitives.size(); surfID++){
            const fastgltf::Primitive* p = &mesh.primitives[surfID];
            SerialTypes::Model::MeshSurface* newSurface = &newMesh->surfaces[surfID];
            std::size_t indicesOffset = newMesh->indices.size();
            std::size_t verticesOffset = newMesh->vertices.size();
            initialVtx = newMesh->vertices.size();
            newSurface->startIndex = indicesOffset;
            newSurface->count = gltf.accessors[p->indicesAccessor.value()].count;

            // Load indices
            {
                fastgltf::Accessor& indexAccessor = gltf.accessors[p->indicesAccessor.value()];
                newMesh->indices.resize(newMesh->indices.size() + indexAccessor.count);

                fastgltf::iterateAccessorWithIndex<uint32_t>(gltf, indexAccessor,
                                                             [&](uint32_t idx, std::size_t index){
                                                                 newMesh->indices[indicesOffset + index] = initialVtx + idx;
                                                             });
            }

            // Load positions
            {
                fastgltf::Accessor& posAccessor = gltf.accessors[p->findAttribute("POSITION")->second];
                newMesh->vertices.resize(newMesh->vertices.size() + posAccessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
                                                              [&](glm::vec3 v, size_t index){
                                                                  VktTypes::Vertex vtx;
                                                                  vtx.position = v;
                                                                  newMesh->vertices[verticesOffset + index] = vtx;
                                                              });
            }

            // Load normals
            {
                auto normals = p->findAttribute("NORMAL");
                if (normals != p->attributes.end()) {
                    fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[normals->second],
                                                                  [&](glm::vec3 v, size_t index) {
                                                                      newMesh->vertices[verticesOffset + index].normal = v;
                                                                  });
                }
            }

            // Load UV coords
            {
                auto uv = p->findAttribute("TEXCOORD_0");
                if (uv != p->attributes.end()) {
                    fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[uv->second],
                                                                  [&](glm::vec2 v, size_t index) {
                                                                      newMesh->vertices[verticesOffset + index].uvX = v.x;
                                                                      newMesh->vertices[verticesOffset + index].uvY = v.y;
                                                                  });
                }
            }

            // Load colors
            {
                auto colors = p->findAttribute("COLOR_0");
                if (colors != p->attributes.end()) {
                    fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[colors->second],
                                                                  [&](glm::vec4 v, size_t index) {
                                                                      newMesh->vertices[verticesOffset + index].color = v;
                                                                  });
                }
            }

            // Load joint indices
            {
                auto joints = p->findAttribute("JOINTS_0");
                if(joints != p->attributes.end()){
                    fastgltf::iterateAccessorWithIndex<glm::uvec4>(gltf, gltf.accessors[joints->second],
                                                                   [&](const glm::uvec4& v, size_t index){
                                                                       newMesh->vertices[verticesOffset + index].jointIndices = v;
                                                                   });

                }
            }

            // Load joint weights
            {
                auto weights = p->findAttribute("WEIGHTS_0");
                if(weights != p->attributes.end()){
                    fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[weights->second],
                                                                  [&](glm::vec4 v, size_t index){
                                                                      newMesh->vertices[verticesOffset + index].jointWeights = v;
                                                                  });

                }
            }

            if(p->materialIndex.has_value()){
                newSurface->materialIndex = p->materialIndex.value();
            }else{
                newSurface->materialIndex = SerialTypes::Model::NULL_ID;
            }

            vktLoaderLogger(Logger::DEBUG) << path << " Loaded mesh surface with " << newMesh->indices.size()-indicesOffset << " indices and " << newMesh->vertices.size()-verticesOffset << " vertices\n";
        }

        vktLoaderLogger(Logger::DEBUG) << path << " Loaded mesh with name " << mesh.name.c_str() << " at ID " << std::get<gltf2tec::GLTFResources::SkinnedMeshVec_t>(file.meshes).size()-1 << '\n';
    }
}


void loadNodes(fastgltf::Asset& gltf, gltf2tec::GLTFResources& file, const std::filesystem::path& path) {
    for(fastgltf::Node& node : gltf.nodes){
        file.nodes.push_back(std::make_unique<SerialTypes::Model::Node>());
        SerialTypes::Model::Node* newNode = file.nodes.back().get();
        newNode->name = node.name.c_str();

        if(node.meshIndex.has_value()){
            newNode->mesh = node.meshIndex.value();
        }

        std::visit(fastgltf::visitor{
                [&](const fastgltf::Node::TransformMatrix& matrix){
                    newNode->localTransform = glm::make_mat4(matrix.data());
                },
                [&](const fastgltf::TRS& transform){
                    newNode->translation = glm::make_vec3(transform.translation.data());
                    newNode->rotation = glm::mat4(glm::make_quat(transform.rotation.data()));
                    newNode->scale = glm::make_vec3(transform.scale.data());
                }},node.transform);

        vktLoaderLogger(Logger::DEBUG) << path << " Loaded node with name " << node.name.c_str() << " at ID " << file.nodes.size()-1 << '\n';
    }
}

void loadSkin(fastgltf::Asset& gltf, gltf2tec::GLTFResources& file, const std::filesystem::path& path){

    if(gltf.skins.empty()) return;
    if(gltf.skins.size() > 1){
        vktLoaderLogger(Logger::WARNING) << path << " Model has more than 1 skin, loading the first one\n";
    }
    fastgltf::Skin& skin = gltf.skins[0];
    file.skin = std::make_unique<SerialTypes::Model::Skin>();
    SerialTypes::Model::Skin* newSkin = file.skin.get();
    newSkin->name = skin.name.c_str();
    newSkin->skeletonRoot = skin.skeleton.value();
    std::vector<SerialTypes::Model::NodeID_t> joints(skin.joints.begin(), skin.joints.end());
    newSkin->joints = joints;

    if(skin.inverseBindMatrices.has_value()){
        const fastgltf::Accessor&   accessor    = gltf.accessors[skin.inverseBindMatrices.value()];
        const fastgltf::BufferView& bufferView  = gltf.bufferViews[accessor.bufferViewIndex.value()];
        const fastgltf::Buffer&     buffer      = gltf.buffers[bufferView.bufferIndex];
        newSkin->inverseBindMatrices.resize(accessor.count);
        std::visit(fastgltf::visitor{
            [&accessor, &bufferView, &newSkin](const fastgltf::sources::Vector& vector){
                std::memcpy(newSkin->inverseBindMatrices.data(), vector.bytes.data() + accessor.byteOffset + bufferView.byteOffset, accessor.count * sizeof(glm::mat4));
                },
                [&path](auto){vktLoaderLogger(Logger::WARNING) << path << " Unhandled variant type while loading model inverse bind matrices\n";}},
                buffer.data);
    }
}

/**
 * Connects skin pointer to relevant nodes
 */
void updateSkin(fastgltf::Asset& gltf, gltf2tec::GLTFResources& file, const std::filesystem::path& path) {
    std::vector<SerialTypes::Model::NodeID_t> skinNodesTmp;
    for(std::size_t i = 0; i < gltf.nodes.size(); i++) {
        fastgltf::Node& node = gltf.nodes[i];
        if(node.skinIndex.has_value()){
            assert(node.skinIndex == 0);            // Make sure there aren't pointers to different skins
            file.nodes[i]->skin = i;                // Store index to only the single skin
            skinNodesTmp.push_back(i);              // Store index of this node to the skin for faster joint calc
            vktLoaderLogger(Logger::DEBUG) << path << " Connected model skin [" << file.skin->name.data() << "] with node " << i << '\n';
        }
    }
    file.skin->skinNodes = std::move(SerialTypes::Span<uint32_t,SerialTypes::Model::NodeID_t,true>(skinNodesTmp));
}

void loadAnimations(fastgltf::Asset& gltf, gltf2tec::GLTFResources& file, const std::filesystem::path& path) {
    for(const fastgltf::Animation& animation : gltf.animations){
        file.animations.push_back(std::make_unique<SerialTypes::Model::Animation>());
        SerialTypes::Model::Animation* newAnim = file.animations.back().get();
        newAnim->name = animation.name.c_str();

        newAnim->samplers.resize(animation.samplers.size());
        for(std::size_t samplerID = 0; samplerID < animation.samplers.size(); samplerID++){
            const auto& sampler = animation.samplers[samplerID];
            auto& newSampler = newAnim->samplers[samplerID];

            switch(sampler.interpolation){
                case fastgltf::AnimationInterpolation::Linear:
                    newSampler.interpolation = SerialTypes::Model::Interpolation::LINEAR;
                    break;
                case fastgltf::AnimationInterpolation::Step:
                    newSampler.interpolation = SerialTypes::Model::Interpolation::STEP;
                    break;
                case fastgltf::AnimationInterpolation::CubicSpline:
                    newSampler.interpolation = SerialTypes::Model::Interpolation::CUBICSPLINE;
                    break;
            }

            {
                const fastgltf::Accessor &accessor      = gltf.accessors[sampler.inputAccessor];
                const fastgltf::BufferView &bufferView  = gltf.bufferViews[accessor.bufferViewIndex.value()];
                const fastgltf::Buffer &buffer          = gltf.buffers[bufferView.bufferIndex];

                newSampler.inputs.resize(accessor.count);
                std::visit(fastgltf::visitor{
                        [&accessor, &bufferView, &newSampler](const fastgltf::sources::Vector& vector){
                            memcpy(newSampler.inputs.data(), vector.bytes.data() + accessor.byteOffset + bufferView.byteOffset, accessor.count * sizeof(float));
                        },
                        [&path](auto&){vktLoaderLogger(Logger::WARNING) << path << " Unhandled variant type while loading model animations input sampler\n";}
                }, buffer.data);

                for(float input : newSampler.inputs){
                    if(input < newAnim->start){
                        newAnim->start = input;
                    }
                    if(input > newAnim->end){
                        newAnim->end = input;
                    }
                }
            }

            {
                const fastgltf::Accessor &accessor      = gltf.accessors[sampler.outputAccessor];
                const fastgltf::BufferView &bufferView  = gltf.bufferViews[accessor.bufferViewIndex.value()];
                const fastgltf::Buffer &buffer          = gltf.buffers[bufferView.bufferIndex];

                newSampler.outputsVec4.resize(accessor.count);
                std::visit(fastgltf::visitor {
                        [&accessor, &bufferView, &newSampler, &path](const fastgltf::sources::Vector& vector){
                            const void* dataPtr = vector.bytes.data() + accessor.byteOffset + bufferView.byteOffset;

                            switch(accessor.type){
                                case fastgltf::AccessorType::Vec3:
                                {
                                    const glm::vec3 *data = static_cast<const glm::vec3*>(dataPtr);
                                    for (std::size_t i = 0; i < accessor.count; i++) {
                                        newSampler.outputsVec4[i] = glm::vec4(data[i], 0.0f);
                                    }
                                    break;
                                }
                                case fastgltf::AccessorType::Vec4:
                                {
                                    const glm::vec4 *data = static_cast<const glm::vec4*>(dataPtr);
                                    for (std::size_t i = 0; i < accessor.count; i++) {
                                        newSampler.outputsVec4[i] = glm::vec4(data[i]);
                                    }
                                    break;
                                }
                                default:
                                    vktLoaderLogger(Logger::ERROR) << path << " Invalid animation sampler type\n";
                                    break;
                            }
                        },
                        [&path](auto){vktLoaderLogger(Logger::WARNING) << path << " Unhandled variant type while loading model animation output sampler\n";}

                }, buffer.data);

            }
        }

        newAnim->channels.resize(animation.channels.size());
        std::vector<std::pair<uint32_t,uint32_t>> tmpAnimatedNodes;
        for(std::size_t channelID = 0; channelID < animation.channels.size(); channelID++){
            fastgltf::AnimationChannel channel = animation.channels[channelID];
            SerialTypes::Model::AnimationChannel& newChannel = newAnim->channels[channelID];
            switch(channel.path){
                case fastgltf::AnimationPath::Translation:
                    newChannel.translationSampler = channel.samplerIndex;
                    break;
                case fastgltf::AnimationPath::Rotation:
                    newChannel.rotationSampler = channel.samplerIndex;
                    break;
                case fastgltf::AnimationPath::Scale:
                    newChannel.scaleSampler = channel.samplerIndex;
                    break;
                case fastgltf::AnimationPath::Weights:
                    vktLoaderLogger(Logger::DEBUG) << path << " Ignored WEIGHTS channel\n";
                    break;
            }
            newChannel.node = static_cast<uint32_t>(channel.nodeIndex);
            tmpAnimatedNodes.emplace_back(channel.nodeIndex,channelID);
        }

        // Sort by BFS reordering so that top nodes are before their children
        std::sort(tmpAnimatedNodes.begin(), tmpAnimatedNodes.end(),
                  [&file](const std::pair<uint32_t,uint32_t>& l, const std::pair<uint32_t,uint32_t>& r){
            uint32_t lNodeIndex = l.first;
            uint32_t rNodeIndex = r.first;
            const SerialTypes::Model::Node* lNode = file.nodes[lNodeIndex].get();
            const SerialTypes::Model::Node* rNode = file.nodes[rNodeIndex].get();
            const SerialTypes::Model::Node* root = lNode;
            while(root->parent){
                root = file.nodes[root->parent].get();
            }
            std::queue<const SerialTypes::Model::Node*> q;
            q.push(root);
            while(!q.empty()){
                const SerialTypes::Model::Node* frontNode = q.front();
                if(frontNode == lNode) return true;
                if(frontNode == rNode) return false;
                q.pop();
                for(SerialTypes::Model::NodeID_t nodeID = 0; nodeID < frontNode->children.size(); nodeID++){
                    q.push(file.nodes[frontNode->children[nodeID]].get());
                }
            }
            return false;
        });

        std::vector<std::pair<uint32_t,uint32_t>> finalAnimatedNodes;
        auto lastPair = *tmpAnimatedNodes.begin();
        tmpAnimatedNodes.erase(tmpAnimatedNodes.cbegin());
        for(const auto& [nodeIndex,channelIndex] : tmpAnimatedNodes){
            if(nodeIndex != lastPair.first){
                finalAnimatedNodes.push_back(lastPair);
                lastPair = {nodeIndex, channelIndex};
            }else{
                SerialTypes::Model::AnimationChannel* channel = &newAnim->channels[channelIndex];
                if(channel->scaleSampler != SerialTypes::Model::NULL_ID){
                    newAnim->channels[lastPair.second].scaleSampler = channel->scaleSampler;
                }
                if(channel->rotationSampler != SerialTypes::Model::NULL_ID){
                    newAnim->channels[lastPair.second].rotationSampler = channel->rotationSampler;
                }
                if(channel->translationSampler != SerialTypes::Model::NULL_ID){
                    newAnim->channels[lastPair.second].translationSampler = channel->translationSampler;
                }
            }
        }
        finalAnimatedNodes.push_back(lastPair);
        newAnim->animatedNodes = finalAnimatedNodes;

        vktLoaderLogger(Logger::DEBUG) << path << " Loaded animation [" << newAnim->name.data() << "]\n";
    }
}

void connectNodes(fastgltf::Asset& gltf, gltf2tec::GLTFResources& file, const std::filesystem::path& path){
    // Connect nodes to create trees
    for(uint32_t i = 0; i < gltf.nodes.size(); i++){
        fastgltf::Node& gltfNode = gltf.nodes[i];
        auto* node = file.nodes[i].get();

        node->children.resize(gltfNode.children.size());
        for(SerialTypes::Model::NodeID_t nodeID = 0; nodeID < gltfNode.children.size(); nodeID++){
            SerialTypes::Model::NodeID_t childNodeID = gltfNode.children[nodeID];
            node->children[nodeID] = childNodeID;
            file.nodes[childNodeID]->parent = i;

            vktLoaderLogger(Logger::DEBUG) << path
                                           << " Node with name " << file.nodes[childNodeID]->name.data()
                                           << " at ID " << childNodeID
                                           << " connected to parent with name " << node->name.data()
                                           << " at ID " << i << '\n';
        }
    }
}

void createTree(gltf2tec::GLTFResources& file, const std::filesystem::path& path) {
    // Store root nodes as reference
    for(std::size_t i = 0; i < file.nodes.size(); i++){
        auto* node = file.nodes[i].get();
        if(node->parent == SerialTypes::Model::NULL_ID){
            if(file.topNode != SerialTypes::Model::NULL_ID){
                vktLoaderLogger(Logger::WARNING) << path << " Found multiple root nodes | Current root at ID "
                                                 << file.topNode << " and another at ID " << i
                                                 << " | Ignoring the second one" << '\n';
                continue;
            }
            file.topNode = i;
            vktLoaderLogger(Logger::DEBUG) << path << " Node with name " << node->name.data() << " at ID " << i << " used as a root node" << '\n';
        }
    }
}

void updateWorldTransform(gltf2tec::GLTFResources& file, const std::filesystem::path& path){
    std::queue<std::pair<uint32_t, const glm::mat4>> q;
    q.emplace(file.topNode, glm::identity<glm::mat4>());

    while(!q.empty()) {
        auto [nID,t] = q.front();
        q.pop();
        auto* n = file.nodes[nID].get();

        n->worldTransform = t * glm::translate(glm::identity<glm::mat4>(), n->translation)
                              * glm::toMat4(n->rotation)
                              * glm::scale(glm::identity<glm::mat4>(), n->scale)
                              * n->localTransform;
        n->animationTransform = n->worldTransform;

        for(SerialTypes::Model::NodeID_t nodeID = 0; nodeID < n->children.size(); nodeID++){
            q.emplace(n->children[nodeID], n->worldTransform);
        }
    }
}

/**
 * @brief Loads a glTF file.
 * @param filePath Path to glTF file
 * @return GLTFResources or nullptr
 */
gltf2tec::GLTFResources* loadGltfModel(const std::filesystem::path& filePath){
    vktLoaderLogger(Logger::INFO) << "Loading GLTF: " << filePath << '\n';
    gltf2tec::GLTFResources* scene = new gltf2tec::GLTFResources();
    gltf2tec::GLTFResources& file = *scene;

    if(!exists(filePath)){
        vktLoaderLogger(Logger::ERROR) << "File " << filePath.string() << " doesn't exist\n";
        return nullptr;
    }

    std::optional<fastgltf::Asset> loadedGLTF = loadFile(filePath);
    if(!loadedGLTF.has_value()) return nullptr;

    fastgltf::Asset gltf = std::move(loadedGLTF.value());

    if(!gltf.skins.empty()){
        file.isSkinned = true;
    }

    // Check if the mesh is skinned and if so load the joints indices and weights as well
    if(file.isSkinned) {
        file.meshes = gltf2tec::GLTFResources::SkinnedMeshVec_t{};
        loadSkinnedMeshes(gltf, file, filePath);
    }else{
        file.meshes = gltf2tec::GLTFResources::StaticMeshVec_t{};
        loadMeshes(gltf, file, filePath);
    }

    loadSamplers(gltf,file,filePath);
    loadImages(gltf,file,filePath);
    loadMaterials(gltf,file,filePath);
    loadNodes(gltf,file,filePath);
    connectNodes(gltf,file,filePath);
    createTree(file,filePath);
    updateWorldTransform(file, filePath);
    if(file.isSkinned) {
        loadSkin(gltf, file, filePath);
        updateSkin(gltf, file, filePath);
        loadAnimations(gltf, file, filePath);
    }

    return scene;
}

template<bool S>
void writeMesh(SerialTypes::BinDataVec_t& data, const SerialTypes::Model::MeshAsset<S>& mesh){
    // Write material surface span size + vector [32bits + sizeof(MeshSurface) * size]
    Serial::pushData<SerialTypes::Model::MeshSurface>(data, mesh.surfaces);

    // Write indices size + vector [32bits + 32bits * size]
    Serial::pushData<uint32_t>(data, mesh.indices);

    // Write vertices size + vector [32bits + sizeof(Vertex) * size]
    Serial::pushData<VktTypes::Vertex<S>>(data, mesh.vertices);

}

void writeImage(SerialTypes::BinDataVec_t& data, const SerialTypes::Model::Image& image){
    // Write image name size + data [32bits + 8bits * size]
    Serial::pushData<char>(data, image.name);

    // Write image extent data [sizeof(VkExtent3D)]
    Serial::pushData<VkExtent3D>(data, image.extent);

    // Write image format [sizeof(VkFormat)]
    Serial::pushData<VkFormat>(data, image.format);

    // Write image data [8bits * size]
    // Size can be calculated from extent
    Serial::pushData<std::byte>(data, image.data);
}

void writeNode(SerialTypes::BinDataVec_t& data, const SerialTypes::Model::Node& node){
    // Write node name size + data [32bits + 8bits * size]
    Serial::pushData<char>(data, node.name);

    // Write node parent index [32bits]
    Serial::pushData<uint32_t>(data, node.parent);

    // Write node children size + data [32bits + 32bits * size]
    Serial::pushData<uint32_t>(data, node.children);

    // Write node mesh index [32bits]
    Serial::pushData<uint32_t>(data, node.mesh);

    // Write node local transform matrix [sizeof(glm::mat4)]
    Serial::pushData<glm::mat4>(data, node.localTransform);

    // Write node world transform matrix [sizeof(glm::mat4)]
    Serial::pushData<glm::mat4>(data, node.worldTransform);

    // Write node animation transform matrix [sizeof(glm::mat4)]
    Serial::pushData<glm::mat4>(data, node.animationTransform);

    // Write node translation transformation [sizeof(glm::vec3)]
    Serial::pushData<glm::vec3>(data, node.translation);

    // Write node scale transformation [sizeof(glm::vec3)]
    Serial::pushData<glm::vec3>(data, node.scale);

    // Write node rotation transformation [sizeof(glm::quat)]
    Serial::pushData<glm::quat>(data, node.rotation);

    // Write node skin index [32bits]
    Serial::pushData<uint32_t>(data, node.skin);
}

void writeMaterial(SerialTypes::BinDataVec_t& data, const SerialTypes::Model::GLTFMaterial& material){
    // Write material resources [sizeof(MaterialResources)]
    Serial::pushData<SerialTypes::Model::MaterialResources>(data, material.resources);

    // Write material constants [sizeof(MaterialConstants)]
    Serial::pushData<VktTypes::GLTFMetallicRoughness::MaterialConstants>(data, material.constants);

    // Write material pass type [8bits]
    Serial::pushData<VktTypes::MaterialPass>(data, material.pass);
}

void writeSkin(SerialTypes::BinDataVec_t& data, const SerialTypes::Model::Skin& skin){
    // Write skin name size + data [32bits + 8bits * size]
    Serial::pushData<char>(data, skin.name);

    // Write skin skeleton root node index [32bits]
    Serial::pushData<uint32_t>(data, skin.skeletonRoot);

    // Write skin node indices vector size + data [32bits + 32bits * size]
    Serial::pushData<uint32_t>(data, skin.skinNodes);

    // Write skin inverse bind matrices vector size + data [32bits + sizeof(glm::mat4) * size]
    Serial::pushData<glm::mat4>(data, skin.inverseBindMatrices);

    // Write skin joints node indices vector size + data [32bits + 32bits * size]
    Serial::pushData<uint32_t>(data, skin.joints);
}

void writeAnimationSampler(SerialTypes::BinDataVec_t& data, const SerialTypes::Model::AnimationSampler& sampler){
    // Write sampler interpolation [8bits]
    Serial::pushData<SerialTypes::Model::Interpolation>(data, sampler.interpolation);

    // Write sampler inputs vector size + data [32bits + sizeof(float) * size]
    Serial::pushData<float>(data, sampler.inputs);

    // Write sampler output vec4 vector size + data [32bits + sizeof(glm::vec4) * size]
    Serial::pushData<glm::vec4>(data, sampler.outputsVec4);
}

void writeAnimation(SerialTypes::BinDataVec_t& data, const SerialTypes::Model::Animation& animation){
    // Write animation name size + data [32bits + 8bits * size]
    Serial::pushData(data, animation.name);

    // Write animation start time [sizeof(float)]
    Serial::pushData(data, animation.start);

    // Write animation end time [sizeof(float)]
    Serial::pushData(data, animation.end);

    // Write animation samplers vector size + data [32bits + sizeof(AnimationSampler) * size]
    Serial::pushData<uint32_t>(data, animation.samplers.size());
    for(const auto& sampler : animation.samplers){
        writeAnimationSampler(data, sampler);
    }

    // Write animation channel vector size + data [32bits + sizeof(AnimationChannel) * size]
    Serial::pushData<SerialTypes::Model::AnimationChannel>(data, animation.channels);

    // Write animated nodes vector size + data [32bits + 64bits * size]
    Serial::pushData<std::pair<uint32_t, uint32_t>>(data, animation.animatedNodes);
}


gltf2tec::TectonicResources convertGLTFModel(const gltf2tec::GLTFResources& gltfResources) {
    gltf2tec::TectonicResources tecResources;
    SerialTypes::BinDataVec_t& data = tecResources.data;
    std::size_t index;

    // Write version for compatibility reasons [8bits]
    data.push_back(std::byte{GLTF2TEC_VERSION});

    uint8_t metaByte = 0;
    if(gltfResources.isSkinned){
        metaByte |= Utils::enumVal(SerialTypes::Model::MetaBits::SKINNED);  // 1 if the asset is skinned
    }

    // Write meta info bits about asset
    data.push_back(std::byte{metaByte});

    // Reserve pointer space
    Serial::pushData<uint32_t>(data, 0);   // Meshes [32bits]
    Serial::pushData<uint32_t>(data, 0);   // Images [32bits]
    Serial::pushData<uint32_t>(data, 0);   // Samplers [32bits]
    Serial::pushData<uint32_t>(data, 0);   // Nodes [32bits]
    Serial::pushData<uint32_t>(data, 0);   // Materials [32bits]
    if(gltfResources.isSkinned) {
        Serial::pushData<uint32_t>(data, 0);   // Skin [32bits]
        Serial::pushData<uint32_t>(data, 0);   // Animations [32bits]
    }

    // Write index to mesh data
    index = data.size();
    Serial::pushData<uint32_t>(data, index, SerialTypes::Model::MESHES_INDEX);

    // Write meshes size + vector [32bits + sizeof(MeshAsset) * size]
    std::visit([&data](auto &&meshVec){
        Serial::pushData<uint32_t>(data, meshVec.size());
        for(const auto& mesh : meshVec){
            writeMesh(data, *mesh);
        }
    }, gltfResources.meshes);

    // Write index to image data
    index = data.size();
    Serial::pushData<uint32_t>(data, index, SerialTypes::Model::IMAGES_INDEX);

    // Write images size + vector [32bits + sizeof(Image) * size]
    Serial::pushData<uint32_t>(data, gltfResources.images.size());
    for(const auto& img : gltfResources.images){
        writeImage(data, *img);
    }

    // Write index to samplers data
    index = data.size();
    Serial::pushData<uint32_t>(data, index, SerialTypes::Model::SAMPLERS_INDEX);

    // Write samplers size + vector [32bits + sizeof(VkSamplerCreateInfo) * size]
    Serial::pushData<uint32_t>(data, gltfResources.samplers.size());
    for(const auto& sampler : gltfResources.samplers){
        Serial::pushData<VkSamplerCreateInfo>(data, sampler);
    }

    // Write index to nodes data
    index = data.size();
    Serial::pushData<uint32_t>(data, index, SerialTypes::Model::NODES_INDEX);

    // Writes root node index [32bits]
    Serial::pushData<uint32_t>(data, gltfResources.topNode);

    // Writes nodes size + vector [32bits + sizeof(Node) * size]
    Serial::pushData<uint32_t>(data, gltfResources.nodes.size());
    for(const auto& node : gltfResources.nodes){
        writeNode(data, *node);
    }

    // Write index to material data
    index = data.size();
    Serial::pushData<uint32_t>(data, index, SerialTypes::Model::MATERIALS_INDEX);

    // Write material size + vector [32bits + sizeof(GLTFMaterial) * size]
    Serial::pushData<uint32_t>(data, gltfResources.materials.size());
    for(const auto& material : gltfResources.materials){
        writeMaterial(data, *material);
    }

    if(gltfResources.isSkinned) {
        // Write index to skin data
        index = data.size();
        Serial::pushData<uint32_t>(data, index, SerialTypes::Model::SKIN_INDEX);

        // Write skin [sizeof(Skin)]
        writeSkin(data, *gltfResources.skin);

        // Write index to animation data
        index = data.size();
        Serial::pushData<uint32_t>(data, index, SerialTypes::Model::ANIMATION_INDEX);

        // Write animation size + vector [32bits + sizeof(Animation) * size]
        Serial::pushData<uint32_t>(data, gltfResources.animations.size());
        for (const auto &animation: gltfResources.animations) {
            writeAnimation(data, *animation);
        }
    }

    return tecResources;
}

int main(){
    std::filesystem::path bottle{"meshes/WaterBottle.glb"};
    std::filesystem::path outBottle{"meshes/WaterBottle.tecm"};
    std::filesystem::path shrek{"meshes/shrek.glb"};
    std::filesystem::path outShrek{"meshes/shrek.tecm"};
    gltf2tec::GLTFResources* resources = loadGltfModel(bottle);
    if(resources) {
        gltf2tec::TectonicResources tecResources = convertGLTFModel(*resources);
        std::ofstream outFile(outBottle, std::ios::out | std::ios::binary);
        outFile.write(reinterpret_cast<const char *>(&tecResources.data[0]), tecResources.data.size());
    }
    delete(resources);

    resources = loadGltfModel(shrek);
    if(resources) {
        gltf2tec::TectonicResources tecResources = convertGLTFModel(*resources);
        std::ofstream outFile(outShrek, std::ios::out | std::ios::binary);
        outFile.write(reinterpret_cast<const char *>(&tecResources.data[0]), tecResources.data.size());
    }

    delete(resources);
}