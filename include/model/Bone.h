#ifndef TECTONIC_BONE_H
#define TECTONIC_BONE_H

#include <vector>
#include <string>
#include <limits>

#include <glm/mat4x4.hpp>
#include <glm/gtx/quaternion.hpp>

#include <assimp/scene.h>

#include "utils.h"

struct KeyPosition{
    glm::vec3 position;
    double timestamp;
};

struct KeyRotation{
    glm::quat orientation;
    double timestamp;
};

struct KeyScale{
    glm::vec3 scale;
    double timestamp;
};

class Bone {
public:
    Bone(std::string name, int32_t id, const aiNodeAnim* channel);
    void update(double animTime);
    uint32_t getPositionIndex(double animTime);
    uint32_t getRotationIndex(double animTime);
    uint32_t getScaleIndex(double animTime);

    glm::mat4 getLocalTransform() const { return m_localTransform; }
    const std::string& getName() const { return m_name; }
    int32_t getBoneId() const { return m_id; }
    double getLastTimestamp() const { return m_lastTimestamp; }

private:
    double getScaleFactor(double lastTimestamp, double nextTimeStamp, double animTime);
    glm::mat4 interpolatePosition(double animTime);
    glm::mat4 interpolateRotation(double animTime);
    glm::mat4 interpolateScaling(double animTime);

    std::vector<KeyPosition> m_positions;
    std::vector<KeyRotation> m_rotations;
    std::vector<KeyScale> m_scales;
    uint32_t m_positionCount = 0;
    uint32_t m_rotationCount = 0;
    uint32_t m_scalingCount = 0;

    glm::mat4 m_localTransform;
    std::string m_name;
    int32_t m_id;
    double m_lastTimestamp = -std::numeric_limits<double>::infinity();
};


#endif //TECTONIC_BONE_H
