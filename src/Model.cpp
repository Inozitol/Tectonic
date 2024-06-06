#include "engine/model/Model.h"

#include <iostream>
#include <set>
#include <queue>
#include <glm/gtx/quaternion.hpp>

#include "engine/vulkan/VktTypes.h"
#include "engine/vulkan/VktCore.h"
#include "utils/Serial.h"
#include "utils/SerialTypes.h"

Logger Model::m_logger = Logger("Model");
std::unordered_map<std::filesystem::path, SerialTypes::BinDataVec_t> Model::m_loadedModels = std::unordered_map<std::filesystem::path, SerialTypes::BinDataVec_t>{};

void Model::readMesh(VktTypes::MeshAsset& dst, SerialTypes::BinDataVec_t& src, std::size_t& offset){
    SerialTypes::Span<uint32_t,VktTypes::MeshSurface> surfaces = SerialTypes::Span<uint32_t,VktTypes::MeshSurface>(src, offset);
    dst.surfaces.resize(surfaces.size());
    std::memcpy(dst.surfaces.data(),surfaces.data(),surfaces.size()*sizeof(VktTypes::MeshSurface));
    SerialTypes::Span<uint32_t,uint32_t,false> indices(src,offset);
    SerialTypes::Span<uint32_t,VktTypes::Vertex,false> vertices(src,offset);

    dst.meshBuffers = VktCore::uploadMesh(std::span<uint32_t>(indices.data(), indices.size()),
                                          std::span<VktTypes::Vertex>(vertices.data(),vertices.size()));
}

void Model::readImage(VktTypes::AllocatedImage& dst, SerialTypes::BinDataVec_t& src, std::size_t& offset){
    SerialTypes::Span<uint32_t,char,false> name = SerialTypes::Span<uint32_t,char,false>(src, offset);
    VkExtent3D extent   = Serial::readDataAtInc<VkExtent3D>(src, offset);
    VkFormat   format   = Serial::readDataAtInc<VkFormat>(src, offset);
    SerialTypes::Span<uint32_t,std::byte,false> imgData = SerialTypes::Span<uint32_t,std::byte,false>(src,offset);
    dst = VktCore::createImage(imgData.data(), extent, format, VK_IMAGE_USAGE_SAMPLED_BIT, false);
}

void Model::readSampler(VkSampler& dst, SerialTypes::BinDataVec_t& src, std::size_t& offset){
    VkSamplerCreateInfo info = Serial::readDataAtInc<VkSamplerCreateInfo>(src, offset);
    vkCreateSampler(VktCore::device(), &info, nullptr, &dst);
}

void Model::readNode(ModelTypes::Node& dst, SerialTypes::BinDataVec_t& src, std::size_t& offset) {
    dst.name                = SerialTypes::Span<uint32_t,char,false>(src, offset);
    dst.parent              = Serial::readDataAtInc<ModelTypes::NodeID_t>(src, offset);
    dst.children            = SerialTypes::Span<uint32_t,ModelTypes::NodeID_t,false>(src, offset);
    dst.mesh                = Serial::readDataAtInc<ModelTypes::MeshID_t>(src, offset);
    dst.localTransform      = Serial::readDataAtInc<glm::mat4>(src, offset);
    dst.worldTransform      = Serial::readDataAtInc<glm::mat4>(src, offset);
    dst.animationTransform  = Serial::readDataAtInc<glm::mat4>(src, offset);
    dst.translation         = Serial::readDataAtInc<glm::vec3>(src, offset);
    dst.scale               = Serial::readDataAtInc<glm::vec3>(src, offset);
    dst.rotation            = Serial::readDataAtInc<glm::quat>(src, offset);
    dst.skin                = Serial::readDataAtInc<ModelTypes::SkinID_t>(src, offset);
}

