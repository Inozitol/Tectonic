#include "Scene.h"

Logger Scene::m_logger = Logger("Scene");

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

modelIndex_t Scene::insertModel(const std::shared_ptr<Model>& model) {
    for(modelIndex_t index = std::numeric_limits<modelIndex_t>::min();; index++){
        if(!m_usedModelIndexes.contains(index)){
            m_modelMap.insert({index, model});
            m_usedModelIndexes.insert(index);

            m_logger(Logger::INFO) << "Inserted model with index " << index << '\n';
            return index;
        }
    }
}

skinnedModelIndex_t Scene::insertSkinnedModel(const std::shared_ptr<SkinnedModel> &model) {
    for(modelIndex_t index = std::numeric_limits<skinnedModelIndex_t>::min();; index++){
        if(!m_usedSkinnedModelIndexes.contains(index)){
            m_skinnedModelMap.insert({index, model});
            m_usedSkinnedModelIndexes.insert(index);

            m_logger(Logger::INFO) << "Inserted skinned model with index " << index << '\n';
            return index;
        }
    }
}

std::shared_ptr<SkinnedModel> Scene::getSkinnedModel(skinnedModelIndex_t skinnedModelIndex) {
    return m_skinnedModelMap.at(skinnedModelIndex);
}

objectIndex_t Scene::createObject(modelIndex_t modelIndex) {
    if(!m_modelMap.contains(modelIndex))
        throw sceneException("Scene doesn't contain provided mesh index");

    for(objectIndex_t index = std::numeric_limits<objectIndex_t>::min(); ; index++){
        if(!m_usedObjectIndexes.contains(index)){
            ObjectData newObject;
            newObject.index = index;
            m_objectMap.insert({index, {newObject,modelIndex}});
            m_usedObjectIndexes.insert(index);

            m_logger(Logger::INFO) << "Created object with index " << index << " from model with index " << modelIndex <<  '\n';
            return index;
        }
    }
}

skinnedObjectIndex_t Scene::createSkinnedObject(skinnedModelIndex_t skinnedModelIndex) {
    if(!m_skinnedModelMap.contains(skinnedModelIndex))
        throw sceneException("Scene doesn't contain provided mesh index");

    for(objectIndex_t index = std::numeric_limits<objectIndex_t>::min(); ; index++){
        if(!m_usedSkinnedObjectIndexes.contains(index)){
            SkinnedObjectData newObject;
            newObject.index = index;
            newObject.animator.setModel(m_skinnedModelMap.at(skinnedModelIndex).get());
            m_skinnedObjectMap.insert({index, {newObject,skinnedModelIndex}});
            m_usedSkinnedObjectIndexes.insert(index);

            m_logger(Logger::INFO) << "Created skinned object with index " << index << " from skinned model with index " << skinnedModelIndex <<  '\n';
            return index;
        }
    }
}

SkinnedObjectData &Scene::getSkinnedObject(skinnedObjectIndex_t skinnedObjectIndex) {
    return m_skinnedObjectMap.at(skinnedObjectIndex).first;
}

std::shared_ptr<Model> Scene::getModel(modelIndex_t modelIndex) {
    return m_modelMap.at(modelIndex);
}

ObjectData& Scene::getObject(objectIndex_t objectIndex) {
    return m_objectMap.at(objectIndex).first;
}

DirectionalLight& Scene::getDirectionalLight() {
    return m_dirLight;
}

spotLightIndex_t Scene::createSpotLight(){
    if(m_spotLightsCount == MAX_SPOT_LIGHTS){
        m_logger(Logger::WARNING) << "Exceeded maximum amount of spot lights: " << MAX_SPOT_LIGHTS << '\n';
        throw sceneException("Maximum amount of spot lights reached");
    }

    spotLightIndex_t spotLightIndex = m_spotLightsCount;
    m_spotLightsCount++;

    m_renderer.setSpotLightCount(m_spotLightsCount);

    m_logger(Logger::INFO) << "Created spot light at index " << spotLightIndex << '\n';

    return spotLightIndex;
}

SpotLight& Scene::getSpotLight(spotLightIndex_t spotLightIndex) {
    return m_spotLights[spotLightIndex];
}

pointLightIndex_t Scene::createPointLight(){
    if(m_pointLightsCount == MAX_POINT_LIGHTS){
        m_logger(Logger::WARNING) << "Exceeded maximum amount of point lights: " << MAX_POINT_LIGHTS << '\n';
        throw sceneException("Maximum amount of point lights reached");
    }

    pointLightIndex_t pointLightIndex = m_pointLightsCount;
    m_pointLightsCount++;

    m_renderer.setPointLightCount(m_pointLightsCount);

    m_logger(Logger::INFO) << "Created point light at index " << pointLightIndex << '\n';

    return pointLightIndex;
}

PointLight& Scene::getPointLight(pointLightIndex_t pointLightIndex) {
    return m_pointLights[pointLightIndex];
}

void Scene::renderScene() {
    for(auto& [index,object] : m_objectMap){
        m_renderer.queueModelRender(object.first, m_modelMap.at(object.second).get());
    }

    for(auto& [index, object] : m_skinnedObjectMap){
        m_renderer.queueSkinnedModelRender(object.first, m_skinnedModelMap.at(object.second).get());
    }

    m_renderer.renderQueues();
}

void Scene::handleMouseEvent(double x, double y) {
    if(m_gameCamera)
        m_gameCamera->handleMouseEvent(x, y);
}

void Scene::handleKeyEvent(int32_t key) {
    if(m_gameCamera) {
        m_gameCamera->handleKeyEvent(key);

        /*
        if(m_terrain) {
            glm::vec3 cameraPos = m_gameCamera->getPosition();

            cameraPos.y = m_terrain->hMapBaryWCoord(cameraPos.x,cameraPos.z) + 0.2f;

            m_gameCamera->setPosition(cameraPos);
        }
         */
    }
}

void Scene::insertTerrain(const std::shared_ptr<Terrain> &terrain) {
    m_terrain = terrain;
    m_renderer.setTerrainModelRender(terrain);
}

std::shared_ptr<Terrain> Scene::getTerrain() {
    return m_terrain;
}

void Scene::insertSkybox(const std::shared_ptr<Skybox> &skybox) {
    m_skybox = skybox;
    m_renderer.setSkyboxModelRender(skybox);
}

std::shared_ptr<Skybox> Scene::getSkybox() {
    return m_skybox;
}



