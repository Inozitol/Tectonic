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
#include <glm/gtc/type_ptr.hpp>

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

void loadSamplers(fastgltf::Asset& gltf, VktModelResources& file, const std::filesystem::path& path){
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

void loadImages(fastgltf::Asset& gltf, VktModelResources& file, const std::filesystem::path& path){
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

void loadMaterials(fastgltf::Asset& gltf, VktModelResources& file, const std::filesystem::path& path) {
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

void loadMeshes(fastgltf::Asset& gltf, VktModelResources& file, const std::filesystem::path& path) {
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

            // Load joint indices
            {
                auto joints = p.findAttribute("JOINTS_0");
                if(joints != p.attributes.end()){
                    fastgltf::iterateAccessorWithIndex<glm::u16vec4>(gltf, gltf.accessors[joints->second],
                                                                  [&](const glm::u16vec4& v, size_t index){
                                                                      vertices[initialVtx + index].jointIndices = v;
                                                                  });

                }
            }

            // Load joint weights
            {
                auto weights = p.findAttribute("WEIGHTS_0");
                if(weights != p.attributes.end()){
                    fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[weights->second],
                                                                  [&](glm::vec4 v, size_t index){
                                                                      vertices[initialVtx + index].jointWeights = v;
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

void loadNodes(fastgltf::Asset& gltf, VktModelResources& file, const std::filesystem::path& path) {
    for(fastgltf::Node& node : gltf.nodes){
        file.nodes.push_back(std::make_unique<VktTypes::Node>());
        file.namedNodes[node.name.c_str()] = file.nodes.back().get();
        VktTypes::Node* newNode = file.nodes.back().get();
        newNode->name = node.name;

        if(node.meshIndex.has_value()){
            newNode->mesh = file.meshes[*node.meshIndex].get();
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

void loadSkins(fastgltf::Asset& gltf, VktModelResources& file, const std::filesystem::path& path){
    std::size_t index = 0;
    for(const fastgltf::Skin& skin : gltf.skins){
        file.skins.push_back(std::make_unique<VktTypes::Skin>());
        file.namedSkins[skin.name.c_str()] = file.skins.back().get();
        VktTypes::Skin* newSkin = file.skins.back().get();
        newSkin->name = skin.name;

        newSkin->skeletonRoot = file.nodes[skin.skeleton.value()].get();
        newSkin->index = index++;

        for(auto jointIndex : skin.joints){
            VktTypes::Node* node = file.nodes[jointIndex].get();
            if(node) {
                newSkin->joints.push_back(node);
            }
        }

        if(skin.inverseBindMatrices.has_value()){
            const fastgltf::Accessor&   accessor    = gltf.accessors[skin.inverseBindMatrices.value()];
            const fastgltf::BufferView& bufferView  = gltf.bufferViews[accessor.bufferViewIndex.value()];
            const fastgltf::Buffer&     buffer      = gltf.buffers[bufferView.bufferIndex];
            newSkin->inverseBindMatrices.resize(accessor.count);
            std::visit(fastgltf::visitor{
                    [&accessor, &bufferView, &newSkin](const fastgltf::sources::Vector& vector){
                        memcpy(newSkin->inverseBindMatrices.data(), vector.bytes.data() + accessor.byteOffset + bufferView.byteOffset, accessor.count * sizeof(glm::mat4));
                    },
                    [&path](auto){vktLoaderLogger(Logger::WARNING) << path << " Unhandled variant type while loading model inverse bind matrices\n";}
            }, buffer.data);
            //newSkin->jointsBuffer = VktCore::uploadJoints(newSkin->inverseBindMatrices);
        }
    }
    file.isSkinned = !file.skins.empty();
}

/**
 * Connects skin pointers to relevant nodes
 */
void updateSkins(fastgltf::Asset& gltf, VktModelResources& file, const std::filesystem::path& path) {
    for(std::size_t i = 0; i < gltf.nodes.size(); i++) {
        fastgltf::Node& node = gltf.nodes[i];
        if(node.skinIndex.has_value()){
            file.nodes[i]->skin = file.skins[node.skinIndex.value()].get();
        }
    }
}

void loadAnimations(fastgltf::Asset& gltf, VktModelResources& file, const std::filesystem::path& path) {
    for(const fastgltf::Animation& animation : gltf.animations){
        file.animations.push_back(std::make_unique<VktTypes::Animation>());
        file.namedAnimations[animation.name.c_str()] = file.animations.back().get();
        VktTypes::Animation* newAnim = file.animations.back().get();
        newAnim->name = animation.name;

        newAnim->samplers.reserve(animation.samplers.size());
        for(const auto& sampler : animation.samplers){
            newAnim->samplers.emplace_back();
            VktTypes::AnimationSampler& newSampler = newAnim->samplers.back();

            switch(sampler.interpolation){
                case fastgltf::AnimationInterpolation::Linear:
                    newSampler.interpolation = VktTypes::AnimationSampler::Interpolation::LINEAR;
                    break;
                case fastgltf::AnimationInterpolation::Step:
                    newSampler.interpolation = VktTypes::AnimationSampler::Interpolation::STEP;
                    break;
                case fastgltf::AnimationInterpolation::CubicSpline:
                    newSampler.interpolation = VktTypes::AnimationSampler::Interpolation::CUBICSPLINE;
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

                for(auto input : newSampler.inputs){
                    if(input < newAnim->start){
                        newAnim->start = input;
                    }
                    if(input > newAnim->end){
                        newAnim->end = input;
                    }
                }
            }

            {
                const fastgltf::Accessor &accessor = gltf.accessors[sampler.outputAccessor];
                const fastgltf::BufferView &bufferView = gltf.bufferViews[accessor.bufferViewIndex.value()];
                const fastgltf::Buffer &buffer = gltf.buffers[bufferView.bufferIndex];
                std::visit(fastgltf::visitor {
                        [&accessor, &bufferView, &newSampler, &path](const fastgltf::sources::Vector& vector){
                            const void* dataPtr = vector.bytes.data() + accessor.byteOffset + bufferView.byteOffset;

                            switch(accessor.type){
                                case fastgltf::AccessorType::Vec3:
                                {
                                    const glm::vec3 *data = static_cast<const glm::vec3*>(dataPtr);
                                    for (std::size_t i = 0; i < accessor.count; i++) {
                                        newSampler.outputsVec4.emplace_back(data[i], 0.0f);
                                    }
                                    break;
                                }
                                case fastgltf::AccessorType::Vec4:
                                {
                                    const glm::vec4 *data = static_cast<const glm::vec4*>(dataPtr);
                                    for (std::size_t i = 0; i < accessor.count; i++) {
                                        newSampler.outputsVec4.emplace_back(data[i]);
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

        newAnim->channels.reserve(animation.channels.size());
        for(const auto& channel : animation.channels) {
            newAnim->channels.emplace_back();
            VktTypes::AnimationChannel &newChannel = newAnim->channels.back();
            switch(channel.path){
                case fastgltf::AnimationPath::Translation:
                    newChannel.path = VktTypes::AnimationChannel::Path::TRANSLATION;
                    break;
                case fastgltf::AnimationPath::Rotation:
                    newChannel.path = VktTypes::AnimationChannel::Path::ROTATION;
                    break;
                case fastgltf::AnimationPath::Scale:
                    newChannel.path = VktTypes::AnimationChannel::Path::SCALE;
                    break;
                case fastgltf::AnimationPath::Weights:
                    newChannel.path = VktTypes::AnimationChannel::Path::WEIGHTS;
                    break;
            }
            newChannel.samplerIndex = channel.samplerIndex;
            newChannel.node = file.nodes[channel.nodeIndex].get();
        }

        vktLoaderLogger(Logger::DEBUG) << path << " Loaded animation [" << newAnim->name << "]\n";
    }
}

void connectNodes(fastgltf::Asset& gltf, VktModelResources& file, const std::filesystem::path& path){
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

void createTrees(VktModelResources& file, const std::filesystem::path& path) {
    // Store root nodes as reference
    for(std::size_t i = 0; i < file.nodes.size(); i++){
        VktTypes::Node* node = file.nodes[i].get();
        if(node->parent == nullptr){
            file.topNodes.push_back(node);
            if(hasCycle(node, {})){
                vktLoaderLogger(Logger::ERROR) << path << " Root node with name " << node->name << " at ID " << i << " contains a cycle" << '\n';
            }
            vktLoaderLogger(Logger::DEBUG) << path << " Node with name " << node->name << " at ID " << i << " used as a root node" << '\n';
        }
    }
}

/**
 * @brief Loads a glTF file.
 * @param filePath Path to glTF file
 * @return VktModelResources or Nothing
 */
VktModelResources* loadGltfModel(const std::filesystem::path& filePath){
    vktLoaderLogger(Logger::INFO) << "Loading GLTF: " << filePath << '\n';
    VktModelResources* scene = new VktModelResources();
    VktModelResources& file = *scene;

    std::optional<fastgltf::Asset> loadedGLTF = loadFile(filePath);
    if(!loadedGLTF.has_value()) return nullptr;

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
    loadSkins(gltf,file,filePath);
    updateSkins(gltf,file,filePath);
    loadAnimations(gltf,file,filePath);

    connectNodes(gltf,file,filePath);
    createTrees(file,filePath);

    return scene;
}

void VktModelResources::gatherContext(const glm::mat4 &topMatrix, const std::vector<VktTypes::GPUJointsBuffers>& jointsBuffers, VktTypes::DrawContext &ctx) {
    for(auto& n : topNodes){
        VktTypes::Node::gatherContext(*n, topMatrix, jointsBuffers, ctx);
    }
}

void VktModelResources::clean() {
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

void VktModelResources::updateAnimation(VktTypes::Animation* animation, const std::vector<VktTypes::GPUJointsBuffers>& jointsBuffers, float delta) {
    animation->currTime += delta;
    if(animation->currTime > animation->end){
        animation->currTime -= animation->end;
    }

    for(auto& channel : animation->channels){
        VktTypes::AnimationSampler& sampler = animation->samplers[channel.samplerIndex];
        for(std::size_t i = 0; i < sampler.inputs.size() - 1; i++){
            if(sampler.interpolation != VktTypes::AnimationSampler::Interpolation::LINEAR){
                vktLoaderLogger(Logger::WARNING) << "Non-linear interpolation is not supported\n";
                continue;
            }

            if((animation->currTime >= sampler.inputs[i]) && (animation->currTime <= sampler.inputs[i+1])){
                float a = (animation->currTime - sampler.inputs[i]) / (sampler.inputs[i+1] - sampler.inputs[i]);
                switch(channel.path){
                    case VktTypes::AnimationChannel::Path::TRANSLATION:
                    {
                        channel.node->translation = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i+1], a);
                        break;
                    }
                    case VktTypes::AnimationChannel::Path::ROTATION:
                    {
                        glm::quat q1;
                        q1.x = sampler.outputsVec4[i].x;
                        q1.y = sampler.outputsVec4[i].y;
                        q1.z = sampler.outputsVec4[i].z;
                        q1.w = sampler.outputsVec4[i].w;

                        glm::quat q2;
                        q2.x = sampler.outputsVec4[i+1].x;
                        q2.y = sampler.outputsVec4[i+1].y;
                        q2.z = sampler.outputsVec4[i+1].z;
                        q2.w = sampler.outputsVec4[i+1].w;

                        channel.node->rotation = glm::normalize(glm::slerp(q1, q2, a));
                        break;
                    }
                    case VktTypes::AnimationChannel::Path::SCALE:
                    {
                        channel.node->scale = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i+1], a);
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }
    for(auto& node : topNodes){
        VktTypes::Node::updateJoints(*node, jointsBuffers);
    }
}
