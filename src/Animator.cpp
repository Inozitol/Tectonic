#include "model/Animator.h"

Animator::Animator(){
    m_currentTime = 0.0f;
    m_finalBoneMatrices.reserve(MAX_BONES);

    for(uint32_t i = 0; i < MAX_BONES; i++){
        m_finalBoneMatrices.emplace_back(1.0f);
    }
}

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
        calcBoneTransform(&m_animationModel->getRootNode(), glm::mat4(1.0f));
    }
}

void Animator::playAnimation(Animation *animation) {
    m_currentAnimation = animation;
    m_currentTime = 0.0f;
}

void Animator::calcBoneTransform(const NodeData *node, glm::mat4 parentTransform) {
    std::string nodeName = node->name;
    glm::mat4 nodeTransform = node->transformation;

    Bone* bone = m_currentAnimation->findBone(nodeName);

    if(bone){
        bone->update(m_currentTime);
        nodeTransform = bone->getLocalTransform();
    }

    glm::mat4 globalTransformation = parentTransform * nodeTransform;

    const BoneInfo* info = m_animationModel->getBoneInfo(nodeName);
    if(info)
        m_finalBoneMatrices.at(info->id) = globalTransformation * info->offset;

    for (int i = 0; i < node->childCount; i++){
        calcBoneTransform(&node->children[i], globalTransformation);
    }
}