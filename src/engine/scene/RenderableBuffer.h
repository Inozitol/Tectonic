#pragma once
#include "engine/vulkan/VktTypes.h"
#include "utils/IndexableStorage.h"

template<typename ObjectType, typename Renderable, typename ObjectIndex = uint32_t>
    requires VktConstraints::IsRenderable<Renderable> &&
             std::is_unsigned_v<ObjectIndex> &&
             std::is_integral_v<ObjectIndex>
struct RenderableBuffer {

    using renderCountF_t = std::function<std::size_t(const ObjectType&)>;
    using renderUpdateF_t = std::function<void(const ObjectType&, VktTypes::DrawContext<Renderable>&, std::size_t)>;
    using objStoreF_t = std::function<void(ObjectType&, ObjectIndex)>;

    renderCountF_t renderablesCountFunc;
    renderUpdateF_t renderableUpdateFunc;
    objStoreF_t storeFunc;

    RenderableBuffer() = delete;

    /** The RenderableBuffer has the following mandatory callbacks in the constructor input:
     *    renderCountF_t renderablesCountFunc = Takes the ObjectType and has to the number of renderables that object contains.
     *    renderUpdateF_t renderableUpdateFunc = Takes an ObjectType, DrawContext and offset, and has to create new Renderable on offset of the draw context.
     */
    RenderableBuffer(renderCountF_t renderablesCountFunc, renderUpdateF_t renderableUpdateFunc)
    : renderablesCountFunc(renderablesCountFunc), renderableUpdateFunc(renderableUpdateFunc) {
        assert(renderablesCountFunc);
        assert(renderableUpdateFunc);

        objects.sig_stored.connect(slt_objectCreated);
        objects.sig_erased.connect(slt_objectErased);
    }

    void updateRenderableById(ObjectIndex id) {
        renderableUpdateFunc(objects.data(id), renderObjects, renderObjectsOffsets.data(id));
    }

    void syncRigidRenderOffsetsFromId(ObjectIndex id) {
        if(id == objects.unknown) return;
        const ObjectIndex after = objects.after(id);
        if(after == objects.unknown) return;
        std::size_t offset = renderObjectsOffsets.data(id);
        for(auto iter = objects.at(id); iter != objects.end(); ++iter) {
            auto [objID, obj] = *iter;
            renderObjectsOffsets.storeDataAt(offset, objID, false);
            offset += renderablesCountFunc(objects.data(objID));
        }
    }

    /** Called on object that's been just changed.
     * Same as slt_objectChanged. */
    void objectChanged(ObjectIndex id) {
        updateRenderableById(id);
    }

    IndexableStorage<ObjectType, ObjectIndex> objects;
    IndexableStorage<std::size_t, ObjectIndex> renderObjectsOffsets;
    VktTypes::DrawContext<Renderable> renderObjects;


    /** Should be called on object that's been changed.
     * Same as objectChanged. */
    Slot<ObjectIndex> slt_objectChanged{[this](ObjectIndex id) {
        objectChanged(id);
    }};

private:

    /** Called on object that's been stored in objects. */
    Slot<ObjectIndex> slt_objectCreated{[this](ObjectIndex id) {
        const ObjectIndex before = objects.before(id);
        if(before == objects.unknown) {
            renderObjectsOffsets.storeDataAt(0, id, false);
        }else {
            const std::size_t offsetBefore = renderObjectsOffsets.data(before);
            renderObjectsOffsets.storeDataAt(offsetBefore + renderablesCountFunc(objects.data(id)), id, false);
        }
        syncRigidRenderOffsetsFromId(id);
        updateRenderableById(id);

        if(storeFunc) storeFunc(objects.data(id), id);
    }};

    /** Should be called on object that's been erased from objects. */
    Slot<ObjectIndex> slt_objectErased{[this](ObjectIndex id) {
        renderObjectsOffsets.erase(id);
        syncRigidRenderOffsetsFromId(objects.before(id));
    }};
};