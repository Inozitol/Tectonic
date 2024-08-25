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

private:
    bool loadArmature();
    void updateDistances();

    struct JointInfo {
        float distance;
        ModelTypes::NodeID_t nodeID;
        uint32_t jointID;

        glm::vec3 position;
        glm::vec3 direction;
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

    Model* m_model;
    bool m_isLoaded = false;

    static inline const std::regex m_skinNodeRegex = std::regex(R"~(^(spine|leg|arm|head)\.(\d{3})(?:\.([RL]))?$)~");
    static inline Logger m_logger = Logger("Animatrix");
};
