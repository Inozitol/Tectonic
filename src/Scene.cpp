#include "Scene.h"

Scene::Scene() {
    m_renderer.setDirectionalLight(&m_dirLight);
    m_renderer.setSpotLight(&m_spotLights);
    m_renderer.setPointLight(&m_pointLights);
}

void Scene::setGameCamera(const std::shared_ptr<GameCamera>& gameCamera) {
    m_gameCamera = gameCamera;
    m_renderer.setGameCamera(m_gameCamera);
}

std::shared_ptr<GameCamera> Scene::getGameCamera() {
    return m_gameCamera;
}

void Scene::setWindowDimension(std::pair<int32_t, int32_t> dimensions) {
    m_winDimensions = dimensions;
    m_pickingTexture.init(dimensions.first, dimensions.second);
    m_renderer.setWindowSize(dimensions.first, dimensions.second);
}

meshIndex_t Scene::insertMesh(const std::shared_ptr<Model>& mesh) {
    for(meshIndex_t index = std::numeric_limits<meshIndex_t>::min();;index++){
        if(!m_usedMeshIndexes.contains(index)){
            m_modelMap.insert({index, mesh});
            m_usedMeshIndexes.insert(index);
            return index;
        }
    }
}

objectIndex_t Scene::createObject(meshIndex_t modelIndex) {
    if(!m_modelMap.contains(modelIndex))
        throw sceneException("Scene doesn't contain provided mesh index");

    for(objectIndex_t index = std::numeric_limits<objectIndex_t>::min(); ; index++){
        if(!m_usedObjectIndexes.contains(index)){
            ObjectData newObject;
            newObject.index = index;
            newObject.animator.setModel(m_modelMap.at(modelIndex).get());
            m_objectMap.insert({index, {newObject,modelIndex}});
            m_usedObjectIndexes.insert(index);
            return index;
        }
    }
}

std::shared_ptr<Model> Scene::getMesh(meshIndex_t meshIndex) {
    return m_modelMap.at(meshIndex);
}

ObjectData& Scene::getObject(objectIndex_t objectIndex) {
    return m_objectMap.at(objectIndex).first;
}

DirectionalLight& Scene::getDirectionalLight() {
    return m_dirLight;
}

spotLightIndex_t Scene::createSpotLight(){
    if(m_spotLightsCount == MAX_SPOT_LIGHTS){
        throw sceneException("Maximum amount of spot lights reached");
    }

    spotLightIndex_t spotLightIndex = m_spotLightsCount;
    m_spotLightsCount++;

    m_renderer.setSpotLightCount(m_spotLightsCount);

    return spotLightIndex;
}

SpotLight& Scene::getSpotLight(spotLightIndex_t spotLightIndex) {
    return m_spotLights[spotLightIndex];
}

pointLightIndex_t Scene::createPointLight(){
    if(m_pointLightsCount == MAX_POINT_LIGHTS){
        throw sceneException("Maximum amount of point lights reached");
    }

    pointLightIndex_t pointLightIndex = m_pointLightsCount;
    m_pointLightsCount++;

    m_renderer.setPointLightCount(m_pointLightsCount);

    return pointLightIndex;
}

PointLight& Scene::getPointLight(pointLightIndex_t pointLightIndex) {
    return m_pointLights[pointLightIndex];
}

void Scene::renderScene() {
    for(auto& [index,object] : m_objectMap){
        m_renderer.queueRender(object.first, m_modelMap.at(object.second).get());
    }

    m_renderer.renderQueue();
}

void Scene::handleMouseEvent(double x, double y) {
    if(m_gameCamera)
        m_gameCamera->handleMouseEvent(x, y);
}

void Scene::handleKeyEvent(int32_t key) {
    if(m_gameCamera)
        m_gameCamera->handleKeyEvent(key);
}



