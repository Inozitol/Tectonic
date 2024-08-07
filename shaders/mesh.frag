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
const float PI_R = 1.0/PI;

vec3 N = normalize(inNormal);
vec3 V = normalize(sceneData.cameraPosition - inWPos);
vec3 R = reflect(-V, N);

vec3 L = normalize(-sceneData.sunlightDirection).xyz;
vec3 H = normalize(V + L);

// Fresnel-Schlick Roughness
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

vec3 getIBLContrib(PBRInfo pbrInputs, vec3 reflection){
    vec3 diffuse = texture(IBLDiffuseCube, N).rgb;
    vec3 specular = texture(IBLDiffuseCube, reflection).rgb;
    diffuse *= pbrInputs.diffuseColor;
    specular *= pbrInputs.specularColor;
    return diffuse + specular;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float DistributionGGX(vec3 N, vec3 H, float roughness){
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness){
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    float MatOcclusion = 1.0f;
    float MatRougness  = materialData.metalRoughFactors.y;
    float MatMetalness = materialData.metalRoughFactors.x;
    vec4  MatColor     = materialData.colorFactors;

    if(bitSetM(MFlag_MetallicRoughnessTex)){
        MatOcclusion  = texture(metalRoughTex,inUV).x;
        MatRougness  *= texture(metalRoughTex,inUV).y;
        MatMetalness *= texture(metalRoughTex,inUV).z;
    }else{
        MatRougness = clamp(MatRougness, 0.04, 1.0);
        MatMetalness = clamp(MatMetalness, 0.0, 1.0);
    }

    if(bitSetM(MFlag_ColorTex)){
        MatColor *= texture(colorTex, inUV);
    }

    vec3 albedo = vec3(MatColor);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, MatMetalness);

    vec3 Lo;
    {
        float NDF = DistributionGGX(N, H, MatRougness);
        float G = GeometrySmith(N, V, L, MatRougness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - MatMetalness;
        float NdotL = max(dot(N,L), 0.0);
        Lo = (kD * albedo / PI + specular) * sceneData.sunlightColor * NdotL;
    }

    vec3 F = fresnelSchlickRoughness(max(dot(N,V), 0.0), F0, MatRougness);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - MatMetalness;

    vec3 irradiance = texture(IBLDiffuseCube, N).rgb;
    vec3 diffuse = irradiance * albedo;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(IBLSpecularCube, R, MatRougness * MAX_REFLECTION_LOD).rgb;
    vec2 envBRDF = texture(IBLBRDFTexture, vec2(max(dot(N,V),0.0), MatRougness)).rg;
    vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

    vec3 ambient = (kD * diffuse + specular) * MatOcclusion;

    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0));

    outFragColor = vec4(color, 1.0f);
}