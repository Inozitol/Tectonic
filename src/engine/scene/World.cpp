#include "World.h"

#include "../GlobalMemory.h"

#include <ranges>

World::World() {
    rigidObjects.storeFunc = [this](WorldObject_t& obj, const objectID_t id) {
        obj.model.sig_change.connect(rigidObjects.slt_objectChanged);
        obj.model.id = id;
    };
    skinnedObjects.storeFunc = [this](WorldObject_t& obj, const objectID_t id) {
        obj.model.sig_change.connect(skinnedObjects.slt_objectChanged);
        obj.model.id = id;
    };
}

void World::clear() const {
    LOG(LOG_INFO, "Clearing world resources");
    if(terrain) terrain->clear();
    if(skybox) skybox->clear();
}

void World::updateScene() {
    sceneData.view = PlayerPtr->camera.viewMatrix;
    sceneData.proj = PlayerPtr->camera.projectionMatrix;

    sceneData.proj[1][1] *= -1;
    sceneData.viewproj = sceneData.proj * sceneData.view;
    sceneData.ambientColor = glm::vec3(0.1f);
    sceneData.sunlightColor = glm::vec3(1.0f);
    const glm::vec4 sunPos = glm::rotate(glm::identity<glm::mat4>(), glm::radians(TecCorePtr->currTime * 50.f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    sceneData.sunlightDirection = glm::vec3(sunPos - glm::vec4(0.0f));
    sceneData.cameraPosition = PlayerPtr->camera.position;
    sceneData.cameraDirection = PlayerPtr->camera.getDirection();
    sceneData.time = TecCorePtr->currTime;

    for(auto [id,obj] : skinnedObjects.objects) {
        if(obj.model.currentAnimation() != ModelTypes::NULL_ID) {
            obj.model.updateAnimationTime();
            obj.model.updateJoints();
        }
    }
}
/*
void World::gatherDrawContext(VktTypes::DrawContext &ctx) {
    // Gather Terrain context
    auto renderIter = terrain->renderIter();
    while(renderIter) {
        ctx.opaqueSurfaces.push_back(*renderIter);
        ++renderIter;
    }

    for(auto [id,obj] : objects) {
        obj.model->gatherDrawContext(VktCorePtr->mainDrawContext);
    }
}*/