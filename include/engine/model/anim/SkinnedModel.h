#ifndef TECTONIC_SKINNEDMODEL_H
#define TECTONIC_SKINNEDMODEL_H

#include "engine/model/Model.h"

/**
 * @brief Model with bones and animation logic.
 */
class SkinnedModel : public Model {
    friend class AssimpLoader;
public:
    SkinnedModel() = default;
    explicit SkinnedModel(const Model& model);
    explicit SkinnedModel(const std::shared_ptr<Model>& model);

    const BoneInfo* getBoneInfo(const std::string& boneName) const;
    const BoneInfo* getBoneInfo(int32_t boneID) const;
    const Animation* getAnimation(uint32_t animIndex) const;

    void bufferMeshes() override;

    using BoneInfoMap_t = std::unordered_map<std::string, BoneInfo>;
    using BoneInfoVec_t = std::vector<const BoneInfo*>;
private:
    int32_t getBoneID(const std::string& boneName, const glm::mat4& offsetMatrix);

    BoneInfoMap_t m_boneInfoMap;
    BoneInfoVec_t m_boneInfoVec;
    int32_t m_boneCounter = 0;
    std::vector<Animation> m_animations;
};


#endif //TECTONIC_SKINNEDMODEL_H
