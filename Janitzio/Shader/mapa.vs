#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 vertexColor;
layout (location = 2) in float vertexMaterial;
layout (location = 3) in float vertexStone;
layout (location = 4) in vec3 modelNormal;
layout (location = 5) in vec2 modelUV;
layout (location = 6) in float modelLayer;
layout (location = 7) in vec3 instancePosition;
layout (location = 8) in vec3 instanceScale;
layout (location = 9) in vec2 instanceRotation;
layout (location = 10) in vec2 paperCoordinates;
uniform mat4 transform;
uniform bool sky;
uniform bool importedModel;
out vec3 color;
out vec3 world;
out vec2 screen;
flat out float material;
out float stone;
out vec2 uv;
flat out float layer;
flat out float houseSeed;

vec3 rotateModel(vec3 p)
{
    return vec3(instanceRotation.x*p.x+instanceRotation.y*p.z,p.y,
        -instanceRotation.y*p.x+instanceRotation.x*p.z);
}

void main()
{
    color = vertexColor;
    world = position;
    material = vertexMaterial;
    stone = vertexStone;
    uv = paperCoordinates;
    layer = 0.0;
    houseSeed = 0.0;
    if (importedModel) {
        world = instancePosition+rotateModel(position*instanceScale);
        vec3 normal = rotateModel(modelNormal/instanceScale);
        normal /= max(0.0001,length(normal));
        float light = max(0.0,dot(normal,normalize(vec3(-0.6,1.0,0.55))));
        houseSeed = fract(sin(dot(instancePosition.xz,vec2(37.2,78.4)))*41357.71);
        color = (0.61+0.39*light)*mix(vec3(0.92,0.94,0.96),vec3(1.04,1.02,1.0),houseSeed);
        material = 5.0;
        stone = 0.0;
        uv = vec2(modelUV.x,1.0-modelUV.y);
        layer = modelLayer;
    }
    screen = vec2((gl_VertexID & 1) * 4 - 1, (gl_VertexID >> 1) * 4 - 1);
    gl_Position = sky ? vec4(screen, 1.0, 1.0) : transform * vec4(world, 1.0);
}