void Model::readMaterial(ModelTypes::GLTFMaterial& dst, SerialTypes::BinDataVec_t& src, std::size_t& offset, uint32_t mIndex) {
    VktTypes::GLTFMetallicRoughness::MaterialConstants* gpuBuffer = static_cast<VktTypes::GLTFMetallicRoughness::MaterialConstants*>(m_materialBuffer.info.pMappedData);

    // Read material data
    ModelTypes::MaterialResources loadedResources = Serial::readDataAtInc<ModelTypes::MaterialResources>(src, offset);
    VktTypes::GLTFMetallicRoughness::MaterialConstants loadedConstants = Serial::readDataAtInc<VktTypes::GLTFMetallicRoughness::MaterialConstants>(src, offset);
    VktTypes::MaterialPass loadedPass = Serial::readDataAtInc<VktTypes::MaterialPass>(src, offset);

    // Default error textures for missing textures
    VktTypes::GLTFMetallicRoughness::MaterialResources resources;
    resources.colorImage        = VktCore::getInstance().m_errorCheckboardImage;
    resources.colorSampler      = VktCore::getInstance().m_defaultSamplerLinear;
    resources.metalRoughImage   = VktCore::getInstance().m_errorCheckboardImage;
    resources.metalRoughSampler = VktCore::getInstance().m_defaultSamplerLinear;

    // Upload constants to GPU buffer
    gpuBuffer[mIndex] = loadedConstants;

    if(loadedResources.colorImage != ModelTypes::NULL_ID && loadedResources.colorSampler != ModelTypes::NULL_ID){
        resources.colorImage = m_images[loadedResources.colorImage];
        resources.colorSampler = m_samplers[loadedResources.colorSampler];
    }

    if(loadedResources.metalRoughImage != ModelTypes::NULL_ID && loadedResources.metalRoughSampler != ModelTypes::NULL_ID){
        resources.metalRoughImage = m_images[loadedResources.metalRoughImage];
        resources.metalRoughSampler = m_samplers[loadedResources.metalRoughSampler];
    }

    resources.dataBuffer = m_materialBuffer.buffer;
    resources.dataBufferOffset = mIndex*sizeof(VktTypes::GLTFMetallicRoughness::MaterialConstants);

    // Create material in GPU
    dst.data = VktCore::getInstance().writeMaterial(VktCore::device(), loadedPass, resources, m_descriptorPool);
}

void Model::readSkin(ModelTypes::Skin& dst, SerialTypes::BinDataVec_t& src, std::size_t& offset){
    dst.name                = SerialTypes::Span<uint32_t,char,false>(src, offset);
    dst.skeletonRoot        = Serial::readDataAtInc<uint32_t>(src, offset);
    dst.skinNodes           = SerialTypes::Span<uint32_t,ModelTypes::NodeID_t,false>(src, offset);
    dst.inverseBindMatrices = SerialTypes::Span<uint32_t,glm::mat4,false>(src, offset);
    dst.joints              = SerialTypes::Span<uint32_t,ModelTypes::NodeID_t,false>(src, offset);
}

void Model::readAnimationSampler(ModelTypes::AnimationSampler& dst, SerialTypes::BinDataVec_t &src, std::size_t &offset) {
    dst.interpolation = Serial::readDataAtInc<ModelTypes::AnimationSampler::Interpolation>(src, offset);
    dst.inputs        = SerialTypes::Span<uint32_t,float,false>(src, offset);
    dst.outputsVec4   = SerialTypes::Span<uint32_t,glm::vec4,false>(src, offset);
}

void Model::readAnimation(ModelTypes::Animation &dst, SerialTypes::BinDataVec_t &src, std::size_t &offset) {
    dst.name = SerialTypes::Span<uint32_t,char,false>(src, offset);
    dst.start = Serial::readDataAtInc<float>(src, offset);
    dst.end = Serial::readDataAtInc<float>(src, offset);
    uint32_t samplerCount = Serial::readDataAtInc<uint32_t>(src, offset);
    dst.samplers.resize(samplerCount);
    for(uint32_t i = 0; i < samplerCount; i++){
        readAnimationSampler(dst.samplers[i], src, offset);
    }
    dst.channels = SerialTypes::Span<uint32_t,ModelTypes::AnimationChannel,false>(src, offset);
    dst.animatedNodes = SerialTypes::Span<uint32_t,std::pair<uint32_t,uint32_t>,false>(src, offset);
    dst.currentTime = dst.start;
}

