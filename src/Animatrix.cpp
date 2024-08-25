
#include <utility>

#include "../include/engine/Animatrix.h"

#include "engine/TecCache.h"

#include <bits/ranges_algo.h>
#include <camera/Camera.h>
#include <glm/gtx/quaternion.hpp>
#include <utils/utils.h>

Animatrix::Animatrix(Model* model) : m_model(model) {
    assert(m_model);
    if(!loadArmature()) {
        m_logger(Logger::ERROR) << "Model [" << m_model->path() << "] has invalid armature\n";
        return;
    }
}

bool Animatrix::isLoaded() const {
    return m_isLoaded;
}

void Animatrix::updateActions() {
    for(auto &[k, action]: actions) {
        switch(action.type) {
            case ActionType::PULLING: {
                glm::vec3 pos = m_bodyNodes[action.body][0].position;
                glm::vec3 newPos = Utils::interpolateBetween(pos, action.target, TecCache::deltaTime);

                glm::vec3 direction = m_bodyNodes[action.body][0].direction;
                glm::vec3 targetDirection = glm::normalize(newPos - pos);

                glm::quat quatDirection = glm::conjugate(glm::quatLookAt(direction, Axis::POS_Y));
                glm::quat quatTargetDirection = glm::conjugate(glm::quatLookAt(targetDirection, Axis::POS_Y));
                glm::quat sle = glm::slerp(quatDirection, quatTargetDirection, TecCache::deltaTime);
                //glm::quat sle = quatTargetDirection;
                glm::quat diff = sle * glm::inverse(quatDirection);
                m_bodyNodes[action.body][0].direction = glm::normalize(direction * diff);
                glm::mat4 toOrigin = glm::translate(glm::identity<glm::mat4>(), -pos);
                glm::mat4 fromOrigin = glm::translate(glm::identity<glm::mat4>(), pos);

                //if(m_bodyNodes[action.body].size() > 1) {
                    m_model->nodes()[m_bodyNodes[action.body][0].nodeID].animationTransform = fromOrigin * glm::toMat4(diff) * toOrigin * m_model->nodes()[m_bodyNodes[action.body][0].nodeID].animationTransform;
                //}

                glm::vec3 translatePoint = newPos - pos;
                m_bodyNodes[action.body][0].position = newPos;
                m_model->nodes()[m_bodyNodes[action.body][0].nodeID].animationTransform = glm::translate(glm::identity<glm::mat4>(), translatePoint) * m_model->nodes()[m_bodyNodes[action.body][0].nodeID].animationTransform;

                for(size_t jointInfoIndex = 1; jointInfoIndex < m_bodyNodes[action.body].size(); jointInfoIndex++) {
                    float newDistance = glm::distance(m_bodyNodes[action.body][jointInfoIndex-1].position, m_bodyNodes[action.body][jointInfoIndex].position);
                    if(newDistance > m_bodyNodes[action.body][jointInfoIndex].distance) {
                        float distanceDiff = newDistance - m_bodyNodes[action.body][jointInfoIndex].distance;
                        pos = m_bodyNodes[action.body][jointInfoIndex].position;
                        newPos = Utils::interpolateBetween(pos, m_bodyNodes[action.body][jointInfoIndex-1].position, distanceDiff/newDistance);

                        direction = m_bodyNodes[action.body][jointInfoIndex].direction;
                        targetDirection = glm::normalize(newPos - pos);

                        quatDirection = glm::conjugate(glm::quatLookAt(direction, Axis::POS_Y));
                        quatTargetDirection = glm::conjugate(glm::quatLookAt(targetDirection, Axis::POS_Y));
                        sle = glm::slerp(quatDirection, quatTargetDirection, TecCache::deltaTime);
                        //sle = quatTargetDirection;
                        diff = sle * glm::inverse(quatDirection);
                        toOrigin = glm::translate(glm::identity<glm::mat4>(), -pos);
                        fromOrigin = glm::translate(glm::identity<glm::mat4>(), pos);
                        if(jointInfoIndex != m_bodyNodes[action.body].size()-1) {
                            m_bodyNodes[action.body][jointInfoIndex].direction = glm::normalize(direction * diff);
                            m_model->nodes()[m_bodyNodes[action.body][jointInfoIndex].nodeID].animationTransform = fromOrigin * glm::toMat4(diff) * toOrigin * m_model->nodes()[m_bodyNodes[action.body][jointInfoIndex].nodeID].animationTransform;
                        }

                        translatePoint = newPos - pos;
                        m_bodyNodes[action.body][jointInfoIndex].position = newPos;
                        m_model->nodes()[m_bodyNodes[action.body][jointInfoIndex].nodeID].animationTransform = glm::translate(glm::identity<glm::mat4>(), translatePoint) * m_model->nodes()[m_bodyNodes[action.body][jointInfoIndex].nodeID].animationTransform;
                    }
                }

            } break;
        }
    }
    m_model->uploadJointsMatrices();
}

Model * Animatrix::model() const{
    return m_model;
}

