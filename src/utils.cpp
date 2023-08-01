
#include <array>
#include "utils.h"

namespace Utils{
    bool readFile(const char* filename, std::string& content){
        std::ifstream f(filename);
        bool ret = false;

        if(f.is_open()){
            std::string line;
            while(std::getline(f, line)){
                content.append(line);
                content.append("\n");
            }
            f.close();
            ret = true;
        }
        return ret;
    }

    void Frustum::calcCorners(const Camera &camera) {
        const PerspProjInfo perspInfo = camera.getPerspectiveInfo();

        float nearHalfHeight = tanf(glm::radians(perspInfo.fov / 2.0f)) * perspInfo.zNear;
        float nearHalfWidth = nearHalfHeight * perspInfo.aspect;
        float farHalfHeight = tanf(glm::radians(perspInfo.fov / 2.0f)) * perspInfo.zFar;
        float farHalfWidth = farHalfHeight * perspInfo.aspect;

        glm::vec3 nearCenter_ = camera.getPosition() + camera.forward() * perspInfo.zNear;
        glm::vec3 nearTop = camera.up() * nearHalfHeight;
        glm::vec3 nearRight = camera.right() * nearHalfWidth;

        glm::vec3 farCenter = camera.getPosition() + camera.forward() * perspInfo.zFar;
        glm::vec3 farTop = camera.up() * farHalfHeight;
        glm::vec3 farRight = camera.right() * farHalfWidth;

        nearBottomRight = {nearCenter_ + nearRight - nearTop, 1.0f};
        nearTopRight = {nearCenter_ + nearRight + nearTop, 1.0f};
        nearBottomLeft = {nearCenter_ - nearRight - nearTop, 1.0f};
        nearTopLeft = {nearCenter_ - nearRight + nearTop, 1.0f};

        farBottomRight = {farCenter + farRight - farTop, 1.0f};
        farTopRight = {farCenter + farRight + farTop, 1.0f};
        farBottomLeft = {farCenter - farRight - farTop, 1.0f};
        farTopLeft = {farCenter - farRight + farTop, 1.0f};

        nearCenter = {nearCenter_, 1.0f};
    }

    OrthoProjInfo createTightOrthographicInfo(Camera &lightCamera, const Camera &gameCamera) {

        // Calc view frustum in view space
        Frustum frustum{};
        frustum.calcCorners(gameCamera);

        glm::mat4 lightView = lightCamera.getViewMatrix();

        std::array<glm::vec3, 8> frustumToLightView{
            lightView * frustum.nearBottomRight,
            lightView * frustum.nearTopRight,
            lightView * frustum.nearBottomLeft,
            lightView * frustum.nearTopLeft,
            lightView * frustum.farBottomRight,
            lightView * frustum.farTopRight,
            lightView * frustum.farBottomLeft,
            lightView * frustum.farTopLeft
        };

        glm::vec3 min{INFINITY, INFINITY, INFINITY};
        glm::vec3 max{-INFINITY, -INFINITY, -INFINITY};

        for(std::size_t i = 0; i < frustumToLightView.size(); i++){
            if(frustumToLightView[i].x < min.x)
                min.x = frustumToLightView[i].x;
            if(frustumToLightView[i].y < min.y)
                min.y = frustumToLightView[i].y;
            if(frustumToLightView[i].z < min.z)
                min.z = frustumToLightView[i].z;

            if(frustumToLightView[i].x > max.x)
                max.x = frustumToLightView[i].x;
            if(frustumToLightView[i].y > max.y)
                max.y = frustumToLightView[i].y;
            if(frustumToLightView[i].z > max.z)
                max.z = frustumToLightView[i].z;
        }

        for(auto &point : frustumToLightView){
            point = glm::inverse(lightView) * glm::vec4(point, 1.0f);
        }

        lightCamera.setPosition(frustum.nearCenter);

        lightCamera.createView();
        lightView = lightCamera.getViewMatrix();

        for(auto &point : frustumToLightView){
            point = lightView * glm::vec4(point, 1.0f);
        }

        for(std::size_t i = 0; i < frustumToLightView.size(); i++){
            if(frustumToLightView[i].x < min.x)
                min.x = frustumToLightView[i].x;
            if(frustumToLightView[i].y < min.y)
                min.y = frustumToLightView[i].y;
            if(frustumToLightView[i].z < min.z)
                min.z = frustumToLightView[i].z;

            if(frustumToLightView[i].x > max.x)
                max.x = frustumToLightView[i].x;
            if(frustumToLightView[i].y > max.y)
                max.y = frustumToLightView[i].y;
            if(frustumToLightView[i].z > max.z)
                max.z = frustumToLightView[i].z;
        }

        lightCamera.setPosition({0.0f, 0.0f, 0.0f});

        return OrthoProjInfo{min.x, max.x, min.y, max.y, min.z, max.z};
    }
}