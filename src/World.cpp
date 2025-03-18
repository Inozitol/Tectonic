#include "World.h"

#include "engine/GlobalMemory.h"

void World::clear() {
    if(terrain) terrain->clear();
    if(skybox) skybox->clear();
}

void World::updateScene() {
    sceneData.view = TecCorePtr->gameCamera->viewMatrix;
    sceneData.proj = TecCorePtr->gameCamera->projectionMatrix;

    sceneData.proj[1][1] *= -1;
    sceneData.viewproj = sceneData.proj * sceneData.view;
    sceneData.ambientColor = glm::vec3(0.1f);
    sceneData.sunlightColor = glm::vec3(1.0f);
    const glm::vec4 sunPos = glm::rotate(glm::identity<glm::mat4>(), glm::radians(TecCorePtr->currTime * 50.f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    sceneData.sunlightDirection = glm::vec3(sunPos - glm::vec4(0.0f));
    sceneData.cameraPosition = TecCorePtr->gameCamera->position;
    sceneData.cameraDirection = TecCorePtr->gameCamera->getDirection();
    sceneData.time = TecCorePtr->currTime;
}

void World::gatherDrawContext(VktTypes::DrawContext &ctx) const {
    // Gather Terrain context
    auto renderIter = terrain->renderIter();
    while(renderIter) {
        ctx.opaqueSurfaces.push_back(*renderIter);
        ++renderIter;
    }
}