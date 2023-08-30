#ifndef TECTONIC_ANIMATION_H
#define TECTONIC_ANIMATION_H

#include <unordered_map>

#include "Bone.h"
#include "ModelTypes.h"

class Animation {
public:
    explicit Animation(const aiAnimation* animation);
    Bone* findBone(const std::string& name);
    void insertBone(const Bone& bone);
    [[nodiscard]] inline double getTicksPerSecond() const { return m_ticksPerSecond; }
    [[nodiscard]] inline double getDuration() const { return m_duration; }

private:
    double m_duration = 0.0;
    double m_ticksPerSecond = 0.0;
    std::vector<Bone> m_bones;
};


#endif //TECTONIC_ANIMATION_H