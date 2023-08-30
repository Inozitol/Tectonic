#include "model/SkinnedModel.h"

const BoneInfo *SkinnedModel::getBoneInfo(const std::string &boneName) const {
    auto it = m_boneInfoMap.find(boneName);
    if(it != m_boneInfoMap.end())
        return &it->second;
    else
        return nullptr;
}

const Animation *SkinnedModel::getAnimation(uint32_t animIndex) const {
    if(animIndex < m_animations.size())
        return &m_animations.at(animIndex);
    else
        return nullptr;
}
