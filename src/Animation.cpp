#include "model/Animation.h"


Animation::Animation(const aiAnimation* animation){
    assert(animation);
    m_duration = animation->mDuration;
    m_ticksPerSecond = animation->mTicksPerSecond;
    std::cout << "SIZE: " << m_bones.size() << std::endl;
}

Bone *Animation::findBone(const std::string &name) {
    auto iter = std::find_if(m_bones.begin(), m_bones.end(),
                             [&](const Bone& bone){
        return bone.getName() == name;
    });
    if(iter == m_bones.end()) return nullptr;
    else return &(*iter);
}

void Animation::insertBone(const Bone& bone) {
    if(m_duration > bone.getLastTimestamp())
        m_duration = bone.getLastTimestamp();
    m_bones.push_back(bone);
}