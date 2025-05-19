#pragma once

#include "engine/scene/model/Model.h"
#include "engine/TecCore.h"
#include "engine/scene/terrain/Terrain.h"
#include "engine/scene/Skybox.h"
#include "utils/IndexableStorage.h"
#include "engine/scene/RenderableBuffer.h"

struct World {
    World();
    ~World() = default;

    void clear() const;

    std::vector<Model> models{};
    Terrain *terrain = nullptr;
    Skybox *skybox = nullptr;
    Transformation transformation{};

    VktTypes::GPU::SceneData sceneData;

    void updateScene();

    using objectID_t = uint32_t;

    /**
     * @brief Represents an objects created from 3D model.
     */
    struct WorldObject_t {
        std::string name;
        Model model;
    };

    RenderableBuffer<WorldObject_t, VktTypes::RigidRenderObject, objectID_t> rigidObjects{
            [](const WorldObject_t &obj) { return obj.model.renderables(); },
            [](const WorldObject_t &obj, VktTypes::DrawContext<VktTypes::RigidRenderObject> &ctx, const std::size_t offset) { return obj.model.updateDrawContextFromOffset(ctx, offset); }
    };
    RenderableBuffer<WorldObject_t, VktTypes::SkinnedRenderObject, objectID_t> skinnedObjects{
            [](const WorldObject_t &obj) { return obj.model.renderables(); },
            [](const WorldObject_t &obj, VktTypes::DrawContext<VktTypes::SkinnedRenderObject> &ctx, const std::size_t offset) { return obj.model.updateDrawContextFromOffset(ctx, offset); }
    };

    /*
    IndexableStorage<WorldObject_t> rigidObjects;
    IndexableStorage<std::size_t> renderRigidObjectOffsets;
    VktTypes::DrawContext<VktTypes::RigidRenderObject> rigidRenderObjects;

    std::vector<VktTypes::SkinnedRenderObject> skinnedRenderObjects;

    void syncRigidRenderOffsetsFromId(objectID_t id) {
        if(id == rigidObjects.unknown) return;
        const objectID_t after = rigidObjects.after(id);
        if(after == rigidObjects.unknown) return;
        std::size_t offset = renderRigidObjectOffsets.data(id);
        for(auto iter = rigidObjects.at(id); iter != rigidObjects.end(); ++iter) {
            auto [objID, obj] = *iter;
            renderRigidObjectOffsets.storeDataAt(offset, objID, false);
            offset += rigidObjects.data(objID).model->renderables();
        }
    }

    Slot<objectID_t> slt_objectStored{[this](objectID_t id) {
        const objectID_t before = rigidObjects.before(id);
        Model* model = rigidObjects.data(id).model;
        assert(model);
        if(before == rigidObjects.unknown) {
            renderRigidObjectOffsets.storeDataAt(0, id, false);
        }else {
            const std::size_t offsetBefore = renderRigidObjectOffsets.data(before);
            renderRigidObjectOffsets.storeDataAt(offsetBefore + model->renderables(), id, false);
        }
        syncRigidRenderOffsetsFromId(id);
        updateRenderableById(id);
    }};

    Slot<objectID_t> slt_objectErased{[this](objectID_t id) {
        renderRigidObjectOffsets.erase(id);
        syncRigidRenderOffsetsFromId(rigidObjects.before(id));
    }};

    Slot<objectID_t> slt_objectChanged{[this](objectID_t id) {
        updateRenderableById(id);
    }};
*/
    /*
    DirectionalLight                            m_dirLight;
    std::array<SpotLight, MAX_SPOT_LIGHTS>      m_spotLights;
    decltype(MAX_SPOT_LIGHTS)                   m_spotLightsCount;
    std::array<PointLight, MAX_POINT_LIGHTS>    m_pointLights;
    decltype(MAX_POINT_LIGHTS)                  m_pointLightsCount;
    */

};