Model::Model(const std::filesystem::path& path) {
    if(!m_loadedModels.contains(path)){
        loadModelData(path);
    }

    SerialTypes::BinDataVec_t& data = m_loadedModels[path];

    const uint8_t version          = Serial::readDataAt<uint8_t>(data, SerialTypes::Model::VERSION_OFFSET);
    const uint32_t meshIndex       = Serial::readDataAt<uint32_t>(data, SerialTypes::Model::MESHES_INDEX);
    const uint32_t imageIndex      = Serial::readDataAt<uint32_t>(data, SerialTypes::Model::IMAGES_INDEX);
    const uint32_t samplerIndex    = Serial::readDataAt<uint32_t>(data, SerialTypes::Model::SAMPLERS_INDEX);
    const uint32_t nodeIndex       = Serial::readDataAt<uint32_t>(data, SerialTypes::Model::NODES_INDEX);
    const uint32_t materialIndex   = Serial::readDataAt<uint32_t>(data, SerialTypes::Model::MATERIALS_INDEX);
    const uint32_t skinIndex       = Serial::readDataAt<uint32_t>(data, SerialTypes::Model::SKIN_INDEX);
    const uint32_t animationIndex  = Serial::readDataAt<uint32_t>(data, SerialTypes::Model::ANIMATION_INDEX);

    std::size_t meshCount = Serial::readDataAt<uint32_t>(data, meshIndex);
    m_meshes.resize(meshCount);
    std::size_t index = meshIndex + sizeof(uint32_t);
    for(std::size_t i = 0; i < meshCount; i++){
        readMesh(m_meshes[i], data, index);
        m_logger(Logger::DEBUG) << path << " Loaded mesh with " << m_meshes[i].surfaces.size() << " surfaces\n";
    }
    m_logger(Logger::DEBUG) << path << " Finished loading " << meshCount << " meshes\n";

    std::size_t imageCount = Serial::readDataAt<uint32_t>(data, imageIndex);
    m_images.resize(imageCount);
    index = imageIndex + sizeof(uint32_t);
    for(std::size_t i = 0; i < imageCount; i++){
        readImage(m_images[i], data, index);
        m_logger(Logger::DEBUG) << path << " Loaded image with height " << m_images[i].extent.height << " and width " << m_images[i].extent.width << '\n';
    }
    m_logger(Logger::DEBUG) << path << " Finished loading " << imageCount << " images\n";

    std::size_t samplerCount = Serial::readDataAt<uint32_t>(data, samplerIndex);
    m_samplers.resize(samplerCount);
    index = samplerIndex + sizeof(uint32_t);
    for(std::size_t i = 0; i < samplerCount; i++){
        readSampler(m_samplers[i], data, index);
    }
    m_logger(Logger::DEBUG) << path << " Finished loading " << samplerCount << " samplers\n";

    m_rootNode = Serial::readDataAt<uint32_t>(data, nodeIndex);
    std::size_t nodeCount = Serial::readDataAt<uint32_t>(data, nodeIndex + sizeof(uint32_t));
    m_nodes.resize(nodeCount);
    index = nodeIndex + sizeof(uint32_t) * 2;
    for(std::size_t i = 0; i < nodeCount; i++){
        readNode(m_nodes[i], data, index);
        m_logger(Logger::DEBUG) << path << " Loaded node named [" << m_nodes[i].name.data() << "] with " << m_nodes[i].children.size() << " child nodes\n";
    }
    m_logger(Logger::DEBUG) << path << " Finished loading " << nodeCount << " nodes\n";

    std::size_t materialCount = Serial::readDataAt<uint32_t>(data,materialIndex);
    m_materials.resize(materialCount);
    std::vector<DescriptorAllocatorDynamic::PoolSizeRatio> sizes = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}
    };
    m_descriptorPool.initPool(VktCore::device(), materialCount, sizes);

    m_materialBuffer = VktCore::createBuffer(sizeof(VktTypes::GLTFMetallicRoughness::MaterialConstants)*materialCount,
                                             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    index = materialIndex + sizeof(uint32_t);
    for(std::size_t i = 0; i < materialCount; i++){
        readMaterial(m_materials[i], data, index, i);
    }
    m_logger(Logger::DEBUG) << path << " Finished loading " << materialCount << " materials\n";

    index = skinIndex;
    readSkin(m_skin, data, index);
    m_logger(Logger::DEBUG) << path << " Finished loading skin\n";
    m_jointsBuffer = VktCore::uploadJoints(std::span<glm::mat4>(m_skin.inverseBindMatrices.data(),m_skin.inverseBindMatrices.size()));

    std::size_t animationCount = Serial::readDataAt<uint32_t>(data,animationIndex);
    m_animations.resize(animationCount);
    index = animationIndex + sizeof(uint32_t);
    for(std::size_t i = 0; i < animationCount; i++){
        readAnimation(m_animations[i], data, index);
        m_logger(Logger::DEBUG) << path
                                << " Loaded animation named ["
                                << m_animations[i].name.data()
                                << "] with "
                                << m_animations[i].samplers.size()
                                << " samplers/channels\n";
    }
    m_logger(Logger::DEBUG) << path << " Finished loading " << animationCount << " animations\n";
}

