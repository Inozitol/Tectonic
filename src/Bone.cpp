#include "Bone.h"

Bone::Bone(const std::string &name, int32_t id, const aiNodeAnim *channel)
    :m_name(name), m_id(id), m_localTransform(1.0f){

    m_positionCount = channel->mNumPositionKeys;
    for(uint32_t posIndex = 0; posIndex < m_positionCount; posIndex++){
        aiVector3D aiPos = channel->mPositionKeys[posIndex].mValue;
        float timestamp = channel->mPositionKeys[posIndex].mTime;
        KeyPosition key{
            Utils::aiVecToGLM(aiPos),
            timestamp
        };
        m_positions.push_back(key);
    }

    m_rotationCount = channel->mNumRotationKeys;
    for(uint32_t rotIndex = 0; rotIndex < m_rotationCount; rotIndex++){
        aiQuaternion aiOrient = channel->mRotationKeys[rotIndex].mValue;
        float timestamp = channel->mRotationKeys[rotIndex].mTime;
        KeyRotation key{
            Utils::aiQuatToGLM(aiOrient),
            timestamp
        };
        m_rotations.push_back(key);
    }

    m_scalingCount = channel->mNumScalingKeys;
    for(uint32_t scaleIndex = 0; scaleIndex < m_scalingCount; scaleIndex++){
        aiVector3D aiScale = channel->mScalingKeys[scaleIndex].mValue;
        float timestamp = channel->mScalingKeys[scaleIndex].mTime;
        KeyScale key{
            Utils::aiVecToGLM(aiScale),
            timestamp
        };
        m_scales.push_back(key);
    }
}

void Bone::update(float animTime) {
    glm::mat4 translation = interpolatePosition(animTime);
    glm::mat4 rotation = interpolateRotation(animTime);
    glm::mat4 scale = interpolateScaling(animTime);
    m_localTransform = translation * rotation * scale;
}

uint32_t Bone::getPositionIndex(float animTime) {
    for(uint32_t index = 0; index < m_positionCount - 1; index++){
        if(animTime < m_positions[index + 1].timestamp)
            return index;
    }
    assert(false);
}
uint32_t Bone::getRotationIndex(float animTime) {
    for(uint32_t index = 0; index < m_rotationCount - 1; index++){
        if(animTime < m_rotations[index + 1].timestamp)
            return index;
    }
    assert(false);
}

uint32_t Bone::getScaleIndex(float animTime) {
    for(uint32_t index = 0; index < m_scalingCount- 1; index++){
        if(animTime < m_scales[index + 1].timestamp)
            return index;
    }
    assert(false);
}

float Bone::getScaleFactor(float lastTimestamp, float nextTimeStamp, float animTime) {
    float scaleFactor = 0.0f;
    float midWayLen = animTime - lastTimestamp;
    float framesDiff = nextTimeStamp - lastTimestamp;
    scaleFactor = midWayLen / framesDiff;
    return scaleFactor;
}

glm::mat4 Bone::interpolatePosition(float animTime) {
    if(1 == m_positionCount)
        return glm::translate(glm::mat4(1.0f), m_positions[0].position);

    uint32_t p0Index = getPositionIndex(animTime);
    uint32_t p1Index = p0Index + 1;
    float scaleFactor = getScaleFactor(m_positions[p0Index].timestamp, m_positions[p1Index].timestamp, animTime);
    glm::vec3 finalPosition = glm::mix(m_positions[p0Index].position, m_positions[p1Index].position, scaleFactor);
    return glm::translate(glm::mat4(1.0f), finalPosition);
}

glm::mat4 Bone::interpolateRotation(float animTime) {
    if(1 == m_rotationCount) {
        auto rotation = glm::normalize(m_rotations[0].orientation);
        return glm::toMat4(rotation);
    }

    uint32_t p0Index = getRotationIndex(animTime);
    uint32_t p1Index = p0Index + 1;
    float scaleFactor = getScaleFactor(m_rotations[p0Index].timestamp, m_rotations[p1Index].timestamp, animTime);
    glm::quat finalRotation = glm::slerp(m_rotations[p0Index].orientation, m_rotations[p1Index].orientation, scaleFactor);
    return glm::toMat4(finalRotation);
}

glm::mat4 Bone::interpolateScaling(float animTime) {
    if(1 == m_scalingCount)
        return glm::scale(glm::mat4(1.0f), m_scales[0].scale);

    uint32_t p0Index = getScaleIndex(animTime);
    uint32_t p1Index = p0Index + 1;
    float scaleFactor = getScaleFactor(m_scales[p0Index].timestamp, m_scales[p1Index].timestamp, animTime);
    glm::vec3 finalScale = glm::mix(m_scales[p0Index].scale, m_scales[p1Index].scale, scaleFactor);
    return glm::scale(glm::mat4(1.0f), finalScale);
}