#include "Animatrix.h"

#include <utility>
#include <bits/ranges_algo.h>
#include <cmath>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <numeric>
#include <queue>
#include <ranges>

#include "engine/GlobalMemory.h"
#include "../../imgui/ImGuiUtils.h"
#include "engine/camera/Camera.h"
#include "utils/Utils.h"

Animatrix::Animatrix(Model *model) : m_model(model) {
    assert(m_model);
    if(!loadArmature()) {
        LOG(LOG_ERROR, "Model [" << m_model->path() << "] has invalid armature");
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
                if(action.currentTime >= action.destinationTime) break;
                JointInfo *joint = bodyPartJoint(action.body, action.bodyPartID);
                glm::vec3 &pos = joint->currPosition;
                action.currentTime += TecCorePtr->deltaTime;
                float ratio = action.currentTime / action.destinationTime;
                if(ratio > 1.0f) ratio = 1.0f;
                glm::vec3 newPos = Utils::interpolateBetween(pos, action.target, ratio);
                pullJoint(joint, newPos);
                break;
            }
        }
    }
    for(auto &[seqName, seq]: actionSequences) {
        if(seq.groups.empty()) break;

    repeat:
        ActionGroup &grp = seq.groups[seq.currentGroup];
        if(grp.currentTime >= grp.destinationTime) {
            grp.currentTime = 0.0f;
            seq.currentGroup = (seq.currentGroup + 1) % seq.groups.size();
            for(auto &act: grp.actions) act.currentTime = 0.0f;
            goto repeat;
        }
        for(auto &act: grp.actions) {
            switch(act.type) {
                case ActionType::PULLING: {
                    if(act.currentTime >= act.destinationTime) break;
                    JointInfo *joint = bodyPartJoint(act.body, act.bodyPartID);
                    glm::vec3 &pos = joint->currPosition;
                    act.currentTime += TecCorePtr->deltaTime;
                    float ratio = act.currentTime / act.destinationTime;
                    if(ratio > 1.0f) ratio = 1.0f;
                    glm::vec3 newPos = Utils::interpolateBetween(pos, act.target, ratio);
                    pullJoint(joint, newPos);
                    break;
                }
            }
        }
        grp.currentTime += TecCorePtr->deltaTime;
    }
    applyConstraints();

    updateModel();
    updateDebug();
    m_model->uploadJointsMatrices();
}

void writeAction(SerialTypes::BinDataVec_t &data, const Animatrix::Action &act) {
    // Write action body part [8bits]
    Serial::pushData<Animatrix::BodyPart>(data, act.body);

    // Write action body part ID [8bits]
    Serial::pushData<uint8_t>(data, act.bodyPartID);

    // Write action target [sizeof(glm::vec3)]
    Serial::pushData<glm::vec3>(data, act.target);

    // Write action destiation time [sizeof(float)]
    Serial::pushData<float>(data, act.destinationTime);
}

void writeActionGroup(SerialTypes::BinDataVec_t &data, const Animatrix::ActionGroup &grp) {
    // Write group name size + data [32bits + 8bits * size]
    Serial::pushData<char>(data, grp.name);

    // Write group destiation time [sizeof(float)]
    Serial::pushData<float>(data, grp.destinationTime);

    // Write actions size + data [32bits + sizeof(Action) * size]
    Serial::pushData<uint32_t>(data, grp.actions.size());
    for(const auto &act: grp.actions) {
        writeAction(data, act);
    }
}

std::optional<SerialTypes::BinDataVec_t> Animatrix::serializeSequence(const char *name) const {
    if(!actionSequences.contains(name)) return {};
    const ActionSequence &actionSequence = actionSequences.at(name);
    SerialTypes::BinDataVec_t data;

    // Write sequence name size + data [32bits + 8bits * size]
    Serial::pushData<char>(data, actionSequence.name.data());

    // Write groups size + data [32bits + sizeof(ActionGroup) * size]
    Serial::pushData<uint32_t>(data, actionSequence.groups.size());
    for(const auto &grp: actionSequence.groups) {
        writeActionGroup(data, grp);
    }

    return data;
}

std::optional<Animatrix::Action> readAction(SerialTypes::BinDataVec_t &data, std::size_t &offset) {
    Animatrix::Action act;

    // Read action body part [8bits]
    act.body = Serial::readDataAtInc<Animatrix::BodyPart>(data, offset);

    // Read action body part ID [8bits]
    act.bodyPartID = Serial::readDataAtInc<uint8_t>(data, offset);

    // Read action target [sizeof(glm::vec3)]
    act.target = Serial::readDataAtInc<glm::vec3>(data, offset);

    // Read action destiation time [sizeof(float)]
    act.destinationTime = Serial::readDataAtInc<float>(data, offset);

    return act;
}

std::optional<Animatrix::ActionGroup> readActionGroup(SerialTypes::BinDataVec_t &data, std::size_t &offset) {
    Animatrix::ActionGroup grp;

    // Read group name size + data [32bits + 8bits * size]
    grp.name = std::string(Serial::readSpanAtInc<char>(data, offset).data());

    // Write group destiation time [sizeof(float)]
    grp.destinationTime = Serial::readDataAtInc<float>(data, offset);

    // Write actions size + data [32bits + sizeof(Action) * size]
    uint32_t actCount = Serial::readDataAtInc<uint32_t>(data, offset);
    while(actCount) {
        auto act = readAction(data, offset);
        if(act.has_value()) grp.actions.push_back(std::move(act.value()));
        else
            return {};
        --actCount;
    }

    return grp;
}

bool Animatrix::deserializeSequence(SerialTypes::BinDataVec_t &data) {
    ActionSequence seq;
    std::size_t offset = 0;

    const std::span<char> name = Serial::readSpanAtInc<char>(data, offset);
    seq.name = std::string(name.cbegin(), name.cend());
    uint32_t grpCount = Serial::readDataAtInc<uint32_t>(data, offset);
    while(grpCount) {
        auto grp = readActionGroup(data, offset);
        if(grp.has_value()) seq.groups.push_back(std::move(grp.value()));
        else
            return false;
        --grpCount;
    }

    actionSequences[seq.name] = std::move(seq);

    return true;
}

Model *Animatrix::model() const {
    return m_model;
}

