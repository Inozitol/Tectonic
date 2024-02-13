#ifndef TECTONIC_ANIMATOR_H
#define TECTONIC_ANIMATOR_H

#include <queue>

#include "Animation.h"
#include "SkinnedModel.h"

class Animator {
public:
    Animator();
    void setModel(const SkinnedModel* model) { m_animationModel = model; }
    void updateAnimation(float dt);
    void playAnimation(uint32_t animationIndex);
    void calcBoneTransform();
    void calcBoneTransformBlended();
    [[nodiscard]] const boneTransfoms_t& getFinalBoneMatrices() const { return m_finalBoneMatrices; }

public:
    boneTransfoms_t m_finalBoneMatrices;
    const Animation* m_currentAnimation = nullptr;
    const Animation* m_nextAnimation = nullptr;
    const SkinnedModel* m_animationModel = nullptr;

    double m_currentTime = 0.0;
    double m_nextTime = 0.0;
    float m_blendFactor = 0.0f;
    float m_blendDirection = 0.005f;
    double m_deltaTime = 0.0;
};


#endif //TECTONIC_ANIMATOR_H
