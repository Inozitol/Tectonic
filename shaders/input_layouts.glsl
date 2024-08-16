
layout(set = 0, binding = 0) uniform SceneData{
    mat4 view;
    mat4 proj;
    mat4 viewproj;
    vec3 ambientColor;
    vec3 sunlightDirection;
    vec3 sunlightColor;
    vec3 cameraPosition;
    vec3 cameraDirection;
    float time;
} sceneData;

layout(set = 0, binding = 1) uniform samplerCube IBLDiffuseCube;
layout(set = 0, binding = 2) uniform samplerCube IBLSpecularCube;
layout(set = 0, binding = 3) uniform sampler2D IBLBRDFTexture;

layout(set = 1, binding = 0) uniform GLTFMaterialData{
    vec4 colorFactors;
    vec2 metalRoughFactors; // metallicFactor -> x | roughnessFactor -> y
    uint bitFlagsM;
    uint bitFlagsL;
    vec4 extra[14];
} materialData;

const uint MFlag_ColorTex             = 1 << 0;
const uint MFlag_MetallicRoughnessTex = 1 << 1;

bool bitSetM(uint bit){
    return (materialData.bitFlagsM & bit) == bit;
}

bool bitSetL(uint bit){
    return (materialData.bitFlagsL & bit) == bit;
}

layout(set = 1, binding = 1) uniform sampler2D colorTex;

// Occlusion -> x | Roughness -> y | Metalness -> z
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;