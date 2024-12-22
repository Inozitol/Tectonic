
#include <utility>

#include "../include/engine/Animatrix.h"

#include "engine/TecCache.h"

#include <bits/ranges_algo.h>
#include <camera/Camera.h>
#include <cmath>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <numeric>
#include <utils/Utils.h>

Animatrix::Animatrix(Model *model) : m_model(model) {
    assert(m_model);
    if(!loadArmature()) {
        m_logger(Logger::ERROR) << "Model [" << m_model->path() << "] has invalid armature\n";
        return;
    }
    m_instances.insert(this);
}

Animatrix::~Animatrix() {
    m_instances.erase(this);
}

void Animatrix::updateActions() {
    for(auto &[k, action]: actions) {
        switch(action.type) {
            case ActionType::PULLING: {
                glm::vec3 &pos = m_bodyNodes[action.body][0].currPosition;
                glm::vec3 newPos = Utils::interpolateBetween(pos, action.target, 0.01);
                pullBodyPart(action.body, newPos);
                break;
            }
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
    /*std::unordered_map<BodyPart, std::vector<std::tuple<uint32_t, uint32_t, ModelTypes::NodeID_t>>> bodyNodes{
            {BodyPart::SPINE, {}},
            {BodyPart::HEAD, {}},
            {BodyPart::LLEG, {}},
            {BodyPart::RLEG, {}},
            {BodyPart::LARM, {}},
            {BodyPart::RARM, {}},
    };*/
    struct loadedJoint {
        BodyPart bodyPart;
        uint32_t bodyPartID;
        uint32_t jointID;
        uint32_t nodeID;
    };

    ModelTypes::Skin &skin = m_model->skin();
    std::vector<loadedJoint> bodyJoints;
    uint32_t numOfJoints = skin.joints.size();
    bodyJoints.reserve(numOfJoints);
    uint32_t jointID = 0;
    for(const auto &nID: skin.joints) {
        ModelTypes::Node &skinJoint = m_model->nodes()[nID];
        std::cmatch m;
        if(!std::regex_match(skinJoint.name.data(), m, m_skinNodeRegex)) {
            m_logger(Logger::ERROR) << "Model [" << m_model->path() << "] has invalid skin joint name [" << skinJoint.name.data() << "]\n";
            return false;
        }

        // Switching based on first char of name
        switch(m[1].str()[0]) {
            case 's':// spine
                bodyJoints.push_back(loadedJoint{
                        .bodyPart = BODYPART_SPINE,
                        .bodyPartID = static_cast<uint32_t>(std::stoi(m[2])),
                        .jointID = jointID++,
                        .nodeID = nID});
                break;

            case 'h':// head
                bodyJoints.push_back(loadedJoint{
                        .bodyPart = BODYPART_HEAD,
                        .bodyPartID = static_cast<uint32_t>(std::stoi(m[2])),
                        .jointID = jointID++,
                        .nodeID = nID});
                break;

            case 'l':// leg
                switch(m[3].str()[0]) {
                    case 'L':
                        bodyJoints.push_back(loadedJoint{
                                .bodyPart = BODYPART_LLEG,
                                .bodyPartID = static_cast<uint32_t>(std::stoi(m[2])),
                                .jointID = jointID++,
                                .nodeID = nID});
                        break;
                    case 'R':
                        bodyJoints.push_back(loadedJoint{
                                .bodyPart = BODYPART_RLEG,
                                .bodyPartID = static_cast<uint32_t>(std::stoi(m[2])),
                                .jointID = jointID++,
                                .nodeID = nID});
                        break;
                }
                break;

            case 'a':// arm
                switch(m[3].str()[0]) {
                    case 'L':
                        bodyJoints.push_back(loadedJoint{
                                .bodyPart = BODYPART_LARM,
                                .bodyPartID = static_cast<uint32_t>(std::stoi(m[2])),
                                .jointID = jointID++,
                                .nodeID = nID});
                        break;
                    case 'R':
                        bodyJoints.push_back(loadedJoint{
                                .bodyPart = BODYPART_RARM,
                                .bodyPartID = static_cast<uint32_t>(std::stoi(m[2])),
                                .jointID = jointID++,
                                .nodeID = nID});
                        break;
                }
                break;
        }
    }

    std::ranges::sort(bodyJoints.begin(), bodyJoints.end(), [](const loadedJoint &a, const loadedJoint &b) {
        return a.jointID > b.jointID;
    });

    assert(std::accumulate(bodyJoints.begin(), bodyJoints.end(), 0,
                           [](uint32_t acc, const loadedJoint &joint) { return acc + joint.jointID; }) == numOfJoints * (numOfJoints + 1) << 1);

    /*
        if(bodyNodes[BodyPart::LLEG].size() != bodyNodes[BodyPart::RLEG].size()) {
            m_logger(Logger::WARNING) << "Model [" << m_model->path() << "] has different number of joins in legs\n";
        }
        if(bodyNodes[BodyPart::LARM].size() != bodyNodes[BodyPart::RARM].size()) {
            m_logger(Logger::WARNING) << "Model [" << m_model->path() << "] has different number of joins in arms\n";
        }
        */

    auto copyFunc = [](const loadedJoint &joint) {
        return JointInfo{.distance = 0.0f, .nodeID = joint.nodeID, .jointID = joint.jointID, .bodyPart = joint.bodyPart, .bodyPartID = joint.bodyPartID};
    };

    std::ranges::transform(bodyJoints.begin(), bodyJoints.end(), std::back_inserter(m_bodyJoints), copyFunc);
    m_bodyJoints.shrink_to_fit();

    loadBodyPartVectors();
    Utils::enumSetBits( m_bodyJoints[m_bodyPartsJoints[BODYPART_SPINE].back()].flags, JointInfo::Flags::IS_ROOT);
    loadJointGeometry();
    createDebugLines();

    return true;
}

void Animatrix::loadBodyPartVectors() {
    for(auto& joint : m_bodyJoints) {
        switch(joint.bodyPart) {
            case BODYPART_SPINE:
                m_bodyPartsJoints[BODYPART_SPINE].push_back(joint.jointID);
            break;
            case BODYPART_HEAD:
                m_bodyPartsJoints[BODYPART_HEAD].push_back(joint.jointID);
            break;
            case BODYPART_LLEG:
                m_bodyPartsJoints[BODYPART_LLEG].push_back(joint.jointID);
            break;
            case BODYPART_RLEG:
                m_bodyPartsJoints[BODYPART_RLEG].push_back(joint.jointID);
            break;
            case BODYPART_LARM:
                m_bodyPartsJoints[BODYPART_LARM].push_back(joint.jointID);
            break;
            case BODYPART_RARM:
                m_bodyPartsJoints[BODYPART_RARM].push_back(joint.jointID);
            break;
            default:
                assert(false);
        }
    }

    for(auto& jointVec : m_bodyPartsJoints) {
        std::ranges::sort(jointVec.begin(), jointVec.end(), [this](uint32_t a, uint32_t b) {
            return m_bodyJoints[a].bodyPartID < m_bodyJoints[b].bodyPartID;
        });
    }
}

void Animatrix::loadJointGeometry() {
    auto updateFunc = [this](std::vector<uint32_t> &jointIDs) {
        auto joints = [this, &jointIDs](uint32_t index)->JointInfo&{ return m_bodyJoints[jointIDs[index]]; };

        // The position of each bone can be calculated by transforming an origin point by inverse of inverse bind transformation of that bone (this took me whole night)
        joints(0).origPosition = glm::inverse(m_model->skin().inverseBindMatrices[joints(0).jointID]) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        joints(0).currPosition = joints(0).tempPosition = joints(0).origPosition;
        joints(0).origAnimTransform = m_model->nodes()[joints(0).nodeID].animationTransform;
        joints(0).origAnimTransformInverse = glm::inverse(joints(0).origAnimTransform);

        JointInfo& prevJoint = joints(0);
        joints(0).inner = joints(1).jointID;

        for(size_t jointInfoIndex = 1; jointInfoIndex < jointIDs.size(); jointInfoIndex++) {

            JointInfo &currJoint = joints(jointInfoIndex);
            ModelTypes::Node &currJointNode = m_model->nodes()[currJoint.nodeID];
            ModelTypes::Node &prevJointNode = m_model->nodes()[prevJoint.nodeID];

            assert(prevJointNode.parent == currJoint.nodeID);
            if(currJointNode.parent != ModelTypes::NULL_ID) {
                currJoint.origPosition = glm::inverse(m_model->skin().inverseBindMatrices[currJoint.jointID]) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                currJoint.currPosition = currJoint.tempPosition = currJoint.origPosition;
                currJoint.origAnimTransform = m_model->nodes()[currJoint.nodeID].animationTransform;
                currJoint.origAnimTransformInverse = glm::inverse(currJoint.origAnimTransform);
                currJoint.distance = glm::distance(currJoint.currPosition, prevJoint.currPosition);
                currJoint.origDirection = glm::normalize(joints(jointInfoIndex - 1).currPosition - currJoint.currPosition);
                currJoint.currDirection = currJoint.origDirection;
                currJoint.tempDirection = currJoint.origDirection;
                currJoint.origBasis.y = currJoint.origDirection;
                currJoint.origBasis.x = Utils::closestOrthonormal(currJoint.origBasis.y, Axis::POS_X);
                currJoint.origBasis.z = glm::normalize(glm::cross(currJoint.origBasis.x, currJoint.origBasis.y));
                if(!Utils::isRightHanded(currJoint.origBasis.x, currJoint.origBasis.y, currJoint.origBasis.z)) {
                    currJoint.origBasis.z = -currJoint.origBasis.z;
                }
                currJoint.currBasis = currJoint.origBasis;

                currJoint.outer[0] = joints(jointInfoIndex - 1).jointID;
                currJoint.inner = jointInfoIndex < jointIDs.size() - 1 ? joints(jointInfoIndex + 1).jointID : nullptr;
            }
            prevJoint = currJoint;
        }
        if(jointIDs.size() > 1) {
            joints(0).origDirection = joints(1).currDirection;
            joints(0).currDirection = joints(0).origDirection;
            joints(0).tempDirection = joints(0).origDirection;
            joints(0).origBasis.y = joints(1).currDirection;
            joints(0).origBasis.x = Utils::closestOrthonormal(joints(0).origBasis.y, Axis::POS_X);
            joints(0).origBasis.z = glm::normalize(glm::cross(joints(0).origBasis.x, joints(0).origBasis.y));
            if(!Utils::isRightHanded(joints(0).origBasis.x, joints(0).origBasis.y, joints(0).origBasis.z)) {
                joints(0).origBasis.z = -joints(0).origBasis.z;
            }
            joints(0).currBasis = joints(0).origBasis;
        }
    };

    for(auto &partVector: m_bodyPartsJoints) {
        updateFunc(partVector);
    }

    // TODO Temp modification for testing, erase this later
    //m_bodyJoints[m_bodyPartsJoints[BODYPART_RARM].back()].inner = m_bodyJoints[m_bodyPartsJoints[BODYPART_SPINE].back()].jointID;
    Utils::enumSetBits(m_bodyJoints[m_bodyPartsJoints[BODYPART_RARM].back()].flags, JointInfo::Flags::FIXED_DIR);
}

void Animatrix::createDebugLines() {
    for(std::underlying_type_t<BodyPart> bodyPartID = 0; bodyPartID < m_bodyPartsJoints.size(); bodyPartID++) {
        const std::size_t lineCount = m_bodyPartsJoints[bodyPartID].size() - 1;
        const std::size_t indicesCount = lineCount * 2;
        const std::size_t verticesCount = m_bodyPartsJoints[bodyPartID].size();
        if(verticesCount <= 1) continue;
        debugJointLines[Utils::enumVal()bodyPartID].vertices.resize(verticesCount);
        debugJointLines[bodyPart].indices.resize(indicesCount);
        for(std::size_t lineIndex = 0; lineIndex < lineCount; lineIndex++) {
            debugJointLines[bodyPart].indices[lineIndex * 2] = lineIndex;
            debugJointLines[bodyPart].indices[lineIndex * 2 + 1] = lineIndex + 1;
        }
        for(std::size_t vertexIndex = 0; vertexIndex < verticesCount; vertexIndex++) {
            debugJointLines[bodyPart].vertices[vertexIndex].position = m_model->transformation.getMatrix() *
                                                                       glm::scale(glm::vec3(debugScaleFactor)) *
                                                                       (glm::vec4(joints[vertexIndex].currPosition - m_bodyNodes[BodyPart::SPINE].back().currPosition, 1.0f));
            debugJointLines[bodyPart].vertices[vertexIndex].color = {1.0f, 1.0f, 1.0f};// White
        }
    }

    for(const auto &[bodyPart, joints]: m_bodyNodes) {
        const std::size_t lineCount = joints.size() * 3;
        const std::size_t indicesCount = lineCount * 2;
        const std::size_t verticesCount = joints.size() * 6;
        if(joints.size() <= 1) continue;
        debugJointBasis[bodyPart].vertices.resize(verticesCount);
        debugJointBasis[bodyPart].indices.resize(indicesCount);
        for(std::size_t index = 0; index < indicesCount; index++) {
            debugJointBasis[bodyPart].indices[index] = index;
        }
        for(std::size_t vertexIndex = 0; vertexIndex < verticesCount; vertexIndex++) {
            debugJointBasis[bodyPart].vertices[vertexIndex].color = {0.0f, 0.0f, 1.0f};// Blue
            size_t jointIndex = std::floor(vertexIndex / 6.0f);
            switch(vertexIndex % 6) {
                case 0:
                    debugJointBasis[bodyPart].vertices[vertexIndex].position = debugJointLines[bodyPart].vertices[jointIndex].position;
                    debugJointBasis[bodyPart].vertices[vertexIndex].color = {1.0f, 0.0f, 0.0f};// Red
                    break;
                case 1:
                    debugJointBasis[bodyPart].vertices[vertexIndex].position = debugJointLines[bodyPart].vertices[jointIndex].position + (0.2f * joints[jointIndex].currBasis.x);
                    debugJointBasis[bodyPart].vertices[vertexIndex].color = {1.0f, 0.0f, 0.0f};// Red
                    break;
                case 2:
                    debugJointBasis[bodyPart].vertices[vertexIndex].position = debugJointLines[bodyPart].vertices[jointIndex].position;
                    debugJointBasis[bodyPart].vertices[vertexIndex].color = {0.0f, 1.0f, 0.0f};// Green
                    break;
                case 3:
                    debugJointBasis[bodyPart].vertices[vertexIndex].position = debugJointLines[bodyPart].vertices[jointIndex].position + (0.2f * joints[jointIndex].currBasis.y);
                    debugJointBasis[bodyPart].vertices[vertexIndex].color = {0.0f, 1.0f, 0.0f};// Green
                    break;
                case 4:
                    debugJointBasis[bodyPart].vertices[vertexIndex].position = debugJointLines[bodyPart].vertices[jointIndex].position;
                    debugJointBasis[bodyPart].vertices[vertexIndex].color = {0.0f, 0.0f, 1.0f};// Blue
                    break;
                case 5:
                    debugJointBasis[bodyPart].vertices[vertexIndex].position = debugJointLines[bodyPart].vertices[jointIndex].position + (0.2f * joints[jointIndex].currBasis.z);
                    debugJointBasis[bodyPart].vertices[vertexIndex].color = {0.0f, 0.0f, 1.0f};// Blue
                    break;
            }
        }
    }
}

void Animatrix::pullBodyPart(BodyPart bodyPart, const glm::vec3 &dest) {
    //m_bodyNodes[bodyPart][0].setPosition(m_model, dest);
    auto &joints = m_bodyNodes[bodyPart];
    joints[0].tempPosition = dest;
    joints[0].fixTempDirection();

    if(joints.size() > 1) {
        glm::vec3 rootPos = joints.back().currPosition;
        cascadeChange(bodyPart, false);

        bool fromRoot = true;
        float distance = glm::distance(joints.back().tempPosition, rootPos);
        while(distance != 0.0f) {
            std::size_t jointIndex = fromRoot ? joints.size() - 1 : 0;

            //glm::vec3 destPos = fromRoot ? rootPos : (joints[1].tempPosition + joints[1].tempDirection*joints[1].distance);
            //joints[jointIndex].tempPosition = destPos;
            if(fromRoot) joints[jointIndex].tempPosition = rootPos;
            if(fromRoot && Utils::enumCheckBit(joints[jointIndex].flags, JointInfo::Flags::FIXED_DIR)) {
                if(joints.size() > 1) {
                    joints[jointIndex - 1].tempPosition = joints[jointIndex].tempPosition + joints[jointIndex].currDirection * joints[jointIndex].distance;
                    joints[jointIndex - 1].fixTempDirection();
                    joints[jointIndex].fixTempDirection();
                }
            } else {
                joints[jointIndex].fixTempDirection();
            }

            cascadeChange(bodyPart, fromRoot);
            fromRoot = !fromRoot;
            distance = glm::distance(joints.back().tempPosition, rootPos);
        }

        do {
            joints.back().tempPosition = rootPos;
            cascadeConstraint(bodyPart, true);
            distance = glm::distance(joints.back().tempPosition, rootPos);
        } while(distance != 0.0f);

        for(std::size_t jointIndex = 0; jointIndex < joints.size(); jointIndex++) {
            //if(jointIndex == 0) {
            //    joints[jointIndex].currDirection = glm::normalize(joints[jointIndex].tempPosition - joints[jointIndex+1].tempPosition);
            //}else {
            //    joints[jointIndex].currDirection = glm::normalize(joints[jointIndex-1].tempPosition - joints[jointIndex].tempPosition);
            //}

            joints[jointIndex].currPosition = joints[jointIndex].tempPosition;
            joints[jointIndex].currDirection = joints[jointIndex].tempDirection;
            joints[jointIndex].clearModelTransform(m_model);
            joints[jointIndex].setModelPosition(m_model);
            joints[jointIndex].setModelDirection(m_model);
        }
    } else if(joints.size() == 1) {
        joints[0].currPosition = joints[0].tempPosition;
        joints[0].clearModelTransform(m_model);
        joints[0].setModelPosition(m_model);
    }

    updateDebug();
}

void Animatrix::cascadeChange(BodyPart bodyPart, bool fromRoot) {
    // Limbs that have a single bone have nothing to cascade
    if(m_bodyNodes[bodyPart].size() <= 1) return;
    glm::vec3 destPos;

    if(fromRoot) {
        // When cascading from root (closer to body center), we adjust joints in reverse
        for(std::size_t jointInfoIndex = m_bodyNodes[bodyPart].size() - 2; jointInfoIndex != SIZE_MAX; jointInfoIndex--) {

            JointInfo &outerJoint = m_bodyNodes[bodyPart][jointInfoIndex];
            JointInfo &innerJoint = m_bodyNodes[bodyPart][jointInfoIndex + 1];

            if(Utils::enumCheckBit(innerJoint.flags, JointInfo::Flags::FIXED_DIR)) {
                destPos = innerJoint.tempPosition + (innerJoint.currDirection * innerJoint.distance);
            } else {
                float newDistance = glm::distance(innerJoint.tempPosition, outerJoint.tempPosition);
                if(newDistance > innerJoint.distance) {// The joint moved further away
                    float distanceDiff = newDistance - innerJoint.distance;
                    destPos = Utils::interpolateBetween(outerJoint.tempPosition, innerJoint.tempPosition, distanceDiff / newDistance);
                } else if(newDistance < innerJoint.distance) {// The joint moved closer
                    float distanceDiff = innerJoint.distance - newDistance;
                    destPos = Utils::interpolateBetween(innerJoint.tempPosition, outerJoint.tempPosition, 1.0f + distanceDiff / innerJoint.distance);
                } else {
                    // If this joint doesn't have to adjust, the rest wouldn't have to as well
                    // This seems improbable but keeping it here anyway
                    break;
                }
            }

            outerJoint.tempPosition = destPos;
            outerJoint.fixTempDirection();
        }
    } else {
        //m_bodyNodes[bodyPart][0].applyConstraint(fromRoot);

        for(std::size_t jointInfoIndex = 1; jointInfoIndex < m_bodyNodes[bodyPart].size(); jointInfoIndex++) {
            JointInfo &innerJoint = m_bodyNodes[bodyPart][jointInfoIndex];
            JointInfo &outerJoint = m_bodyNodes[bodyPart][jointInfoIndex - 1];

            if(Utils::enumCheckBit(innerJoint.flags, JointInfo::Flags::FIXED_DIR)) {
                destPos = outerJoint.tempPosition + ((-innerJoint.currDirection) * innerJoint.distance);
            } else {
                const float newDistance = glm::distance(innerJoint.tempPosition, outerJoint.tempPosition);
                if(newDistance > innerJoint.distance) {// The joint moved further away
                    float distanceDiff = newDistance - innerJoint.distance;
                    destPos = Utils::interpolateBetween(innerJoint.tempPosition, outerJoint.tempPosition, distanceDiff / newDistance);
                } else if(newDistance < innerJoint.distance) {// The joint moved closer
                    float distanceDiff = innerJoint.distance - newDistance;
                    destPos = Utils::interpolateBetween(outerJoint.tempPosition, innerJoint.tempPosition, 1.0f + distanceDiff / innerJoint.distance);
                } else {
                    // If this joint doesn't have to adjust, the rest wouldn't have to as well
                    // This seems improbable but keeping it here anyway
                    break;
                }
            }

            innerJoint.tempPosition = destPos;
            innerJoint.fixTempDirection();
        }
    }
}

void Animatrix::cascadeConstraint(BodyPart bodyPart, bool fromRoot) {
    // Limbs that have a single bone have nothing to cascade
    if(m_bodyNodes[bodyPart].size() <= 1) return;

    if(fromRoot) {
        for(std::size_t jointInfoIndex = m_bodyNodes[bodyPart].size() - 1; jointInfoIndex != SIZE_MAX; jointInfoIndex--) {
            if(jointInfoIndex != m_bodyNodes[bodyPart].size() - 1 && Utils::enumCheckBit(m_bodyNodes[bodyPart][jointInfoIndex + 1].flags, JointInfo::Flags::FIXED_DIR)) continue;
            m_bodyNodes[bodyPart][jointInfoIndex].applyConstraint(fromRoot);
            m_bodyNodes[bodyPart][jointInfoIndex].fixTempDirection();
        }
    } else {
        for(std::size_t jointInfoIndex = 1; jointInfoIndex < m_bodyNodes[bodyPart].size(); jointInfoIndex++) {
            if(Utils::enumCheckBit(m_bodyNodes[bodyPart][jointInfoIndex - 1].flags, JointInfo::Flags::FIXED_DIR)) continue;
            m_bodyNodes[bodyPart][jointInfoIndex].applyConstraint(fromRoot);
            m_bodyNodes[bodyPart][jointInfoIndex].fixTempDirection();
        }
    }
}

void Animatrix::JointInfo::applyTransformation(Model *model, const glm::mat4 &t) const {
    model->nodes()[nodeID].animationTransform = t * model->nodes()[nodeID].animationTransform;
}

void Animatrix::JointInfo::setTransformation(Model *model, const glm::mat4 &t) const {
    model->nodes()[nodeID].animationTransform = t;
}

void Animatrix::JointInfo::setModelPosition(Model *model) const {
    if(currPosition == origPosition) return;

    // The difference between original and current position
    // is used to create a translation matrix that transforms this difference
    glm::vec3 diffVector = currPosition - origPosition;
    applyTransformation(model, glm::translate(glm::identity<glm::mat4>(), diffVector));
}

void Animatrix::JointInfo::setModelDirection(Model *model) {
    if(currDirection == origDirection) return;

    // Creates an angle and rotation axis on which to rotate between the two directions
    glm::vec3 axis = glm::normalize(glm::cross(currDirection, origDirection));
    const float angle = -glm::acos(glm::dot(currDirection, origDirection));
    const glm::mat4 rotMat = glm::rotate(glm::identity<glm::mat4>(), angle, axis);
    const glm::mat4 toOrig = glm::translate(glm::identity<glm::mat4>(), -currPosition);
    const glm::mat4 fromOrig = glm::translate(glm::identity<glm::mat4>(), currPosition);

    currBasis.x = glm::rotate(origBasis.x, angle, axis);
    currBasis.y = glm::rotate(origBasis.y, angle, axis);
    currBasis.z = glm::rotate(origBasis.z, angle, axis);

    applyTransformation(model, fromOrig * rotMat * toOrig);
}

void Animatrix::JointInfo::clearModelTransform(Model *model) const {
    setTransformation(model, origAnimTransform);
}

void Animatrix::JointInfo::fixTempDirection() {
    if(outer) {
        this->tempDirection = glm::normalize(outer->tempPosition - this->tempPosition);
    } else if(inner) {
        this->tempDirection = glm::normalize(this->tempPosition - inner->tempPosition);
    }
}

void Animatrix::JointInfo::applyConstraint(bool fromRoot) {
    JointInfo *changed = fromRoot ? inner : outer;
    if(!changed) return;
    if(fromRoot) {
        if(!changed->inner) return;
    } else {
        if(!changed->outer) return;
    }

    glm::vec3 L1;
    if(fromRoot) {
        L1 = changed->inner->tempDirection;
    } else {
        L1 = -changed->outer->tempDirection;
    }
    glm::vec3 Svec = glm::dot(this->tempPosition - changed->tempPosition, L1) * L1;

    float S = glm::length(Svec);
    glm::vec3 O = changed->tempPosition + Svec;

    glm::vec4 cLengths{};
    for(int i = 0; i < 4; i++) {
        cLengths[i] = S * std::tan(angleConstraints[i]);
    }
    glm::vec3 axis_Y = glm::cross(L1, Axis::POS_Y);

    float len_Y = glm::length(axis_Y);

    if(len_Y > 1.0f) len_Y = 1.0f;

    float angle_Y = glm::acos(glm::dot(L1, Axis::POS_Y));

    const glm::mat4 toOrig = glm::translate(glm::identity<glm::mat4>(), -O);
    const glm::mat4 fromOrig = glm::translate(glm::identity<glm::mat4>(), O);

    glm::vec3 onOrig = toOrig * glm::vec4(this->tempPosition, 1.0f);
    onOrig = glm::rotate(onOrig, angle_Y, glm::normalize(axis_Y));

    int indexA = -1;
    int indexB = -1;
    float &x = onOrig.x;
    float &y = onOrig.z;

    if(x > 0 && y > 0) {// Quadrant I
        indexA = 0;
        indexB = 1;
    } else if(x < 0 && y > 0) {// Quadrant II
        indexA = 2;
        indexB = 1;
    } else if(x < 0 && y < 0) {// Quadrant III
        indexA = 2;
        indexB = 3;
    } else if(x > 0 && y < 0) {// Quadrant IV
        indexA = 0;
        indexB = 3;
    } else {
        return;// No need to correct
    }

    assert(indexA != -1 && indexB != -1);

    float &a = cLengths[indexA];
    float &b = cLengths[indexB];

    if(Utils::ellipse(x, y, a, b) > 1.0f) {
        float d = std::atan2(y, x);
        float k = (a * b) / std::sqrtf(b * b * std::cosf(d) * std::cosf(d) + a * a * std::sinf(d) * std::sinf(d));
        x = k * std::cosf(d);
        y = k * std::sinf(d);
    } else {
        return;
    }

    onOrig = glm::rotate(onOrig, -angle_Y, axis_Y);
    this->tempPosition = fromOrig * glm::vec4(onOrig, 1.0f);
    const float newDistance = glm::distance(this->tempPosition, changed->tempPosition);
    glm::vec3 destPos;
    if(fromRoot) {
        if(newDistance > changed->distance) {// The joint moved further away
            float distanceDiff = newDistance - changed->distance;
            destPos = Utils::interpolateBetween(this->tempPosition, changed->tempPosition, distanceDiff / newDistance);
        } else if(newDistance < changed->distance) {// The joint moved closer
            float distanceDiff = changed->distance - newDistance;
            destPos = Utils::interpolateBetween(changed->tempPosition, this->tempPosition, 1.0f + distanceDiff / changed->distance);
        } else {
            return;
        }
    } else {
        if(newDistance > this->distance) {// The joint moved further away
            float distanceDiff = newDistance - this->distance;
            destPos = Utils::interpolateBetween(this->tempPosition, changed->tempPosition, distanceDiff / newDistance);
        } else if(newDistance < this->distance) {// The joint moved closer
            float distanceDiff = this->distance - newDistance;
            destPos = Utils::interpolateBetween(changed->tempPosition, this->tempPosition, 1.0f + distanceDiff / this->distance);
        } else {
            return;
        }
    }

    this->tempPosition = destPos;
}


void Animatrix::updateDebug() {
    for(const auto &[bodyPart, joints]: m_bodyNodes) {
        if(joints.size() <= 1) continue;
        for(std::size_t jointIndex = 0; jointIndex < joints.size(); jointIndex++) {
            debugJointLines[bodyPart].vertices[jointIndex].position = m_model->transformation.getMatrix() *
                                                                      glm::scale(glm::vec3(debugScaleFactor)) *
                                                                      (glm::vec4(joints[jointIndex].currPosition - m_bodyNodes[BodyPart::SPINE].back().currPosition, 1.0f));
        }
    }

    for(const auto &[bodyPart, joints]: m_bodyNodes) {
        const std::size_t verticesCount = joints.size() * 6;
        if(joints.size() <= 1) continue;
        for(std::size_t vertexIndex = 0; vertexIndex < verticesCount; vertexIndex++) {
            size_t jointIndex = std::floor(vertexIndex / 6.0f);
            switch(vertexIndex % 6) {
                case 4:
                case 2:
                case 0:
                    debugJointBasis[bodyPart].vertices[vertexIndex].position = debugJointLines[bodyPart].vertices[jointIndex].position;
                    break;
                case 1:
                    debugJointBasis[bodyPart].vertices[vertexIndex].position = debugJointLines[bodyPart].vertices[jointIndex].position + (0.2f * joints[jointIndex].currBasis.x);
                    break;
                case 3:
                    debugJointBasis[bodyPart].vertices[vertexIndex].position = debugJointLines[bodyPart].vertices[jointIndex].position + (0.2f * joints[jointIndex].currBasis.y);
                    break;
                case 5:
                    debugJointBasis[bodyPart].vertices[vertexIndex].position = debugJointLines[bodyPart].vertices[jointIndex].position + (0.2f * joints[jointIndex].currBasis.z);
                    break;
            }
        }
    }
}

const char *bodyPart2string(Animatrix::BodyPart part) {
    switch(part) {
        case Animatrix::BodyPart::HEAD:
            return "Head";
        case Animatrix::BodyPart::LARM:
            return "Left Arm";
        case Animatrix::BodyPart::RARM:
            return "Right Arm";
        case Animatrix::BodyPart::LLEG:
            return "Left Leg";
        case Animatrix::BodyPart::RLEG:
            return "Right Leg";
        case Animatrix::BodyPart::SPINE:
            return "Spine";
        default:
            return "Unknown";
    }
}

void Animatrix::runImGui() {
    if(!ImGui::Begin("Animatrix")) {
        ImGui::End();
        return;
    }
    for(auto &instance: m_instances) {
        if(!ImGui::TreeNode(instance->model()->path().c_str())) continue;

        ImGui::SeparatorText("Actions");
        ImGui::PushID("Actions");
        for(auto &[id, action]: instance->actions) {
            if(!ImGui::TreeNode(bodyPart2string(action.body))) continue;
            ImGui::DragFloat3("Position", &action.target[0]);
            ImGui::TreePop();
        }
        ImGui::PopID();

        ImGui::SeparatorText("Joints");
        ImGui::PushID("Joints");
        for(auto &[bodyPart, partVector]: instance->m_bodyNodes) {
            if(!ImGui::TreeNode(bodyPart2string(bodyPart))) continue;
            for(auto &joint: partVector) {
                if(!ImGui::TreeNode(std::to_string(joint.jointID).c_str())) continue;
                ImGui::Text("Distance: %f", joint.distance);
                ImGui::Text("Current position: %f,%f,%f", joint.currPosition.x, joint.currPosition.y, joint.currPosition.z);
                ImGui::Text("Temporary position: %f,%f,%f", joint.tempPosition.x, joint.tempPosition.y, joint.tempPosition.z);
                ImGui::Text("Original position: %f,%f,%f", joint.origPosition.x, joint.origPosition.y, joint.origPosition.z);
                ImGui::Text("Current direction: %f,%f,%f", joint.currDirection.x, joint.currDirection.y, joint.currDirection.z);
                ImGui::Text("Temporary direction: %f,%f,%f", joint.tempDirection.x, joint.tempDirection.y, joint.tempDirection.z);
                ImGui::Text("Original direction: %f,%f,%f", joint.origDirection.x, joint.origDirection.y, joint.origDirection.z);
                glm::vec4 degConstraint = glm::degrees(joint.angleConstraints);
                if(ImGui::DragFloat4("Constraints", &degConstraint[0])) {
                    joint.angleConstraints = glm::radians(degConstraint);
                }
                ImGui::Text("Inner/Outer IDs: %d/%d", joint.inner ? joint.inner->jointID : -1, joint.outer ? joint.outer->jointID : -1);
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
        ImGui::TreePop();
    }
    ImGui::End();
}