#include "Scene.h"

void Scene::setLightingShader(const std::shared_ptr<LightingShader>& lightingShader) {
    m_lightingShader = lightingShader;
}

void Scene::setShadowShader(const std::shared_ptr<ShadowMapShader>& shadowShader) {
    m_shadowShader = shadowShader;
}

void Scene::setShadowMapFBO(const std::shared_ptr<ShadowMapFBO>& shadowMapFBO) {
    m_shadowMapFBO = shadowMapFBO;
}

void Scene::setShadowCubeMapFBO(const std::shared_ptr<ShadowCubeMapFBO>& shadowCubeMapFBO) {
    m_shadowCubeMapFBO = shadowCubeMapFBO;
}

void Scene::setGameCamera(const std::shared_ptr<GameCamera>& gameCamera) {
    m_gameCamera = gameCamera;
}

std::shared_ptr<GameCamera> Scene::getGameCamera() {
    return m_gameCamera;
}

void Scene::setWindowDimension(std::pair<int32_t, int32_t> dimensions) {
    m_winDimensions = dimensions;
}

Scene::meshIndex_t Scene::insertMesh(const std::shared_ptr<Model>& mesh) {
    for(meshIndex_t index = std::numeric_limits<meshIndex_t>::min();;index++){
        if(!m_usedMeshIndexes.contains(index)){
            m_meshMap.insert({index, mesh});
            m_usedMeshIndexes.insert(index);
            return index;
        }
    }
}

Scene::objectIndex_t Scene::createObject(Scene::meshIndex_t meshIndex) {
    if(!m_meshMap.contains(meshIndex))
        throw sceneException("Scene doesn't contain provided mesh index");

    for(objectIndex_t index = std::numeric_limits<objectIndex_t>::min(); ; index++){
        if(!m_usedObjectIndexes.contains(index)){
            m_objectMap.insert({index, {{Transformation(),Animation()}, meshIndex}});
            m_usedObjectIndexes.insert(index);
            return index;
        }
    }
}

std::shared_ptr<Model> Scene::getMesh(Scene::meshIndex_t meshIndex) {
    return m_meshMap.at(meshIndex);
}

ObjectData & Scene::getObject(Scene::objectIndex_t objectIndex) {
    return m_objectMap.at(objectIndex).first;
}

void Scene::insertDirectionalLight(std::shared_ptr <DirectionalLight> dirLight) {
    m_dirLight = std::move(dirLight);
}

std::shared_ptr<DirectionalLight> Scene::getDirectionalLight() {
    return m_dirLight;
}

void Scene::eraseDirectionalLight() {
    m_dirLight.reset();
}

Scene::spotLightIndex_t Scene::insertSpotLight(const std::shared_ptr <SpotLight>& spotLight) {
    for(spotLightIndex_t index = 0; index < MAX_SPOT_LIGHTS; index++){
        if(!m_spotLights[index]) {
            m_spotLights[index] = spotLight;
            return index;
        }
    }
    throw sceneException("Scene doesn't allow more spot lights");
}

std::shared_ptr<SpotLight> Scene::getSpotLight(Scene::spotLightIndex_t spotLightIndex) {
    return m_spotLights[spotLightIndex];
}

void Scene::eraseSpotLight(Scene::spotLightIndex_t spotLightIndex) {
    m_spotLights[spotLightIndex].reset();
}

Scene::pointLightIndex_t Scene::insertPointLight(const std::shared_ptr <PointLight>& pointLight) {
    for(pointLightIndex_t index = 0; index < MAX_POINT_LIGHTS; index++){
        if(!m_pointLights[index]){
            m_pointLights[index] = pointLight;
            return index;
        }
    }
    throw sceneException("Scene doesn't allow more point lights");
}

std::shared_ptr<PointLight> Scene::getPointLight(Scene::pointLightIndex_t pointLightIndex) {
    return m_pointLights[pointLightIndex];
}

void Scene::erasePointLight(Scene::pointLightIndex_t pointLightIndex) {
    m_pointLights[pointLightIndex].reset();
}

void Scene::renderScene() {
    //shadowPass();
    lightingPass();
}

void Scene::shadowPass() {
    m_shadowMapFBO->bind4writing();
    glClear(GL_DEPTH_BUFFER_BIT);
    m_shadowShader->enable();
    glCullFace(GL_FRONT);
    m_gameCamera->createView();
    m_dirLight->updateTightOrthoProjection(*m_gameCamera);
    m_gameCamera->setOrthographicInfo(m_dirLight->shadowOrthoInfo);
    m_dirLight->createView();

    for(auto &object : m_objectMap){
        std::shared_ptr<Model> mesh = m_meshMap.at(object.second.second);
        Transformation& transformation = object.second.first.transformation;
        renderMeshShadow(mesh, transformation);
    }
}

void Scene::lightingPass() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0,0,m_winDimensions.first,m_winDimensions.second);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_lightingShader->enable();

    glCullFace(GL_BACK);

    m_shadowMapFBO->bind4reading(SHADOW_TEXTURE_UNIT);
    //m_shadowCubeMapFBO->bind4reading(SHADOW_CUBE_MAP_TEXTURE_UNIT);
    m_gameCamera->createView();


    for(auto &object : m_objectMap){
        std::shared_ptr<Model> mesh = m_meshMap.at(object.second.second);
        Transformation& transformation = object.second.first.transformation;
        renderMeshLight(mesh, transformation);
    }
}

void Scene::renderMeshLight(const std::shared_ptr<Model>& mesh, Transformation &transformation) {
    glm::mat4 mashMatrix = transformation.getMatrix();

    // Camera point of view
    glm::mat4 wvp = m_gameCamera->getWVP(mashMatrix);
    m_lightingShader->setWVP(wvp);
    m_lightingShader->setWorld(mashMatrix);

    // Light point of view
    glm::mat4 light_wvp = m_dirLight->getWVP(mashMatrix);
    m_lightingShader->setLightWVP(light_wvp);

    // Setup dir light
    m_dirLight->calcLocalDirection(transformation);
    m_lightingShader->setDirectionalLight(*m_dirLight);

    m_lightingShader->setLocalCameraPos(transformation.invertPosition(m_gameCamera->getPosition()));

    for(auto& spotLight : m_spotLights){
        if(spotLight)
            spotLight->calcLocalDirectionPosition(transformation);
    }

    //m_lightingShader->setSpotLights(1, m_spotLights);

    m_lightingShader->setMaterial(mesh->material());

    //std::vector<glm::mat4> transforms;
    //mesh->getBoneTransforms(transforms);
    //for(uint32_t i = 0; i < transforms.size(); i++){
    //    m_lightingShader->setBoneTransform(i, transforms[i]);
    //}

    mesh->render();
}

void Scene::renderMeshShadow(const std::shared_ptr<Model> &mesh, Transformation &transformation) {
    glm::mat4 mashMatrix = transformation.getMatrix();

    glm::mat4 wvp = m_dirLight->getWVP(mashMatrix);
    m_shadowShader->setWVP(wvp);
    m_shadowShader->setWorld(mashMatrix);
    mesh->render();
}

void Scene::handleMouseEvent(double x, double y) {
    m_gameCamera->handleMouseEvent(x, y);
}

void Scene::handleKeyEvent(int32_t key) {
    m_gameCamera->handleKeyEvent(key);
}