bool Animatrix::loadArmature() {
    if(!m_model->isSkinned()) {
        LOG(LOG_ERROR, "Model [" << m_model->path() << "] has no skin\n");
        return false;
    }

    // Empheral joint struct when loading from model skeleton
    struct loadedJoint {
        BodyPart bodyPart;
        uint32_t bodyPartID;
        uint32_t jointID;
        uint32_t nodeID;
    };

    ModelTypes::Skin &skin = m_model->skin();
    std::vector<loadedJoint> bodyJointsBuffer;
    uint32_t numOfJoints = skin.joints.size();
    bodyJointsBuffer.reserve(numOfJoints);
    uint32_t jointID = 0;
    for(const auto &nID: skin.joints) {
        ModelTypes::Node &skinJoint = m_model->nodes()[nID];
        std::cmatch m;
        if(!std::regex_match(skinJoint.name.data(), m, m_skinNodeRegex)) {
            LOG(LOG_ERROR, "Model [" << m_model->path() << "] has invalid skin joint name [" << skinJoint.name.data() << ']');
            return false;
        }

        // Switching based on first char of name
        switch(m[1].str()[0]) {
            case 's':// spine
                bodyJointsBuffer.push_back(loadedJoint{
                        .bodyPart = BODYPART_SPINE,
                        .bodyPartID = static_cast<uint32_t>(std::stoi(m[2])),
                        .jointID = jointID++,
                        .nodeID = nID});
                break;

            case 'h':// head
                bodyJointsBuffer.push_back(loadedJoint{
                        .bodyPart = BODYPART_HEAD,
                        .bodyPartID = static_cast<uint32_t>(std::stoi(m[2])),
                        .jointID = jointID++,
                        .nodeID = nID});
                break;

            case 'l':// leg
                switch(m[3].str()[0]) {
                    case 'L':
                        bodyJointsBuffer.push_back(loadedJoint{
                                .bodyPart = BODYPART_LLEG,
                                .bodyPartID = static_cast<uint32_t>(std::stoi(m[2])),
                                .jointID = jointID++,
                                .nodeID = nID});
                        break;
                    case 'R':
                        bodyJointsBuffer.push_back(loadedJoint{
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
                        bodyJointsBuffer.push_back(loadedJoint{
                                .bodyPart = BODYPART_LARM,
                                .bodyPartID = static_cast<uint32_t>(std::stoi(m[2])),
                                .jointID = jointID++,
                                .nodeID = nID});
                        break;
                    case 'R':
                        bodyJointsBuffer.push_back(loadedJoint{
                                .bodyPart = BODYPART_RARM,
                                .bodyPartID = static_cast<uint32_t>(std::stoi(m[2])),
                                .jointID = jointID++,
                                .nodeID = nID});
                        break;
                }
                break;
        }
    }

    std::ranges::sort(bodyJointsBuffer.begin(), bodyJointsBuffer.end(), [](const loadedJoint &a, const loadedJoint &b) {
        return a.jointID < b.jointID;
    });

    assert(std::accumulate(bodyJointsBuffer.begin(), bodyJointsBuffer.end(), 0,
                           [](uint32_t acc, const loadedJoint &joint) { return acc + joint.jointID; }) == ((numOfJoints - 1) * (numOfJoints)) / 2);

    /*
        if(bodyNodes[BodyPart::LLEG].size() != bodyNodes[BodyPart::RLEG].size()) {
            LOG(LOG_WARNING,"Model [" << m_model->path() << "] has different number of joins in legs");
        }
        if(bodyNodes[BodyPart::LARM].size() != bodyNodes[BodyPart::RARM].size()) {
            LOG(LOG_WARNING,"Model [" << m_model->path() << "] has different number of joins in arms");
        }
    */

    auto copyFunc = [](const loadedJoint &joint) {
        JointInfo info;
        info.distance = 0.0f;
        info.nodeID = joint.nodeID;
        info.jointID = joint.jointID;
        info.bodyPart = joint.bodyPart;
        info.bodyPartID = joint.bodyPartID;
        return info;
    };

    std::ranges::transform(bodyJointsBuffer.begin(), bodyJointsBuffer.end(), std::back_inserter(bodyJoints), copyFunc);
    bodyJoints.shrink_to_fit();

    loadBodyPartVectors();
    loadJointGeometry();
    connectBodyParts();
    createDebugLines();

    return true;
}

void Animatrix::loadBodyPartVectors() {
    for(auto &joint: bodyJoints) {
        switch(joint.bodyPart) {
            case BODYPART_SPINE:
                bodyPartsJoints[BODYPART_SPINE].push_back(&joint);
                break;
            case BODYPART_HEAD:
                bodyPartsJoints[BODYPART_HEAD].push_back(&joint);
                break;
            case BODYPART_LLEG:
                bodyPartsJoints[BODYPART_LLEG].push_back(&joint);
                break;
            case BODYPART_RLEG:
                bodyPartsJoints[BODYPART_RLEG].push_back(&joint);
                break;
            case BODYPART_LARM:
                bodyPartsJoints[BODYPART_LARM].push_back(&joint);
                break;
            case BODYPART_RARM:
                bodyPartsJoints[BODYPART_RARM].push_back(&joint);
                break;
            default:
                assert(false);
        }
    }

    for(auto &jointVec: bodyPartsJoints) {
        std::ranges::sort(jointVec.begin(), jointVec.end(), [this](JointInfo *a, JointInfo *b) {
            return a->bodyPartID < b->bodyPartID;
        });
    }
}

Animatrix::JointInfo::FixedJointInfo::FixedJointInfo(JointInfo &splitJoint, JointInfo &fixedJoint) {
    this->joint = &fixedJoint;
    this->distance = glm::distance(splitJoint.currPosition, fixedJoint.currPosition);
    this->direction = glm::normalize(fixedJoint.currPosition - splitJoint.currPosition);
}

void Animatrix::connectBodyParts() {
    /// TODO Connections between the body parts, this should be in the file/model
    bodyPartJointInner(BODYPART_LARM)->inner = bodyPartJointOuter(BODYPART_SPINE);
    bodyPartJointInner(BODYPART_RARM)->inner = bodyPartJointOuter(BODYPART_SPINE);
    bodyPartJointInner(BODYPART_HEAD)->inner = bodyPartJointOuter(BODYPART_SPINE);
    bodyPartJointOuter(BODYPART_SPINE)->outer = bodyPartJointInner(BODYPART_HEAD);
    bodyPartJointOuter(BODYPART_SPINE)->outerFixed[0] = JointInfo::FixedJointInfo(*bodyPartJointOuter(BODYPART_SPINE), *bodyPartJointInner(BODYPART_LARM));
    bodyPartJointInner(BODYPART_LARM)->outerFixedID = 0;
    bodyPartJointOuter(BODYPART_SPINE)->outerFixed[1] = JointInfo::FixedJointInfo(*bodyPartJointOuter(BODYPART_SPINE), *bodyPartJointInner(BODYPART_RARM));
    bodyPartJointInner(BODYPART_RARM)->outerFixedID = 1;
    bodyPartJointInner(BODYPART_LLEG)->inner = bodyPartJointInner(BODYPART_SPINE);
    bodyPartJointInner(BODYPART_RLEG)->inner = bodyPartJointInner(BODYPART_SPINE);
    bodyPartJointInner(BODYPART_SPINE)->outerFixed[0] = JointInfo::FixedJointInfo(*bodyPartJointInner(BODYPART_SPINE), *bodyPartJointInner(BODYPART_LLEG));
    bodyPartJointInner(BODYPART_LLEG)->outerFixedID = 0;
    bodyPartJointInner(BODYPART_SPINE)->outerFixed[1] = JointInfo::FixedJointInfo(*bodyPartJointInner(BODYPART_SPINE), *bodyPartJointInner(BODYPART_RLEG));
    bodyPartJointInner(BODYPART_RLEG)->outerFixedID = 1;

    Utils::enumSetBits(bodyPartJointInner(BODYPART_SPINE)->flags, JointInfo::Flags::IS_SPLIT);
    Utils::enumSetBits(bodyPartJointOuter(BODYPART_SPINE)->flags, JointInfo::Flags::IS_SPLIT);

    Utils::enumSetBits(bodyPartJointInner(BODYPART_RARM)->flags, JointInfo::Flags::FIXED_DIR);
    Utils::enumSetBits(bodyPartJointInner(BODYPART_LARM)->flags, JointInfo::Flags::FIXED_DIR);
    Utils::enumSetBits(bodyPartJointInner(BODYPART_RLEG)->flags, JointInfo::Flags::FIXED_DIR);
    Utils::enumSetBits(bodyPartJointInner(BODYPART_LLEG)->flags, JointInfo::Flags::FIXED_DIR);

    Utils::enumSetBits(bodyPartJointInner(BODYPART_SPINE)->flags, JointInfo::Flags::IS_ROOT);
    rootJoint = bodyPartJointInner(BODYPART_SPINE);

    splitJoints.push_back(bodyPartJointInner(BODYPART_SPINE));
    splitJoints.push_back(bodyPartJointOuter(BODYPART_SPINE));

    fixedJoints.push_back(bodyPartJointInner(BODYPART_RARM));
    fixedJoints.push_back(bodyPartJointInner(BODYPART_LARM));
    fixedJoints.push_back(bodyPartJointInner(BODYPART_RLEG));
    fixedJoints.push_back(bodyPartJointInner(BODYPART_LLEG));
}

void Animatrix::loadJointGeometry() {
    auto updateFunc = [this](std::underlying_type_t<BodyPart> bodyPart) {
        //auto joints = [this, &jointIDs](uint32_t index)->JointInfo&{ return bodyJoints[jointIDs[index]]; };

        // The position of each bone can be calculated by transforming an origin point by inverse of inverse bind transformation of that bone (this took me whole night)
        bodyPartJointOuter(bodyPart)->origPosition = glm::inverse(m_model->skin().inverseBindMatrices[bodyPartJointOuter(bodyPart)->jointID]) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        bodyPartJointOuter(bodyPart)->currPosition = bodyPartJointOuter(bodyPart)->tempPosition = bodyPartJointOuter(bodyPart)->origPosition;
        bodyPartJointOuter(bodyPart)->origAnimTransform = m_model->nodes()[bodyPartJointOuter(bodyPart)->nodeID].animationTransform;
        bodyPartJointOuter(bodyPart)->origAnimTransformInverse = glm::inverse(bodyPartJointOuter(bodyPart)->origAnimTransform);

        JointInfo *prevJoint = bodyPartJointOuter(bodyPart);
        if(bodyPartSize(bodyPart) > 1) {
            bodyPartJointOuter(bodyPart)->inner = bodyPartJoint(bodyPart, bodyPartSize(bodyPart) - 2);
        }

        for(size_t jointInfoIndex = bodyPartSize(bodyPart) - 2; jointInfoIndex != std::numeric_limits<std::size_t>::max(); jointInfoIndex--) {

            JointInfo *currJoint = bodyPartJoint(bodyPart, jointInfoIndex);
            ModelTypes::Node &currJointNode = m_model->nodes()[currJoint->nodeID];
            ModelTypes::Node &prevJointNode = m_model->nodes()[prevJoint->nodeID];

            assert(prevJointNode.parent == currJoint->nodeID);
            if(currJointNode.parent != ModelTypes::NULL_ID) {
                currJoint->origPosition = glm::inverse(m_model->skin().inverseBindMatrices[currJoint->jointID]) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                currJoint->currPosition = currJoint->tempPosition = currJoint->origPosition;
                currJoint->origAnimTransform = m_model->nodes()[currJoint->nodeID].animationTransform;
                currJoint->origAnimTransformInverse = glm::inverse(currJoint->origAnimTransform);
                currJoint->distance = glm::distance(currJoint->origPosition, prevJoint->origPosition);

                currJoint->outer = prevJoint;
                currJoint->inner = jointInfoIndex > 0 ? bodyPartJoint(bodyPart, jointInfoIndex - 1) : nullptr;
            }
            prevJoint = currJoint;
        }

        for(size_t jointInfoIndex = bodyPartSize(bodyPart) - 2; jointInfoIndex != std::numeric_limits<std::size_t>::max(); jointInfoIndex--) {
            JointInfo *currJoint = bodyPartJoint(bodyPart, jointInfoIndex);

            currJoint->origDirection = glm::normalize(bodyPartJoint(bodyPart, jointInfoIndex + 1)->currPosition - currJoint->currPosition);
            currJoint->currDirection = currJoint->origDirection;
            currJoint->tempDirection = currJoint->origDirection;
            currJoint->origBasis.y = currJoint->origDirection;
            currJoint->origBasis.x = Utils::closestOrthonormal(currJoint->origBasis.y, Axis::POS_X);
            currJoint->origBasis.z = glm::normalize(glm::cross(currJoint->origBasis.x, currJoint->origBasis.y));
            if(!Utils::isRightHanded(currJoint->origBasis.x, currJoint->origBasis.y, currJoint->origBasis.z)) {
                currJoint->origBasis.z = -currJoint->origBasis.z;
            }
            currJoint->currBasis = currJoint->origBasis;
        }

        if(bodyPartSize(bodyPart) > 1) {
            bodyPartJointOuter(bodyPart)->distance = glm::distance(bodyPartJointOuter(bodyPart)->origPosition, bodyPartJointOuter(bodyPart)->inner->origPosition);
            bodyPartJointOuter(bodyPart)->origDirection = bodyPartJointOuter(bodyPart)->inner->origDirection;
            bodyPartJointOuter(bodyPart)->currDirection = bodyPartJointOuter(bodyPart)->origDirection;
            bodyPartJointOuter(bodyPart)->tempDirection = bodyPartJointOuter(bodyPart)->origDirection;
            bodyPartJointOuter(bodyPart)->origBasis.y = bodyPartJointOuter(bodyPart)->origDirection;
            bodyPartJointOuter(bodyPart)->origBasis.x = Utils::closestOrthonormal(bodyPartJointOuter(bodyPart)->origBasis.y, Axis::POS_X);
            bodyPartJointOuter(bodyPart)->origBasis.z = glm::normalize(glm::cross(bodyPartJointOuter(bodyPart)->origBasis.x, bodyPartJointOuter(bodyPart)->origBasis.y));
            if(!Utils::isRightHanded(bodyPartJointOuter(bodyPart)->origBasis.x, bodyPartJointOuter(bodyPart)->origBasis.y, bodyPartJointOuter(bodyPart)->origBasis.z)) {
                bodyPartJointOuter(bodyPart)->origBasis.z = -bodyPartJointOuter(bodyPart)->origBasis.z;
            }
            bodyPartJointOuter(bodyPart)->currBasis = bodyPartJointOuter(bodyPart)->origBasis;
        }
    };

    for(std::underlying_type_t<BodyPart> bodyPartID = 0; bodyPartID < bodyPartsJoints.size(); bodyPartID++) {
        updateFunc(bodyPartID);
    }
}

void Animatrix::createDebugLines() {
    for(std::underlying_type_t<BodyPart> bodyPartID = 0; bodyPartID < bodyPartsJoints.size(); bodyPartID++) {
        const std::size_t lineCount = bodyPartSize(bodyPartID) - 1;
        const std::size_t indicesCount = lineCount * 2;
        const std::size_t verticesCount = bodyPartSize(bodyPartID);
        if(verticesCount <= 1) continue;
        debugJointLines[bodyPartID].vertices.resize(verticesCount);
        debugJointLines[bodyPartID].indices.resize(indicesCount);
        for(std::size_t lineIndex = 0; lineIndex < lineCount; lineIndex++) {
            debugJointLines[bodyPartID].indices[lineIndex * 2] = lineIndex;
            debugJointLines[bodyPartID].indices[lineIndex * 2 + 1] = lineIndex + 1;
        }
        for(std::size_t vertexIndex = 0; vertexIndex < verticesCount; vertexIndex++) {
            debugJointLines[bodyPartID].vertices[vertexIndex].position = glm::vec4(bodyPartJoint(bodyPartID, vertexIndex)->currPosition, 1.0f);

            debugJointLines[bodyPartID].vertices[vertexIndex].color = {1.0f, 1.0f, 1.0f};// White
        }
    }

    for(const auto &joint: splitJoints) {
        debugJointSplits.push_back({});
        VktTypes::PointMesh &mesh = debugJointSplits.back();
        std::size_t lineCount = 0;
        while(joint->outerFixed[lineCount].joint != nullptr) lineCount++;
        const std::size_t indicesCount = lineCount * 2;
        const std::size_t verticesCount = lineCount * 2;
        mesh.vertices.resize(verticesCount);
        mesh.indices.resize(indicesCount);
        for(std::size_t index = 0; index < indicesCount; index++) {
            mesh.indices[index] = index;
        }

        for(std::size_t vertexIndex = 0; vertexIndex < verticesCount; vertexIndex++) {
            switch(vertexIndex % 2) {
                case 0:
                    mesh.vertices[vertexIndex].position = glm::vec4(joint->currPosition, 1.0f);
                    mesh.vertices[vertexIndex].color = {0.6313f, 0.0470f, 0.6745f};// Purple-ish
                    break;
                case 1:
                    mesh.vertices[vertexIndex].position = glm::vec4(joint->outerFixed[std::floor(vertexIndex / 2.0)].joint->currPosition, 1.0f);
                    mesh.vertices[vertexIndex].color = {0.6313f, 0.0470f, 0.6745f};// Purple-ish
                    break;
            }
        }
    }

    for(std::underlying_type_t<BodyPart> bodyPartID = 0; bodyPartID < bodyPartsJoints.size(); bodyPartID++) {
        const std::size_t lineCount = bodyPartSize(bodyPartID) * 3;
        const std::size_t indicesCount = lineCount * 2;
        const std::size_t verticesCount = bodyPartSize(bodyPartID) * 6;
        if(bodyPartSize(bodyPartID) <= 1) continue;
        debugJointBasis[bodyPartID].vertices.resize(verticesCount);
        debugJointBasis[bodyPartID].indices.resize(indicesCount);
        for(std::size_t index = 0; index < indicesCount; index++) {
            debugJointBasis[bodyPartID].indices[index] = index;
        }
        for(std::size_t vertexIndex = 0; vertexIndex < verticesCount; vertexIndex++) {
            debugJointBasis[bodyPartID].vertices[vertexIndex].color = {0.0f, 0.0f, 1.0f};// Blue
            size_t jointIndex = std::floor(vertexIndex / 6.0f);
            switch(vertexIndex % 6) {
                case 0:
                    debugJointBasis[bodyPartID].vertices[vertexIndex].position = debugJointLines[bodyPartID].vertices[jointIndex].position;
                    debugJointBasis[bodyPartID].vertices[vertexIndex].color = {1.0f, 0.0f, 0.0f};// Red
                    break;
                case 1:
                    debugJointBasis[bodyPartID].vertices[vertexIndex].position = debugJointLines[bodyPartID].vertices[jointIndex].position + (0.2f * bodyPartJoint(bodyPartID, jointIndex)->currBasis.x);
                    debugJointBasis[bodyPartID].vertices[vertexIndex].color = {1.0f, 0.0f, 0.0f};// Red
                    break;
                case 2:
                    debugJointBasis[bodyPartID].vertices[vertexIndex].position = debugJointLines[bodyPartID].vertices[jointIndex].position;
                    debugJointBasis[bodyPartID].vertices[vertexIndex].color = {0.0f, 1.0f, 0.0f};// Green
                    break;
                case 3:
                    debugJointBasis[bodyPartID].vertices[vertexIndex].position = debugJointLines[bodyPartID].vertices[jointIndex].position + (0.2f * bodyPartJoint(bodyPartID, jointIndex)->currBasis.y);
                    debugJointBasis[bodyPartID].vertices[vertexIndex].color = {0.0f, 1.0f, 0.0f};// Green
                    break;
                case 4:
                    debugJointBasis[bodyPartID].vertices[vertexIndex].position = debugJointLines[bodyPartID].vertices[jointIndex].position;
                    debugJointBasis[bodyPartID].vertices[vertexIndex].color = {0.0f, 0.0f, 1.0f};// Blue
                    break;
                case 5:
                    debugJointBasis[bodyPartID].vertices[vertexIndex].position = debugJointLines[bodyPartID].vertices[jointIndex].position + (0.2f * bodyPartJoint(bodyPartID, jointIndex)->currBasis.z);
                    debugJointBasis[bodyPartID].vertices[vertexIndex].color = {0.0f, 0.0f, 1.0f};// Blue
                    break;
            }
        }
    }
}

void Animatrix::initPullingQueue(pullingQueue_t &queue, const JointInfo &joint) {

    if(joint.inner) {
        queue.emplace(PullAction{
                .targetJoint = joint.inner,
                .changedJoint = &joint,
                .fromInner = false,
        });
    }
    if(joint.outer) {
        queue.emplace(PullAction{
                .targetJoint = joint.outer,
                .changedJoint = &joint,
                .fromInner = true,
        });
    }

    if(joint.isSplit()) {
        std::size_t id = 0;
        while(joint.outerFixed[id].joint != nullptr) {
            queue.emplace(PullAction{
                    .targetJoint = joint.outerFixed[id].joint,
                    .changedJoint = &joint,
                    .fromInner = true,
            });
            id++;
        }
    }
}

void Animatrix::populatePullingQueue(pullingQueue_t &queue, const PullAction &action) {
    JointInfo *targetJoint = action.targetJoint;
    JointInfo *nextJoint = action.fromInner ? targetJoint->outer : targetJoint->inner;
    if(nextJoint) {
        queue.emplace(PullAction{
                .targetJoint = nextJoint,
                .changedJoint = targetJoint,
                .fromInner = action.fromInner,
        });
    }

    if(targetJoint->isSplit()) {
        std::size_t id = 0;
        while(targetJoint->outerFixed[id].joint) {
            if(targetJoint->outerFixed[id].joint != action.changedJoint) {
                queue.emplace(PullAction{
                        .targetJoint = targetJoint->outerFixed[id].joint,
                        .changedJoint = targetJoint,
                        .fromInner = true,
                });
            }
            id++;
        }
    }
}

void Animatrix::updateModel() {
    for(auto &joint: bodyJoints) {
        joint.currPosition = joint.tempPosition;
    }
    for(auto &joint: bodyJoints) {
        joint.fixTempDirection();
        joint.currDirection = joint.tempDirection;
    }
    for(auto &joint: bodyJoints) {
        joint.clearModelTransform(m_model);
        joint.setModelPosition(m_model);
        joint.setModelDirection(m_model);
    }
}


void Animatrix::pullJoint(JointInfo *pulledJoint, const glm::vec3 &dest) {
    pullingQueue_t pullQueue;

repeat:

    pulledJoint->tempPosition = dest;
    initPullingQueue(pullQueue, *pulledJoint);

    while(!pullQueue.empty()) {
        PullAction pullAction = pullQueue.front();
        pullQueue.pop();
        JointInfo *targetJoint = pullAction.targetJoint;
        const JointInfo *changedJoint = pullAction.changedJoint;


        if((!targetJoint->isSplit() && !targetJoint->isFixed()) ||
           (targetJoint->isSplit() && !changedJoint->isFixed()) ||
           (targetJoint->isFixed() && !pullAction.fromInner)) {
            targetJoint->fixTempPositionInterpolate(*pullAction.changedJoint, pullAction.fromInner, false);
            goto finish_move;
        }

        if(targetJoint->isFixed() && pullAction.fromInner) {
            targetJoint->tempPosition = changedJoint->tempPosition + changedJoint->outerFixed[targetJoint->outerFixedID].direction * changedJoint->outerFixed[targetJoint->outerFixedID].distance;
            //targetJoint->fixTempPositionExtrapolate(*pullAction.changedJoint, pullAction.fromInner, pullAction.targetDistance);
            goto finish_move;
        }

        //targetJoint->fixTempPositionInterpolate(*pullAction.changedJoint, pullAction.fromInner);


        if(targetJoint->isSplit() && changedJoint->isFixed()) {
            glm::vec3 changedDir = glm::normalize(changedJoint->tempPosition - targetJoint->tempPosition);
            glm::vec3 expectedDir = targetJoint->outerFixed[changedJoint->outerFixedID].direction;
            if(changedDir == expectedDir) goto finish_move;

            // Creates an angle and rotation axis on which to rotate between the two directions
            glm::vec3 cross = glm::cross(changedDir, expectedDir);
            if(cross == glm::vec3(0.0)) goto finish_move;

            glm::vec3 axis = glm::normalize(cross);
            const float angle = -glm::angle(changedDir, expectedDir);
            if(angle == 0.0f) goto finish_move;
            std::size_t id = 0;
            while(targetJoint->outerFixed[id].joint) {
                targetJoint->outerFixed[id].direction = glm::normalize(glm::rotate(targetJoint->outerFixed[id].direction, angle, axis));
                id++;
            }
            targetJoint->fixTempPositionInterpolate(*pullAction.changedJoint, pullAction.fromInner, true);
        }


    finish_move:

        populatePullingQueue(pullQueue, pullAction);
    }

    if(pulledJoint->tempPosition != dest) {
        goto repeat;
    }
}

void Animatrix::cascadeChange(BodyPart bodyPart, bool fromRoot) {
    // Limbs that have a single bone have nothing to cascade
    if(bodyPartSize(bodyPart) <= 1) return;
    glm::vec3 destPos;

    if(fromRoot) {
        // When cascading from root (closer to body center), we adjust joints in reverse
        for(std::size_t jointInfoIndex = bodyPartSize(bodyPart) - 2; jointInfoIndex != SIZE_MAX; jointInfoIndex--) {

            JointInfo *outerJoint = bodyPartJoint(bodyPart, jointInfoIndex);
            JointInfo *innerJoint = bodyPartJoint(bodyPart, jointInfoIndex + 1);

            if(Utils::enumCheckBit(innerJoint->flags, JointInfo::Flags::FIXED_DIR)) {
                destPos = innerJoint->tempPosition + (innerJoint->currDirection * innerJoint->distance);
            } else {
                float newDistance = glm::distance(innerJoint->tempPosition, outerJoint->tempPosition);
                if(newDistance > innerJoint->distance) {// The joint moved further away
                    float distanceDiff = newDistance - innerJoint->distance;
                    destPos = Utils::interpolateBetween(outerJoint->tempPosition, innerJoint->tempPosition, distanceDiff / newDistance);
                } else if(newDistance < innerJoint->distance) {// The joint moved closer
                    float distanceDiff = innerJoint->distance - newDistance;
                    destPos = Utils::interpolateBetween(innerJoint->tempPosition, outerJoint->tempPosition, 1.0f + distanceDiff / innerJoint->distance);
                } else {
                    // If this joint doesn't have to adjust, the rest wouldn't have to as well
                    // This seems improbable but keeping it here anyway
                    break;
                }
            }

            outerJoint->tempPosition = destPos;
            outerJoint->fixTempDirection();
        }
    } else {
        //m_bodyNodes[bodyPart][0].applyConstraint(fromRoot);

        for(std::size_t jointInfoIndex = 1; jointInfoIndex < bodyPartSize(bodyPart); jointInfoIndex++) {
            JointInfo *innerJoint = bodyPartJoint(bodyPart, jointInfoIndex);
            JointInfo *outerJoint = bodyPartJoint(bodyPart, jointInfoIndex - 1);

            if(Utils::enumCheckBit(innerJoint->flags, JointInfo::Flags::FIXED_DIR)) {
                destPos = outerJoint->tempPosition + ((-innerJoint->currDirection) * innerJoint->distance);
            } else {
                const float newDistance = glm::distance(innerJoint->tempPosition, outerJoint->tempPosition);
                if(newDistance > innerJoint->distance) {// The joint moved further away
                    float distanceDiff = newDistance - innerJoint->distance;
                    destPos = Utils::interpolateBetween(innerJoint->tempPosition, outerJoint->tempPosition, distanceDiff / newDistance);
                } else if(newDistance < innerJoint->distance) {// The joint moved closer
                    float distanceDiff = innerJoint->distance - newDistance;
                    destPos = Utils::interpolateBetween(outerJoint->tempPosition, innerJoint->tempPosition, 1.0f + distanceDiff / innerJoint->distance);
                } else {
                    // If this joint doesn't have to adjust, the rest wouldn't have to as well
                    // This seems improbable but keeping it here anyway
                    break;
                }
            }

            innerJoint->tempPosition = destPos;
            innerJoint->fixTempDirection();
        }
    }
}

void Animatrix::applyConstraints() {
    cosntraintQueue_t constrQueue;

    constrQueue.emplace(ConstraintAction{
            .targetJoint = rootJoint,
            .changedJoint = nullptr});

    while(!constrQueue.empty()) {
        ConstraintAction constr = constrQueue.front();
        constrQueue.pop();
        JointInfo *targetJoint = constr.targetJoint;
        if(targetJoint->outer) {
            constrQueue.emplace(ConstraintAction{
                .targetJoint = targetJoint->outer,
                .changedJoint = targetJoint}
            );
        }
        if(targetJoint->isSplit()) {
            std::size_t id = 0;
            while(targetJoint->outerFixed[id].joint != nullptr) {
                constrQueue.emplace(ConstraintAction{
                        .targetJoint = targetJoint->outerFixed[id].joint,
                        .changedJoint = nullptr
                });
                id++;
            }
        }
        if(constr.changedJoint) {
            targetJoint->applyConstraint();
        }
    }

    // Limbs that have a single bone have nothing to cascade
    /*if(bodyPartSize(bodyPart) <= 1) return;

    if(fromRoot) {
        for(std::size_t jointInfoIndex = bodyPartSize(bodyPart) - 1; jointInfoIndex != SIZE_MAX; jointInfoIndex--) {
            if(jointInfoIndex != bodyPartSize(bodyPart) - 1 && Utils::enumCheckBit(bodyPartJoint(bodyPart, jointInfoIndex + 1)->flags, JointInfo::Flags::FIXED_DIR)) continue;
            bodyPartJoint(bodyPart, jointInfoIndex)->applyConstraint(fromRoot);
            bodyPartJoint(bodyPart, jointInfoIndex)->fixTempDirection();
        }
    } else {
        for(std::size_t jointInfoIndex = 1; jointInfoIndex < bodyPartSize(bodyPart); jointInfoIndex++) {
            if(Utils::enumCheckBit(bodyPartJoint(bodyPart, jointInfoIndex - 1)->flags, JointInfo::Flags::FIXED_DIR)) continue;
            bodyPartJoint(bodyPart, jointInfoIndex)->applyConstraint(fromRoot);
            bodyPartJoint(bodyPart, jointInfoIndex)->fixTempDirection();
        }
    }*/
}

Animatrix::JointInfo::JointInfo() {
    outerFixed.fill(FixedJointInfo());
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
    if(currDirection == origDirection || glm::dot(currDirection, origDirection) > DOT_MARGIN) return;

    // Creates an angle and rotation axis on which to rotate between the two directions
    // glm::rotate will normalize this
    glm::vec3 axis = glm::cross(currDirection, origDirection);
    if(axis == glm::vec3(0.0f)) return;

    const float angle = -glm::angle(currDirection, origDirection);
    if(abs(angle) == 0.0f) return;

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
        if(outer->tempPosition != this->tempPosition) {
            this->tempDirection = glm::normalize(outer->tempPosition - this->tempPosition);
        }
    } else if(inner) {
        if(inner->tempPosition != this->tempPosition) {
            this->tempDirection = glm::normalize(this->tempPosition - inner->tempPosition);
        }
    }
}

void Animatrix::JointInfo::fixTempPositionInterpolate(const JointInfo &changedJoint, bool fromInner, bool fromFixed = false) {
    float newDistance = glm::distance(this->tempPosition, changedJoint.tempPosition);

    glm::vec3 destPos = this->tempPosition;
    if(fromInner) {
        float changedDistance = fromFixed ? this->outerFixed[changedJoint.outerFixedID].distance : changedJoint.distance;
        if(abs(changedDistance - newDistance) < FLT_MARGIN) return;

        if(newDistance > changedDistance) {// The joint moved further away
            float distanceDiff = newDistance - changedDistance;
            destPos = Utils::interpolateBetween(this->tempPosition, changedJoint.tempPosition, distanceDiff / newDistance);
        } else if(newDistance < changedDistance) {
            // The joint moved closer
            float distanceDiff = changedDistance - newDistance;
            destPos = Utils::interpolateBetween(changedJoint.tempPosition, this->tempPosition, 1.0f + distanceDiff / changedDistance);
        }
    } else {
        float changedDistance = fromFixed ? this->outerFixed[changedJoint.outerFixedID].distance : this->distance;
        if(abs(changedDistance - newDistance) < FLT_MARGIN) return;

        if(newDistance > changedDistance) {// The joint moved further away
            float distanceDiff = newDistance - changedDistance;
            destPos = Utils::interpolateBetween(this->tempPosition, changedJoint.tempPosition, distanceDiff / newDistance);
        } else if(newDistance < changedDistance) {// The joint moved closer
            float distanceDiff = changedDistance - newDistance;
            destPos = Utils::interpolateBetween(changedJoint.tempPosition, this->tempPosition, 1.0f + distanceDiff / changedDistance);
        }
    }
    this->tempPosition = destPos;
}

void Animatrix::JointInfo::fixTempPositionExtrapolate(const JointInfo &changedJoint, bool fromInner, float distance) {
    this->tempPosition = changedJoint.tempPosition + changedJoint.tempDirection * distance;
}

void Animatrix::JointInfo::applyConstraint() {
    JointInfo *changed = inner;
    assert(changed);
    glm::vec3 L1 = changed->tempDirection;
    if(L1 == Axis::POS_Y || abs(glm::dot(L1, Axis::POS_Y)) > DOT_MARGIN) return;

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
    if(newDistance > changed->distance) {// The joint moved further away
        float distanceDiff = newDistance - changed->distance;
        destPos = Utils::interpolateBetween(this->tempPosition, changed->tempPosition, distanceDiff / newDistance);
    } else if(newDistance < changed->distance) {// The joint moved closer
        float distanceDiff = changed->distance - newDistance;
        destPos = Utils::interpolateBetween(changed->tempPosition, this->tempPosition, 1.0f + distanceDiff / changed->distance);
    } else {
        return;
    }

    this->tempPosition = destPos;
}

bool Animatrix::JointInfo::isRoot() const {
    return Utils::enumCheckBit(this->flags, Flags::IS_ROOT);
}

bool Animatrix::JointInfo::isSplit() const {
    return Utils::enumCheckBit(this->flags, Flags::IS_SPLIT);
}

bool Animatrix::JointInfo::isFixed() const {
    return Utils::enumCheckBit(this->flags, Flags::FIXED_DIR);
}

void Animatrix::updateDebug() {
    for(std::underlying_type_t<BodyPart> bodyPart = 0; bodyPart < BODYPART_MAX; bodyPart++) {
        const auto &joints = bodyPartsJoints[bodyPart];
        if(joints.size() <= 1) continue;
        for(std::size_t jointIndex = 0; jointIndex < joints.size(); jointIndex++) {
            /*debugJointLines[bodyPart].vertices[jointIndex].position = m_model->transformation.getMatrix() *
                                                                      glm::scale(glm::vec3(debugScaleFactor)) *
                                                                      (glm::vec4(joints[jointIndex]->currPosition - bodyPartJointInner(BODYPART_SPINE)->currPosition, 1.0f));*/
            debugJointLines[bodyPart].vertices[jointIndex].position = glm::vec4(joints[jointIndex]->currPosition, 1.0f);
        }
    }

    for(const auto &[mesh, joint]: std::views::zip(debugJointSplits, splitJoints)) {
        std::size_t lineCount = 0;
        while(joint->outerFixed[lineCount].joint != nullptr) lineCount++;
        const std::size_t verticesCount = lineCount * 2;
        for(std::size_t vertexIndex = 0; vertexIndex < verticesCount; vertexIndex++) {
            switch(vertexIndex % 2) {
                case 0:
                    /*mesh.vertices[vertexIndex].position = m_model->transformation.getMatrix() *
                                                             glm::scale(glm::vec3(debugScaleFactor)) *
                                                             (glm::vec4(joint->currPosition - bodyPartJointInner(BODYPART_SPINE)->currPosition, 1.0f));*/
                    mesh.vertices[vertexIndex].position = glm::vec4(joint->currPosition, 1.0f);

                    break;
                case 1:
                    /*mesh.vertices[vertexIndex].position = m_model->transformation.getMatrix() *
                                                 glm::scale(glm::vec3(debugScaleFactor)) *
                                                 (glm::vec4(joint->outerFixed[std::floor(vertexIndex/2.0)].joint->currPosition - bodyPartJointInner(BODYPART_SPINE)->currPosition, 1.0f));*/
                    mesh.vertices[vertexIndex].position = glm::vec4(joint->outerFixed[std::floor(vertexIndex / 2.0)].joint->currPosition, 1.0f);
                    break;
            }
        }
    }


    for(std::underlying_type_t<BodyPart> bodyPart = 0; bodyPart < BODYPART_MAX; bodyPart++) {
        const auto &joints = bodyPartsJoints[bodyPart];
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
                    debugJointBasis[bodyPart].vertices[vertexIndex].position = debugJointLines[bodyPart].vertices[jointIndex].position + (0.2f * joints[jointIndex]->currBasis.x);
                    break;
                case 3:
                    debugJointBasis[bodyPart].vertices[vertexIndex].position = debugJointLines[bodyPart].vertices[jointIndex].position + (0.2f * joints[jointIndex]->currBasis.y);
                    break;
                case 5:
                    debugJointBasis[bodyPart].vertices[vertexIndex].position = debugJointLines[bodyPart].vertices[jointIndex].position + (0.2f * joints[jointIndex]->currBasis.z);
                    break;
            }
        }
    }
}

