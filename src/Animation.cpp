#include "model/anim/Animation.h"

Animation::Animation(double duration, double ticksPerSec)
    : m_duration(duration), m_ticksPerSecond(ticksPerSec){}

const Bone *Animation::findBone(int32_t boneID) const {
    const auto it = m_bones.find(boneID);
    if(it != m_bones.end()){
        return &(it->second);
    }else{
        return nullptr;
    }
}

const Bone *Animation::findBone(const std::string& boneName) const {
    auto iter = std::find_if(m_boneVec.begin(), m_boneVec.end(),
                             [&](const Bone& bone){
                                 return bone.getName() == boneName;
                             });
    if(iter == m_boneVec.end()) return nullptr;
    else return &(*iter);
}

void Animation::insertBone(const Bone& bone) {
    if(m_duration > bone.getLastTimestamp())
        m_duration = bone.getLastTimestamp();

    m_bones.insert({bone.getBoneId(), bone});
    m_boneVec.push_back(bone);
}