#version 330 core

const int MAX_POINT_LIGHTS = 2;
const int MAX_SPOT_LIGHTS = 2;

in vec2 TexCoord0;
in vec3 Normal0;
in vec3 LocalPos0;

out vec4 FragColor;

struct BaseLight{
    vec3 color;
    float ambient_intensity;
    float diffuse_intensity;
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
    vec3 local_pos;
    Atteniuation atten;
};

struct SpotLight {
    PointLight base;
    vec3 direction;
    float angle;    // Cosine of the angle
};

struct Material {
    vec3 ambient_color;
    vec3 diffuse_color;
    vec3 specular_color;
};

struct Sampler {
    sampler2D diffuse;
    sampler2D specular;
};

uniform DirectionalLight directional_light;

uniform PointLight point_lights[MAX_POINT_LIGHTS];
uniform int num_point_lights;

uniform SpotLight spot_lights[MAX_SPOT_LIGHTS];
uniform int num_spot_lights;

uniform Material material;
uniform Sampler sampler;
uniform vec3 local_camera_pos;

vec4 calc_light_internal_color(BaseLight base, vec3 direction, vec3 normal){
    vec4 ambient_color = vec4(base.color, 1.0f) *
                         base.ambient_intensity *
                         vec4(material.ambient_color, 1.0f);

    float diffuse_factor = dot(normal, -direction);

    vec4 diffuse_color = vec4(0.0f,0.0f,0.0f,0.0f);
    vec4 specular_color = vec4(0.0f,0.0f,0.0f,0.0f);

    if(diffuse_factor > 0){
        diffuse_color = vec4(base.color, 1.0f) *
                        base.diffuse_intensity *
                        vec4(material.diffuse_color, 1.0f) *
                        diffuse_factor;

        vec3 pix2camera = normalize(local_camera_pos - LocalPos0);
        vec3 light_reflect = normalize(reflect(direction, normal));
        float specular_factor = dot(pix2camera, light_reflect);
        if(specular_factor > 0){
            float specular_exp = texture2D(sampler.specular, TexCoord0).r * 255.0;
            specular_factor = pow(specular_factor, specular_exp);
            specular_color = vec4(base.color, 1.0f) *
                             vec4(material.specular_color, 1.0f) *
                             specular_factor;
        }
    }

    return texture2D(sampler.diffuse, TexCoord0.xy) *
           clamp((ambient_color + diffuse_color + specular_color), 0, 1);
}

vec4 calc_directional_light(vec3 normal){
    return calc_light_internal_color(directional_light.base, directional_light.direction, normal);
}

vec4 calc_point_light(PointLight l, vec3 normal){
    vec3 light_direction = LocalPos0 - l.local_pos;
    float distance = length(light_direction);
    light_direction = normalize(light_direction);

    vec4 color = calc_light_internal_color(l.base, light_direction, normal);
    float atten = l.atten.constant +
                  l.atten.linear * distance +
                  l.atten.exp * distance * distance;
    return color / atten;
}

vec4 calc_spot_light(SpotLight l, vec3 normal){
    vec3 light2pixel = normalize(LocalPos0 - l.base.local_pos);
    float spot_factor = dot(light2pixel, l.direction);

    if(spot_factor > l.angle){
        vec4 color = calc_point_light(l.base, normal);
        float light_intensity = (1.0 - (1.0 - spot_factor)/(1.0 - l.angle));
        return color * light_intensity;
    }else{
        return vec4(0,0,0,0);
    }
}

void main(){
    vec3 normal = normalize(Normal0);
    vec4 total_light = calc_directional_light(normal);

    for(int i = 0; i < num_point_lights; i++){
        total_light += calc_point_light(point_lights[i], normal);
    }
    for(int i = 0; i < num_spot_lights; i++){
        total_light += calc_spot_light(spot_lights[i], normal);
    }

    FragColor = texture2D(sampler.diffuse, TexCoord0.xy) * total_light;
}