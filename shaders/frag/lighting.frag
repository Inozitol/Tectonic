in vec2 TexCoord0;
in vec3 Normal0;
in vec3 WorldPos0;
in vec4 LightSpacePos;
in mat3 TBN;

out vec4 FragColor;

struct BaseLight{
    vec3 color;
    float ambientIntensity;
    float diffuseIntensity;
};

struct DirectionalLight {
    BaseLight base;
    vec3 direction;
};

struct Atteniuation {
    float constant;
    float linear;
    float exp;
};

struct PointLight {
    BaseLight base;
    vec3 pos;
    Atteniuation atten;
};

struct SpotLight {
    PointLight base;
    vec3 direction;
    float angle;    // Cosine of the angle
};

struct Material {
    vec3 ambientColor;
    vec3 diffuseColor;
    vec3 specularColor;
    float shininess;
};

struct Sampler {
    sampler2D diffuse;
    sampler2D specular;
    sampler2D normal;
    sampler2D shadowMap;
    samplerCube shadowCubeMap;
};

uniform DirectionalLight u_directionalLight;

uniform PointLight u_pointLights[MAX_POINT_LIGHTS];
uniform int u_pointLightsCount;

uniform SpotLight u_spotLights[MAX_SPOT_LIGHTS];
uniform int u_spotLightsCount;

uniform Material u_material;
uniform Sampler u_samplers;

uniform vec3 u_worldCameraPos;

uniform vec4 u_colorMod;

vec3 calcShadowCoords(){
    vec3 ProjCoords = LightSpacePos.xyz / LightSpacePos.w;
    vec3 ShadowCoords = ProjCoords * 0.5 + vec3(0.5);
    return ShadowCoords;
}

float calcShadowFactorBasic(vec3 lightDirection, vec3 normal){

    vec3 shadowCoords = calcShadowCoords();

    if(shadowCoords.z > 1.0){
        return 1.0;
    }

    vec2 texelSize = 1.0 / textureSize(u_samplers.shadowMap, 0);

    float diffuseFactor = dot(normal, -lightDirection);
    float bias = mix(0.001, 0.0, diffuseFactor);

    // Calculating PCF shadow
    float shadow = 0.0;
    for(int x = -1; x <= 1; ++x){
        for(int y = -1; y <= 1; ++y){
            float pcfDepth = texture(u_samplers.shadowMap, shadowCoords.xy + vec2(x,y) * texelSize).x;
            shadow += shadowCoords.z > pcfDepth + bias ? 0.0 : 1.0;
        }
    }

    shadow /= 9;
    return shadow*0.5;
}

float calcShadowFactorPointLight(vec3 lightToPixel, vec3 normal){
    float diffuseFactor = dot(normal, -lightToPixel);
    float bias = mix(0.001, 0.0, diffuseFactor);

    float dist = length(lightToPixel);
    lightToPixel.y = -lightToPixel.y;
    lightToPixel.z = -lightToPixel.z;
    float sampledDist = texture(u_samplers.shadowCubeMap, lightToPixel).r;

    if(sampledDist + bias < dist){
        return 0.5;
    }else{
        return 1.0;
    }
}

vec4 calcLightInternalColor(BaseLight baseLight, vec3 direction, vec3 normal, float shadowFactor){
    // Base ambient color
    vec4 ambientColor = vec4(baseLight.color, 1.0f) *
                        baseLight.ambientIntensity *
                        vec4(u_material.ambientColor, 1.0f);

    vec4 diffuseColor  = vec4(0.0f,0.0f,0.0f,0.0f);
    vec4 specularColor = vec4(0.0f,0.0f,0.0f,0.0f);

    // Diffusion factor from the angle between normal and direction of light
    float diffuseFactor = dot(normal, -direction);

    if(diffuseFactor > 0){
        diffuseColor = vec4(baseLight.color, 1.0f) *
                       baseLight.diffuseIntensity *
                       vec4(u_material.diffuseColor, 1.0f) *
                       diffuseFactor;

        if(diffuseColor != vec4(0.0,0.0,0.0,0.0)){
            // To get specular factor, we need direction from camera to pixel
            vec3 pix2camera = normalize(u_worldCameraPos - WorldPos0);

            // Angle of reflected light (having opposite angle then the direction of light)
            vec3 lightReflect = normalize(reflect(direction, normal));

            // Specular factor from the angle between a direction from pixel to camera and reflected light
            float specularFactor = dot(pix2camera, lightReflect);

            if(specularFactor > 0){
                // Specular factor received from a separate specular texture
                float specularExp = texture2D(u_samplers.specular, TexCoord0).r * 255.0;
                specularFactor = pow(specularFactor, specularExp);

                specularColor = vec4(baseLight.color, 1.0f) *
                                vec4(u_material.specularColor, 1.0f) *
                                specularFactor *
                                u_material.shininess;
            }
        }
    }

    // Returning the color from diffuse texture
    // Diffuse and specular color is affected by shadow factor
    return texture2D(u_samplers.diffuse, TexCoord0.xy) *
           clamp((ambientColor + shadowFactor * (diffuseColor + specularColor)), 0, 1);
}

