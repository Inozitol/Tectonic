#version 330 core

in vec2 TexCoord0;
in vec3 Normal0;
in vec3 LocalPos0;
out vec4 FragColor;

struct DirectionalLight {
    vec3 color;
    float ambient_intensity;
    float diffuse_intensity;
    vec3 direction;
};

struct Material {
    vec3 ambient_color;
    vec3 diffuse_color;
    vec3 specular_color;
};

struct Sampler{
    sampler2D diffuse;
    sampler2D specular;
};

uniform DirectionalLight directional_light;
uniform Material material;
uniform Sampler sampler;
uniform vec3 local_camera_pos;

void main(){
    vec4 ambient_color = vec4(directional_light.color, 1.0f) *
                         directional_light.ambient_intensity *
                         vec4(material.ambient_color, 1.0f);

    vec3 normal = normalize(Normal0);

    float diffuse_factor = dot(normal, -directional_light.direction);

    vec4 diffuse_color = vec4(0.0f,0.0f,0.0f,0.0f);
    vec4 specular_color = vec4(0.0f,0.0f,0.0f,0.0f);

    if(diffuse_factor > 0){
        diffuse_color = vec4(directional_light.color, 1.0f) *
                        directional_light.diffuse_intensity *
                        vec4(material.diffuse_color, 1.0f) *
                        diffuse_factor;
        vec3 pix2camera = normalize(local_camera_pos - LocalPos0);
        vec3 light_reflect = normalize(reflect(directional_light.direction, normal));
        float specular_factor = dot(pix2camera, light_reflect);
        if(specular_factor > 0){
            float specular_exponent = texture2D(sampler.specular, TexCoord0).r * 255.0;
            specular_factor = pow(specular_factor, specular_exponent);
            specular_color = vec4(directional_light.color, 1.0f) *
                             vec4(material.specular_color, 1.0f) *
                             specular_factor;
        }
    }

    FragColor = texture2D(sampler.diffuse, TexCoord0.xy) *
                clamp((ambient_color + diffuse_color + specular_color), 0, 1);
}