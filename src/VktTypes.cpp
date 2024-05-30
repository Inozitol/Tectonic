#include <queue>
#include <glm/gtx/quaternion.hpp>
#include "engine/vulkan/VktTypes.h"

void VktTypes::Node::gatherContext(const Node& root, const glm::mat4& topMatrix, const std::vector<VktTypes::GPUJointsBuffers>& jointsBuffers, DrawContext& ctx){
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
                    def.jointsBufferAddress = jointsBuffers[n->skin->index].jointsBufferAddress;
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

void VktTypes::Node::updateJoints(const VktTypes::Node &root, const std::vector<VktTypes::GPUJointsBuffers>& jointsBuffers) {
    std::queue<const Node*> q;
    q.push(&root);

    while(!q.empty()) {
        const Node *n = q.front();
        q.pop();

        if(n->skin) {
            Skin *skin = n->skin;
            glm::mat4 inverseTransform = glm::inverse(n->nodeMatrix());
            std::size_t numJoints = skin->joints.size();
            std::vector<glm::mat4> jointMatrices(numJoints);
            for (std::size_t i = 0; i < numJoints; i++) {
                jointMatrices[i] = inverseTransform * skin->joints[i]->nodeMatrix() * skin->inverseBindMatrices[i];
            }
            memcpy(jointsBuffers[skin->index].jointsBuffer.info.pMappedData, jointMatrices.data(), jointMatrices.size() * sizeof(glm::mat4));
        }

        // Continue updating the children
        for(const auto& c : n->children){
            q.push(c);
        }

    }
}

glm::mat4 VktTypes::Node::localMatrix() const {
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
