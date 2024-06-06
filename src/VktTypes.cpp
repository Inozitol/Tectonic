#include <queue>
#include <glm/gtx/quaternion.hpp>
#include "engine/vulkan/VktTypes.h"
#include "engine/model/ModelTypes.h"

/*
void Node::gatherContext(const Node& root, const glm::mat4& topMatrix, const VktTypes::GPUJointsBuffers& jointsBuffer, VktTypes::DrawContext& ctx){
    std::queue<const Node*> q;
    q.push(&root);

    while(!q.empty()){
        const Node* n = q.front();
        q.pop();

        // If there's a mesh in this node, update DrawContext with surfaces
        if(n->mesh){
            glm::mat4 nodeMatrix = topMatrix; // * n->worldTransform;
            for(const auto& s : n->mesh->surfaces) {
                VktTypes::RenderObject def;
                def.indexCount = s.count;
                def.firstIndex = s.startIndex;
                def.indexBuffer = n->mesh->meshBuffers.indexBuffer.buffer;
                def.material = &s.material->data;

                def.transform = nodeMatrix;
                def.vertexBufferAddress = n->mesh->meshBuffers.vertexBufferAddress;
                if(n->skin) {
                    def.jointsBufferAddress = jointsBuffer.jointsBufferAddress;
                }

                switch (s.material->data.passType) {
                    case MaterialPass::OPAQUE:
                        ctx.opaqueSurfaces.push_back(def);
                        break;
                    case MaterialPass::TRANSPARENT:
                        ctx.transparentSurfaces.push_back(def);
                        break;
                    case MaterialPass::OTHER:
                        break;
                }
            }
        }

        // Continue adding context from the tree
        for(const auto& c : n->children){
            q.push(c);
        }
    }
}
*/
/*
void VktTypes::Skin::updateJoints(VktTypes::Animation* animation, const VktTypes::GPUJointsBuffers& jointsBuffer) const {

    glm::mat4 identity = glm::identity<glm::mat4>();
    const glm::mat4* parentTransform = &identity;

    for(std::size_t nodeID = 0; nodeID < animation->animatedNodes.size(); nodeID++) {
        auto [node, channel] = animation->animatedNodes[nodeID];

        if (node->parent) {
            parentTransform = &node->parent->animationTransform;
        }

        glm::mat4 sm = glm::scale(glm::identity<glm::mat4>(), node->scale);
        glm::mat4 rm = glm::toMat4(node->rotation);
        glm::mat4 tm = glm::translate(glm::identity<glm::mat4>(), node->translation);

        if(channel->scaleSampler){
            VktTypes::AnimationSampler* sampler = channel->scaleSampler;
            std::size_t i = sampler->lastInput;
            do{
                if ((animation->currTime >= sampler->inputs[i]) && (animation->currTime <= sampler->inputs[i + 1])) {
                    float a = (animation->currTime - sampler->inputs[i]) / (sampler->inputs[i + 1] - sampler->inputs[i]);
                    glm::vec3 scale = glm::mix(sampler->outputsVec4[i], sampler->outputsVec4[i + 1], a);
                    sm = glm::scale(glm::identity<glm::mat4>(),scale);
                    node->scale = scale;
                    sampler->lastInput = i;
                    break;
                }
                i++;
                if(i >= sampler->inputs.size()) i = 0;
            }while(i != sampler->lastInput);
        }

        if(channel->rotationSampler){
            VktTypes::AnimationSampler* sampler = channel->rotationSampler;
            std::size_t i = sampler->lastInput;
            do{
                if ((animation->currTime >= sampler->inputs[i]) && (animation->currTime <= sampler->inputs[i + 1])) {
                    float a = (animation->currTime - sampler->inputs[i]) / (sampler->inputs[i + 1] - sampler->inputs[i]);
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
                if(i >= sampler->inputs.size()) i = 0;
            }while(i != sampler->lastInput);
        }

        if(channel->translationSampler){
            VktTypes::AnimationSampler* sampler = channel->translationSampler;
            std::size_t i = sampler->lastInput;
            do{
                if ((animation->currTime >= sampler->inputs[i]) && (animation->currTime <= sampler->inputs[i + 1])) {
                    float a = (animation->currTime - sampler->inputs[i]) / (sampler->inputs[i + 1] - sampler->inputs[i]);
                    glm::vec3 translation = glm::mix(sampler->outputsVec4[i], sampler->outputsVec4[i + 1], a);
                    tm = glm::translate(glm::identity<glm::mat4>(), translation);
                    node->translation = translation;
                    sampler->lastInput = i;
                    break;
                }
                i++;
                if(i >= sampler->inputs.size()) i = 0;
            }while(i != sampler->lastInput);
        }

        glm::mat4 animTransform = tm * rm * sm * node->localTransform;
        node->animationTransform = (*parentTransform) * animTransform;
    }

    std::size_t numJoints = joints.size();
    std::vector<glm::mat4> jointMatrices(numJoints);
    /*
    for(std::size_t nodeID = 0; nodeID < skinNodes.size(); nodeID++){
        VktTypes::Node* node = skinNodes[nodeID];
        glm::mat4 inverseTransform = glm::inverse(node->animationTransform);

        for (std::size_t i = 0; i < numJoints; i++) {
            jointMatrices[i] = inverseTransform * joints[i]->animationTransform * inverseBindMatrices[i];
        }
        memcpy(jointsBuffer.jointsBuffer.info.pMappedData, jointMatrices.data(), jointMatrices.size() * sizeof(glm::mat4));
    }*/
/*
    VktTypes::Node* node = skinNodes[0];
    glm::mat4 inverseTransform = glm::inverse(node->animationTransform);

    for (std::size_t i = 0; i < numJoints; i++) {
        jointMatrices[i] = inverseTransform * joints[i]->animationTransform * inverseBindMatrices[i];
    }
    memcpy(jointsBuffer.jointsBuffer.info.pMappedData, jointMatrices.data(), jointMatrices.size() * sizeof(glm::mat4));

}

glm::mat4 VktTypes::Node::localMatrix() const {
    //return localTransform;
    return glm::translate(glm::mat4(1.0f), translation) * glm::toMat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * localTransform;
}

glm::mat4 VktTypes::Node::nodeMatrix() const {
    glm::mat4 nodeMatrix = localMatrix();
    const Node* currParent = parent;
    while(currParent){
        nodeMatrix = currParent->localMatrix() * nodeMatrix;
        currParent = currParent->parent;
    }
    return nodeMatrix;
}

void VktTypes::Animation::updateTime(float delta) {
    currTime += delta;
    if(currTime > end){
        currTime -= end;
    }
}
*/