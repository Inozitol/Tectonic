
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

layout(set = 1, binding = 0) uniform GLTFMaterialData{
    vec4 colorFactors;
    vec4 metalRoughFactors;
    vec4 extra[14];
} materialData;

layout(set = 1, binding = 1) uniform sampler2D colorTex;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;
