#ifndef TECTONIC_ANIMATOR_H
#define TECTONIC_ANIMATOR_H

#include "Animation.h"

class Animator {
public:
    explicit Animator(Animation* animation);
    void updateAnimation(float dt);
    void playAnimation(Animation* animation);
    void calcBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform);
    std::vector<glm::mat4> getFinalBoneMatrices(){ return m_finalBoneMatrices; }

public:
    std::vector<glm::mat4> m_finalBoneMatrices;
    Animation* m_currentAnimation = nullptr;
    float m_currentTime = 0.0f;
    float m_deltaTime = 0.0f;
};


#endif //TECTONIC_ANIMATOR_H