bool Animatrix::loadArmature() {
    if(!m_model->isSkinned()) {
        m_logger(Logger::ERROR) << "Model [" << m_model->path() << "] has no skin\n";
        return false;
    }

    // First is joint index from name string, second is JointID, third is NodeID
    std::unordered_map<BodyPart, std::vector<std::tuple<uint32_t,uint32_t,ModelTypes::NodeID_t>>> bodyNodes{
            {BodyPart::SPINE, {}},
            {BodyPart::HEAD, {}},
            {BodyPart::LLEG, {}},
            {BodyPart::RLEG, {}},
            {BodyPart::LARM, {}},
            {BodyPart::RARM, {}},
    };

    ModelTypes::Skin &skin = m_model->skin();
    uint32_t jointID = 0;
    for(const auto &nID: skin.joints) {
        ModelTypes::Node &skinJoint = m_model->nodes()[nID];
        std::cmatch m;
        if(!std::regex_match(skinJoint.name.data(), m, m_skinNodeRegex)) {
            m_logger(Logger::ERROR) << "Model [" << m_model->path() << "] has invalid skin joint name [" << skinJoint.name.data() << "]\n";
            return false;
        }

        if(m[1] == "spine") {
            bodyNodes[BodyPart::SPINE].emplace_back(std::stoi(m[2]), jointID++, nID);
        } else if(m[1] == "head") {
            bodyNodes[BodyPart::HEAD].emplace_back(std::stoi(m[2]), jointID++, nID);
        } else if(m[1] == "leg") {
            if(m[3] == "L") {
                bodyNodes[BodyPart::LLEG].emplace_back(std::stoi(m[2]), jointID++, nID);
            } else if(m[3] == "R") {
                bodyNodes[BodyPart::RLEG].emplace_back(std::stoi(m[2]), jointID++, nID);
            }
        } else if(m[1] == "arm") {
            if(m[3] == "L") {
                bodyNodes[BodyPart::LARM].emplace_back(std::stoi(m[2]), jointID++, nID);
            } else if(m[3] == "R") {
                bodyNodes[BodyPart::RARM].emplace_back(std::stoi(m[2]), jointID++, nID);
            }
        }
    }

    auto comparFunc = [](const std::tuple<uint32_t,uint32_t,ModelTypes::NodeID_t> &a, const std::tuple<uint32_t,uint32_t,ModelTypes::NodeID_t> &b) {
        return std::get<0>(a) > std::get<0>(b);
    };

    for(auto &[partType, partVector]: bodyNodes) {
        std::ranges::sort(partVector.begin(), partVector.end(), comparFunc);
    }

    if(bodyNodes[BodyPart::LLEG].size() != bodyNodes[BodyPart::RLEG].size()) {
        m_logger(Logger::WARNING) << "Model [" << m_model->path() << "] has different number of joins in legs\n";
    }
    if(bodyNodes[BodyPart::LARM].size() != bodyNodes[BodyPart::RARM].size()) {
        m_logger(Logger::WARNING) << "Model [" << m_model->path() << "] has different number of joins in arms\n";
    }
    auto copyFunc = [](const std::tuple<uint32_t,uint32_t,ModelTypes::NodeID_t> &joint) {
        return JointInfo{.distance = 0.0f, .nodeID = std::get<2>(joint), .jointID = std::get<1>(joint)};
    };
    for(auto &[partType, partVector]: bodyNodes) {
        std::ranges::transform(partVector.begin(), partVector.end(), std::back_inserter(m_bodyNodes[partType]), copyFunc);
        m_bodyNodes[partType].shrink_to_fit();
    }

    updateDistances();

    m_isLoaded = true;
    return true;
}

void Animatrix::updateDistances() {
    auto updateFunc = [this](BodyPart bodyPart, std::vector<JointInfo> &joints) {

        // The position of each bone can be calculated by transforming an origin point by inverse of inverse bind transformation of that bone (this bs took me whole night)
        joints[0].position = glm::inverse(m_model->skin().inverseBindMatrices[joints[0].jointID]) * glm::vec4(0.0f,0.0f,0.0f,1.0f);
        JointInfo prevJoint = joints[0];

        for(size_t jointInfoIndex = 1; jointInfoIndex < joints.size(); jointInfoIndex++) {

            ModelTypes::Node &currJointNode = m_model->nodes()[joints[jointInfoIndex].nodeID];
            ModelTypes::Node &prevJointNode = m_model->nodes()[prevJoint.nodeID];

            assert(prevJointNode.parent == joints[jointInfoIndex].nodeID);
            if(currJointNode.parent != ModelTypes::NULL_ID) {
                joints[jointInfoIndex].position = glm::inverse(m_model->skin().inverseBindMatrices[joints[jointInfoIndex].jointID]) * glm::vec4(0.0f,0.0f,0.0f,1.0f);
                joints[jointInfoIndex].distance = glm::distance(joints[jointInfoIndex].position, prevJoint.position);

                // TODO find direction of last limb parts
                joints[jointInfoIndex-1].direction = glm::normalize(joints[jointInfoIndex-1].position - joints[jointInfoIndex].position);
            }
            prevJoint = joints[jointInfoIndex];
        }
    };

    for(auto &[partType, partVector]: m_bodyNodes) {
        updateFunc(partType, partVector);
    }

    for(auto &[partType, partVector]: m_bodyNodes) {
        switch(partType) {
            case BodyPart::SPINE:
                partVector.back().direction = {0.0f, 1.0f, 0.0f};
            break;
            case BodyPart::HEAD:
                partVector.back().direction = glm::normalize(partVector.back().position - m_bodyNodes[BodyPart::SPINE][0].position);
            break;
            case BodyPart::LLEG:
                partVector.back().direction = glm::normalize(partVector.back().position - m_bodyNodes[BodyPart::SPINE].back().position);
            break;
            case BodyPart::RLEG:
                partVector.back().direction = glm::normalize(partVector.back().position - m_bodyNodes[BodyPart::SPINE].back().position);
            break;
            case BodyPart::LARM:
                partVector.back().direction = glm::normalize(partVector.back().position - m_bodyNodes[BodyPart::SPINE][0].position);
            break;
            case BodyPart::RARM:
                partVector.back().direction = glm::normalize(partVector.back().position - m_bodyNodes[BodyPart::SPINE][0].position);
            break;
        }
    }

}