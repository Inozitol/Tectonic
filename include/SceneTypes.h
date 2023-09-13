#ifndef TECTONIC_SCENETYPES_H
#define TECTONIC_SCENETYPES_H

#include "Transformation.h"
#include "model/anim/Animation.h"
#include "model/anim/Animator.h"
#include "model/Model.h"
#include "meta/Slot.h"

using modelIndex_t = uint32_t;
using skinnedModelIndex_t = uint32_t;
using objectIndex_t = uint32_t;
using skinnedObjectIndex_t = uint32_t;
using spotLightIndex_t = uint32_t;
using pointLightIndex_t = uint32_t;

struct ObjectData{
    objectIndex_t index{};
    Transformation transformation;
    glm::vec4 colorMod = {1.0f, 1.0f, 1.0f, 1.0f};

    void clicked(){
        static bool isColored = false;

        if(!isColored)
            colorMod = {1.0f, 0.0f, 0.0f, 1.0f};
        else
            colorMod = {1.0f, 1.0f, 1.0f, 1.0f};

        isColored = !isColored;
    };
};

struct SkinnedObjectData : ObjectData{
    Animator animator;
};

struct Drawable{
    ObjectData object;
    const Model* model = nullptr;
    const MeshInfo* mesh = nullptr;
};

struct SkinnedDrawable{
    SkinnedObjectData object;
    const SkinnedModel* skinnedModel = nullptr;
    const MeshInfo* mesh = nullptr;
};

#endif //TECTONIC_SCENETYPES_H
