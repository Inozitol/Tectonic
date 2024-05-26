#include <queue>
#include "engine/vulkan/VktTypes.h"

void VktTypes::Node::refreshTransform(Node& root, const glm::mat4& parentMatrix){
    std::queue<std::pair<Node*, const glm::mat4>> q;
    q.emplace(&root, parentMatrix);

    while(!q.empty()) {
        auto [n,t] = q.front();
        q.pop();
        n->worldTransform = t * n->localTransform;

        for(const auto& c : n->children){
            q.emplace(c, n->worldTransform);
        }
    }
}

void VktTypes::Node::gatherContext(const Node& root, const glm::mat4& topMatrix, DrawContext& ctx){
    std::queue<const Node*> q;
    q.push(&root);

    while(!q.empty()){
        const Node* n = q.front();
        q.pop();

        // If there's a mesh in this node, update DrawContext with surfaces
        if(n->mesh){
            glm::mat4 nodeMatrix = topMatrix * n->worldTransform;
            for(const auto& s : n->mesh->surfaces) {
                VktTypes::RenderObject def;
                def.indexCount = s.count;
                def.firstIndex = s.startIndex;
                def.indexBuffer = n->mesh->meshBuffers.indexBuffer.buffer;
                def.material = &s.material->data;

                def.transform = nodeMatrix;
                def.vertexBufferAddress = n->mesh->meshBuffers.vertexBufferAddress;

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