#include "Animation.h"

Animation::Animation(const std::string &animationPath, Model *model) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate);
    assert(scene && scene->mRootNode);
    auto animation = scene->mAnimations[0];
    m_duration = animation->mDuration;
    m_ticksPerSecond = animation->mTicksPerSecond;
    readHeirarchyData(m_rootNode, scene->mRootNode);
    readMissingBones(animation, *model);
}

Bone *Animation::findBone(const std::string &name) {
    auto iter = std::find_if(m_bones.begin(), m_bones.end(),
                             [&](const Bone& bone){
        return bone.getName() == name;
    });
    if(iter == m_bones.end()) return nullptr;
    else return &(*iter);
}

void Animation::readMissingBones(const aiAnimation *animation, Model &model) {
    uint32_t size = animation->mNumChannels;

    auto& boneInfoMap = model.getBoneInfoMap();
    int& boneCount = model.getBoneCount();
    for(uint32_t i = 0; i < size; i++){
        auto channel = animation->mChannels[i];
        std::string boneName = channel->mNodeName.C_Str();
        if(boneInfoMap.find(boneName) == boneInfoMap.end()){
            boneInfoMap[boneName].id = boneCount;
            boneCount++;
        }
        m_bones.emplace_back(channel->mNodeName.C_Str(), boneInfoMap[channel->mNodeName.C_Str()].id, channel);
    }
    m_boneInfoMap = boneInfoMap;
}

void Animation::readHeirarchyData(AssimpNodeData &dest, const aiNode *src) {
    assert(src);
    dest.name = src->mName.C_Str();
    dest.transformation = Utils::aiMatToGLM(src->mTransformation);
    dest.childCount = src->mNumChildren;

    for(int i = 0; i < src->mNumChildren; i++){
        AssimpNodeData newData;
        readHeirarchyData(newData, src->mChildren[i]);
        dest.children.push_back(newData);
    }
}