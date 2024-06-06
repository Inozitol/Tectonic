
#include <array>
#include "utils/utils.h"

namespace Utils{
    WindowDimension::WindowDimension(uint32_t width, uint32_t height) {
        this->width = width;
        this->height = height;
    }

    WindowDimension::WindowDimension(int32_t width, int32_t height) {
        this->width = static_cast<uint32_t>(width);
        this->height = static_cast<uint32_t>(height);
    }

    float WindowDimension::ratio() const {
        return (float)width/(float)height;
    }

    bool readFile(const char* filename, std::string& content){
        std::ifstream f;

        f.open(filename);

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

/*
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

    uint32_t nextPowerOf(uint32_t in, uint32_t power){
        double l = log(in)/log(power);
        return static_cast<uint32_t>(std::pow(power,static_cast<uint32_t>(std::ceil(l))));
    }

    int64_t binPow(int32_t exp){
        return 1 << exp;
    }

    void FrustumCulling::update(const glm::mat4 &VP) {
        glm::mat4 tVP = glm::transpose(VP);

        m_leftClipPlane     = tVP[3] + tVP[0];
        m_rightClipPlane    = tVP[3] - tVP[0];
        m_bottomClipPlane   = tVP[3] + tVP[1];
        m_topClipPlane      = tVP[3] - tVP[1];
        m_nearClipPlane     = tVP[3] + tVP[2];
        m_farClipPlane      = tVP[3] - tVP[2];
    }

    bool FrustumCulling::isPointInside(const glm::vec3 &point) const {
        glm::vec4 point4D(point,1.0f);

        return  (glm::dot(m_leftClipPlane,point4D) >= m_bias) &&
                (glm::dot(m_rightClipPlane, point4D) >= m_bias) &&
                (glm::dot(m_bottomClipPlane, point4D) >= m_bias) &&
                (glm::dot(m_topClipPlane, point4D) >= m_bias) &&
                (glm::dot(m_nearClipPlane, point4D) >= m_bias) &&
                (glm::dot(m_farClipPlane, point4D) >= m_bias);
    }

    void barycentric(glm::vec2 p, glm::vec2 a, glm::vec2 b, glm::vec2 c, float &u, float &v, float &w){
        glm::vec2 v0 = b-a, v1 = c-a, v2 = p-a;
        float d00 = glm::dot(v0,v0);
        float d01 = glm::dot(v0,v1);
        float d11 = glm::dot(v1,v1);
        float d20 = glm::dot(v2,v0);
        float d21 = glm::dot(v2,v1);
        float denom = d00*d11 - d01*d01;
        v = (d11*d20 - d01*d21) / denom;
        w = (d00*d21 - d01*d20) / denom;
        u = 1.0f-v-w;
    }
    */
}