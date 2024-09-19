#pragma once

#include "model/Model.h"
#include <regex>

class Animatrix {
public:
    Animatrix() = default;
    explicit Animatrix(Model* model);

    bool isLoaded() const;

    enum class ActionType : uint8_t {
        PULLING
    };

    enum class BodyPart : uint8_t {
        SPINE,
        HEAD,
        LLEG,
        RLEG,
        LARM,
        RARM
    };

    struct Action {
        ActionType type;
        BodyPart body;
        glm::vec3 target;
    };

    std::unordered_map<uint32_t, Action> actions;

    void updateActions();

    Model* model() const;

    std::unordered_map<BodyPart, VktTypes::PointMesh> debugJointLines;
    std::unordered_map<BodyPart, VktTypes::PointMesh> debugJointBasis;

private:
    bool loadArmature();
    void updateDistances();

    void pullBodyPart(BodyPart bodyPart, const glm::vec3& dest);
    void cascadeChange(BodyPart bodyPart, bool fromRoot);
    void updateDebug();

    struct JointInfo {
        /** The distance from this joint to the next out-going joint */
        float distance;

        ModelTypes::NodeID_t nodeID;
        uint32_t jointID;

        glm::vec3 position;
        glm::vec3 direction;

        struct Basis {
            glm::vec3 x;
            glm::vec3 y;
            glm::vec3 z;
        } basis;

        void applyTransformation(Model* model, const glm::mat4& t) const;
        void setPosition(Model* model, const glm::vec3& p);
        void setDirection(Model* model, const glm::vec3& d);
    };

    // End of limb is at start of vectors
    std::unordered_map<BodyPart, std::vector<JointInfo>> m_bodyNodes{
            {BodyPart::SPINE, {}},
            {BodyPart::HEAD, {}},
            {BodyPart::LLEG, {}},
            {BodyPart::RLEG, {}},
            {BodyPart::LARM, {}},
            {BodyPart::RARM, {}},
    };

    Model* m_model = nullptr;
    bool m_isLoaded = false;

    static inline const std::regex m_skinNodeRegex = std::regex(R"~(^(spine|leg|arm|head)\.(\d{3})(?:\.([RL]))?$)~");
    static inline Logger m_logger = Logger("Animatrix");
};