void Model::loadModelData(const std::filesystem::path& path){
    if(!exists(path)){
        m_logger(Logger::ERROR) << path << " Unable to find model file\n";
        throw modelException("Missing model file");
    }

    std::ifstream file(path, std::ios::binary);
    file.unsetf(std::ios::skipws);
    std::size_t fileSize = std::filesystem::file_size(path);
    m_loadedModels[path] = SerialTypes::BinDataVec_t(fileSize);
    file.read(reinterpret_cast<char*>(m_loadedModels[path].data()), static_cast<long>(fileSize));
}

void Model::gatherDrawContext(VktTypes::DrawContext &ctx){
    std::queue<uint32_t> q;
    q.push(m_rootNode);

    glm::mat4 worldM = transformation.getMatrix();
    while(!q.empty()) {
        const ModelTypes::Node *n = &m_nodes[q.front()];
        q.pop();

        // TODO Pre-calc a vector with mesh nodes to not traverse the whole tree every frame
        // If there's a mesh in this node, update DrawContext with surfaces
        if (n->mesh != ModelTypes::NULL_ID) {
            const VktTypes::MeshAsset *mesh = &m_meshes[n->mesh];
            for (const auto &s: mesh->surfaces) {
                VktTypes::RenderObject def;
                def.indexCount = s.count;
                def.firstIndex = s.startIndex;
                def.indexBuffer = mesh->meshBuffers.indexBuffer.buffer;
                def.material = &m_materials[s.materialIndex].data;

                def.transform = worldM;
                def.vertexBufferAddress = mesh->meshBuffers.vertexBufferAddress;
                if (n->skin != ModelTypes::NULL_ID) {
                    def.jointsBufferAddress = m_jointsBuffer.jointsBufferAddress;
                }

                switch (m_materials[s.materialIndex].data.passType) {
                    case VktTypes::MaterialPass::OPAQUE:
                        ctx.opaqueSurfaces.push_back(def);
                        break;
                    case VktTypes::MaterialPass::TRANSPARENT:
                        ctx.transparentSurfaces.push_back(def);
                        break;
                    case VktTypes::MaterialPass::OTHER:
                        break;
                }
            }
        }

        // Continue adding context from the tree
        for (unsigned int nodeID : n->children) {
            q.push(nodeID);
        }
    }
}

void Model::updateAnimationTime(float delta) {
    ModelTypes::Animation* a = &m_animations[m_activeAnimation];
    a->currentTime += delta;
    if(a->currentTime > a->end){
        a->currentTime -= a->end;
    }
}

