#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 color;

struct Light
{
    vec3  position;
    vec3  color;
    float intensity;   // 0.0 = luz apagada
};

uniform Light sun;
uniform Light moon;

uniform vec3 ambientColor;
uniform vec3 viewPos;

// Los fija Model.h por cada malla (igual que en modelLoading.frag)
uniform sampler2D texture_diffuse1;
uniform bool  hasTexture;
uniform vec3  materialDiffuse;   // Kd del .mtl cuando no hay textura

vec3 CalcLight(Light light, vec3 N, vec3 V, vec3 albedo)
{
    if (light.intensity <= 0.0)
        return vec3(0.0);

    vec3  L    = normalize(light.position - FragPos);
    float diff = max(dot(N, L), 0.0);

    vec3  H    = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 32.0);

    float d   = length(light.position - FragPos);
    float att = 1.0 / (1.0 + 0.007 * d + 0.0002 * d * d);

    return light.intensity * att * light.color * (diff * albedo + 0.3 * spec);
}

void main()
{
    vec3 albedo;

    if (hasTexture)
    {
        vec4 tex = texture(texture_diffuse1, TexCoords);
        if (tex.a < 0.05)
            discard;
        albedo = tex.rgb;
    }
    else
    {
        albedo = materialDiffuse;
    }

    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);

    vec3 result = ambientColor * albedo;
    result += CalcLight(sun,  N, V, albedo);
    result += CalcLight(moon, N, V, albedo);

    color = vec4(result, 1.0);
}
