#version 460

#extension GL_GOOGLE_include_directive : require

#include "input_layouts.glsl"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inWPos;

layout (location = 0) out vec4 outFragColor;

struct PBRInfo
{
    float NdotL;                  // cos angle between normal and light direction
    float NdotV;                  // cos angle between normal and view direction
    float NdotH;                  // cos angle between normal and half vector
    float LdotH;                  // cos angle between light direction and half vector
    float VdotH;                  // cos angle between view direction and half vector
    float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)
    float metalness;              // metallic value at the surface
    vec3 reflectance0;            // full reflectance color (normal incidence angle)
    vec3 reflectance90;           // reflectance color at grazing angle
    float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])
    vec3 diffuseColor;            // color contribution from diffuse lighting
    vec3 specularColor;           // color contribution from specular lighting
};

const float PI = 3.14159265;

vec3 N = normalize(inNormal);
vec3 V = normalize(sceneData.cameraPosition - inWPos);

vec3 L = normalize(-sceneData.sunlightDirection).xyz;
vec3 H = normalize(V + L);

float D_GGX(float NoH, float roughness){
    float a = NoH * roughness;
    float k = roughness / (1.0f - NoH*NoH + a*a);
    return k*k*(1.0f/PI);
}

float V_SmithGGXCorrelated(float NoV, float NoL, float roughness) {
    float a2 = roughness * roughness;
    float GGXV = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
    float GGXL = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
    return 0.5 / (GGXV + GGXL);
}

vec3 F_Schlick(float u, vec3 f0) {
    float f = pow(1.0 - u, 5.0);
    return f + f0 * (1.0 - f);
}

vec3 Fd_Lambert(vec3 color){
    return (1/PI) * color;
}

vec3 specularReflection(PBRInfo pbrInputs)
{
    return pbrInputs.reflectance0 + (pbrInputs.reflectance90 - pbrInputs.reflectance0) * pow(clamp(1.0 - pbrInputs.VdotH, 0.0, 1.0), 5.0);
}

float geometricOcclusion(PBRInfo pbrInputs)
{
    float NdotL = pbrInputs.NdotL;
    float NdotV = pbrInputs.NdotV;
    float r = pbrInputs.alphaRoughness;

    float attenuationL = 2.0 * NdotL / (NdotL + sqrt(r * r + (1.0 - r * r) * (NdotL * NdotL)));
    float attenuationV = 2.0 * NdotV / (NdotV + sqrt(r * r + (1.0 - r * r) * (NdotV * NdotV)));
    return attenuationL * attenuationV;
}

float microfacetDistribution(PBRInfo pbrInputs)
{
    float roughnessSq = pbrInputs.alphaRoughness * pbrInputs.alphaRoughness;
    float f = (pbrInputs.NdotH * roughnessSq - pbrInputs.NdotH) * pbrInputs.NdotH + 1.0;
    return roughnessSq / (PI * f * f);
}

vec3 diffuse(PBRInfo pbrInputs)
{
    return pbrInputs.diffuseColor / PI;
}
void main() {
    float MatOcculsion = texture(metalRoughTex,inUV).x;
    float MatRougness  = texture(metalRoughTex,inUV).y * materialData.metalRoughFactors.y;
    float MatMetalness = texture(metalRoughTex,inUV).z * materialData.metalRoughFactors.x;
    vec4  MatColor     = texture(colorTex,inUV) * materialData.colorFactors;

    float alphaRoughness = MatRougness * MatRougness;
    vec3 f0 = vec3(0.04);

    MatColor *= vec4(inColor,1.0f);

    vec3 diffuseColor = MatColor.rgb * (vec3(1.0f) - f0);
    diffuseColor *= 1.0f - MatMetalness;

    vec3 specularColor = mix(f0, MatColor.rgb, MatMetalness);

    float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

    float reflectance90 = clamp(reflectance * 25.0f, 0.0f, 1.0f);
    vec3 specularEnvironmentR0 = specularColor.rgb;
    vec3 specularEnvironmentR90 = vec3(1.0f, 1.0f, 1.0f) * reflectance90;

    float NdotL = clamp(dot(N, L), 0.001, 1.0);
    float NdotV = clamp(abs(dot(N, V)), 0.001, 1.0);
    float NdotH = clamp(dot(N, H), 0.0, 1.0);
    float LdotH = clamp(dot(L, H), 0.0, 1.0);
    float VdotH = clamp(dot(V, H), 0.0, 1.0);

    PBRInfo pbrInputs = PBRInfo(
        NdotL,
        NdotV,
        NdotH,
        LdotH,
        VdotH,
        MatRougness,
        MatMetalness,
        specularEnvironmentR0,
        specularEnvironmentR90,
        alphaRoughness,
        diffuseColor,
        specularColor
    );

    vec3 F = specularReflection(pbrInputs);
    float G = geometricOcclusion(pbrInputs);
    float D = microfacetDistribution(pbrInputs);

    vec3 diffuseContrib = (1.0f - F) * diffuse(pbrInputs);
    vec3 specContrib = F * G * D / (4.0f * NdotL * NdotV);
    vec3 color = NdotL * (diffuseContrib + specContrib); // TODO multiply with light color
    
    color = color * MatOcculsion;
    outFragColor = vec4(color, 1.0f);

    /*
    float NoV = abs(dot(N,V)) + 1e-5;
    float NoL = clamp(dot(N, L), 0.0f, 1.0f);
    float NoH = clamp(dot(N, H), 0.0f, 1.0f);
    float LoH = clamp(dot(L, H), 0.0f, 1.0f);

    float roughness = MatRougness * MatRougness;

    float D = D_GGX(NoH, MatRougness);
    vec3  F = F_Schlick(LoH, MatColor);
    float V = V_SmithGGXCorrelated(NoV, NoL, roughness);

    vec3 Fr = (D * V) * F;
    vec3 Fd = Fd_Lambert(MatColor);

    outFragColor = vec4(Fr + Fd,1.0f);
    */
}