const char *bodyPart2string(Animatrix::BodyPart part) {
    switch(part) {
        case Animatrix::BODYPART_HEAD:
            return "Head";
        case Animatrix::BODYPART_LARM:
            return "Left Arm";
        case Animatrix::BODYPART_RARM:
            return "Right Arm";
        case Animatrix::BODYPART_LLEG:
            return "Left Leg";
        case Animatrix::BODYPART_RLEG:
            return "Right Leg";
        case Animatrix::BODYPART_SPINE:
            return "Spine";
        default:
            return "Unknown";
    }
}

const char *bodyPart2string(std::underlying_type_t<Animatrix::BodyPart> part) {
    return bodyPart2string(static_cast<Animatrix::BodyPart>(part));
}


void Animatrix::runImGui() {

    if(!ImGui::Begin("Animatrix")) {
        ImGui::End();
        return;
    }
    for(auto &instance: m_instances) {
        if(!ImGui::TreeNode(instance->model()->path().c_str())) continue;

        ImGui::SeparatorText("Action Sequences");
        ImGui::PushID("Action Sequences");

        for(auto it = instance->actionSequences.begin(); it != instance->actionSequences.end();) {
            auto &[seqName, seq] = *it;
            auto &ctx = seq.imGuiCtx;
            if(!ImGui::TreeNode(seqName.c_str())) {
                ++it;
                continue;
            }

            strcpy(IMGUI_CHAR_BUFFER, seqName.c_str());
            ctx->showFunc(&seq);

            if(ctx->shouldDelete) {
                instance->actionSequences.erase(it++);
                ImGui::TreePop();
                continue;
            }

            if(ctx->shouldRename) {
                ctx->shouldRename = false;

                seq.name = IMGUI_CHAR_BUFFER;
                instance->actionSequences[IMGUI_CHAR_BUFFER] = std::move(seq);
                instance->actionSequences.erase(it++);
                ImGui::TreePop();
                continue;
            }
            if(ImGui::Button("Save Sequence")) {
                auto seqData = instance->serializeSequence(seqName.c_str());
                if(seqData.has_value()) {
                    auto data = seqData.value();
                    std::ofstream outFile("./seqs/" + seqName + ".tecseq", std::ios::out | std::ios::binary);
                    outFile.write(reinterpret_cast<const char *>(&data[0]), data.size());
                }
            }

            ImGui::TreePop();
            ++it;
        }

        if(ImGui::Button("New Sequence")) {
            instance->actionSequences.emplace("New sequence", ActionSequence{});
        }
        ImGui::PopID();

        ImGui::SeparatorText("Actions");
        ImGui::PushID("Actions");
        for(auto &[id, action]: instance->actions) {
            if(!ImGui::TreeNode((std::string(bodyPart2string(action.body)) + " " + std::to_string(action.bodyPartID)).c_str())) continue;
            if(ImGui::DragFloat3("Position", &action.target[0])) action.currentTime = 0.0f;
            ImGui::TreePop();
        }
        ImGui::PopID();

        ImGui::SeparatorText("Joints");
        ImGui::PushID("Joints");
        for(std::underlying_type_t<BodyPart> bodyPart = 0; bodyPart < BODYPART_MAX; bodyPart++) {
            const auto &partVector = instance->bodyPartsJoints[bodyPart];
            if(!ImGui::TreeNode(bodyPart2string(bodyPart))) continue;
            for(auto &joint: partVector) {
                if(!ImGui::TreeNode((std::string(bodyPart2string(bodyPart)) + " " + std::string(std::to_string(joint->jointID))).c_str())) continue;
                ImGui::Text("Distance: %f", joint->distance);
                ImGui::Text("Current position: %f,%f,%f", joint->currPosition.x, joint->currPosition.y, joint->currPosition.z);
                ImGui::Text("Temporary position: %f,%f,%f", joint->tempPosition.x, joint->tempPosition.y, joint->tempPosition.z);
                ImGui::Text("Original position: %f,%f,%f", joint->origPosition.x, joint->origPosition.y, joint->origPosition.z);
                ImGui::Text("Current direction: %f,%f,%f", joint->currDirection.x, joint->currDirection.y, joint->currDirection.z);
                ImGui::Text("Temporary direction: %f,%f,%f", joint->tempDirection.x, joint->tempDirection.y, joint->tempDirection.z);
                ImGui::Text("Original direction: %f,%f,%f", joint->origDirection.x, joint->origDirection.y, joint->origDirection.z);
                glm::vec4 degConstraint = glm::degrees(joint->angleConstraints);
                if(ImGui::DragFloat4("Constraints", &degConstraint[0])) {
                    joint->angleConstraints = glm::radians(degConstraint);
                }
                ImGui::Text("Inner/Outer IDs: %d/%d", joint->inner ? joint->inner->jointID : -1, joint->outer ? joint->outer->jointID : -1);
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
        ImGui::TreePop();
    }
    ImGui::End();
}

Animatrix::JointInfo *Animatrix::bodyPartJoint(std::underlying_type_t<BodyPart> bodyPart, std::size_t bodyPartIndex) {
    return bodyPartsJoints[bodyPart].at(bodyPartIndex);
}

Animatrix::JointInfo *Animatrix::bodyPartJoint(BodyPart bodyPart, std::size_t bodyPartIndex) {
    return bodyPartsJoints[bodyPart].at(bodyPartIndex);
}

Animatrix::JointInfo *Animatrix::bodyPartJointOuter(std::underlying_type_t<BodyPart> bodyPart) {
    return bodyPartsJoints[bodyPart].back();
}

Animatrix::JointInfo *Animatrix::bodyPartJointOuter(BodyPart bodyPart) {
    return bodyPartsJoints[bodyPart].back();
}

Animatrix::JointInfo *Animatrix::bodyPartJointInner(std::underlying_type_t<BodyPart> bodyPart) {
    return bodyPartsJoints[bodyPart].front();
}

Animatrix::JointInfo *Animatrix::bodyPartJointInner(BodyPart bodyPart) {
    return bodyPartsJoints[bodyPart].front();
}

std::size_t Animatrix::bodyPartSize(std::underlying_type_t<BodyPart> bodyPart) const {
    return bodyPartsJoints[bodyPart].size();
}

std::size_t Animatrix::bodyPartSize(BodyPart bodyPart) const {
    return bodyPartsJoints[bodyPart].size();
}

Animatrix::Action::Action() {
    imGuiCtx = std::make_unique<ImGuiTec::Empheral>();
    imGuiCtx->showFunc = show;
}

void Animatrix::Action::show(void *data) {
    assert(data);
    auto &act = *static_cast<Action *>(data);

    if(ImGui::DragFloat3("Position", &act.target[0], 0.1)) act.currentTime = 0.0f;

    ImGui::DragFloat("Destination time", &act.destinationTime, 0.1, 0.0f);

    // Selection box for body parts
    if(ImGui::TreeNode("Body Part")) {
        for(uint8_t bodyPartID = 0; bodyPartID < BODYPART_MAX; bodyPartID++) {
            if(ImGui::Selectable(BODYPART_STRINGS[bodyPartID], bodyPartID == act.body)) {
                act.body = static_cast<BodyPart>(bodyPartID);
            }
        }
        ImGui::TreePop();
    }

    // Setting body part ID
    int bodyID = act.bodyPartID;
    if(ImGui::InputInt("Body Part ID", &bodyID)) {
        if(bodyID > BODYPART_MAXID[act.body]) {
            act.bodyPartID = BODYPART_MAXID[act.body];
        } else if(bodyID < 0) {
            act.bodyPartID = 0;
        } else {
            act.bodyPartID = bodyID;
        }
    }

    if(ImGui::Button("Delete Action")) {
        act.imGuiCtx->shouldDelete = true;
    }
}

Animatrix::ActionGroup::ActionGroup() {
    imGuiCtx = std::make_unique<ImGuiTec::Empheral>();
    imGuiCtx->showFunc = show;
    name.reserve(IMGUI_CHAR_BUFFER_SIZE);
}

void Animatrix::ActionGroup::show(void *data) {
    assert(data);
    auto &grp = *static_cast<ActionGroup *>(data);

    ImGui::Text("Current time: %f", grp.currentTime);
    ImGui::DragFloat("Destination time", &grp.destinationTime);
    for(auto it = grp.actions.begin(); it != grp.actions.end();) {
        auto &act = *it;
        const auto &actCtx = act.imGuiCtx;

        if(!ImGui::TreeNode((std::string(bodyPart2string(act.body)) + " " + std::to_string(act.bodyPartID)).c_str())) {
            ++it;
            continue;
        }

        actCtx->showFunc(&act);

        if(actCtx->shouldDelete) {
            it = grp.actions.erase(it);
            ImGui::TreePop();
            continue;
        }

        ImGui::TreePop();
        ++it;
    }
    if(ImGui::Button("New Action")) {
        grp.actions.emplace_back();
    }
}

Animatrix::ActionSequence::ActionSequence() {
    imGuiCtx = std::make_unique<ImGuiTec::Empheral>();
    imGuiCtx->showFunc = show;
    name.reserve(IMGUI_CHAR_BUFFER_SIZE);
}

void Animatrix::ActionSequence::show(void *data) {
    assert(data);
    auto &seq = *static_cast<ActionSequence *>(data);
    const auto &ctx = seq.imGuiCtx;

    // Change the sequence name
    if(ImGui::InputText("Name", IMGUI_CHAR_BUFFER, IMGUI_CHAR_BUFFER_SIZE, ImGuiInputTextFlags_EnterReturnsTrue)) {
        ctx->shouldRename = true;
        return;
    }

    // Delete the sequence
    if(ImGui::Button("Delete Sequence")) {
        ctx->shouldDelete = true;
        return;
    }


    // List sequence groups
    ImGui::Text("Current Group: %u", seq.currentGroup);
    std::size_t grpIndex = 0;
    for(auto it = seq.groups.begin(); it != seq.groups.end();) {
        auto &grp = *it;

        if(!ImGui::TreeNode((std::string("Group ") + std::to_string(grpIndex)).c_str())) {
            ++it;
            ++grpIndex;
            continue;
        }
        auto &grpCtx = grp.imGuiCtx;
        grpCtx->showFunc(&grp);
        ImGui::TreePop();

        ++it;
        ++grpIndex;
    }


    if(ImGui::Button("New Group")) {
        seq.groups.emplace_back();
    }
}
