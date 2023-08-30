#ifndef TECTONIC_SKINNEDMODEL_H
#define TECTONIC_SKINNEDMODEL_H

#include "Model.h"

/**
 * @brief Model with bones and animation logic.
 */
class SkinnedModel : public Model {
public:
    const BoneInfo* getBoneInfo(const std::string& boneName) const;
    const Animation* getAnimation(uint32_t animIndex) const;

    using BoneInfoMap_t = std::unordered_map<std::string, BoneInfo>;
private:
    BoneInfoMap_t m_boneInfoMap;
    int32_t m_boneCounter = 0;
    std::vector<Animation> m_animations;
    uint32_t m_currentAnim = -1;
    std::array<glm::mat4, MAX_BONES> m_finalBoneMatrices{};

};


#endif //TECTONIC_SKINNEDMODEL_H