void Model::updateJoints() {
    if(m_activeAnimation != ModelTypes::NULL_ID) {
        glm::mat4 identity = glm::identity<glm::mat4>();
        const glm::mat4 *parentTransform = &identity;
        const ModelTypes::Animation *animation = &m_animations[m_activeAnimation];

        for (std::size_t nodeID = 0; nodeID < animation->animatedNodes.size(); nodeID++) {
            auto [nID, chID] = animation->animatedNodes[nodeID];
            ModelTypes::Node *node = &m_nodes[nID];
            ModelTypes::AnimationChannel *channel = &animation->channels[chID];
            if (node->parent != ModelTypes::NULL_ID) {
                parentTransform = &m_nodes[node->parent].animationTransform;
            }

            glm::mat4 sm = glm::scale(glm::identity<glm::mat4>(), node->scale);
            glm::mat4 rm = glm::toMat4(node->rotation);
            glm::mat4 tm = glm::translate(glm::identity<glm::mat4>(), node->translation);

            if (channel->scaleSampler != ModelTypes::NULL_ID) {
                const ModelTypes::AnimationSampler *sampler = &animation->samplers[channel->scaleSampler];
                std::size_t i = sampler->lastInput;
                do {
                    if ((animation->currentTime >= sampler->inputs[i]) &&
                        (animation->currentTime <= sampler->inputs[i + 1])) {
                        float a = (animation->currentTime - sampler->inputs[i]) /
                                  (sampler->inputs[i + 1] - sampler->inputs[i]);
                        glm::vec3 scale = glm::mix(sampler->outputsVec4[i], sampler->outputsVec4[i + 1], a);
                        sm = glm::scale(glm::identity<glm::mat4>(), scale);
                        node->scale = scale;
                        sampler->lastInput = i;
                        break;
                    }
                    i++;
                    if (i >= sampler->inputs.size()) i = 0;
                } while (i != sampler->lastInput);
            }

            if (channel->rotationSampler != ModelTypes::NULL_ID) {
                const ModelTypes::AnimationSampler *sampler = &animation->samplers[channel->rotationSampler];
                std::size_t i = sampler->lastInput;
                do {
                    if ((animation->currentTime >= sampler->inputs[i]) &&
                        (animation->currentTime <= sampler->inputs[i + 1])) {
                        float a = (animation->currentTime - sampler->inputs[i]) /
                                  (sampler->inputs[i + 1] - sampler->inputs[i]);
                        glm::quat q1;
                        q1.x = sampler->outputsVec4[i].x;
                        q1.y = sampler->outputsVec4[i].y;
                        q1.z = sampler->outputsVec4[i].z;
                        q1.w = sampler->outputsVec4[i].w;

                        glm::quat q2;
                        q2.x = sampler->outputsVec4[i + 1].x;
                        q2.y = sampler->outputsVec4[i + 1].y;
                        q2.z = sampler->outputsVec4[i + 1].z;
                        q2.w = sampler->outputsVec4[i + 1].w;

                        glm::quat q = glm::normalize(glm::slerp(q1, q2, a));
                        rm = glm::toMat4(q);
                        node->rotation = q;
                        sampler->lastInput = i;
                        break;
                    }
                    i++;
                    if (i >= sampler->inputs.size()) i = 0;
                } while (i != sampler->lastInput);
            }

            if (channel->translationSampler != ModelTypes::NULL_ID) {
                const ModelTypes::AnimationSampler *sampler = &animation->samplers[channel->translationSampler];
                std::size_t i = sampler->lastInput;
                do {
                    if ((animation->currentTime >= sampler->inputs[i]) &&
                        (animation->currentTime <= sampler->inputs[i + 1])) {
                        float a = (animation->currentTime - sampler->inputs[i]) /
                                  (sampler->inputs[i + 1] - sampler->inputs[i]);
                        glm::vec3 translation = glm::mix(sampler->outputsVec4[i], sampler->outputsVec4[i + 1], a);
                        tm = glm::translate(glm::identity<glm::mat4>(), translation);
                        node->translation = translation;
                        sampler->lastInput = i;
                        break;
                    }
                    i++;
                    if (i >= sampler->inputs.size()) i = 0;
                } while (i != sampler->lastInput);
            }

            glm::mat4 animTransform = tm * rm * sm * node->localTransform;
            node->animationTransform = (*parentTransform) * animTransform;
        }
    }

    /*
    for(unsigned int skinNode : m_skin.skinNodes){
        ModelTypes::Node* node = &m_nodes[skinNode];
        glm::mat4 inverseTransform = glm::inverse(node->animationTransform);

        for (std::size_t i = 0; i < numJoints; i++) {
            jointMatrices[i] = inverseTransform * m_nodes[m_skin.joints[i]].animationTransform * m_skin.inverseBindMatrices[i];
        }
        memcpy(m_jointsBuffer.jointsBuffer.info.pMappedData, jointMatrices.data(), jointMatrices.size() * sizeof(glm::mat4));
    }
     */

    std::size_t numJoints = m_skin.joints.size();
    std::vector<glm::mat4> jointMatrices(numJoints);

    ModelTypes::Node* node = &m_nodes[m_skin.skinNodes[0]]; // For some reason this works too?
    glm::mat4 inverseTransform = glm::inverse(node->animationTransform);

    for (std::size_t i = 0; i < numJoints; i++) {
        jointMatrices[i] = inverseTransform * m_nodes[m_skin.joints[i]].animationTransform * m_skin.inverseBindMatrices[i];
    }
    memcpy(m_jointsBuffer.jointsBuffer.info.pMappedData, jointMatrices.data(), jointMatrices.size() * sizeof(glm::mat4));
}

void Model::setAnimation(uint32_t aID) {
    if(aID < m_animations.size()){
        m_activeAnimation = aID;
        m_animations[m_activeAnimation].currentTime = m_animations[m_activeAnimation].start;
    }
}

std::string_view Model::animationName(uint32_t aID) {
    if(aID < m_animations.size()){
        return m_animations[aID].name.data();
    }
    return "";
}

uint32_t Model::animationCount() {
    return m_animations.size();
}

uint32_t Model::currentAnimation() const {
    return m_activeAnimation;
}

void Model::clear() {
    for(const auto& mesh : m_meshes){
        VktCore::destroyBuffer(mesh.meshBuffers.indexBuffer);
        VktCore::destroyBuffer(mesh.meshBuffers.vertexBuffer);
    }
    for(const auto& sampler : m_samplers){
        vkDestroySampler(VktCore::device(), sampler, nullptr);
    }
    VktCore::destroyBuffer(m_materialBuffer);
    VktCore::destroyBuffer(m_jointsBuffer.jointsBuffer);
    m_descriptorPool.destroyPool(VktCore::device());
}

