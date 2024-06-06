#ifndef TECTONIC_BONE_H
#define TECTONIC_BONE_H

#include <vector>
#include <string>
#include <limits>

#include <glm/mat4x4.hpp>
#include <glm/gtx/quaternion.hpp>

#include <assimp/scene.h>
#include <map>

#include "utils/utils.h"

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

    Bone() = default;
    void update(double animTime) const;
    void updateBlend(double animTime, const Bone* otherBone, double otherTime, float blendFactor) const;

    glm::mat4 getLocalTransform() const { return m_localTransform; }
    const std::string& getName() const { return m_name; }
    int32_t getBoneId() const { return m_id; }
    double getLastTimestamp() const { return m_lastTimestamp; }

    std::map<double, KeyPosition> m_positions;
    std::map<double, KeyRotation> m_rotations;
    std::map<double, KeyScale> m_scales;

    uint32_t m_positionCount = 0;
    uint32_t m_rotationCount = 0;
    uint32_t m_scalingCount = 0;

    mutable glm::mat4 m_localTransform = glm::mat4(1.0f);
    std::string m_name;
    int32_t m_id = -1;
    double m_lastTimestamp = -std::numeric_limits<double>::infinity();

private:

    std::map<double, KeyPosition>::const_iterator getPositionIndex(double animTime) const;
    std::map<double, KeyRotation>::const_iterator getRotationIndex(double animTime) const;
    std::map<double, KeyScale>::const_iterator getScaleIndex(double animTime) const;
    inline static double getScaleFactor(double lastTimestamp, double nextTimeStamp, double animTime) ;
    glm::vec3 interpolatePosition(double animTime) const;
    glm::quat interpolateRotation(double animTime) const;
    glm::vec3 interpolateScaling(double animTime) const;

};


#endif //TECTONIC_BONE_H
