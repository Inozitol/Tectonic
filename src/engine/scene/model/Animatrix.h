#pragma once

#include "engine/imgui/ImGuiHandler.h"
#include "defs/CompilerDefs.h"
#include "defs/ConfigDefs.h"
#include "Model.h"
#include "../../imgui/ImGuiUtils.h"
#include "utils/FloatLimit.h"
#include "utils/Utils.h"

#include <list>
#include <queue>
#include <regex>
#include <set>
#include <optional>

#define ANIMATRIX_VERSION 0

struct Animatrix {
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

    constexpr static std::array<const char*, BODYPART_MAX> BODYPART_STRINGS {
        "Spine",
        "Head",
        "Left Leg",
        "Right Leg",
        "Left Arm",
        "Right Arm"
    };

    constexpr static std::array<uint8_t, BODYPART_MAX> BODYPART_MAXID {
        3, // Spine
        0, // Head
        2, // Left Leg
        2, // Right Leg
        2, // Left Arm
        2, // Right Arm
    };

    struct Action {
        Action();

        ActionType type = ActionType::PULLING;
        BodyPart body = BODYPART_SPINE;
        uint8_t bodyPartID = 0;
        glm::vec3 target = glm::vec3(0.0f);
        float currentTime = 0.0f;
        float destinationTime = 1.0f;

        std::unique_ptr<ImGuiTec::Empheral> imGuiCtx;
        static void show(void* data);
    };

    struct ActionGroup {
        ActionGroup();
        std::string name;

        float currentTime = 0.0f;
        float destinationTime = 1.0f;
        std::vector<Action> actions;

        std::unique_ptr<ImGuiTec::Empheral> imGuiCtx;
        static void show(void* data);
    };

    struct ActionSequence {
        ActionSequence();
        std::string name;

        std::vector<ActionGroup> groups;
        uint8_t currentGroup = 0;

        std::unique_ptr<ImGuiTec::Empheral> imGuiCtx;
        static void show(void* data);
    };

    std::unordered_map<uint32_t, Action> actions;
    std::unordered_map<std::string, ActionSequence> actionSequences;

    void updateActions();
    std::optional<SerialTypes::BinDataVec_t> serializeSequence(const char* name) const;
    bool deserializeSequence(SerialTypes::BinDataVec_t& data);

    Model* model() const;

    std::unordered_map<std::underlying_type_t<BodyPart>, VktTypes::PointMesh> debugJointLines;
    std::unordered_map<std::underlying_type_t<BodyPart>, VktTypes::PointMesh> debugJointBasis;
    std::list<VktTypes::PointMesh> debugJointSplits;

    float debugScaleFactor = 1.0f;

    bool loadArmature();
    void loadBodyPartVectors();
    void connectBodyParts();
    void loadJointGeometry();
    void createDebugLines();

    void cascadeChange(BodyPart bodyPart, bool fromRoot);
    void applyConstraints();

    void updateDebug();

    struct JointInfo {
        /** The distance from this joint to the next out-going joint */
        float distance = 0.0f;

        /** ID of this node in the space of all model nodes */
        ModelTypes::NodeID_t nodeID = 0;

        /**
         *  ID of the joint in the space of all model joints.
         *  Used for indexing joint aware arrays like ModelTypes->Skin->inverseBindMatrices.
         */
        uint32_t jointID = 0;

        glm::mat4 origAnimTransform = glm::mat4(1.0f);
        glm::mat4 origAnimTransformInverse = glm::mat4(1.0f);

        /** Original position of the join directly after loading the model */
        glm::vec3 origPosition = glm::vec3(0.0f);

        /** Current position of the join */
        glm::vec3 currPosition = glm::vec3(0.0f);

        /** Temporary position used to store position while cascading joints */
        glm::vec3 tempPosition = glm::vec3(0.0f);

        /** Original direction of the join directly after loading the model */
        glm::vec3 origDirection = glm::vec3(0.0f);

        /** Current direction of the join */
        glm::vec3 currDirection = glm::vec3(0.0f);

        /** Temporary direction used to store direction while cascading joints */
        glm::vec3 tempDirection = glm::vec3(0.0f);


        /** Orthonormal basis of the joint */
        struct Basis {
            glm::vec3 x;
            glm::vec3 y;
            glm::vec3 z;
        } origBasis, currBasis;

