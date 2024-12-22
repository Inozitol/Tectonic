#pragma once

#include "defs/ConfigDefs.h"
#include "model/Model.h"
#include <regex>
#include <set>
#include <utils/Utils.h>

class Animatrix {
public:
    explicit Animatrix(Model* model);

    ~Animatrix();

    static void runImGui();

    enum class ActionType : uint8_t {
        PULLING
    };

    enum BodyPart : uint8_t {
        BODYPART_SPINE,
        BODYPART_HEAD,
        BODYPART_LLEG,
        BODYPART_RLEG,
        BODYPART_LARM,
        BODYPART_RARM,
        BODYPART_MAX
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

    float debugScaleFactor = 1.0f;

private:
    bool loadArmature();
    void loadBodyPartVectors();
    void loadJointGeometry();
    void createDebugLines();

    void pullBodyPart(BodyPart bodyPart, const glm::vec3& dest);
    void cascadeChange(BodyPart bodyPart, bool fromRoot);
    void cascadeConstraint(BodyPart bodyPart, bool fromRoot);

    void updateDebug();

    struct JointInfo {
        /** The distance from this joint to the next out-going joint */
        float distance;

        /** ID of this node in the space of all model nodes */
        ModelTypes::NodeID_t nodeID;

        /**
         *  ID of the joint in the space of all model joints.
         *  Used for indexing joint aware arrays like ModelTypes->Skin->inverseBindMatrices.
         */
        uint32_t jointID;

        glm::mat4 origAnimTransform;
        glm::mat4 origAnimTransformInverse;

        /** Original position of the join directly after loading the model */
        glm::vec3 origPosition;

        /** Current position of the join */
        glm::vec3 currPosition;

        /** Temporary position used to store position while cascading joints */
        glm::vec3 tempPosition;

        /** Original direction of the join directly after loading the model */
        glm::vec3 origDirection;

        /** Current direction of the join */
        glm::vec3 currDirection;

        /** Temporary direction used to store direction while cascading joints */
        glm::vec3 tempDirection;


        /** Orthonormal basis of the joint */
        struct Basis {
            glm::vec3 x;
            glm::vec3 y;
            glm::vec3 z;
        } origBasis, currBasis;

        /** Anglar constraints
         * By order these define posX, posY, negX, negY in a circle, ellipse, or parabole
         */
        glm::vec4 angleConstraints = {glm::radians(5.0f), glm::radians(5.0f), glm::radians(5.0f), glm::radians(5.0f)};

        enum class Flags {
            NO_FLAG = 0,

            /** Sets the joint as root */
            IS_ROOT = 1 << 0,

            /** Fixes the joint position */
            FIXED_DIR = 1 << 1
        };
        Flags flags = Flags::NO_FLAG;

        uint32_t inner;
        std::array<uint32_t, ANIMATRIX_MAX_CHILD_JOINTS> outer;

        BodyPart bodyPart;
        uint32_t bodyPartID;

        void applyTransformation(Model* model, const glm::mat4& t) const;
        void setTransformation(Model* model, const glm::mat4& t) const;
        void clearModelTransform(Model* model) const;
        void setModelPosition(Model* model) const;
        void setModelDirection(Model* model);

        void fixTempDirection();
        void applyConstraint(bool fromRoot);
    };


    // End of limb is at start of vectors
    std::vector<JointInfo> m_bodyJoints;
    std::array<std::vector<uint32_t>, BODYPART_MAX> m_bodyPartsJoints;

    Model* m_model = nullptr;

    static inline std::set<Animatrix*> m_instances = std::set<Animatrix*>();
    static inline const std::regex m_skinNodeRegex = std::regex(R"~(^(spine|leg|arm|head)\.(\d{3})(?:\.([RL]))?$)~");
    static inline Logger m_logger = Logger("Animatrix");
};
