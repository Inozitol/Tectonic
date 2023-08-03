#ifndef TECTONIC_BONE_H
#define TECTONIC_BONE_H

#include <vector>
#include <string>

#include <glm/mat4x4.hpp>
#include <glm/gtx/quaternion.hpp>

#include <assimp/scene.h>

#include "utils.h"

struct KeyPosition{
    glm::vec3 position;
    float timestamp;
};

struct KeyRotation{
    glm::quat orientation;
    float timestamp;
};

struct KeyScale{
    glm::vec3 scale;
    float timestamp;
};

class Bone {
public:
    Bone(const std::string& name, int32_t id, const aiNodeAnim* channel);
    void update(float animTime);
    uint32_t getPositionIndex(float animTime);
    uint32_t getRotationIndex(float animTime);
    uint32_t getScaleIndex(float animTime);

    glm::mat4 getLocalTransform() { return m_localTransform; }
    std::string getName() const { return m_name; }
    int32_t getBoneId() { return m_id; }


private:
    float getScaleFactor(float lastTimestamp, float nextTimeStamp, float animTime);
    glm::mat4 interpolatePosition(float animTime);
    glm::mat4 interpolateRotation(float animTime);
    glm::mat4 interpolateScaling(float animTime);

    std::vector<KeyPosition> m_positions;
    std::vector<KeyRotation> m_rotations;
    std::vector<KeyScale> m_scales;
    uint32_t m_positionCount;
    uint32_t m_rotationCount;
    uint32_t m_scalingCount;

    glm::mat4 m_localTransform;
    std::string m_name;
    int32_t m_id;
};


#endif //TECTONIC_BONE_H