        /** Anglar constraints
         * By order these define posX, posY, negX, negY in a circle, ellipse, or parabole
         */
        glm::vec4 angleConstraints = {glm::radians(1.0f), glm::radians(1.0f), glm::radians(1.0f), glm::radians(1.0f)};

        enum class Flags {
            NO_FLAG = 0,

            /** Sets the joint as root */
            IS_ROOT = 1 << 0,

            /** Sets the joint as a split joint
             *
             * Split joints connect multiple joints together */
            IS_SPLIT = 1 << 1,

            /** Fixes the joint position relative to its root */
            FIXED_DIR = 1 << 2
        };

        JointInfo* inner = nullptr;
        JointInfo* outer = nullptr;
        struct FixedJointInfo {
            JointInfo* joint = nullptr;
            float distance = 0.0f;
            glm::vec3 direction = glm::vec3(0.0f);

            FixedJointInfo(JointInfo& splitJoint, JointInfo& fixedJoint);
            FixedJointInfo() = default;
        };
        std::array<FixedJointInfo, ANIMATRIX_MAX_CHILD_JOINTS> outerFixed = {};

        BodyPart bodyPart;
        uint32_t bodyPartID;

        uint8_t outerFixedID;

        Flags flags = Flags::NO_FLAG;

        JointInfo();
        void applyTransformation(Model* model, const glm::mat4& t) const;
        void setTransformation(Model* model, const glm::mat4& t) const;
        void clearModelTransform(Model* model) const;
        void setModelPosition(Model* model) const;
        void setModelDirection(Model* model);

        void fixTempDirection();
        void fixTempPositionInterpolate(const JointInfo& changedJoint, bool fromInner, bool fromFixed);
        void fixTempPositionExtrapolate(const JointInfo& changedJoint, bool fromInner, float distance);
        void applyConstraint();

        [[nodiscard]] always_inline bool isRoot() const;
        [[nodiscard]] always_inline bool isSplit() const;
        [[nodiscard]] always_inline bool isFixed() const;
    };

    /// Pulling logic

    struct PullAction {
        JointInfo* targetJoint;
        const JointInfo* changedJoint;
        bool fromInner;
    };

    struct ConstraintAction {
        JointInfo* targetJoint;
        const JointInfo* changedJoint;
    };

    using pullingQueue_t = std::queue<PullAction>;
    using cosntraintQueue_t = std::queue<ConstraintAction>;

    static void initPullingQueue(pullingQueue_t& queue, const JointInfo& joint);
    static void populatePullingQueue(pullingQueue_t& queue, const PullAction& action);
    void updateModel();
    static void pullJoint(JointInfo* joint, const glm::vec3& dest);

    // End of limb is at start of vectors
    std::vector<JointInfo> bodyJoints;
    std::array<std::vector<JointInfo*>, BODYPART_MAX> bodyPartsJoints;
    std::list<JointInfo *> splitJoints;
    std::list<JointInfo *> fixedJoints;

    /// Pointer to the root joint
    JointInfo* rootJoint = nullptr;

    /// Joints accessor functions

    always_inline JointInfo* bodyPartJoint(std::underlying_type_t<BodyPart> bodyPart, std::size_t bodyPartIndex);
    always_inline JointInfo* bodyPartJoint(BodyPart bodyPart, std::size_t bodyPartIndex);
    always_inline JointInfo* bodyPartJointInner(std::underlying_type_t<BodyPart> bodyPart);
    always_inline JointInfo* bodyPartJointInner(BodyPart bodyPart);
    always_inline JointInfo* bodyPartJointOuter(std::underlying_type_t<BodyPart> bodyPart);
    always_inline JointInfo* bodyPartJointOuter(BodyPart bodyPart);
    always_inline std::size_t bodyPartSize(std::underlying_type_t<BodyPart> bodyPart) const;
    always_inline std::size_t bodyPartSize(BodyPart bodyPart) const;

    Model* m_model = nullptr;

    static inline std::set<Animatrix*> m_instances = std::set<Animatrix*>();
    static inline const std::regex m_skinNodeRegex = std::regex(R"~(^(spine|leg|arm|head)\.(\d{3})(?:\.([RL]))?$)~");
};