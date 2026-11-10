#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 color;

// ---------- Estructuras de luz ----------
struct PointLight
{
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
    int  enabled;
};

struct SpotLight
{
    vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
    float cutOff;
    float outerCutOff;
    int  enabled;
};

struct Material { float shininess; };

#define NR_POINT_LIGHTS 5

uniform vec3 viewPos;
uniform vec3 globalAmbient;               // ambiente minimo (casi oscuro)
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight  spotLight;             // luz de la camara
uniform Material   material;

// Los manda Mesh.h
uniform sampler2D texture_diffuse1;
uniform int  hasTexture;
uniform vec3 materialDiffuse;
uniform float opacity;

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo)
{
    if (light.enabled == 0) return vec3(0.0);

    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    float d = length(light.position - fragPos);
    float att = 1.0 / (light.constant + light.linear * d + light.quadratic * d * d);

    vec3 ambient  = light.ambient  * albedo;
    vec3 diffuse  = light.diffuse  * diff * albedo;
    vec3 specular = light.specular * spec * vec3(0.5);

    return (ambient + diffuse + specular) * att;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo)
{
    if (light.enabled == 0) return vec3(0.0);

    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    float d = length(light.position - fragPos);
    float att = 1.0 / (light.constant + light.linear * d + light.quadratic * d * d);

    float theta = dot(lightDir, normalize(-light.direction));
    float eps = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / eps, 0.0, 1.0);

    vec3 ambient  = light.ambient  * albedo;
    vec3 diffuse  = light.diffuse  * diff * albedo;
    vec3 specular = light.specular * spec * vec3(0.5);

    return (ambient + diffuse + specular) * att * intensity;
}

void main()
{
    vec4 tex = vec4(materialDiffuse, 1.0);
    if (hasTexture == 1) tex = texture(texture_diffuse1, TexCoords);

    vec3 albedo = tex.rgb;
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 result = globalAmbient * albedo;

    for (int i = 0; i < NR_POINT_LIGHTS; i++)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, albedo);

    result += CalcSpotLight(spotLight, norm, FragPos, viewDir, albedo);

    color = vec4(result, tex.a * opacity);
}
