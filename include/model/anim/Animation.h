#ifndef TECTONIC_ANIMATION_H
#define TECTONIC_ANIMATION_H

#include <unordered_map>

#include "Bone.h"
#include "model/ModelTypes.h"

class Animation {
public:
    Animation(double duration, double ticksPerSec);
    const Bone* findBone(int32_t boneID) const;
    const Bone* findBone(const std::string& boneName) const;
    void insertBone(const Bone& bone);
    [[nodiscard]] inline double getTicksPerSecond() const { return m_ticksPerSecond; }
    [[nodiscard]] inline double getDuration() const { return m_duration; }

private:
    double m_duration = 0.0;
    double m_ticksPerSecond = 0.0;
    std::unordered_map<int32_t, Bone> m_bones;
    std::vector<Bone> m_boneVec;
};

#endif //TECTONIC_ANIMATION_H