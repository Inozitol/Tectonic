
#include <utility>

#include "../include/engine/Animatrix.h"

#include "engine/TecCache.h"

#include <bits/ranges_algo.h>
#include <camera/Camera.h>
#include <glm/gtx/quaternion.hpp>
#include <utils/Utils.h>

Animatrix::Animatrix(Model *model) : m_model(model) {
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
                glm::vec3 newPos = Utils::interpolateBetween(pos, action.target, 0.01);
                pullBodyPart(action.body, newPos);

                break;

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
                //m_model->nodes()[m_bodyNodes[action.body][0].nodeID].animationTransform = fromOrigin * glm::toMat4(diff) * toOrigin * m_model->nodes()[m_bodyNodes[action.body][0].nodeID].animationTransform;
                //}

                glm::vec3 translatePoint = newPos - pos;
                m_bodyNodes[action.body][0].position = newPos;
                m_model->nodes()[m_bodyNodes[action.body][0].nodeID].animationTransform = glm::translate(glm::identity<glm::mat4>(), translatePoint) * m_model->nodes()[m_bodyNodes[action.body][0].nodeID].animationTransform;

                glm::vec3 oldRootPos = m_bodyNodes[action.body].back().position;

                for(size_t jointInfoIndex = 1; jointInfoIndex < m_bodyNodes[action.body].size(); jointInfoIndex++) {
                    float newDistance = glm::distance(m_bodyNodes[action.body][jointInfoIndex - 1].position, m_bodyNodes[action.body][jointInfoIndex].position);
                    if(newDistance > m_bodyNodes[action.body][jointInfoIndex].distance) {
                        float distanceDiff = newDistance - m_bodyNodes[action.body][jointInfoIndex].distance;
                        pos = m_bodyNodes[action.body][jointInfoIndex].position;
                        newPos = Utils::interpolateBetween(pos, m_bodyNodes[action.body][jointInfoIndex - 1].position, distanceDiff / newDistance);

                        direction = m_bodyNodes[action.body][jointInfoIndex].direction;
                        targetDirection = glm::normalize(newPos - pos);

                        quatDirection = glm::conjugate(glm::quatLookAt(direction, Axis::POS_Y));
                        quatTargetDirection = glm::conjugate(glm::quatLookAt(targetDirection, Axis::POS_Y));
                        sle = glm::slerp(quatDirection, quatTargetDirection, TecCache::deltaTime);
                        //sle = quatTargetDirection;
                        diff = sle * glm::inverse(quatDirection);
                        toOrigin = glm::translate(glm::identity<glm::mat4>(), -pos);
                        fromOrigin = glm::translate(glm::identity<glm::mat4>(), pos);
                        /*if(jointInfoIndex != m_bodyNodes[action.body].size()-1) {
                            m_bodyNodes[action.body][jointInfoIndex].direction = glm::normalize(direction * diff);
                            m_model->nodes()[m_bodyNodes[action.body][jointInfoIndex].nodeID].animationTransform = fromOrigin * glm::toMat4(diff) * toOrigin * m_model->nodes()[m_bodyNodes[action.body][jointInfoIndex].nodeID].animationTransform;
                        }*/

                        translatePoint = newPos - pos;
                        m_bodyNodes[action.body][jointInfoIndex].position = newPos;
                        m_model->nodes()[m_bodyNodes[action.body][jointInfoIndex].nodeID].animationTransform = glm::translate(glm::identity<glm::mat4>(), translatePoint) * m_model->nodes()[m_bodyNodes[action.body][jointInfoIndex].nodeID].animationTransform;
                    }
                }

                for(size_t jointInfoIndex = m_bodyNodes[action.body].size() - 1; jointInfoIndex != SIZE_MAX; jointInfoIndex--) {
                    float newDistance = glm::distance(m_bodyNodes[action.body][jointInfoIndex - 1].position, m_bodyNodes[action.body][jointInfoIndex].position);
                    if(newDistance > m_bodyNodes[action.body][jointInfoIndex - 1].distance) {
                        float distanceDiff = newDistance - m_bodyNodes[action.body][jointInfoIndex].distance;
                        pos = m_bodyNodes[action.body][jointInfoIndex].position;
                        if(jointInfoIndex == m_bodyNodes[action.body].size() - 1) {
                            newPos = oldRootPos;
                        } else {
                            newPos = Utils::interpolateBetween(pos, m_bodyNodes[action.body][jointInfoIndex - 1].position, distanceDiff / newDistance);
                        }
                        direction = m_bodyNodes[action.body][jointInfoIndex].direction;
                        targetDirection = glm::normalize(newPos - pos);

                        quatDirection = glm::conjugate(glm::quatLookAt(direction, Axis::POS_Y));
                        quatTargetDirection = glm::conjugate(glm::quatLookAt(targetDirection, Axis::POS_Y));
                        sle = glm::slerp(quatDirection, quatTargetDirection, TecCache::deltaTime);
                        //sle = quatTargetDirection;
                        diff = sle * glm::inverse(quatDirection);
                        toOrigin = glm::translate(glm::identity<glm::mat4>(), -pos);
                        fromOrigin = glm::translate(glm::identity<glm::mat4>(), pos);
                        /*if(jointInfoIndex != m_bodyNodes[action.body].size()-1) {
                            m_bodyNodes[action.body][jointInfoIndex].direction = glm::normalize(direction * diff);
                            m_model->nodes()[m_bodyNodes[action.body][jointInfoIndex].nodeID].animationTransform = fromOrigin * glm::toMat4(diff) * toOrigin * m_model->nodes()[m_bodyNodes[action.body][jointInfoIndex].nodeID].animationTransform;
                        }*/

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

Model *Animatrix::model() const {
    return m_model;
}

bool Animatrix::loadArmature() {
    if(!m_model->isSkinned()) {
        m_logger(Logger::ERROR) << "Model [" << m_model->path() << "] has no skin\n";
        return false;
    }

    // First is joint index from name string, second is JointID, third is NodeID
    std::unordered_map<BodyPart, std::vector<std::tuple<uint32_t, uint32_t, ModelTypes::NodeID_t>>> bodyNodes{
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

    auto comparFunc = [](const std::tuple<uint32_t, uint32_t, ModelTypes::NodeID_t> &a, const std::tuple<uint32_t, uint32_t, ModelTypes::NodeID_t> &b) {
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
    auto copyFunc = [](const std::tuple<uint32_t, uint32_t, ModelTypes::NodeID_t> &joint) {
        return JointInfo{.distance = 0.0f, .nodeID = std::get<2>(joint), .jointID = std::get<1>(joint)};
    };
    for(auto &[partType, partVector]: bodyNodes) {
        std::ranges::transform(partVector.begin(), partVector.end(), std::back_inserter(m_bodyNodes[partType]), copyFunc);
        m_bodyNodes[partType].shrink_to_fit();
    }

    updateDistances();

    for(const auto &[bodyPart, joints]: m_bodyNodes) {
        const std::size_t lineCount = joints.size() - 1;
        const std::size_t indicesCount = lineCount * 2;
        const std::size_t verticesCount = joints.size();
        if(verticesCount <= 1) continue;
        debugJointLines[bodyPart].vertices.resize(verticesCount);
        debugJointLines[bodyPart].indices.resize(indicesCount);
        for(std::size_t lineIndex = 0; lineIndex < lineCount; lineIndex++) {
            debugJointLines[bodyPart].indices[lineIndex * 2] = lineIndex;
            debugJointLines[bodyPart].indices[lineIndex * 2 + 1] = lineIndex + 1;
        }
        for(std::size_t vertexIndex = 0; vertexIndex < verticesCount; vertexIndex++) {
            debugJointLines[bodyPart].vertices[vertexIndex].position = m_model->transformation.getMatrix() * glm::vec4(joints[vertexIndex].position, 1.0f);
            debugJointLines[bodyPart].vertices[vertexIndex].color = {1.0f, 0.0f, 0.0f};// Red
        }
    }

    for(const auto &[bodyPart, joints]: m_bodyNodes) {
        const std::size_t lineCount = joints.size() * 3;
        const std::size_t indicesCount = lineCount * 2;
        const std::size_t verticesCount = joints.size() * 6;
        if(joints.size() <= 1) continue;
        debugJointBasis[bodyPart].vertices.resize(verticesCount);
        debugJointBasis[bodyPart].indices.resize(indicesCount);
        for(std::size_t lineIndex = 0; lineIndex < lineCount; lineIndex++) {
            debugJointBasis[bodyPart].indices[lineIndex * 2] = lineIndex;
            debugJointBasis[bodyPart].indices[lineIndex * 2 + 1] = lineIndex + 1;
        }
        for(std::size_t vertexIndex = 0; vertexIndex < verticesCount; vertexIndex++) {
            debugJointBasis[bodyPart].vertices[vertexIndex].color = {0.0f, 0.0f, 1.0f};// Blue
            size_t jointIndex = std::floor(vertexIndex / 6.0f);
            switch(vertexIndex % 6) {
                case 0:
                case 2:
                case 4:
                    debugJointBasis[bodyPart].vertices[vertexIndex].position = debugJointLines[bodyPart].vertices[jointIndex].position;
                break;
                case 1:
                    debugJointBasis[bodyPart].vertices[vertexIndex].position = debugJointLines[bodyPart].vertices[jointIndex].position + (0.2f * joints[jointIndex].basis.x);
                break;
                case 3:
                    debugJointBasis[bodyPart].vertices[vertexIndex].position = debugJointLines[bodyPart].vertices[jointIndex].position + (0.2f * joints[jointIndex].basis.y);
                break;
                case 5:
                    debugJointBasis[bodyPart].vertices[vertexIndex].position = debugJointLines[bodyPart].vertices[jointIndex].position + (0.2f * joints[jointIndex].basis.z);
                break;
            }
        }
    }

    m_isLoaded = true;
    return true;
}

void Animatrix::updateDistances() {
    auto updateFunc = [this](std::vector<JointInfo> &joints) {
        // The position of each bone can be calculated by transforming an origin point by inverse of inverse bind transformation of that bone (this bs took me whole night)
        joints[0].position = glm::inverse(m_model->skin().inverseBindMatrices[joints[0].jointID]) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        JointInfo prevJoint = joints[0];

        for(size_t jointInfoIndex = 1; jointInfoIndex < joints.size(); jointInfoIndex++) {

            ModelTypes::Node &currJointNode = m_model->nodes()[joints[jointInfoIndex].nodeID];
            ModelTypes::Node &prevJointNode = m_model->nodes()[prevJoint.nodeID];

            assert(prevJointNode.parent == joints[jointInfoIndex].nodeID);
            if(currJointNode.parent != ModelTypes::NULL_ID) {
                joints[jointInfoIndex].position = glm::inverse(m_model->skin().inverseBindMatrices[joints[jointInfoIndex].jointID]) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                joints[jointInfoIndex].distance = glm::distance(joints[jointInfoIndex].position, prevJoint.position);
                joints[jointInfoIndex].direction = glm::normalize(joints[jointInfoIndex - 1].position - joints[jointInfoIndex].position);
                joints[jointInfoIndex].basis.y = glm::normalize(joints[jointInfoIndex - 1].position - joints[jointInfoIndex].position);
                joints[jointInfoIndex].basis.x = Utils::closestOrthonormal(joints[jointInfoIndex].basis.y, Axis::POS_X);
                joints[jointInfoIndex].basis.z = glm::normalize(glm::cross(joints[jointInfoIndex].basis.x, joints[jointInfoIndex].basis.y));
                if(!Utils::isRightHanded(joints[jointInfoIndex].basis.x, joints[jointInfoIndex].basis.y, joints[jointInfoIndex].basis.z)) {
                    joints[jointInfoIndex].basis.z = -joints[jointInfoIndex].basis.z;
                }
            }
            prevJoint = joints[jointInfoIndex];
        }
        if(joints.size() > 1) {
            joints[0].direction = joints[1].direction;
            joints[0].basis.y = joints[1].direction;
            joints[0].basis.x = Utils::closestOrthonormal(joints[0].basis.y, Axis::POS_X);
            joints[0].basis.z = glm::normalize(glm::cross(joints[0].basis.x, joints[0].basis.y));
            if(!Utils::isRightHanded(joints[0].basis.x, joints[0].basis.y, joints[0].basis.z)) {
                joints[0].basis.z = -joints[0].basis.z;
            }
        }
    };

    for(auto &[partType, partVector]: m_bodyNodes) {
        updateFunc(partVector);
    }
}

void Animatrix::pullBodyPart(BodyPart bodyPart, const glm::vec3 &dest) {
    m_bodyNodes[bodyPart][0].setPosition(m_model, dest);
    glm::vec3 rootPos = m_bodyNodes[bodyPart].back().position;
    cascadeChange(bodyPart, false);

    bool fromRoot = true;
    float distance = glm::distance(m_bodyNodes[bodyPart].back().position, rootPos);
    while(distance > 0.0001) {
        std::size_t jointIndex = fromRoot ? m_bodyNodes[bodyPart].size() - 1 : 0;
        glm::vec3 destPos = fromRoot ? rootPos : dest;
        m_bodyNodes[bodyPart][jointIndex].setPosition(m_model, destPos);

        cascadeChange(bodyPart, fromRoot);
        fromRoot = !fromRoot;
        distance = glm::distance(m_bodyNodes[bodyPart].back().position, rootPos);
    }
    updateDebug();
}

void Animatrix::cascadeChange(BodyPart bodyPart, bool fromRoot) {
    // Limbs that have a single bone have nothing to cascade
    if(m_bodyNodes[bodyPart].size() <= 1) return;

    if(fromRoot) {
        // When cascading from root (closer to body center), we adjust joints in reverse
        for(std::size_t jointInfoIndex = m_bodyNodes[bodyPart].size() - 2; jointInfoIndex != SIZE_MAX; jointInfoIndex--) {

            JointInfo &currJoint = m_bodyNodes[bodyPart][jointInfoIndex];
            JointInfo &prevJoint = m_bodyNodes[bodyPart][jointInfoIndex + 1];

            float newDistance = glm::distance(prevJoint.position, currJoint.position);
            if(newDistance > prevJoint.distance) {
                float distanceDiff = newDistance - prevJoint.distance;
                glm::vec3 currPos = currJoint.position;
                glm::vec3 destPos = Utils::interpolateBetween(currPos, prevJoint.position, distanceDiff / newDistance);

                // The rotation is applied to the bone closer to root, because the next position is still unknown
                // This eliminates the need to rotate the root bone before cascading
                glm::vec3 destDir = glm::normalize(destPos - prevJoint.position);
                prevJoint.setDirection(m_model, destDir);

                // The difference between current and destination position
                // This is used to create a translation matrix that transforms this difference
                glm::vec3 diffVector = destPos - currPos;
                currJoint.applyTransformation(m_model, glm::translate(glm::identity<glm::mat4>(), diffVector));
                currJoint.position = destPos;

            } else if(newDistance < prevJoint.distance) {
                float distanceDiff = prevJoint.distance - newDistance;
                glm::vec3 currPos = currJoint.position;
                glm::vec3 destPos = Utils::interpolateBetween(prevJoint.position, currPos, 1.0f + distanceDiff / prevJoint.distance);

                // The rotation is applied to the bone closer to root, because the next position is still unknown
                // This eliminates the need to rotate the root bone before cascading
                glm::vec3 destDir = glm::normalize(destPos - prevJoint.position);
                prevJoint.setDirection(m_model, destDir);

                // The difference between current and destination position
                // This is used to create a translation matrix that transforms this difference
                glm::vec3 diffVector = destPos - currPos;
                currJoint.applyTransformation(m_model, glm::translate(glm::identity<glm::mat4>(), diffVector));
                currJoint.position = destPos;

            } else {
                // If this joint doesn't have to adjust, the rest wouldn't have to as well
                // This seems improbable but keeping it here anyway
                break;
            }
        }
    } else {
        for(std::size_t jointInfoIndex = 1; jointInfoIndex < m_bodyNodes[bodyPart].size(); jointInfoIndex++) {
            JointInfo &currJoint = m_bodyNodes[bodyPart][jointInfoIndex];
            JointInfo &nextJoint = m_bodyNodes[bodyPart][jointInfoIndex - 1];

            float newDistance = glm::distance(nextJoint.position, currJoint.position);
            if(newDistance > currJoint.distance) {
                float distanceDiff = newDistance - currJoint.distance;
                glm::vec3 currPos = currJoint.position;
                glm::vec3 destPos = Utils::interpolateBetween(currPos, nextJoint.position, distanceDiff / newDistance);
                glm::vec3 destDir = glm::normalize(nextJoint.position - destPos);
                currJoint.setDirection(m_model, destDir);

                // The difference between current and destination position
                // This is used to create a translation matrix that transforms this difference
                glm::vec3 diffVector = destPos - currPos;
                currJoint.applyTransformation(m_model, glm::translate(glm::identity<glm::mat4>(), diffVector));
                currJoint.position = destPos;


            } else if(newDistance < currJoint.distance) {
                float distanceDiff = currJoint.distance - newDistance;
                glm::vec3 currPos = currJoint.position;
                glm::vec3 destPos = Utils::interpolateBetween(nextJoint.position, currPos, 1.0f + distanceDiff / currJoint.distance);
                glm::vec3 destDir = glm::normalize(nextJoint.position - destPos);
                currJoint.setDirection(m_model, destDir);

                // The difference between current and destination position
                // This is used to create a translation matrix that transforms this difference
                glm::vec3 diffVector = destPos - currPos;
                currJoint.applyTransformation(m_model, glm::translate(glm::identity<glm::mat4>(), diffVector));
                currJoint.position = destPos;

            } else {
                // If this joint doesn't have to adjust, the rest wouldn't have to as well
                // This seems improbable but keeping it here anyway
                break;
            }
        }
    }
}

void Animatrix::JointInfo::applyTransformation(Model *model, const glm::mat4 &t) const {
    model->nodes()[nodeID].animationTransform = t * model->nodes()[nodeID].animationTransform;
}

void Animatrix::JointInfo::setPosition(Model *model, const glm::vec3 &p) {
    // The difference between current and destination position
    // is used to create a translation matrix that transforms this difference
    glm::vec3 diffVector = p - position;
    applyTransformation(model, glm::translate(glm::identity<glm::mat4>(), diffVector));
    position = p;
}

void Animatrix::JointInfo::setDirection(Model *model, const glm::vec3 &d) {
    const glm::quat currRot = glm::conjugate(glm::quatLookAt(direction, Axis::POS_Y));
    const glm::quat destRot = glm::conjugate(glm::quatLookAt(d, Axis::POS_Y));
    const glm::quat diffRot = currRot * glm::conjugate(destRot);
    const glm::mat4 toOrig = glm::translate(glm::identity<glm::mat4>(), -position);
    const glm::mat4 fromOrig = glm::translate(glm::identity<glm::mat4>(), position);
    applyTransformation(model, fromOrig * glm::toMat4(diffRot) * toOrig);
    basis.x = glm::toMat4(diffRot) * glm::vec4(basis.x, 1.0f);
    basis.y = glm::toMat4(diffRot) * glm::vec4(basis.y, 1.0f);
    basis.z = glm::toMat4(diffRot) * glm::vec4(basis.z, 1.0f);

    direction = d;
}

void Animatrix::updateDebug() {
    for(const auto &[bodyPart, joints]: m_bodyNodes) {
        if(joints.size() <= 1) continue;
        for(std::size_t jointIndex = 0; jointIndex < joints.size(); jointIndex++) {
            debugJointLines[bodyPart].vertices[jointIndex].position = m_model->transformation.getMatrix() * glm::vec4(joints[jointIndex].position, 1.0f);
            debugJointLines[bodyPart].vertices[jointIndex].color = glm::normalize(joints[jointIndex].direction * glm::vec3(jointIndex + 1));
        }
    }

    for(const auto &[bodyPart, joints]: m_bodyNodes) {
        const std::size_t verticesCount = joints.size() * 6;
        if(joints.size() <= 1) continue;
        for(std::size_t vertexIndex = 0; vertexIndex < verticesCount; vertexIndex++) {
            size_t jointIndex = std::floor(vertexIndex / 6.0f);
            switch(vertexIndex % 6) {
                case 0:
                case 2:
                case 4:
                    debugJointBasis[bodyPart].vertices[vertexIndex].position = debugJointLines[bodyPart].vertices[jointIndex].position;
                    break;
                case 1:
                    debugJointBasis[bodyPart].vertices[vertexIndex].position = debugJointLines[bodyPart].vertices[jointIndex].position + (0.2f * joints[jointIndex].basis.x);
                    break;
                case 3:
                    debugJointBasis[bodyPart].vertices[vertexIndex].position = debugJointLines[bodyPart].vertices[jointIndex].position + (0.2f * joints[jointIndex].basis.y);
                    break;
                case 5:
                    debugJointBasis[bodyPart].vertices[vertexIndex].position = debugJointLines[bodyPart].vertices[jointIndex].position + (0.2f * joints[jointIndex].basis.z);
                    break;
            }
        }
    }
}