#ifndef TECTONIC_ANIMATOR_H
#define TECTONIC_ANIMATOR_H

#include "Animation.h"
#include "Model.h"

class Animator {
public:
    Animator();
    explicit Animator(Animation* animation);
    void setModel(const Model* model) { m_animationModel = model; }
    void updateAnimation(float dt);
    void playAnimation(Animation* animation);
    void calcBoneTransform(const NodeData* node, glm::mat4 parentTransform);
    const std::vector<glm::mat4>& getFinalBoneMatrices() const { return m_finalBoneMatrices; }

public:
    std::vector<glm::mat4> m_finalBoneMatrices;
    Animation* m_currentAnimation = nullptr;
    const Model* m_animationModel = nullptr;

    double m_currentTime = 0.0f;
    double m_deltaTime = 0.0f;
};


#endif //TECTONIC_ANIMATOR_H
