
out vec4 FragColor;

in vec4 Color0;
in vec2 TexCoord0;
in vec3 Pos0;
in vec3 Normal0;

struct BaseLight{
    vec3 color;
    float ambientIntensity;
    float diffuseIntensity;
};

struct DirectionalLight {
    BaseLight base;
    vec3 direction;
};

struct Sampler {
    sampler2D height[MAX_TERRAIN_HEIGHT_TEXTURE];
};

uniform Sampler u_samplers;

uniform float u_textureHeight[MAX_TERRAIN_HEIGHT_TEXTURE];
uniform uint u_textureHeightCount;

uniform DirectionalLight u_directionalLight;

vec4 calcLightInternalColor(BaseLight baseLight, vec3 direction, vec3 normal){
    // Base ambient color
    vec4 ambientColor = vec4(baseLight.color, 1.0f) *
    baseLight.ambientIntensity;

    vec4 diffuseColor  = vec4(0.0f,0.0f,0.0f,0.0f);
    vec4 specularColor = vec4(0.0f,0.0f,0.0f,0.0f);

    // Diffusion factor from the angle between normal and direction of light
    float diffuseFactor = dot(normal, -direction);

    if(diffuseFactor > 0){
        diffuseColor = vec4(baseLight.color, 1.0f) *
                       baseLight.diffuseIntensity *
                       diffuseFactor;
    }

    return clamp((ambientColor + diffuseColor + specularColor), 0, 1);
}


// Calculates directional light
// There's only one directional light calculated by calcLightInternalColor
vec4 calcDirectionalLight(vec3 normal){
    return calcLightInternalColor(u_directionalLight.base, u_directionalLight.direction, normal);
}

vec4 blendHeightTextures(uint lowerIndex){
    float height = Pos0.y;

    vec4 color0 = texture(u_samplers.height[lowerIndex], TexCoord0);
    vec4 color1 = texture(u_samplers.height[lowerIndex+1], TexCoord0);
    float delta = u_textureHeight[lowerIndex+1] - u_textureHeight[lowerIndex];
    float factor = (height - u_textureHeight[lowerIndex]) / delta;
    return mix(color0, color1, factor);
}

void main(){
    float height = Pos0.y;
    vec3 normal = normalize(Normal0);
    vec4 texColor;

    switch(u_textureHeightCount){
        case 0:
            texColor = Color0;
        break;

        case 1:
            texColor = texture(u_samplers.height[0], TexCoord0);
        break;

        case 2:
            if(height < u_textureHeight[0]){
                texColor = texture(u_samplers.height[0], TexCoord0);
            }else if(height < u_textureHeight[1]){
                texColor = blendHeightTextures(0);
            }else{
                texColor = texture(u_samplers.height[1], TexCoord0);
            }
        break;

        case 3:
            if(height < u_textureHeight[0]){
                texColor = texture(u_samplers.height[0], TexCoord0);
            }else if(height < u_textureHeight[1]){
                texColor = blendHeightTextures(0);
            }else if(height < u_textureHeight[2]){
                texColor = blendHeightTextures(1);
            }else{
                texColor = texture(u_samplers.height[2], TexCoord0);
            }
        break;

        case 4:
            if(height < u_textureHeight[0]){
                texColor = texture(u_samplers.height[0], TexCoord0);
            }else if(height < u_textureHeight[1]){
                texColor = blendHeightTextures(0);
            }else if(height < u_textureHeight[2]){
                texColor = blendHeightTextures(1);
            }else if(height < u_textureHeight[3]){
                texColor = blendHeightTextures(2);
            }else{
                texColor = texture(u_samplers.height[3], TexCoord0);
            }
        break;

        default:
            texColor = Color0;
        break;
    }

    vec4 totalLight = calcDirectionalLight(normal);

    FragColor = texColor * totalLight;

    //FragColor = Color0;
}