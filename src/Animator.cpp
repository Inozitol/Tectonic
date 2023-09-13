#include "model/anim/Animator.h"

Animator::Animator(){
    m_currentTime = 0.0f;
    m_finalBoneMatrices.fill(glm::mat4(1.0f));
}

void Animator::updateAnimation(float dt) {
    m_deltaTime = dt;
    if(m_currentAnimation){
        m_currentTime += m_currentAnimation->getTicksPerSecond() * dt;
        m_currentTime = fmod(m_currentTime, m_currentAnimation->getDuration());
        if(m_nextAnimation){
            m_blendFactor += m_blendDirection;
            if(m_blendFactor > 1.0f){
                m_currentAnimation = m_nextAnimation;
                m_nextAnimation = nullptr;
                m_currentTime = m_nextTime;
                calcBoneTransform();
                m_blendFactor = 0.0f;
                return;
            }
            m_nextTime += m_nextAnimation->getTicksPerSecond() * dt;
            m_nextTime = fmod(m_nextTime, m_nextAnimation->getDuration());
            calcBoneTransformBlended();
        }else{
            calcBoneTransform();
        }
    }
}

void Animator::playAnimation(uint32_t animationIndex) {
    if(m_currentAnimation){
        m_nextAnimation = m_animationModel->getAnimation(animationIndex);
        if(!m_nextAnimation){
            m_currentAnimation = nullptr;
            m_currentTime = 0.0f;
        }
        m_nextTime = 0.0f;
    }else{
        m_currentAnimation = m_animationModel->getAnimation(animationIndex);
        m_currentTime = 0.0f;
    }
}

void Animator::calcBoneTransformBlended() {
    std::queue<std::pair<const NodeData*, const glm::mat4>> processingQueue;
    processingQueue.emplace(&m_animationModel->getRootNode(), glm::mat4(1.0f));

    if(m_blendFactor < 0.0 || m_blendFactor > 1.0f){
        fprintf(stderr, "Invalid blend factor %f\n", m_blendFactor);
        return;
    }

    while(!processingQueue.empty()){
        const NodeData* frontNode = processingQueue.front().first;
        const glm::mat4 parentTrans = processingQueue.front().second;
        processingQueue.pop();

        std::string nodeName = frontNode->name;
        int32_t boneID = frontNode->boneID;
        glm::mat4 nodeTransform = frontNode->transformation;

        const Bone* currBone = m_currentAnimation->findBone(boneID);
        const Bone* nextBone = m_nextAnimation->findBone(boneID);
        if(currBone && nextBone){
            currBone->updateBlend(m_currentTime, nextBone, m_nextTime, m_blendFactor);
            nodeTransform = currBone->getLocalTransform();
        }
        glm::mat4 globalTrans = parentTrans * nodeTransform;

        const BoneInfo* info = m_animationModel->getBoneInfo(boneID);
        if(info){
            m_finalBoneMatrices.at(info->id) = globalTrans * info->offset;
        }

        for(const auto& childNode : frontNode->children){
            processingQueue.emplace(&childNode, globalTrans);
        }
    }
}

void Animator::calcBoneTransform() {
    std::queue<std::pair<const NodeData*, const glm::mat4>> processingQueue;
    processingQueue.emplace(&m_animationModel->getRootNode(), glm::mat4(1.0f));

    while(!processingQueue.empty()){
        const NodeData* frontNode = processingQueue.front().first;
        const glm::mat4 parentTrans = processingQueue.front().second;
        processingQueue.pop();

        std::string nodeName = frontNode->name;
        int32_t boneID = frontNode->boneID;
        glm::mat4 nodeTransform = frontNode->transformation;

        const Bone* bone = m_currentAnimation->findBone(boneID);
        if(bone){
            bone->update(m_currentTime);
            nodeTransform = bone->getLocalTransform();
        }

        glm::mat4 globalTrans = parentTrans * nodeTransform;

        const BoneInfo* info = m_animationModel->getBoneInfo(boneID);
        if(info){
            m_finalBoneMatrices.at(info->id) = globalTrans * info->offset;
        }

        for(const auto& childNode : frontNode->children){
            processingQueue.emplace(&childNode, globalTrans);
        }
    }
}
