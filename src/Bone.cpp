
#include "model/anim/Bone.h"

void Bone::update(double animTime) const{
    glm::vec3 translation = interpolatePosition(animTime);
    glm::quat rotation = interpolateRotation(animTime);
    glm::vec3 scale = interpolateScaling(animTime);
    m_localTransform = glm::translate(glm::mat4(1.0f),translation) * glm::toMat4(rotation) * glm::scale(glm::mat4(1.0f),scale);
}

void Bone::updateBlend(double animTime, const Bone* otherBone, double otherTime, float blendFactor) const{
    glm::vec3 translation = interpolatePosition(animTime);
    glm::quat rotation = interpolateRotation(animTime);
    glm::vec3 scale = interpolateScaling(animTime);

    glm::vec3 otherTranslation = otherBone->interpolatePosition(otherTime);
    glm::quat otherRotation = otherBone->interpolateRotation(otherTime);
    glm::vec3 otherScale = otherBone->interpolateScaling(otherTime);

    glm::vec3 blendedTranslation = (1.0f - blendFactor) * translation + otherTranslation * blendFactor;
    glm::quat blendedRotation = glm::slerp(rotation, otherRotation, blendFactor);
    glm::vec3 blendedScale = (1.0f - blendFactor) * scale + otherScale * blendFactor;

    m_localTransform = glm::translate(glm::mat4(1.0f),blendedTranslation) * glm::toMat4(blendedRotation) * glm::scale(glm::mat4(1.0f),blendedScale);
}

std::map<double, KeyPosition>::const_iterator Bone::getPositionIndex(double animTime) const{
    const auto it = m_positions.lower_bound(animTime);
    if(it == --m_positions.end()){
        return std::prev(m_positions.end(),2);
    }
    return it;
}

std::map<double, KeyRotation>::const_iterator Bone::getRotationIndex(double animTime) const{
    const auto it = m_rotations.lower_bound(animTime);
    if(it == --m_rotations.end()){
        return std::prev(m_rotations.end(),2);
    }
    return it;}

std::map<double, KeyScale>::const_iterator Bone::getScaleIndex(double animTime) const{
    const auto it = m_scales.lower_bound(animTime);
    if(it == --m_scales.end()){
        return std::prev(m_scales.end(),2);
    }
    return it;}

double Bone::getScaleFactor(double lastTimestamp, double nextTimeStamp, double animTime) {
    double scaleFactor;
    double midWayLen = animTime - lastTimestamp;
    double framesDiff = nextTimeStamp - lastTimestamp;
    scaleFactor = midWayLen / framesDiff;
    return scaleFactor;
}

glm::vec3 Bone::interpolatePosition(double animTime) const{
    if(1 == m_positionCount)
        return  m_positions.begin()->second.position;

    const auto p0Index = getPositionIndex(animTime);
    const auto p1Index = std::next(p0Index);
    double scaleFactor = getScaleFactor(p0Index->first, p1Index->first, animTime);
    glm::vec3 finalPosition = glm::mix(p0Index->second.position, p1Index->second.position, scaleFactor);
    return finalPosition;
}

glm::quat Bone::interpolateRotation(double animTime) const{
    if(1 == m_rotationCount) {
        return glm::normalize(m_rotations.begin()->second.orientation);
    }

    const auto p0Index = getRotationIndex(animTime);
    const auto p1Index = std::next(p0Index);
    float scaleFactor = getScaleFactor(p0Index->first, p1Index->first, animTime);
    glm::quat finalRotation = glm::slerp(p0Index->second.orientation, p1Index->second.orientation, scaleFactor);
    return finalRotation;
}

glm::vec3 Bone::interpolateScaling(double animTime) const{
    if(1 == m_scalingCount)
        return m_scales.begin()->second.scale;

    const auto p0Index = getScaleIndex(animTime);
    const auto p1Index = std::next(p0Index);
    double scaleFactor = getScaleFactor(p0Index->first, p1Index->first, animTime);
    glm::vec3 finalScale = glm::mix(p0Index->second.scale, p1Index->second.scale, scaleFactor);
    return finalScale;
}