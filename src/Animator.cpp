#include "Animator.h"

Animator::Animator(Animation *animation) {
    m_currentTime = 0.0f;
    m_currentAnimation = animation;
    m_finalBoneMatrices.reserve(MAX_BONES);

    for(uint32_t i = 0; i < MAX_BONES; i++){
        m_finalBoneMatrices.emplace_back(1.0f);
    }
}

void Animator::updateAnimation(float dt) {
    m_deltaTime = dt;
    if(m_currentAnimation){
        m_currentTime += m_currentAnimation->getTicksPerSecond() * dt;
        m_currentTime = fmod(m_currentTime, m_currentAnimation->getDuration());
        calcBoneTransform(&m_currentAnimation->getRootNode(), glm::mat4(1.0f));
    }
}

void Animator::playAnimation(Animation *animation) {
    m_currentAnimation = animation;
    m_currentTime = 0.0f;
}

void Animator::calcBoneTransform(const AssimpNodeData *node, glm::mat4 parentTransform) {
    std::string nodeName = node->name;
    glm::mat4 nodeTransform = node->transformation;

    Bone* bone = m_currentAnimation->findBone(nodeName);

    if(bone){
        bone->update(m_currentTime);
        nodeTransform = bone->getLocalTransform();
    }
    glm::mat4 globalTransformation = parentTransform * nodeTransform;

    auto boneInfoMap = m_currentAnimation->getBoneIDMap();
    if(boneInfoMap.find(nodeName) != boneInfoMap.end()){
        int32_t index = boneInfoMap[nodeName].id;
        glm::mat4 offset = boneInfoMap[nodeName].offset;
        m_finalBoneMatrices[index] = globalTransformation * offset;
    }

    for (int i = 0; i < node->childCount; i++){
        calcBoneTransform(&node->children[i], globalTransformation);
    }
}