// Calculates directional light
// There's only one directional light calculated by calcLightInternalColor
vec4 calcDirectionalLight(vec3 normal){
    //return calcLightInternalColor(u_directionalLight.base, u_directionalLight.direction, normal, calcShadowFactorBasic(u_directionalLight.direction, normal));
    return calcLightInternalColor(u_directionalLight.base, u_directionalLight.direction, normal, 1.0);
}

// Calculate point lights
// There's multiple point lights, with maximum of MAX_POINT_LIGHTS and actual count in u_pointLightsCount
vec4 calcPointLight(PointLight pointLight, vec3 normal, bool isSpot){

    // Calculating world direction from light to pixel and shader factor
    vec3 lightWorldDir = WorldPos0 - pointLight.pos;
    float shadowFactor = 0.0f;
    if(isSpot){
        shadowFactor = calcShadowFactorBasic(normalize(lightWorldDir), normal);
    }else{
        shadowFactor = calcShadowFactorPointLight(lightWorldDir, normal);
    }

    // Calculating distance from light to pixel
    float lightDistance = length(lightWorldDir);

    // Calculating color of the pixel
    vec4 color = calcLightInternalColor(pointLight.base, normalize(lightWorldDir), normal, shadowFactor);

    // Calculating light attenuation
    float atten = pointLight.atten.constant +
                  pointLight.atten.linear * lightDistance +
                  pointLight.atten.exp * lightDistance * lightDistance;

    // Returning color with attenuation factored in
    return color / atten;
}

// Calculate spot lights
// There's multiple spot lights, with maximum of MAX_SPOT_LIGHTS and actual count in u_spotLightsCount
vec4 calcSpotLight(SpotLight spotLight, vec3 normal){

    // Calculating direction from light to pixel
    vec3 light2pixel = normalize(WorldPos0 - spotLight.base.pos);

    // Dot product between the direction from light to pixel and direction of a given spot light
    float spotFactor = dot(light2pixel, spotLight.direction);

    // The light shines only on parts within defined angle of a light cone
    if(spotFactor > spotLight.angle){

        // Calculates color of the material within the cone
        vec4 color = calcPointLight(spotLight.base, normal, true);

        // Calculates intestity of the light within the cone to smoothly transition the corners
        float intensity = (1.0 - (1.0 - spotFactor)/(1.0 - spotLight.angle));
        return color * intensity;
    }else{
        return vec4(0,0,0,0);
    }
}

void main(){
    vec3 normal = texture(u_samplers.normal, TexCoord0.xy).rgb;
    normal = normal * 2.0 - 1.0;
    normal = normalize(TBN * normal);
    //normal = normalize(Normal0);

    vec4 totalLight = calcDirectionalLight(normal);

    for(int i = 0; i < u_spotLightsCount; i++){
        totalLight += calcSpotLight(u_spotLights[i], normal);
    }

    for(int i = 0; i < u_pointLightsCount; i++){
        totalLight += calcPointLight(u_pointLights[i], normal, false);
    }

    FragColor = texture(u_samplers.diffuse, TexCoord0.xy) * totalLight * u_colorMod;
    //FragColor = vec4(normal, 1.0f);
    //FragColor = vec4(texture(u_samplers.normal, TexCoord0.xy).rgb,1.0f);
}