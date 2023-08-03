#ifndef TECTONIC_ANIMATION_H
#define TECTONIC_ANIMATION_H

#include "Model.h"
#include "Bone.h"

struct AssimpNodeData{
    glm::mat4 transformation;
    std::string name;
    uint32_t childCount;
    std::vector<AssimpNodeData> children;
};

class Animation {
public:
    Animation() = default;

    Animation(const std::string& animationPath, Model* model);
    Bone* findBone(const std::string& name);
    inline float getTicksPerSecond() const { return m_ticksPerSecond; }
    inline float getDuration() const { return m_duration; }
    inline const AssimpNodeData& getRootNode() const { return m_rootNode; }
    inline const std::unordered_map<std::string, BoneInfo>& getBoneIDMap() const { return m_boneInfoMap; }

private:
    void readMissingBones(const aiAnimation* animation, Model& model);
    void readHeirarchyData(AssimpNodeData& dest, const aiNode* src);

    float m_duration;
    float m_ticksPerSecond;
    std::vector<Bone> m_bones;
    AssimpNodeData m_rootNode;
    std::unordered_map<std::string, BoneInfo> m_boneInfoMap;
};


#endif //TECTONIC_ANIMATION_H
