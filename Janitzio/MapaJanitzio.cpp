#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

#include "Camera.h"
#include "Shader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "ModelosJanitzio.h"

namespace {
struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
    float material;
    float piedra = 0;
    glm::vec2 papelUV{0};
};

using Mesh = std::vector<Vertex>;

const glm::vec2 cima(0.0f, -12.5f);
const glm::vec2 plaza(0.0f, -8.0f);
const glm::vec2 ofrenda(0.0f, -6.5f);
enum class FiguraOfrenda { Circulo, Flor, Rombo, Cruz };
struct ConjuntoOfrenda { glm::vec2 centro; float radio; FiguraOfrenda figura; };
using Figura = FiguraOfrenda;
const std::array<ConjuntoOfrenda,10> conjuntosPlaza{{
    {ofrenda+glm::vec2(-2.79f,8.07f),1.76f,Figura::Circulo},
    {ofrenda+glm::vec2(2.79f,8.07f),1.76f,Figura::Flor},
    {ofrenda+glm::vec2(-6.18f,3.27f),1.40f,Figura::Rombo},
    {ofrenda+glm::vec2(6.18f,3.22f),1.41f,Figura::Circulo},
    {ofrenda+glm::vec2(-6.25f,0.18f),1.45f,Figura::Flor},
    {ofrenda+glm::vec2(6.24f,0.12f),1.44f,Figura::Rombo},
    {ofrenda+glm::vec2(-5.93f,-2.94f),1.22f,Figura::Circulo},
    {ofrenda+glm::vec2(5.96f,-2.95f),1.22f,Figura::Cruz},
    {ofrenda+glm::vec2(-2.42f,4.07f),0.67f,Figura::Rombo},
    {ofrenda+glm::vec2(2.42f,4.07f),0.67f,Figura::Flor}
}};
const std::array<glm::vec2,8> macetonesPlaza{{
    ofrenda+glm::vec2(-7.58f,7.23f),ofrenda+glm::vec2(7.57f,7.23f),
    ofrenda+glm::vec2(-8.18f,1.45f),ofrenda+glm::vec2(8.14f,1.40f),
    ofrenda+glm::vec2(-7.46f,-1.65f),ofrenda+glm::vec2(7.49f,-1.62f),
    ofrenda+glm::vec2(-4.66f,-4.86f),ofrenda+glm::vec2(4.67f,-4.86f)
}};

bool ocupaPlaza(glm::vec2 p,float margen)
{
    for (const auto& c : conjuntosPlaza) {
        if (glm::distance(p,c.centro)<c.radio+margen) return true;
    }
    for (const auto centro : macetonesPlaza) {
        if (glm::distance(p,centro)<0.48f+margen) return true;
    }
    return false;
}

const std::array<glm::vec2, 9> callePrincipal{{
    {0, 37}, {0, 33}, {-4, 26}, {-2, 19}, {2, 12}, {3, 4.46f}, {0, 4.34f}, {0, -1}, {0, -2.38f}
}};
const std::array<glm::vec2, 6> callejonOeste{{
    callePrincipal[2], {-13, 27}, {-22, 20}, {-18, 12}, {-10, 7}, callePrincipal[5]
}};
const std::array<glm::vec2, 7> callejonEste{{
    callePrincipal[3], {8, 22}, {18, 19}, {25, 13}, {20, 5}, {12, -1}, callePrincipal[7]
}};
const std::array<glm::vec2, 4> faroles{{{-3.6f,34},{-5.9f,23.8f},{5.5f,6.7f},{4.0f,-4.2f}}};
constexpr size_t maxReflejos = 18;
struct Bosque {
    glm::vec2 centro;
    glm::vec2 radio;
};
const std::array<Bosque,3> bosques{{{{-46,3},{5.4f,10.5f}},{{46,-2},{5.4f,10.5f}},{{-18,-31},{9.7f,4.9f}}}};
const std::array<glm::vec2,4> arbolesPlaza{{plaza+glm::vec2(-6.7f,-4.2f),plaza+glm::vec2(6.8f,-4.2f),
    plaza+glm::vec2(7.3f,2.4f),plaza+glm::vec2(-7.2f,2.3f)}};
constexpr float alturaOjos = 1.65f;
constexpr float alturaMuelle = 0.65f;
constexpr float medioAnchoMuelle = 2.85f;
constexpr float medioAnchoPlataforma = 5.45f;
constexpr float inicioMuelle = 35.6f;
constexpr float inicioPlataforma = 45.8f;
constexpr float finMuelle = 50.7f;

struct Atraque { float x,z,ancho,largo,giro; };
const std::array<Atraque,4> atraques{{
    {-4.47f,41.42f,2.38f,7.15f,0.025f},{4.46f,41.57f,2.39f,7.34f,-0.024f},
    {-7.10f,46.73f,2.48f,7.55f,0.018f},{7.13f,47.06f,2.47f,7.61f,-0.020f}
}};

bool enMuelle(glm::vec2 p)
{
    if (p.y<inicioMuelle || p.y>finMuelle) return false;
    return std::abs(p.x)<=(p.y>=inicioPlataforma ? medioAnchoPlataforma : medioAnchoMuelle);
}

float semilla(int x, int z)
{
    uint32_t hash = static_cast<uint32_t>(x) * 0x8da6b343u ^ static_cast<uint32_t>(z) * 0xd8163841u;
    hash ^= hash >> 16;
    hash *= 0x7feb352du;
    hash ^= hash >> 15;
    return static_cast<float>(hash & 65535u) / 65536.0f;
}

template <size_t N>
glm::vec2 puntoMasCercano(glm::vec2 p, const std::array<glm::vec2, N>& calle)
{
    glm::vec2 closest = calle.front();
    float distance = glm::distance(p, closest);
    for (size_t i = 1; i < calle.size(); ++i) {
        const glm::vec2 start = calle[i - 1];
        const glm::vec2 segment = calle[i] - start;
        const float t = glm::clamp(glm::dot(p - start, segment) / glm::dot(segment, segment), 0.0f, 1.0f);
        const glm::vec2 candidate = start + t * segment;
        const float candidateDistance = glm::distance(p, candidate);
        if (candidateDistance < distance) {
            distance = candidateDistance;
            closest = candidate;
        }
    }
    return closest;
}

float distanciaPlaza(glm::vec2 p)
{
    return glm::length((p - plaza) / glm::vec2(9.1f,8.1f));
}

float relieve(glm::vec2 p)
{
    const float angle = std::atan2(p.y, p.x);
    const float coast = 1.0f + 0.045f * std::sin(3.0f * angle) + 0.027f * std::cos(5.0f * angle);
    const float radius = glm::length(p / glm::vec2(60.0f,42.5f)) / coast;
    const float slope = glm::smoothstep(0.0f,0.64f,1.0f-radius);
    const glm::vec2 summit = p-cima;
    const float hill = 1.9f * std::exp(-glm::dot(summit,summit)/175.0f);
    return (9.2f + hill) * slope - 0.48f;
}

float terrenoBase(glm::vec2 p)
{
    float y = relieve(p);
    static const float alturaPlaza = relieve(plaza);
    y = glm::mix(alturaPlaza,y,glm::smoothstep(1.0f,1.43f,distanciaPlaza(p)));
    const float entrada = (1.0f-glm::smoothstep(1.7f,3.4f,std::abs(p.x)))
        * glm::smoothstep(32.8f,inicioMuelle,p.y) * (1.0f-glm::smoothstep(36.4f,38.5f,p.y));
    return glm::mix(y, alturaMuelle, entrada);
}

float terreno(glm::vec2 p)
{
    const float base = terrenoBase(p);
    const auto main = puntoMasCercano(p,callePrincipal);
    const auto west = puntoMasCercano(p,callejonOeste);
    const auto east = puntoMasCercano(p,callejonEste);
    const float mainWeight = 1.0f-glm::smoothstep(1.75f,3.4f,glm::distance(p,main));
    const float westWeight = 1.0f-glm::smoothstep(0.97f,2.6f,glm::distance(p,west));
    const float eastWeight = 1.0f-glm::smoothstep(0.97f,2.6f,glm::distance(p,east));
    const float total = mainWeight+westWeight+eastWeight;
    if (total == 0.0f) return base;
    const float grade = (mainWeight*terrenoBase(main)+westWeight*terrenoBase(west)+eastWeight*terrenoBase(east))/total;
    return glm::mix(base,grade,std::max({mainWeight,westWeight,eastWeight}));
}

float alturaCaminata(glm::vec2 p)
{
    const float land = terreno(p);
    return enMuelle(p) ? std::max(alturaMuelle,land) : land;
}

glm::vec3 punto(glm::vec2 p, float y)
{
    return {p.x, y, p.y};
}

void triangle(Mesh& mesh, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 color, float material = 0)
{
    mesh.insert(mesh.end(), {{a, color, material}, {b, color, material}, {c, color, material}});
}

void quad(Mesh& mesh, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, glm::vec3 color, float material = 0)
{
    triangle(mesh, a, b, c, color, material);
    triangle(mesh, a, c, d, color, material);
}

void box(Mesh& mesh, glm::vec3 low, glm::vec3 high, glm::vec3 color,float material=0)
{
    const auto [x, y, z] = std::array<float, 3>{low.x, low.y, low.z};
    const auto [X, Y, Z] = std::array<float, 3>{high.x, high.y, high.z};
    quad(mesh, {x,Y,z}, {x,Y,Z}, {X,Y,Z}, {X,Y,z}, color,material);
    quad(mesh, {x,y,z}, {x,Y,z}, {X,Y,z}, {X,y,z}, color * 0.62f,material);
    quad(mesh, {x,y,Z}, {X,y,Z}, {X,Y,Z}, {x,Y,Z}, color * 0.82f,material);
    quad(mesh, {x,y,z}, {x,y,Z}, {x,Y,Z}, {x,Y,z}, color * 0.72f,material);
    quad(mesh, {X,y,z}, {X,Y,z}, {X,Y,Z}, {X,y,Z}, color * 0.52f,material);
    quad(mesh, {x,y,z}, {X,y,z}, {X,y,Z}, {x,y,Z}, color * 0.52f,material);
}

void casa(Mesh& mesh, glm::vec2 p, float width, float depth, float height, float yaw,
    glm::vec3 wall, glm::vec3 roof, bool flat)
{
    const size_t first = mesh.size();
    const float x = width / 2, z = depth / 2;
    const float floor = terreno(p);
    const float base = -0.85f - 0.35f * (width + depth);
    box(mesh, {-x,base,-z}, {x,0.15f,z}, wall * 0.66f);
    box(mesh, {-x,0.15f,-z}, {x,height,z}, wall);
    box(mesh, {-x-0.10f,height-0.11f,-z-0.10f}, {x+0.10f,height,z+0.10f}, wall * 0.83f);

    if (flat) {
        box(mesh, {-x,height,-z}, {x,height+0.15f,z}, roof);
        for (float side : {-1.0f, 1.0f}) {
            const float edge = side * z;
            box(mesh, {-x,height+0.12f,edge-0.06f}, {x,height+0.33f,edge+0.06f}, wall);
        }
    } else {
        const float rise = (0.25f+0.18f*semilla(int(p.x*30),int(p.y*30))) * width;
        const float eave = x + 0.13f;
        const float edge = z + 0.13f;
        for (int tile = 0; tile < 11; ++tile) {
            const float a = -edge + 2 * edge * tile / 11;
            const float b = -edge + 2 * edge * (tile + 1) / 11;
            const glm::vec3 clay = roof * (0.85f + 0.19f * semilla(tile, int(p.x*31+p.y*19)));
            quad(mesh, {-eave,height,a}, {0,height+rise,a}, {0,height+rise,b}, {-eave,height,b}, clay);
            quad(mesh, {0,height+rise,a}, {eave,height,a}, {eave,height,b}, {0,height+rise,b}, clay * 0.80f);
        }
        box(mesh, {-0.065f,height+rise-0.02f,-edge}, {0.065f,height+rise+0.06f,edge}, roof * 1.08f);
        triangle(mesh, {-x,height,z}, {x,height,z}, {0,height+rise,z}, wall * 0.82f);
        triangle(mesh, {-x,height,-z}, {0,height+rise,-z}, {x,height,-z}, wall * 0.62f);
    }

    const glm::vec3 shadow(0.14f,0.17f,0.18f);
    box(mesh, {-0.27f,0.14f,z}, {0.27f,std::min(1.55f,height-0.24f),z+0.025f}, shadow);
    for (float offset : {-0.57f, 0.57f}) {
        const float center = x * offset;
        const float sill = height - 0.87f;
        box(mesh, {center-0.26f,sill-0.08f,z}, {center+0.26f,sill+0.49f,z+0.055f}, wall * 0.84f);
        quad(mesh, {center-0.19f,sill,z+0.059f}, {center+0.19f,sill,z+0.059f},
            {center+0.19f,sill+0.41f,z+0.059f}, {center-0.19f,sill+0.41f,z+0.059f},
            {0.99f,0.66f,0.29f}, semilla(int(p.x*20+offset),int(p.y*24)) > 0.32f ? 2.0f : 0.0f);
    }
    const float c = std::cos(yaw), s = std::sin(yaw);
    for (size_t i = first; i < mesh.size(); ++i) {
        auto& v = mesh[i].position;
        v = {p.x + c*v.x + s*v.z, floor + v.y, p.y - s*v.x + c*v.z};
    }
}

void volumen(Mesh& mesh, glm::vec3 center, glm::vec3 scale, glm::vec3 color, float phase = 0)
{
    constexpr int slices = 9, bands = 6;
    const auto point = [&](int band, int slice) {
        const float y = glm::pi<float>() * (static_cast<float>(band) / bands - 0.5f);
        const float angle = phase + glm::two_pi<float>() * slice / slices;
        return center + scale * glm::vec3(std::cos(y)*std::cos(angle), std::sin(y), std::cos(y)*std::sin(angle));
    };
    const glm::vec3 light = glm::normalize(glm::vec3(-0.6f, 1, 0.55f));
    for (int j = 0; j < bands; ++j) {
        for (int i = 0; i < slices; ++i) {
            const glm::vec3 a = point(j,i), b = point(j,i+1), c = point(j+1,i+1), d = point(j+1,i);
            const glm::vec3 normal = glm::normalize(((a+b+c+d)/4.0f-center) / scale / scale);
            const float shade = 0.52f + 0.48f * std::max(0.0f,glm::dot(normal,light));
            quad(mesh, a,b,c,d,color * shade);
        }
    }
}

void arbol(Mesh& mesh, glm::vec2 p, float h, float tint)
{
    const float y = terreno(p);
    box(mesh, punto(p - glm::vec2(0.12f),y-0.24f), punto(p+glm::vec2(0.12f),y+h*0.65f), {0.28f,0.20f,0.12f});
    const glm::vec3 leaves = glm::mix(glm::vec3(0.20f,0.31f,0.15f),glm::vec3(0.38f,0.45f,0.19f),tint);
    volumen(mesh, punto(p,y+h*0.76f), glm::vec3(h*0.46f,h*0.41f,h*0.41f),leaves,tint);
    volumen(mesh, punto(p+glm::vec2(h*0.21f,0),y+h*0.58f),glm::vec3(h*0.31f,h*0.30f,h*0.35f),leaves*0.9f,tint);
}

void monumento(Mesh& mesh)
{
    const float top = terreno(cima);
    const glm::vec3 stone(0.84f,0.81f,0.70f);
    const glm::vec3 origin = punto(cima,top);
    const auto b = [&](glm::vec3 low, glm::vec3 high, float tone) {
        box(mesh,origin+low,origin+high,stone*tone);
    };
    const auto v = [&](glm::vec3 center, glm::vec3 scale, float tone) {
        volumen(mesh,origin+center,scale,stone*tone,0.2f);
    };
    b({-3.8f,-0.24f,-3.0f},{3.8f,0.18f,3.0f},0.73f);
    b({-2.8f,0.18f,-2.2f},{2.8f,0.48f,2.2f},0.83f);
    const std::array<float, 5> levels{{0.48f,2.9f,5.6f,7.15f,7.65f}};
    const std::array<float, 5> widths{{1.85f,1.60f,1.36f,1.25f,0.71f}};
    for (size_t band=1; band<levels.size(); ++band) {
        for (int i=0; i<14; ++i) {
            const auto fold = [&](size_t level, int index) {
                const float angle = glm::two_pi<float>() * index / 14;
                const float pleat = 1.0f + 0.08f * std::cos(glm::pi<float>()*index);
                return origin + glm::vec3(std::cos(angle)*widths[level]*pleat,levels[level],
                    std::sin(angle)*widths[level]*0.65f*pleat);
            };
            const float brightness = 0.72f + 0.22f * std::cos(i*glm::two_pi<float>()/14 - 2.1f);
            quad(mesh,fold(band-1,i),fold(band-1,i+1),fold(band,i+1),fold(band,i),stone*brightness);
        }
    }
    v({0.16f,8.15f,0.16f},{0.66f,0.82f,0.64f},1.0f);
    v({0.20f,8.10f,0.72f},{0.19f,0.20f,0.24f},1.04f);
    v({0.15f,8.52f,0.05f},{0.69f,0.49f,0.64f},0.76f);
    v({-1.17f,6.90f,0},{0.74f,0.91f,0.69f},0.99f);
    v({-1.64f,8.01f,0.04f},{0.46f,1.26f,0.49f},0.98f);
    v({-1.83f,9.85f,0.08f},{0.35f,1.25f,0.38f},1.0f);
    v({-1.87f,11.13f,0.12f},{0.47f,0.55f,0.42f},1.0f);
    v({1.15f,5.90f,0.22f},{0.52f,1.16f,0.58f},0.86f);
    v({0.89f,4.83f,0.63f},{0.40f,0.90f,0.44f},0.90f);
}

void embarcadero(Mesh& mesh,bool conLanchas)
{
    const glm::vec3 wood(0.51f,0.34f,0.21f);
    constexpr int tablones = 36;
    constexpr float largoTablon = (finMuelle-inicioMuelle)/tablones;
    box(mesh, {-medioAnchoMuelle,0.23f,inicioMuelle}, {medioAnchoMuelle,0.57f,finMuelle}, wood*0.71f,11.0f);
    box(mesh, {-medioAnchoPlataforma,0.23f,inicioPlataforma}, {medioAnchoPlataforma,0.57f,finMuelle}, wood*0.71f,11.0f);
    for (int i=0; i<tablones; ++i) {
        const float z = inicioMuelle+i*largoTablon;
        const float next=z+largoTablon-0.010f;
        if (z<inicioPlataforma) {
            box(mesh,{-medioAnchoMuelle,0.56f,z},{medioAnchoMuelle,alturaMuelle,std::min(inicioPlataforma,next)},
                wood*(0.82f+0.30f*semilla(i,4)),11.0f);
        }
        if (next>inicioPlataforma) {
            box(mesh,{-medioAnchoPlataforma,0.56f,std::max(z,inicioPlataforma)},
                {medioAnchoPlataforma,alturaMuelle,next},wood*(0.82f+0.30f*semilla(i,16)),11.0f);
        }
    }
    const auto pilote = [&](float x,float z,float y) {
        box(mesh,{x-0.14f,-0.48f,z-0.13f},{x+0.14f,y,z+0.13f},wood*0.70f,11.0f);
        box(mesh,{x-0.17f,y-0.065f,z-0.16f},{x+0.17f,y+0.028f,z+0.16f},wood*0.88f,11.0f);
    };
    const auto baranda = [&](float x1,float x2,float z) {
        for (float altura : {1.03f,1.52f}) {
            box(mesh,{x1,altura-0.041f,z-0.04f},{x2,altura+0.041f,z+0.04f},wood*0.81f,11.0f);
        }
    };
    for (float lado : {-1.0f,1.0f}) {
        for (float z : {37.2f,40.2f,44.1f}) pilote(lado*2.68f,z,0.92f);
        for (float z : {46.05f,48.2f,50.50f}) pilote(lado*5.28f,z,1.59f);
        for (float x : {2.75f,3.85f}) pilote(lado*x,50.50f,1.59f);
        baranda(lado<0 ? -5.28f : 2.75f,lado<0 ? -2.75f : 5.28f,50.50f);
        for (float altura : {1.03f,1.52f}) {
            const float x=lado*5.28f;
            box(mesh,{x-0.042f,altura-0.041f,46.05f},{x+0.042f,altura+0.041f,48.2f},wood*0.81f,11.0f);
        }
        const float x=lado*2.53f;
        box(mesh,{x-0.12f,-0.30f,35.68f},{x+0.12f,0.54f,45.77f},wood*0.55f,11.0f);
    }

    if (conLanchas) {
        const glm::vec3 fibra(0.53f,0.40f,0.24f);
        for (size_t i=0; i<2; ++i) {
            const auto& atraque=atraques[i];
            const float lado=atraque.x<0 ? -1.0f : 1.0f;
            const bool exterior=std::abs(atraque.x)>6.0f;
            const glm::vec3 poste(lado*(exterior ? 5.28f : 2.68f),0.94f,exterior ? 46.05f : 40.2f);
            const glm::vec3 casco(atraque.x-lado*atraque.ancho*0.43f,0.76f,atraque.z+0.36f);
            const auto cabo = [&](float t) { return glm::mix(poste,casco,t)-glm::vec3(0,0.12f*t*(1.0f-t),0); };
            for (int i=0; i<7; ++i) {
                const auto a=cabo(i/7.0f), b=cabo((i+1)/7.0f);
                const glm::vec3 grosor(0,0.013f,0);
                quad(mesh,a-grosor,b-grosor,b+grosor,a+grosor,fibra);
            }
        }
        return;
    }
    const std::array<glm::vec3,2> canvas{{{0.91f,0.88f,0.75f},{0.28f,0.52f,0.57f}}};
    for (size_t i=0; i<canvas.size(); ++i) {
        const float center = i == 0 ? -4.20f : 4.20f;
        const float length = i == 0 ? 6.6f : 6.1f;
        const float stern = inicioMuelle + (i == 0 ? 2.45f : 3.25f);
        const auto hull = [&](float x,float y,float z) { return glm::vec3(center+x,y,stern+z); };
        const std::array<glm::vec2,6> edge{{{-0.97f,0},{0.97f,0},{1.05f,length-1.8f},
            {0.0f,length},{-1.05f,length-1.8f},{-0.97f,0.3f}}};
        const glm::vec3 cream(0.91f,0.90f,0.80f);
        for (size_t j=0; j<edge.size(); ++j) {
            const auto a = edge[j], b = edge[(j+1)%edge.size()];
            quad(mesh,hull(a.x*0.68f,-0.09f,a.y),hull(b.x*0.68f,-0.09f,b.y),
                hull(b.x,0.39f,b.y),hull(a.x,0.39f,a.y),j%2 ? cream*0.78f : cream);
            triangle(mesh,hull(0,0.39f,length/2),hull(a.x,0.39f,a.y),hull(b.x,0.39f,b.y),wood);
        }
        for (float x : {-0.72f,0.72f}) {
            for (float z : {0.9f,length-2.1f}) {
                const glm::vec3 post = hull(x,0,z);
                box(mesh,post+glm::vec3(-0.042f,0.39f,-0.042f),post+glm::vec3(0.042f,1.52f,0.042f),cream*0.80f);
            }
        }
        const glm::vec3 cloth = canvas[i];
        quad(mesh,hull(-0.86f,1.50f,0.69f),hull(0,1.70f,0.69f),
            hull(0,1.70f,length-1.87f),hull(-0.86f,1.50f,length-1.87f),cloth);
        quad(mesh,hull(0,1.70f,0.69f),hull(0.86f,1.50f,0.69f),
            hull(0.86f,1.50f,length-1.87f),hull(0,1.70f,length-1.87f),cloth*0.88f);
    }
}

void montanas(Mesh& mesh, float distance, float base, float phase, glm::vec3 color)
{
    const auto crest = [&](int i) {
        const float x = 20.0f*i;
        const float y = base + 15*std::sin(i*0.35f+phase) + 10*std::cos(i*0.64f) + 7*semilla(i,int(phase*20));
        return glm::vec3(x,y,-distance+10*std::sin(i*0.35f));
    };
    for (int i=-32; i<32; ++i) {
        const glm::vec3 a=crest(i), b=crest(i+1);
        const glm::vec3 footA(a.x*0.96f,-1,a.z+43), footB(b.x*0.96f,-1,b.z+43);
        const glm::vec3 midA(a.x,a.y*0.45f,a.z+31), midB(b.x,b.y*0.45f,b.z+31);
        quad(mesh,footA,footB,midB,midA,color*(0.85f+0.13f*semilla(i,26)));
        triangle(mesh,midA,midB,a,color*(0.89f+0.12f*semilla(i,7)));
        triangle(mesh,midB,b,a,color*(0.91f+0.13f*semilla(i,18)));
    }
}

struct Huella {
    glm::vec2 centro;
    glm::vec2 lateral;
    glm::vec2 frente;
    glm::vec2 mitad;
};

struct Vivienda {
    Huella huella;
    float altura;
    float giro;
    size_t acabado;
    bool azotea;
    int indiceLote;
};

constexpr size_t acabadosVivienda = 5;

Huella huellaEn(glm::vec2 centro, glm::vec2 frente, glm::vec2 mitad)
{
    return {centro,{frente.y,-frente.x},frente,mitad};
}

bool seCruzan(const Huella& a, const Huella& b)
{
    const glm::vec2 separation = b.centro-a.centro;
    const glm::vec2 reach = a.mitad+b.mitad;
    if (glm::dot(separation,separation)>2.0f*glm::dot(reach,reach)) return false;
    for (const glm::vec2 axis : {a.lateral,a.frente,b.lateral,b.frente}) {
        const float extentA = a.mitad.x*std::abs(glm::dot(a.lateral,axis))+a.mitad.y*std::abs(glm::dot(a.frente,axis));
        const float extentB = b.mitad.x*std::abs(glm::dot(b.lateral,axis))+b.mitad.y*std::abs(glm::dot(b.frente,axis));
        if (std::abs(glm::dot(separation,axis)) >= extentA+extentB) return false;
    }
    return true;
}

bool contiene(const Huella& lote, glm::vec2 p, float margen)
{
    const glm::vec2 delta = p-lote.centro;
    return std::abs(glm::dot(delta,lote.lateral))<lote.mitad.x+margen
        && std::abs(glm::dot(delta,lote.frente))<lote.mitad.y+margen;
}

struct Farol {
    glm::vec3 base;
    float altura;
};

struct LuzFachada {
    glm::vec3 centro;
    glm::vec2 frente;
};

enum class UsoRincon { Jardineras, Descanso, Tacos, Elotes, Catrina };

bool esPuesto(UsoRincon uso)
{
    return uso==UsoRincon::Tacos || uso==UsoRincon::Elotes;
}

struct ArcoFestivo {
    glm::vec2 centro;
    float mitad;
};

const std::array<ArcoFestivo,3> arcosFestivos{{{{0,47.1f},2.22f},{{0,38.5f},2.32f},{{0,0.25f},1.28f}}};

bool pieDeArco(glm::vec2 p,float margen)
{
    for (const auto& arco : arcosFestivos) {
        for (float lado : {-1.0f,1.0f}) {
            if (glm::distance(p,arco.centro+glm::vec2(lado*arco.mitad,0))<0.22f+margen) return true;
        }
    }
    return false;
}

struct Rincon {
    glm::vec3 centro;
    glm::vec2 frente;
    float radio;
    UsoRincon uso;
};

struct DecoracionIsla {
    std::vector<Farol> faroles;
    std::vector<std::array<glm::vec3,2>> tendederos;
    std::vector<glm::vec3> macetas;
    std::vector<glm::vec3> arboles;
    std::vector<glm::vec3> lucesRibera;
    std::vector<LuzFachada> lucesFachada;
    std::vector<Rincon> rincones;
};

DecoracionIsla prepararDecoracion(const std::vector<Vivienda>& casas)
{
    DecoracionIsla tramo;
    for (float lado : {-1.0f,1.0f}) {
        tramo.faroles.push_back({{lado*2.24f,alturaMuelle,43.18f},2.35f});
        for (float z : {37.7f,39.25f,41.4f,44.5f}) {
            tramo.macetas.push_back({lado*2.38f,alturaMuelle,z});
        }
    }
    for (float lado : {-1.0f,1.0f}) {
        tramo.faroles.push_back({{lado*4.77f,alturaMuelle,49.65f},2.38f});
        tramo.macetas.push_back({lado*4.78f,alturaMuelle,46.78f});
        tramo.macetas.push_back({lado*4.87f,alturaMuelle,50.12f});
    }
    for (float z : {39.85f,45.35f,48.74f}) {
        const float x=z>=inicioPlataforma ? 5.0f : 2.46f;
        tramo.tendederos.push_back({glm::vec3(-x,alturaMuelle,z),glm::vec3(x,alturaMuelle,z)});
    }
    tramo.rincones.push_back({{-3.70f,alturaMuelle,48.15f},{1,0},1.24f,UsoRincon::Tacos});
    tramo.rincones.push_back({{3.81f,alturaMuelle,48.29f},{-1,0},1.24f,UsoRincon::Elotes});
    tramo.rincones.push_back({{-2.28f,alturaMuelle,49.92f},{0,1},0.53f,UsoRincon::Catrina});
    tramo.rincones.push_back({{2.28f,alturaMuelle,49.92f},{0,1},0.53f,UsoRincon::Catrina});
    for (const auto p : faroles) {
        tramo.faroles.push_back({punto(p,terreno(p)),2.44f});
    }

    const auto borde = [&](glm::vec2 a,glm::vec2 b,float t,float lado,float ancho,float radio,
        glm::vec3& salida) {
        const glm::vec2 direccion = glm::normalize(b-a);
        const glm::vec2 lateral(-direccion.y,direccion.x);
        for (float avance : {0.0f,0.19f,-0.19f,0.42f,-0.42f,0.66f,-0.66f}) {
            for (float distancia : {0.05f,0.20f,0.43f,0.68f,0.94f,1.23f}) {
                const auto p = glm::mix(a,b,t)+direccion*avance+lateral*(lado*(ancho+radio+distancia));
                if (terreno(p)<0.22f || distanciaPlaza(p)<0.92f || ocupaPlaza(p,radio+0.12f)
                    || glm::distance(p,puntoMasCercano(p,callePrincipal))<2.12f+radio
                    || glm::distance(p,puntoMasCercano(p,callejonOeste))<1.20f+radio
                    || glm::distance(p,puntoMasCercano(p,callejonEste))<1.20f+radio) continue;
                const bool dentroCasa = std::any_of(casas.begin(),casas.end(),[&](const Vivienda& v) {
                    return contiene(v.huella,p,radio+0.05f);
                });
                if (dentroCasa) continue;
                salida = punto(p,terreno(p));
                return true;
            }
        }
        return false;
    };

    const auto separacion = [](glm::vec3 p,glm::vec3 q) {
        return glm::distance(glm::vec2(p.x,p.z),glm::vec2(q.x,q.z));
    };
    const auto despejado = [&](glm::vec3 p,float margen) {
        return std::all_of(tramo.faroles.begin(),tramo.faroles.end(),[&](const Farol& f) {
            return separacion(p,f.base)>margen+0.18f;
        }) && std::all_of(tramo.macetas.begin(),tramo.macetas.end(),[&](glm::vec3 base) {
            return separacion(p,base)>margen+0.24f;
        }) && std::all_of(tramo.tendederos.begin(),tramo.tendederos.end(),[&](const auto& extremos) {
            return separacion(p,extremos[0])>margen+0.08f && separacion(p,extremos[1])>margen+0.08f;
        });
    };
    const auto decorarSegmento = [&](glm::vec2 a,glm::vec2 b,float ancho,
        const std::vector<float>& cuerdas,const std::vector<float>& flores) {
        for (float t : cuerdas) {
            std::array<glm::vec3,2> anclas;
            if (borde(a,b,t,-1,ancho,0.07f,anclas[0]) && borde(a,b,t,1,ancho,0.07f,anclas[1])
                && despejado(anclas[0],0.07f) && despejado(anclas[1],0.07f)) {
                tramo.tendederos.push_back(anclas);
            }
        }
        for (float t : flores) {
            for (float lado : {-1.0f,1.0f}) {
                glm::vec3 p;
                if (!borde(a,b,t,lado,ancho,0.26f,p)) continue;
                if (despejado(p,0.30f)) tramo.macetas.push_back(p);
            }
        }
    };
    struct BordeFarol { size_t segmento; float avance; float lado; };
    const std::array<BordeFarol,4> nuevos{{{1,0.35f,1}, {2,0.42f,-1}, {2,0.79f,1}, {3,0.32f,-1}}};
    for (const auto& t : nuevos) {
        glm::vec3 base;
        if (borde(callePrincipal[t.segmento-1],callePrincipal[t.segmento],t.avance,t.lado,2.18f,0.21f,base)) {
            tramo.faroles.push_back({base,2.54f});
        }
    }
    decorarSegmento(callePrincipal[0],callePrincipal[1],2.18f,{0.23f,0.80f},{0.10f,0.31f,0.52f,0.73f,0.94f});
    decorarSegmento(callePrincipal[1],callePrincipal[2],2.18f,{0.20f,0.50f,0.79f},
        {0.09f,0.21f,0.33f,0.46f,0.58f,0.71f,0.83f,0.96f});
    const auto cubrirCalle = [&](const auto& calle, size_t desde, float ancho, int ladoInicial) {
        for (size_t i=desde; i<calle.size(); ++i) {
            const auto a=calle[i-1], b=calle[i];
            const float longitud=glm::distance(a,b);
            const int lado=ladoInicial*(i%2==0 ? -1 : 1);
            for (float d=0.85f; d<longitud-0.40f; d+=2.25f) {
                glm::vec3 base;
                if (borde(a,b,d/longitud,static_cast<float>(lado),ancho,0.20f,base) && despejado(base,0.25f)) {
                    tramo.faroles.push_back({base,2.20f+0.30f*semilla(static_cast<int>(i),static_cast<int>(d*11))});
                }
                if (longitud>5.0f && borde(a,b,(d+0.60f)/longitud,static_cast<float>(-lado),ancho,0.20f,base)
                    && despejado(base,0.25f)) {
                    tramo.faroles.push_back({base,2.38f});
                }
            }
            std::vector<float> cuerdas, flores;
            for (float d=0.73f; d<longitud-0.36f; d+=1.92f) cuerdas.push_back(d/longitud);
            for (float d=0.42f; d<longitud-0.34f; d+=0.76f) flores.push_back(d/longitud);
            decorarSegmento(a,b,ancho,cuerdas,flores);
        }
    };
    cubrirCalle(callejonOeste,1,1.26f,-1);
    cubrirCalle(callejonEste,1,1.26f,1);
    cubrirCalle(callePrincipal,3,2.18f,1);

    for (int i=0; i<112; ++i) {
        const float a=glm::two_pi<float>()*i/112;
        const glm::vec2 radio(std::cos(a)*60.0f,std::sin(a)*42.5f);
        float dentro=0.72f, fuera=1.08f;
        for (int paso=0; paso<11; ++paso) {
            const float t=(dentro+fuera)*0.5f;
            if (terrenoBase(radio*t)>0.17f) dentro=t; else fuera=t;
        }
        const auto p=radio*dentro;
        if (p.y>32.0f && std::abs(p.x)<3.8f) continue;
        if (std::any_of(casas.begin(),casas.end(),[p](const Vivienda& v) { return contiene(v.huella,p,0.22f); })) continue;
        tramo.lucesRibera.push_back(punto(p,terreno(p)+0.62f+0.16f*semilla(i,22)));
    }
    for (size_t i=0; i<casas.size(); ++i) {
        const auto& casa=casas[i];
        const float seed=semilla(casa.indiceLote,317);
        if (seed>0.82f) continue;
        const auto& h=casa.huella;
        const auto p=h.centro+h.frente*(h.mitad.y+0.11f)+h.lateral*((seed<0.40f ? -1.0f : 1.0f)*h.mitad.x*0.58f);
        tramo.lucesFachada.push_back({punto(p,terreno(h.centro)+1.04f+casa.altura*0.18f),h.frente});
    }

    const auto plantarArce = [&](glm::vec2 p,float alturaMinima) {
        const auto base=punto(p,terreno(p));
        if (base.y<alturaMinima || distanciaPlaza(p)<0.97f || ocupaPlaza(p,0.55f)
            || glm::distance(p,puntoMasCercano(p,callePrincipal))<3.08f
            || glm::distance(p,puntoMasCercano(p,callejonOeste))<2.13f
            || glm::distance(p,puntoMasCercano(p,callejonEste))<2.13f || !despejado(base,0.60f)) return false;
        if (std::any_of(bosques.begin(),bosques.end(),[p](const Bosque& b) {
            return glm::length((p-b.centro)/(b.radio+glm::vec2(1.35f)))<1.0f;
        })) return false;
        if (std::any_of(casas.begin(),casas.end(),[p](const Vivienda& v) { return contiene(v.huella,p,0.58f); })) return false;
        if (std::any_of(arbolesPlaza.begin(),arbolesPlaza.end(),[p](glm::vec2 tree) {
            return glm::distance(p,tree)<2.60f;
        }) || std::any_of(tramo.arboles.begin(),tramo.arboles.end(),[&](glm::vec3 tree) {
            return separacion(tree,base)<2.85f;
        }) || std::any_of(tramo.lucesRibera.begin(),tramo.lucesRibera.end(),[&](glm::vec3 luz) {
            return separacion(luz,base)<0.60f;
        })) return false;
        tramo.arboles.push_back(base);
        return true;
    };

    // Los troncos se colocan en los claros entre solares, fuera del corredor peatonal.
    for (int fila=-7; fila<=7; ++fila) {
        for (int col=-11; col<=11; ++col) {
            const float seed=semilla(col,fila+37);
            if (seed>0.69f) continue;
            const glm::vec2 p(4.7f*col+2.7f*(seed-0.5f),4.75f*fila+2.3f*(semilla(fila,col+29)-0.5f));
            plantarArce(p,0.40f);
        }
    }
    for (size_t i=0; i<tramo.lucesRibera.size(); ++i) {
        const auto& actual=tramo.lucesRibera[i];
        const auto& siguiente=tramo.lucesRibera[(i+1)%tramo.lucesRibera.size()];
        const glm::vec2 a(actual.x,actual.z), b(siguiente.x,siguiente.z);
        const glm::vec2 centro=glm::mix(a,b,0.44f+0.14f*semilla(static_cast<int>(i),33));
        if (glm::distance(a,b)>5.6f || (centro.y>29.0f && std::abs(centro.x)<8.6f)
            || semilla(static_cast<int>(i),87)>0.72f) continue;
        const auto interior=-glm::normalize(centro);
        for (float paso : {0.34f,0.85f,1.40f,-0.18f}) {
            if (plantarArce(centro+interior*paso,0.10f)) break;
        }
    }

    struct Sitio { glm::vec2 objetivo; UsoRincon uso; };
    using Uso = UsoRincon;
    const std::array<Sitio,18> sitios{{
        {{-43,14},Uso::Jardineras},{{-35,18},Uso::Descanso},{{-31,10},Uso::Jardineras},
        {{-26,21},Uso::Elotes},{{-23,26},Uso::Descanso},{{-17,31},Uso::Jardineras},
        {{-13,33},Uso::Tacos},{{-8,34},Uso::Jardineras},{{7,33},Uso::Descanso},
        {{13,28},Uso::Elotes},{{19,28},Uso::Jardineras},{{26,25},Uso::Descanso},
        {{31,20},Uso::Tacos},{{38,16},Uso::Jardineras},{{36,7},Uso::Descanso},
        {{28,8},Uso::Jardineras},{{-24,5},Uso::Descanso},{{18,3},Uso::Elotes}
    }};
    for (const auto& sitio : sitios) {
        float best=-1000.0f;
        Rincon rincon{};
        for (int z=-12; z<=12; ++z) {
            for (int x=-12; x<=12; ++x) {
                const glm::vec2 offset=glm::vec2(x,z)*0.48f;
                if (glm::dot(offset,offset)>34.5f) continue;
                const glm::vec2 p=sitio.objetivo+offset;
                const float y=terreno(p);
                if (y<0.58f || enMuelle(p) || distanciaPlaza(p)<1.20f) continue;
                const auto principal=puntoMasCercano(p,callePrincipal);
                const auto oeste=puntoMasCercano(p,callejonOeste);
                const auto este=puntoMasCercano(p,callejonEste);
                const glm::vec3 bordes(glm::distance(p,principal)-2.18f,
                    glm::distance(p,oeste)-1.26f,glm::distance(p,este)-1.26f);
                float radio=std::min({2.15f,bordes.x,bordes.y,bordes.z});
                if (radio<0.85f) continue;
                for (const auto& casa : casas) {
                    const glm::vec2 delta=p-casa.huella.centro;
                    const auto& h=casa.huella;
                    const glm::vec2 local(std::abs(glm::dot(delta,h.lateral)),std::abs(glm::dot(delta,h.frente)));
                    radio=std::min(radio,glm::length(glm::max(glm::vec2(0),local-h.mitad))-0.13f);
                    if (radio<0.85f) break;
                }
                if (radio<0.85f) continue;
                const auto base=punto(p,y);
                for (const auto& r : tramo.rincones) radio=std::min(radio,separacion(r.centro,base)-r.radio-0.36f);
                for (const auto& tree : tramo.arboles) radio=std::min(radio,separacion(tree,base)-0.50f);
                for (const auto& lamp : tramo.faroles) radio=std::min(radio,separacion(lamp.base,base)-0.26f);
                for (const auto& maceta : tramo.macetas) radio=std::min(radio,separacion(maceta,base)-0.28f);
                for (const auto& extremo : tramo.tendederos) {
                    radio=std::min({radio,separacion(extremo[0],base)-0.12f,separacion(extremo[1],base)-0.12f});
                }
                for (const auto& b : bosques) {
                    if (glm::length((p-b.centro)/(b.radio+glm::vec2(0.3f)))<1.0f) radio=0;
                }
                if (radio<0.85f) continue;
                const float score=1.37f*radio-0.23f*glm::length(offset);
                if (score<=best) continue;
                const glm::vec2 lado=bordes.y<bordes.z ? oeste : este;
                const glm::vec2 acceso=bordes.x<std::min(bordes.y,bordes.z) ? principal : lado;
                rincon={base,glm::normalize(acceso-p),radio,
                    sitio.uso};
                best=score;
            }
        }
        if (best>-1000.0f) tramo.rincones.push_back(rincon);
    }

    const auto sumarFigura = [&](glm::vec2 objetivo,glm::vec2 frente,float radio,Uso uso,bool enPlaza) {
        glm::vec2 centro{};
        float mejor=25.0f;
        for (int z=-12; z<=12; ++z) {
            for (int x=-12; x<=12; ++x) {
                const glm::vec2 delta=glm::vec2(x,z)*0.28f;
                const glm::vec2 p=objetivo+delta;
                const float distancia=glm::dot(delta,delta);
                if (distancia>=mejor || terreno(p)<0.5f || ocupaPlaza(p,radio+0.15f) || pieDeArco(p,radio)
                    || (enPlaza && std::abs(p.x)<5.1f+radio && p.y< -2.6f)) continue;
                const auto cercaCalle = [p,radio](const auto& calle,float ancho) {
                    return glm::distance(p,puntoMasCercano(p,calle))<ancho+radio;
                };
                if (cercaCalle(callePrincipal,1.40f) || cercaCalle(callejonOeste,0.85f)
                    || cercaCalle(callejonEste,0.85f)) continue;
                const auto base=punto(p,terreno(p));
                if (std::any_of(casas.begin(),casas.end(),[p,radio](const Vivienda& casa) {
                    return contiene(casa.huella,p,radio+0.15f);
                })) continue;
                if (!despejado(base,radio+0.15f) || std::any_of(tramo.rincones.begin(),tramo.rincones.end(),[&](const Rincon& r) {
                    return separacion(base,r.centro)<radio+r.radio+0.14f;
                })) continue;
                if (std::any_of(tramo.arboles.begin(),tramo.arboles.end(),[&](glm::vec3 arbol) {
                    return separacion(base,arbol)<radio+0.5f;
                })) continue;
                centro=p;
                mejor=distancia;
            }
        }
        if (mejor>=25.0f) return false;
        tramo.rincones.push_back({punto(centro,terreno(centro)),frente,radio,uso});
        return true;
    };
    sumarFigura({-5.75f,3.0f},{0.87f,0.50f},1.10f,Uso::Tacos,true);
    if (!sumarFigura({5.90f,3.40f},{-0.87f,0.50f},0.90f,Uso::Elotes,true)) {
        sumarFigura({-5.1f,5.45f},{0.87f,0.50f},0.90f,Uso::Elotes,true);
    }
    sumarFigura({-4.30f,-1.53f},{0,1},0.56f,Uso::Catrina,true);
    sumarFigura({4.28f,-1.43f},{0,1},0.56f,Uso::Catrina,true);
    sumarFigura({-13.4f,24.5f},{0,1},0.53f,Uso::Catrina,false);
    sumarFigura({21.5f,15.7f},{0,1},0.53f,Uso::Catrina,false);
    return tramo;
}

void anillo(Mesh& mesh,glm::vec3 base,float y1,float r1,float y2,float r2,glm::vec3 color,int lados=12,float material=0)
{
    for (int i=0; i<lados; ++i) {
        const float a=glm::two_pi<float>()*i/lados, b=glm::two_pi<float>()*(i+1)/lados;
        const glm::vec3 u(std::cos(a),0,std::sin(a)), v(std::cos(b),0,std::sin(b));
        const float shade=0.77f+0.20f*std::cos((a+b)*0.5f-2.0f);
        quad(mesh,base+u*r1+glm::vec3(0,y1,0),base+v*r1+glm::vec3(0,y1,0),
            base+v*r2+glm::vec3(0,y2,0),base+u*r2+glm::vec3(0,y2,0),color*shade,material);
    }
}

void foco(Mesh& mesh,glm::vec3 centro,float radio,float altura,glm::vec3 color)
{
    const glm::vec3 arriba=centro+glm::vec3(0,altura,0), abajo=centro-glm::vec3(0,altura,0);
    for (int i=0; i<6; ++i) {
        const float a=glm::two_pi<float>()*i/6, b=glm::two_pi<float>()*(i+1)/6;
        const glm::vec3 p=centro+radio*glm::vec3(std::cos(a),0,std::sin(a));
        const glm::vec3 q=centro+radio*glm::vec3(std::cos(b),0,std::sin(b));
        triangle(mesh,arriba,p,q,color,10.0f);
        triangle(mesh,abajo,q,p,color*0.84f,10.0f);
    }
}

void dibujarLuzRibera(Mesh& mesh,glm::vec3 centro)
{
    const glm::vec3 metal(0.23f,0.20f,0.15f);
    anillo(mesh,centro,-0.78f,0.078f,-0.16f,0.056f,metal,6);
    anillo(mesh,centro,-0.16f,0.058f,-0.11f,0.12f,metal,6);
    anillo(mesh,centro,0.09f,0.13f,0.18f,0.025f,metal,6);
    foco(mesh,centro,0.115f,0.135f,{1.0f,0.63f,0.22f});
}

void dibujarLuzFachada(Mesh& mesh,const LuzFachada& luz)
{
    const glm::vec3 o=luz.centro;
    const auto pared=o-0.22f*punto(luz.frente,0);
    const glm::vec3 metal(0.21f,0.17f,0.12f);
    const glm::vec3 side(luz.frente.y,0,-luz.frente.x), alto(0,0.12f,0);
    quad(mesh,pared+alto-side*0.016f,pared+alto+side*0.016f,o+alto+side*0.016f,o+alto-side*0.016f,metal);
    anillo(mesh,o,0.067f,0.10f,0.15f,0.018f,metal,6);
    foco(mesh,o,0.074f,0.088f,{1.0f,0.61f,0.25f});
}

void dibujarFarol(Mesh& mesh,const Farol& f)
{
    const auto base=f.base;
    const float h=f.altura;
    const glm::vec3 hierro(0.17f,0.19f,0.18f), laton(0.64f,0.43f,0.22f), vidrio(1.0f,0.68f,0.29f);
    anillo(mesh,base,-0.15f,0.16f,0.13f,0.14f,hierro,8);
    anillo(mesh,base,0.13f,0.14f,0.26f,0.07f,hierro,8);
    anillo(mesh,base,0.20f,0.065f,h-0.58f,0.043f,hierro,8);
    for (float y : {0.20f,0.38f,h-0.70f}) {
        anillo(mesh,base,y,0.083f,y+0.035f,0.083f,laton,8);
    }
    anillo(mesh,base,h-0.64f,0.07f,h-0.53f,0.18f,hierro,6);
    anillo(mesh,base,h-0.53f,0.18f,h-0.49f,0.18f,laton,6);
    anillo(mesh,base,h-0.15f,0.246f,h-0.105f,0.246f,laton,6);
    anillo(mesh,base,h-0.105f,0.246f,h+0.015f,0.070f,hierro,6);
    anillo(mesh,base,h+0.015f,0.07f,h+0.13f,0.012f,laton,6);
    for (int i=0; i<6; ++i) {
        const float a=glm::two_pi<float>()*i/6, b=glm::two_pi<float>()*(i+1)/6;
        const glm::vec3 u(std::cos(a),0,std::sin(a)), v(std::cos(b),0,std::sin(b));
        const glm::vec3 bottom(0,h-0.50f,0), top(0,h-0.14f,0);
        const auto A=base+bottom+u*0.176f, B=base+bottom+v*0.176f;
        const auto C=base+top+v*0.23f, D=base+top+u*0.23f;
        quad(mesh,A,B,C,D,vidrio*(0.87f+0.10f*std::sin(a)),9.0f);
        quad(mesh,A+u*0.007f,glm::mix(A,B,0.08f)+u*0.007f,glm::mix(D,C,0.08f)+u*0.007f,D+u*0.007f,hierro);
        quad(mesh,glm::mix(B,A,0.08f)+v*0.007f,B+v*0.007f,C+v*0.007f,glm::mix(C,D,0.08f)+v*0.007f,hierro);
    }
}

void dibujarMaceta(Mesh& mesh,glm::vec3 base,int id)
{
    const float size=0.82f+0.28f*semilla(id,80);
    const glm::vec3 barro=glm::mix(glm::vec3(0.62f,0.27f,0.12f),glm::vec3(0.80f,0.44f,0.24f),semilla(id,37));
    anillo(mesh,base,-0.07f,0.15f*size,0.29f*size,0.229f*size,barro);
    anillo(mesh,base,0.26f*size,0.238f*size,0.32f*size,0.238f*size,barro*1.12f);
    anillo(mesh,base,0.32f*size,0.238f*size,0.32f*size,0.204f*size,barro);
    anillo(mesh,base,0.32f*size,0.204f*size,0.27f*size,0.197f*size,barro*0.55f);
    anillo(mesh,base,0.272f*size,0.197f*size,0.272f*size,0.0f,{0.20f,0.12f,0.065f});
    for (int flower=0; flower<7; ++flower) {
        const float angle=2.39996f*flower+id*0.43f;
        const glm::vec3 direction(std::cos(angle),0,std::sin(angle));
        const glm::vec3 side(-direction.z,0,direction.x);
        const glm::vec3 root=base+glm::vec3(0,0.28f*size,0)+direction*(0.105f*size);
        const float height=(0.19f+0.23f*semilla(flower,id))*size;
        const glm::vec3 crown=root+glm::vec3(0,height,0)+direction*(flower==0 ? 0.0f : 0.10f*size);
        quad(mesh,root-side*0.013f,root+side*0.013f,crown+side*0.009f,crown-side*0.009f,{0.29f,0.42f,0.16f});
        for (float sign : {-1.0f,1.0f}) {
            const auto leaf=glm::mix(root,crown,sign<0 ? 0.36f : 0.58f);
            const auto tip=leaf+side*(sign*0.115f*size)+glm::vec3(0,0.062f*size,0);
            quad(mesh,leaf,glm::mix(leaf,tip,0.5f)+direction*0.035f,tip,
                glm::mix(leaf,tip,0.5f)-direction*0.035f,{0.30f,0.47f,0.15f});
        }
        const float radius=(0.081f+0.033f*semilla(flower+70,id))*size;
        const glm::vec3 bloom=glm::mix(glm::vec3(0.98f,0.29f,0.035f),glm::vec3(1.0f,0.63f,0.075f),semilla(flower+71,id));
        constexpr std::array<int,5> petalos{{12,11,10,8,6}};
        for (size_t layer=0; layer<petalos.size(); ++layer) {
            const int petals=petalos[layer];
            const float nivel=static_cast<float>(layer);
            const float ring=radius*(1.0f-0.16f*nivel);
            const glm::vec3 center=crown+glm::vec3(0,0.017f*nivel,0);
            for (int petal=0; petal<petals; ++petal) {
                const float theta=glm::two_pi<float>()*petal/petals+nivel*0.43f;
                const glm::vec3 out(std::cos(theta),0,std::sin(theta));
                const glm::vec3 wing(-out.z,0,out.x);
                const auto foot=center+out*(ring*0.13f);
                const auto end=center+out*ring+glm::vec3(0,ring*(0.25f+0.10f*nivel),0);
                const auto shoulders=glm::mix(foot,end,0.71f);
                const glm::vec3 tint=bloom*(0.80f+0.19f*semilla(petal+static_cast<int>(layer)*12,flower+id));
                quad(mesh,foot,shoulders+wing*(ring*0.33f),end,
                    shoulders-wing*(ring*0.33f),tint,8.0f);
            }
        }
    }
    for (int i=0; i<10; ++i) {
        const float angle=glm::two_pi<float>()*semilla(i,id+43);
        const glm::vec2 eje(std::cos(angle),std::sin(angle)), lateral(-eje.y,eje.x);
        const glm::vec2 p=glm::vec2(base.x,base.z)+eje*(0.21f+0.24f*semilla(id,i+61));
        if (alturaCaminata(p)<0.08f) continue;
        const auto sobrePiso = [](glm::vec2 position) { return punto(position,alturaCaminata(position)+0.028f); };
        quad(mesh,sobrePiso(p-eje*0.029f),sobrePiso(p+lateral*0.014f),
            sobrePiso(p+eje*0.032f),sobrePiso(p-lateral*0.014f),{0.90f,0.36f,0.06f},8.0f);
    }
}

void dibujarPapel(Mesh& mesh,const std::array<glm::vec3,2>& anclas,int id,float alto=3.40f)
{
    const std::array<glm::vec3,6> colores{{{0.94f,0.26f,0.48f},{0.99f,0.62f,0.13f},{0.57f,0.33f,0.78f},
        {0.13f,0.74f,0.66f},{0.94f,0.38f,0.17f},{0.39f,0.52f,0.92f}}};
    for (const auto& p : anclas) {
        anillo(mesh,p,-0.13f,0.065f,alto+0.03f,0.036f,{0.32f,0.23f,0.16f},8);
    }
    const glm::vec3 diff=anclas[1]-anclas[0];
    const glm::vec3 breeze=glm::normalize(glm::vec3(-diff.z,0,diff.x));
    const auto cordel = [&](float t) {
        return glm::mix(anclas[0],anclas[1],t)+glm::vec3(0,alto-1.22f*t*(1.0f-t),0);
    };
    constexpr int tramos=20;
    for (int i=0; i<tramos; ++i) {
        const auto a=cordel(static_cast<float>(i)/tramos), b=cordel(static_cast<float>(i+1)/tramos);
        const glm::vec3 thickness(0,0.012f,0);
        quad(mesh,a-thickness,b-thickness,b+thickness,a+thickness,{0.39f,0.27f,0.17f});
        quad(mesh,a-breeze*0.010f,b-breeze*0.010f,b+breeze*0.010f,a+breeze*0.010f,{0.39f,0.27f,0.17f});
    }
    const auto cableLuces = [&](float t) {
        return cordel(t)+breeze*(0.18f*std::sin(glm::pi<float>()*t))+glm::vec3(0,0.10f,0);
    };
    for (int i=0; i<16; ++i) {
        const float t=static_cast<float>(i)/16, next=static_cast<float>(i+1)/16;
        const auto a=cableLuces(t), b=cableLuces(next);
        quad(mesh,a,b,b-glm::vec3(0,0.013f,0),a-glm::vec3(0,0.013f,0),{0.27f,0.21f,0.16f});
        if (i>0) {
            const glm::vec3 color=i%4==0 ? glm::vec3(1,0.44f,0.21f) : glm::vec3(1,0.76f,0.40f);
            foco(mesh,a-glm::vec3(0,0.043f,0),0.036f,0.048f,color);
        }
    }
    const int flags=static_cast<int>(glm::length(glm::vec2(diff.x,diff.z))/0.51f);
    for (int flag=0; flag<flags; ++flag) {
        const float start=(flag+0.15f)/flags, end=(flag+0.86f)/flags;
        const float height=0.34f+0.10f*semilla(flag,id);
        const glm::vec3 color=colores[(flag+2*id)%colores.size()];
        const auto vertice = [&](float u,float v) {
            const float fold=0.041f*std::sin(7.2f*u+flag*1.6f+id)*v;
            const auto point=cordel(glm::mix(start,end,u))+breeze*fold-glm::vec3(0,height*v,0);
            return Vertex{point,color*(0.92f+0.08f*std::cos(7.2f*u+flag)),flag%2==0 ? 6.0f : 7.0f,0,{u,v}};
        };
        for (int i=0; i<4; ++i) {
            const float u=static_cast<float>(i)/4, next=static_cast<float>(i+1)/4;
            const auto a=vertice(u,0), b=vertice(next,0), c=vertice(next,1), d=vertice(u,1);
            mesh.insert(mesh.end(),{a,b,c,a,c,d});
        }
    }
}

void florOfrenda(Mesh& mesh,glm::vec3 p,float radio,glm::vec3 color,int seed,bool vertical);
void calaverita(Mesh& mesh,glm::vec3 base,float alto,float giro,int id);

void varilla(Mesh& mesh,glm::vec3 a,glm::vec3 b,float grosor,glm::vec3 color)
{
    const auto eje=glm::normalize(b-a);
    const auto guia=std::abs(eje.y)>0.85f ? glm::vec3(1,0,0) : glm::vec3(0,1,0);
    const auto u=glm::normalize(glm::cross(eje,guia))*grosor;
    const auto v=glm::normalize(glm::cross(eje,u))*grosor;
    const std::array<glm::vec3,4> caras{{u+v,-u+v,-u-v,u-v}};
    for (size_t i=0; i<caras.size(); ++i) {
        const auto p=caras[i], q=caras[(i+1)%caras.size()];
        quad(mesh,a+p,a+q,b+q,b+p,color*(1.0f-0.12f*i));
    }
}

void ruedaCarrito(Mesh& mesh,glm::vec3 centro,float radio)
{
    const glm::vec3 goma(0.12f,0.13f,0.15f), acero(0.75f,0.77f,0.78f);
    const auto aro=[&](int i,float escala) {
        const float a=glm::two_pi<float>()*i/16.0f;
        return centro+radio*escala*glm::vec3(0,std::cos(a),std::sin(a));
    };
    for (int i=0; i<16; ++i) {
        varilla(mesh,aro(i,0.98f),aro(i+1,0.98f),0.030f,goma);
        varilla(mesh,aro(i,0.85f),aro(i+1,0.85f),0.010f,acero);
        if (i%2==0) varilla(mesh,centro,aro(i,0.85f),0.006f,acero);
    }
    box(mesh,centro-glm::vec3(0.065f,0.035f,0.035f),centro+glm::vec3(0.065f,0.035f,0.035f),acero);
}

void letreroComida(Mesh& mesh,bool tacos,float y,float z,glm::vec3 color)
{
    const auto glifo=[](char letra) -> std::array<int,5> {
        switch (letra) {
        case 'T': return {7,2,2,2,2};
        case 'A': return {2,5,7,5,5};
        case 'C': return {7,4,4,4,7};
        case 'O': return {7,5,5,5,7};
        case 'S': return {7,4,7,1,7};
        case 'E': return {7,4,6,4,7};
        default:  return {4,4,4,4,7};
        }
    };
    const char* letras=tacos ? "TACOS" : "ELOTES";
    const int cantidad=tacos ? 5 : 6;
    constexpr float pixel=0.034f;
    const float inicio=-0.5f*(cantidad*4-1)*pixel;
    box(mesh,{inicio-0.085f,y-0.046f,z-0.036f},{-inicio+0.085f,y+0.222f,z},color*0.58f);
    for (int i=0; i<cantidad; ++i) {
        const auto filas=glifo(letras[i]);
        for (int fila=0; fila<5; ++fila) {
            for (int col=0; col<3; ++col) {
                if ((filas[fila] & (4>>col))==0) continue;
                const float x=inicio+(4*i+col)*pixel, h=y+(4-fila)*pixel;
                quad(mesh,{x,h,z+0.002f},{x+pixel,h,z+0.002f},
                    {x+pixel,h+pixel,z+0.002f},{x,h+pixel,z+0.002f},{1.0f,0.93f,0.72f},8.0f);
            }
        }
    }
}

void dibujarCarrito(Mesh& mesh,bool tacos,int id)
{
    const glm::vec3 color=tacos ? glm::vec3(0.76f,0.18f,0.16f) : glm::vec3(0.07f,0.58f,0.50f);
    const glm::vec3 acero(0.66f,0.70f,0.70f), crema(0.99f,0.88f,0.67f), negro(0.19f,0.19f,0.19f);
    const float fondo=tacos ? -0.44f : -0.22f;
    const float borde=tacos ? 0.76f : 0.53f;
    const float mostrador=tacos ? 0.97f : 0.83f;
    for (float lado : {-1.0f,1.0f}) ruedaCarrito(mesh,{lado*(borde+0.055f),0.31f,0.12f},0.29f);
    varilla(mesh,{-borde-0.09f,0.31f,0.12f},{borde+0.09f,0.31f,0.12f},0.032f,acero);
    box(mesh,{-borde,0.37f,fondo},{borde,mostrador-0.055f,0.43f},color);
    box(mesh,{-borde-0.055f,mostrador-0.055f,fondo-0.045f},{borde+0.055f,mostrador,0.48f},acero);
    box(mesh,{-borde,0.47f,0.43f},{borde,0.52f,0.444f},crema);
    letreroComida(mesh,tacos,tacos ? 0.60f : 0.56f,0.461f,color);
    if (tacos) {
        for (float x : {-0.71f,0.71f}) {
            box(mesh,{x-0.026f,0.02f,-0.37f},{x+0.026f,2.10f,-0.31f},acero);
            varilla(mesh,{x,0.91f,0.35f},{x,1.94f,0.35f},0.022f,acero);
        }
        for (int i=0; i<9; ++i) {
            const float a=-0.85f+i*0.19f, b=a+0.19f;
            const auto tela=i%2==0 ? color : crema;
            quad(mesh,{a,2.17f,-0.53f},{b,2.17f,-0.53f},{b,2.0f,0.58f},{a,2.0f,0.58f},tela);
            triangle(mesh,{a,2.0f,0.58f},{b,2.0f,0.58f},{(a+b)*0.5f,1.85f,0.60f},tela);
        }
        box(mesh,{-0.70f,mostrador,-0.25f},{-0.05f,mostrador+0.072f,0.28f},negro);
        for (int i=0; i<4; ++i) {
            const glm::vec3 taco(-0.60f+i*0.147f,mostrador+0.094f,0.13f);
            for (float lado : {-1.0f,1.0f}) {
                quad(mesh,taco+glm::vec3(-0.065f,0.060f,lado*0.052f),taco+glm::vec3(0.065f,0.060f,lado*0.052f),
                    taco+glm::vec3(0.055f,-0.006f,lado*0.018f),taco+glm::vec3(-0.055f,-0.006f,lado*0.018f),crema);
            }
            volumen(mesh,taco+glm::vec3(0,0.039f,0),{0.057f,0.031f,0.035f},{0.46f,0.36f,0.14f});
        }
        const glm::vec3 trompo(0.41f,mostrador, -0.09f);
        anillo(mesh,trompo,0.10f,0.085f,0.32f,0.145f,{0.74f,0.33f,0.15f},10);
        anillo(mesh,trompo,0.32f,0.145f,0.43f,0.112f,{0.63f,0.24f,0.12f},10);
        anillo(mesh,trompo,0.43f,0.105f,0.48f,0.082f,{0.96f,0.72f,0.20f},10);
        varilla(mesh,trompo,trompo+glm::vec3(0,0.55f,0),0.012f,acero);
    } else {
        ruedaCarrito(mesh,{0,0.265f,-0.89f},0.245f);
        varilla(mesh,{0,0.27f,-0.89f},{0,0.39f,-0.15f},0.036f,color);
        varilla(mesh,{0,0.27f,-0.89f},{0,0.70f,-0.63f},0.028f,color);
        varilla(mesh,{0,0.39f,-0.15f},{0,0.70f,-0.63f},0.034f,color);
        box(mesh,{-0.11f,0.70f,-0.77f},{0.11f,0.765f,-0.54f},negro);
        varilla(mesh,{0,0.51f,-0.31f},{0,0.94f,-0.32f},0.019f,acero);
        varilla(mesh,{-0.24f,0.94f,-0.32f},{0.24f,0.94f,-0.32f},0.021f,negro);
        const glm::vec3 olla(-0.24f,mostrador,0.14f);
        anillo(mesh,olla,0,0.135f,0.235f,0.15f,acero);
        anillo(mesh,olla,0.224f,0.166f,0.255f,0.152f,acero*1.15f);
        anillo(mesh,olla,0.255f,0.145f,0.275f,0.0f,acero);
        for (int i=0; i<5; ++i) {
            const glm::vec3 elote(0.052f+0.091f*i,mostrador+0.069f,0.24f-0.035f*(i%2));
            volumen(mesh,elote,{0.040f,0.039f,0.115f},{0.99f,0.75f,0.25f},0.21f*i);
            volumen(mesh,elote-glm::vec3(0,0.016f,0.049f),{0.043f,0.027f,0.087f},{0.48f,0.64f,0.25f});
        }
        const glm::vec3 sombrilla(0,1.98f,-0.06f);
        varilla(mesh,{0,mostrador,-0.06f},sombrilla,0.027f,acero);
        for (int i=0; i<10; ++i) {
            const float a=glm::two_pi<float>()*i/10.0f, b=glm::two_pi<float>()*(i+1)/10.0f;
            const glm::vec3 p(0.69f*std::cos(a),-0.24f,0.65f*std::sin(a));
            const glm::vec3 q(0.69f*std::cos(b),-0.24f,0.65f*std::sin(b));
            triangle(mesh,sombrilla,sombrilla+p,sombrilla+q,i%2 ? crema : color);
        }
    }
    for (int i=0; i<3; ++i) {
        const glm::vec3 p(-0.31f+0.31f*i,tacos ? 1.97f : 1.70f,0.36f);
        foco(mesh,p,0.033f,0.046f,{1.0f,0.73f+0.06f*semilla(id,i),0.32f});
    }
}

void dibujarCatrina(Mesh& mesh,int id)
{
    const glm::vec3 hueso(0.93f,0.87f,0.71f), negro(0.24f,0.11f,0.22f), oro(0.98f,0.62f,0.18f);
    const std::array<glm::vec3,3> vestidos{{{0.66f,0.17f,0.36f},{0.21f,0.45f,0.54f},{0.48f,0.25f,0.61f}}};
    const auto vestido=vestidos[id%vestidos.size()];
    const glm::vec3 cero(0);
    anillo(mesh,cero,0.022f,0.43f,0.88f,0.145f,vestido,18);
    for (float y : {0.06f,0.19f,0.32f,0.71f}) {
        const float r=glm::mix(0.43f,0.145f,(y-0.022f)/0.858f);
        anillo(mesh,cero,y,r+0.015f,y+0.040f,r-0.001f,y>0.6f ? negro : oro,18,8.0f);
    }
    anillo(mesh,cero,0.86f,0.143f,1.20f,0.227f,vestido,14);
    volumen(mesh,{0,1.19f,0},{0.23f,0.11f,0.134f},vestido);
    varilla(mesh,{0,1.21f,0},{0,1.39f,0},0.055f,hueso);
    calaverita(mesh,{0,1.30f,0},0.31f,0,id);
    for (float lado : {-1.0f,1.0f}) {
        const glm::vec3 hombro(lado*0.225f,1.19f,0), codo(lado*0.34f,0.98f,0.06f);
        const glm::vec3 mano(lado*0.16f,1.04f,0.26f);
        volumen(mesh,hombro,{0.11f,0.105f,0.11f},vestido);
        varilla(mesh,hombro,codo,0.035f,hueso);
        varilla(mesh,codo,mano,0.027f,hueso);
        volumen(mesh,mano,{0.042f,0.046f,0.035f},hueso);
    }
    const glm::vec3 sombrero(0,1.57f,0);
    anillo(mesh,sombrero,-0.035f,0.12f,-0.006f,0.43f,negro,20);
    anillo(mesh,sombrero,-0.006f,0.43f,0.035f,0.43f,oro,20);
    anillo(mesh,sombrero,0.035f,0.43f,0.037f,0.01f,negro,20);
    anillo(mesh,sombrero,0.03f,0.235f,0.183f,0.195f,negro,16);
    anillo(mesh,sombrero,0.183f,0.195f,0.183f,0.0f,negro,16);
    for (int i=0; i<5; ++i) {
        florOfrenda(mesh,{(i-2)*0.077f,1.67f+0.046f*semilla(id,i),0.21f},
            0.080f,i%2==0 ? oro : vestido,id+i,true);
    }
    for (int i=0; i<5; ++i) {
        florOfrenda(mesh,{0.09f*(i-2),1.08f+0.060f*semilla(id,i+1),0.27f},0.077f,oro,id+i,true);
    }
}

void dibujarRincon(Mesh& mesh,const Rincon& rincon,int id)
{
    const float escala=rincon.uso==UsoRincon::Catrina ? 1.13f
        : esPuesto(rincon.uso) ? std::min(1.0f,rincon.radio/1.20f)
        : glm::clamp(rincon.radio/1.56f,0.58f,1.16f);
    const glm::vec2 lateral(rincon.frente.y,-rincon.frente.x);
    const auto mundo = [&](glm::vec3 local) {
        return rincon.centro+escala*punto(local.x*lateral+local.z*rincon.frente,local.y);
    };
    const auto inicio=mesh.size();
    const glm::vec3 madera(0.56f,0.33f,0.18f), hierro(0.21f,0.21f,0.17f);
    if (esPuesto(rincon.uso)) {
        dibujarCarrito(mesh,rincon.uso==UsoRincon::Tacos,id);
    } else if (rincon.uso==UsoRincon::Catrina) {
        dibujarCatrina(mesh,id);
    } else if (rincon.uso==UsoRincon::Descanso) {
        for (float x : {-0.63f,0.63f}) {
            box(mesh,{x-0.055f,-0.32f,-0.73f},{x+0.055f,0.43f,-0.30f},hierro);
            box(mesh,{x-0.043f,0.36f,-0.73f},{x+0.043f,0.86f,-0.65f},hierro);
        }
        for (int i=0; i<4; ++i) {
            const float z=-0.71f+i*0.103f;
            box(mesh,{-0.81f,0.38f,z},{0.81f,0.44f,z+0.089f},madera,11.0f);
        }
        for (float y : {0.52f,0.66f,0.80f}) {
            box(mesh,{-0.81f,y,-0.728f},{0.81f,y+0.096f,-0.678f},madera,11.0f);
        }
    } else {
        const glm::vec3 piedra(0.49f,0.44f,0.36f);
        anillo(mesh,{0,0,-0.20f},-0.22f,0.71f,0.24f,0.73f,piedra,12);
        anillo(mesh,{0,0,-0.20f},0.23f,0.76f,0.32f,0.76f,piedra*1.10f,12);
        anillo(mesh,{0,0,-0.20f},0.32f,0.76f,0.32f,0.64f,piedra*0.92f,12);
        anillo(mesh,{0,0,-0.20f},0.272f,0.66f,0.272f,0.0f,{0.29f,0.20f,0.12f},12);
        for (int i=0; i<9; ++i) {
            const float a=glm::two_pi<float>()*i/9;
            const glm::vec3 o(0.48f*std::cos(a),0.42f,-0.20f+0.48f*std::sin(a));
            volumen(mesh,o,{0.18f,0.15f,0.18f},{0.27f,0.39f,0.16f},a);
            volumen(mesh,o+glm::vec3(0,0.12f,0),{0.086f,0.069f,0.083f},{0.94f,0.47f,0.11f},a);
        }
    }
    for (size_t i=inicio; i<mesh.size(); ++i) mesh[i].position=mundo(mesh[i].position);
    if (!esPuesto(rincon.uso) && rincon.uso!=UsoRincon::Catrina) {
        for (float lado : {-1.0f,1.0f}) {
            auto base=mundo({lado*0.75f,0,0.45f});
            base.y=alturaCaminata({base.x,base.z});
            dibujarMaceta(mesh,base,id+(lado<0 ? 810 : 990));
        }
    }
}

struct NivelOfrenda { float ancho,frente,altura; glm::vec3 tela; };
const std::array<NivelOfrenda,4> nivelesOfrenda{{
    {4.64f,3.24f,0.32f,{0.43f,0.17f,0.40f}},
    {3.86f,2.13f,0.69f,{0.54f,0.18f,0.24f}},
    {3.08f,1.08f,1.06f,{0.44f,0.16f,0.39f}},
    {2.39f,0.01f,1.43f,{0.61f,0.25f,0.27f}}
}};
constexpr float fondoOfrenda=-2.84f;

struct VeladoraOfrenda { glm::vec3 base; float altura; bool vaso; };

std::vector<ConjuntoOfrenda> distribuirConjuntos(const std::vector<Vivienda>& casas,const DecoracionIsla& tramo)
{
    std::vector<ConjuntoOfrenda> conjuntos(conjuntosPlaza.begin(),conjuntosPlaza.end());
    const auto libre = [&](glm::vec2 p,float radio) {
        if (enMuelle(p) || terreno(p)<0.35f || distanciaPlaza(p)<1.12f
            || glm::distance(p,puntoMasCercano(p,callePrincipal))<2.24f+radio
            || glm::distance(p,puntoMasCercano(p,callejonOeste))<1.33f+radio
            || glm::distance(p,puntoMasCercano(p,callejonEste))<1.33f+radio) return false;
        const auto cerca = [p](glm::vec3 q,float margen) {
            return glm::distance(p,glm::vec2(q.x,q.z))<margen;
        };
        for (const auto& casa : casas) if (contiene(casa.huella,p,radio+0.11f)) return false;
        for (const auto& farol : tramo.faroles) if (cerca(farol.base,radio+0.18f)) return false;
        for (const auto& maceta : tramo.macetas) if (cerca(maceta,radio+0.22f)) return false;
        for (const auto& r : tramo.rincones) if (cerca(r.centro,r.radio*0.73f+radio)) return false;
        for (const auto& arbol : tramo.arboles) if (cerca(arbol,radio+0.23f)) return false;
        for (const auto& cuerda : tramo.tendederos) {
            if (cerca(cuerda[0],radio+0.095f) || cerca(cuerda[1],radio+0.095f)) return false;
        }
        for (const auto& c : conjuntos) {
            if (glm::distance(p,c.centro)<c.radio+radio+0.24f) return false;
        }
        return true;
    };
    const auto rodear = [&](glm::vec2 base,float alcance,int seed) {
        const float fase=glm::two_pi<float>()*semilla(seed,61);
        for (float extra : {0.0f,0.34f,0.68f,1.03f}) {
            for (int i=0; i<12; ++i) {
                const float a=fase+glm::two_pi<float>()*i/12;
                const auto p=base+(alcance+extra)*glm::vec2(std::cos(a),std::sin(a));
                const float radio=0.32f+0.18f*semilla(seed,i);
                if (!libre(p,radio)) continue;
                const auto figura=seed%3==0 ? Figura::Rombo : Figura::Circulo;
                conjuntos.push_back({p,radio,figura});
                return;
            }
        }
    };
    for (size_t i=0; i<tramo.rincones.size(); ++i) {
        const auto& r=tramo.rincones[i];
        if (enMuelle({r.centro.x,r.centro.z})) continue;
        for (int lado=0; lado<2; ++lado) rodear({r.centro.x,r.centro.z},r.radio*0.73f+0.52f,
            19+static_cast<int>(i)*2+lado);
    }
    for (size_t i=0; i<tramo.macetas.size(); i+=2) {
        const auto& m=tramo.macetas[i];
        if (!enMuelle({m.x,m.z})) rodear({m.x,m.z},0.67f,78+static_cast<int>(i));
    }
    return conjuntos;
}

glm::vec2 contornoOfrenda(const ConjuntoOfrenda& c,float avance,float escala=1.0f)
{
    const float a=glm::two_pi<float>()*avance;
    const glm::vec2 direccion(std::cos(a),std::sin(a));
    float factor=1.0f;
    if (c.figura==Figura::Flor) factor=0.82f+0.18f*std::cos(6.0f*a);
    if (c.figura==Figura::Rombo) factor=1.0f/(std::abs(direccion.x)+std::abs(direccion.y));
    if (c.figura==Figura::Cruz) {
        const float x=std::abs(direccion.x), z=std::abs(direccion.y);
        factor=std::min(1.0f/std::max(x,z),0.29f/std::max(0.001f,std::min(x,z)))*0.94f;
    }
    return c.centro+direccion*(c.radio*factor*escala);
}

std::array<std::array<glm::vec3,2>,3> papelPlaza()
{
    const auto base = [](glm::vec2 p) { return punto(p,terreno(p)); };
    return {{{base({-7.15f,0.12f}),base({7.13f,0.12f})},
        {base({-7.63f,-9.25f}),base({-5.17f,-1.97f})},
        {base({7.60f,-9.27f}),base({5.21f,-1.98f})}}};
}

std::vector<VeladoraOfrenda> colocarVeladoras(const std::vector<ConjuntoOfrenda>& conjuntos)
{
    const glm::vec3 centro=punto(ofrenda,terreno(ofrenda));
    std::vector<VeladoraOfrenda> veladoras;
    const auto agregar = [&](glm::vec3 local,bool vaso) {
        const int id=static_cast<int>(veladoras.size());
        const float altura=vaso ? 0.154f+0.051f*semilla(id,39) : 0.44f+0.29f*semilla(id,39);
        veladoras.push_back({centro+local,altura,vaso});
    };
    for (const auto& nivel : nivelesOfrenda) {
        const float limite=nivel.ancho-0.19f;
        const int n=static_cast<int>(limite*2.0f/0.225f);
        for (int i=0; i<=n; ++i) {
            agregar({glm::mix(-limite,limite,static_cast<float>(i)/n),nivel.altura,nivel.frente-0.16f},true);
        }
        for (float lado : {-1.0f,1.0f}) {
            for (float z=nivel.frente-0.39f; z>fondoOfrenda+0.21f; z-=0.244f) {
                agregar({lado*limite,nivel.altura,z},true);
            }
        }
    }
    for (float lado : {-1.0f,1.0f}) {
        for (int i=0; i<7; ++i) {
            agregar({lado*(1.27f+0.14f*(i%4)),1.43f,-0.79f-0.36f*(i/4)},false);
        }
    }
    const auto enPiso = [&](glm::vec2 p) {
        agregar({p.x-ofrenda.x,terreno(p)-centro.y,p.y-ofrenda.y},true);
    };
    for (const auto& c : conjuntos) {
        const bool grande=c.radio>0.60f;
        const int filas=grande ? (c.figura==Figura::Circulo ? 3 : 2) : 1;
        for (int fila=0; fila<filas; ++fila) {
            const float escala=0.93f-0.23f*fila;
            const int n=grande ? static_cast<int>(c.radio*25*escala) : 6;
            for (int i=0; i<n; ++i) {
                enPiso(contornoOfrenda(c,(i+0.37f*fila)/n,escala));
            }
        }
    }
    for (float lado : {-1.0f,1.0f}) {
        for (int i=0; i<12; ++i) {
            enPiso(ofrenda+glm::vec2(lado*(0.91f+0.09f*std::sin(i*0.56f)),3.38f+i*0.36f));
        }
    }
    return veladoras;
}

void dibujarVeladora(Mesh& mesh,const VeladoraOfrenda& vela,int id)
{
    const auto p=vela.base;
    const float h=vela.altura;
    const glm::vec3 cera(0.98f,0.91f,0.72f), laton(0.68f,0.43f,0.19f);
    const float radio=vela.vaso ? 0.061f : 0.035f;
    if (vela.vaso) {
        const glm::vec3 vidrio(0.99f,0.61f+0.14f*semilla(id,7),0.23f);
        anillo(mesh,p,0.0f,radio*0.80f,h,radio,vidrio,8,12.0f);
        anillo(mesh,p,h-0.018f,radio*1.06f,h+0.005f,radio*1.06f,cera,8,12.0f);
        anillo(mesh,p,h+0.006f,radio,h+0.006f,0.0f,{0.92f,0.78f,0.42f},8,12.0f);
    } else {
        anillo(mesh,p,0.0f,0.093f,0.059f,0.062f,laton,8);
        anillo(mesh,p,0.06f,radio,h,radio,cera,8);
    }
    const float giro=semilla(id,37)*glm::pi<float>();
    const glm::vec3 lateral(std::cos(giro),0,std::sin(giro));
    const glm::vec3 base=p+glm::vec3(0,h+0.01f,0), centro=base+glm::vec3(0,0.046f,0);
    const glm::vec3 punta=base+lateral*(0.015f*semilla(id,2))+glm::vec3(0,0.11f+0.025f*semilla(id,13),0);
    for (const auto eje : {lateral,glm::vec3(-lateral.z,0,lateral.x)}) {
        quad(mesh,base,centro-eje*0.028f,punta,centro+eje*0.028f,{1.0f,0.72f,0.21f},3.0f);
        triangle(mesh,base-eje*0.012f,base+eje*0.012f,centro+glm::vec3(0,0.024f,0),
            {1.0f,0.95f,0.70f},3.0f);
    }
}

void florOfrenda(Mesh& mesh,glm::vec3 p,float radio,glm::vec3 color,int seed,bool vertical=false)
{
    const glm::vec3 normal=vertical ? glm::vec3(0,0,1) : glm::vec3(0,1,0);
    const glm::vec3 plano=vertical ? glm::vec3(0,1,0) : glm::vec3(0,0,1);
    for (int nivel=0; nivel<3; ++nivel) {
        const int cantidad=11-2*nivel;
        const float r=radio*(1.0f-0.22f*nivel);
        const auto centro=p+normal*(radio*0.14f*nivel);
        for (int i=0; i<cantidad; ++i) {
            const float a=glm::two_pi<float>()*i/cantidad+0.44f*nivel+semilla(seed,22);
            const glm::vec3 eje=glm::vec3(std::cos(a),0,0)+plano*std::sin(a);
            const glm::vec3 lado=glm::vec3(-std::sin(a),0,0)+plano*std::cos(a);
            const auto pie=centro+eje*r*0.12f;
            const auto punta=centro+eje*r+normal*r*(0.19f+0.14f*nivel);
            const auto hombro=glm::mix(pie,punta,0.68f)+normal*r*0.20f;
            const glm::vec3 luz=color*(0.70f+0.28f*semilla(i+nivel*13,seed));
            quad(mesh,pie,hombro+lado*r*0.32f,punta,hombro-lado*r*0.32f,luz,8.0f);
        }
    }
}

void dibujarArcosFestivos(Mesh& mesh)
{
    const glm::vec3 madera(0.44f,0.25f,0.13f), follaje(0.24f,0.36f,0.11f);
    const glm::vec3 naranja(1.0f,0.42f,0.048f), oro(1.0f,0.67f,0.12f);
    for (size_t id=0; id<arcosFestivos.size(); ++id) {
        const auto& arco=arcosFestivos[id];
        const float z=arco.centro.y;
        const float base=alturaCaminata(arco.centro);
        const auto curva=[&](int i) {
            const float a=glm::pi<float>()*i/34.0f;
            return glm::vec3(arco.centro.x+arco.mitad*std::cos(a),base+2.07f+1.25f*std::sin(a),z);
        };
        for (float lado : {-1.0f,1.0f}) {
            const glm::vec2 p=arco.centro+glm::vec2(lado*arco.mitad,0);
            const auto pie=punto(p,alturaCaminata(p));
            anillo(mesh,pie,-0.08f,0.17f,0.25f,0.22f,madera,10,11.0f);
            anillo(mesh,pie,0.23f,0.22f,0.29f,0.225f,oro,10);
            const auto alto=punto(p,base+2.07f);
            varilla(mesh,pie+glm::vec3(0,0.18f,0),alto,0.076f,madera);
            for (int i=0; i<=12; ++i) {
                const auto o=glm::mix(pie+glm::vec3(0,0.30f,0),alto,i/12.0f);
                florOfrenda(mesh,o+glm::vec3(0.05f*std::sin(i*2.0f),0,0.085f),
                    0.135f,i%3==0 ? oro : naranja,i+static_cast<int>(id)*30,true);
                florOfrenda(mesh,o-glm::vec3(0,0,0.10f),0.122f,oro,i+41,true);
            }
        }
        for (int i=0; i<=34; ++i) {
            const auto o=curva(i);
            if (i<34) varilla(mesh,o,curva(i+1),0.091f,follaje);
            for (float lado : {-1.0f,1.0f}) {
                const auto p=o+glm::vec3(0,0.065f*std::sin(i*1.9f),lado*0.095f);
                florOfrenda(mesh,p,0.155f+0.027f*semilla(i,static_cast<int>(id)),
                    i%3==0 ? oro : naranja,i+static_cast<int>(id)*31,true);
            }
            if (i%3==0) foco(mesh,o-glm::vec3(0,0.095f,0),0.037f,0.050f,oro);
        }
    }
}

void decorarTablones(Mesh& mesh)
{
    const glm::vec3 naranja(1.0f,0.49f,0.07f), oro(1.0f,0.67f,0.12f);
    const glm::vec3 madera(0.48f,0.27f,0.14f);
    for (float lado : {-1.0f,1.0f}) {
        for (int i=0; i<44; ++i) {
            const float z=36.55f+i*0.209f;
            const float x=lado*(1.46f+0.11f*std::sin(z*2.0f)+0.12f*semilla(i,91));
            florOfrenda(mesh,punto({x,z},alturaCaminata({x,z})+0.014f),0.075f,i%3==0 ? oro : naranja,i);
        }
        for (float z : {39.02f,41.98f,44.23f,46.10f,49.56f}) {
            const float x=lado*(z<inicioPlataforma ? 1.97f : 1.63f);
            for (int i=0; i<5; ++i) {
                const float a=glm::two_pi<float>()*i/5.0f;
                const glm::vec2 p(x+0.17f*std::cos(a),z+0.17f*std::sin(a));
                dibujarVeladora(mesh,{punto(p,alturaCaminata(p)),0.14f+0.04f*semilla(i,23),true},i+34);
            }
            florOfrenda(mesh,punto({x,z},alturaCaminata({x,z})+0.022f),0.126f,naranja,82);
        }
        for (int i=0; i<13; ++i) {
            const float z=46.23f+i*0.142f;
            const float y=1.51f-0.25f*std::sin(glm::pi<float>()*i/12.0f);
            const glm::vec3 p(lado*5.25f,y,z);
            florOfrenda(mesh,p,0.123f,i%3==0 ? oro : naranja,i+21);
            if (i%3==0) foco(mesh,p-glm::vec3(0,0.05f,0),0.032f,0.039f,oro);
        }
        const glm::vec2 p(lado*4.12f,50.00f);
        const auto o=punto(p,alturaCaminata(p));
        box(mesh,o+glm::vec3(-0.28f,0,-0.25f),o+glm::vec3(0.28f,0.27f,0.25f),madera,11.0f);
        for (int i=0; i<13; ++i) {
            const float a=i*2.40f;
            const auto flor=o+glm::vec3(0.22f*std::cos(a),0.30f+0.14f*semilla(i,92),0.21f*std::sin(a));
            florOfrenda(mesh,flor,0.105f,i%4==0 ? oro : naranja,i+60);
        }
    }
}

void prepararOfrenda(Mesh& mesh,const std::vector<VeladoraOfrenda>& veladoras)
{
    const glm::vec3 o=punto(ofrenda,terreno(ofrenda));
    const glm::vec3 naranja(1.0f,0.43f,0.045f), oro(1.0f,0.69f,0.095f), magenta(0.87f,0.12f,0.43f);
    const auto flores = [&](glm::vec3 p,float r,int seed,bool vertical=false) {
        const float tono=semilla(seed,27);
        florOfrenda(mesh,o+p,r,tono<0.19f ? magenta : glm::mix(naranja,oro,tono),seed,vertical);
    };
    const auto bordado = [&](glm::vec3 a,glm::vec3 b,glm::vec3 c,glm::vec3 d,glm::vec3 color) {
        const Vertex v1{o+a,color,13.0f,0,{0,0}}, v2{o+b,color,13.0f,0,{1,0}};
        const Vertex v3{o+c,color,13.0f,0,{1,1}}, v4{o+d,color,13.0f,0,{0,1}};
        mesh.insert(mesh.end(),{v1,v2,v3,v1,v3,v4});
    };
    for (size_t i=0; i<nivelesOfrenda.size(); ++i) {
        const auto& n=nivelesOfrenda[i];
        const float y=i==0 ? -0.10f : nivelesOfrenda[i-1].altura;
        box(mesh,o+glm::vec3(-n.ancho,y,fondoOfrenda),o+glm::vec3(n.ancho,n.altura,n.frente),n.tela);
        bordado({-n.ancho,n.altura,n.frente+0.006f},{n.ancho,n.altura,n.frente+0.006f},
            {n.ancho,y,n.frente+0.006f},{-n.ancho,y,n.frente+0.006f},n.tela);
        bordado({-0.58f,n.altura+0.006f,n.frente-0.30f},{0.58f,n.altura+0.006f,n.frente-0.30f},
            {0.58f,n.altura+0.006f,fondoOfrenda+0.10f},{-0.58f,n.altura+0.006f,fondoOfrenda+0.10f},{0.83f,0.73f,0.57f});
        for (int f=0; f<66; ++f) {
            const int seed=static_cast<int>(i)*101+f;
            const float x=glm::mix(-n.ancho+0.31f,n.ancho-0.31f,(f+0.46f)/66);
            const float z=n.frente-0.35f-0.26f*semilla(seed,15);
            flores({x,n.altura+0.043f+0.08f*semilla(seed,20),z},0.074f+0.063f*semilla(seed,30),seed);
        }
        for (float lado : {-1.0f,1.0f}) {
            for (int f=0; f<27; ++f) {
                const int seed=f+static_cast<int>(i)*112+(lado<0 ? 620 : 820);
                const float z=glm::mix(fondoOfrenda+0.26f,n.frente-0.57f,(f+0.44f)/27);
                flores({lado*(n.ancho-0.31f-0.13f*semilla(seed,50)),n.altura+0.064f,z},
                    0.078f+0.051f*semilla(seed,12),seed);
            }
        }
    }
    for (size_t i=0; i<veladoras.size(); ++i) dibujarVeladora(mesh,veladoras[i],static_cast<int>(i));

    // Arco y cruz de flores levantados sobre el último nivel.
    const auto vestirFlor = [&](glm::vec3 p,float r,int seed,bool rosa) {
        florOfrenda(mesh,o+p,r,rosa ? magenta*(0.85f+0.15f*semilla(seed,5)) : oro,seed,true);
    };
    for (int i=0; i<=52; ++i) {
        const float a=glm::pi<float>()*i/52;
        for (int capa=0; capa<3; ++capa) {
            const float radio=1.51f+0.128f*capa;
            const float phi=a+0.019f*capa;
            vestirFlor({radio*std::cos(phi),2.05f+radio*std::sin(phi),-2.42f+0.08f*(2-capa)},
                0.137f+0.025f*semilla(i,capa),i+capa*57,capa!=1);
        }
    }
    for (float lado : {-1.0f,1.0f}) {
        for (int i=0; i<7; ++i) {
            for (int capa=0; capa<3; ++capa) {
                vestirFlor({lado*(1.52f+0.13f*capa),1.50f+i*0.096f,-2.33f+0.03f*capa},
                    0.136f,i+capa*7,lado<0 ? capa!=1 : capa!=2);
            }
        }
    }
    box(mesh,o+glm::vec3(-0.055f,3.65f,-2.50f),o+glm::vec3(0.055f,5.02f,-2.39f),{0.43f,0.24f,0.12f});
    box(mesh,o+glm::vec3(-0.71f,4.46f,-2.50f),o+glm::vec3(0.71f,4.59f,-2.39f),{0.43f,0.24f,0.12f});
    for (int i=0; i<=13; ++i) {
        for (float x : {-0.09f,0.09f}) vestirFlor({x,3.74f+i*0.10f,-2.31f},0.115f,i+12,true);
        for (float y : {4.47f,4.60f}) vestirFlor({-0.69f+i*0.106f,y,-2.30f},0.106f,i+88,i%4!=0);
    }

    const auto canasta = [&](glm::vec3 base,int id,bool mantel) {
        anillo(mesh,o+base,0.0f,0.19f,0.22f,0.32f,{0.63f,0.40f,0.18f},12,11.0f);
        anillo(mesh,o+base,0.16f,0.304f,0.21f,0.335f,{0.79f,0.56f,0.27f},12,11.0f);
        for (int f=0; f<16; ++f) {
            const float a=glm::two_pi<float>()*f/16+semilla(id,f)*0.23f;
            const float r=0.09f+0.20f*semilla(f,id);
            flores(base+glm::vec3(r*std::cos(a),0.28f+0.21f*semilla(f+81,id),r*std::sin(a)),0.11f,id*31+f);
        }
        if (mantel) {
            bordado(base+glm::vec3(-0.24f,0.275f,0.13f),base+glm::vec3(0.22f,0.275f,0.13f),
                base+glm::vec3(0.30f,0.10f,0.31f),base+glm::vec3(-0.20f,0.08f,0.32f),{0.95f,0.86f,0.72f});
        }
    };
    for (int i=0; i<12; ++i) {
        const auto& n=nivelesOfrenda[i%4];
        const float lado=i%2==0 ? -1.0f : 1.0f;
        canasta({lado*(0.59f+0.63f*(i/4)),n.altura,n.frente-0.74f},i,i%2==0);
    }
    for (int i=0; i<580; ++i) {
        const float z=3.30f+1.90f*semilla(i,64);
        const float x=(semilla(i,42)-0.50f)*(6.45f-(z-3.30f)*1.15f);
        const glm::vec2 p=ofrenda+glm::vec2(x,z);
        const float giro=glm::two_pi<float>()*semilla(i,62);
        const glm::vec2 dir(std::cos(giro),std::sin(giro)), perp(-dir.y,dir.x);
        const auto sobreSuelo = [](glm::vec2 q) { return punto(q,terreno(q)+0.021f); };
        quad(mesh,sobreSuelo(p-dir*0.042f),sobreSuelo(p+perp*0.023f),sobreSuelo(p+dir*0.048f),
            sobreSuelo(p-perp*0.020f),glm::mix(naranja,oro,semilla(i,8)),8.0f);
    }
}

void calaverita(Mesh& mesh,glm::vec3 base,float alto,float giro,int id)
{
    const size_t inicio=mesh.size();
    const glm::vec3 azucar(0.95f,0.92f,0.79f), glaseado(0.99f,0.98f,0.89f), sombra(0.19f,0.075f,0.16f);
    const std::array<glm::vec3,4> tonos{{{0.88f,0.21f,0.39f},{0.24f,0.57f,0.67f},
        {0.64f,0.27f,0.76f},{0.95f,0.53f,0.14f}}};
    const auto color=tonos[id%tonos.size()];
    volumen(mesh,{0,0.63f,0},{0.43f,0.41f,0.35f},azucar,0.08f);
    volumen(mesh,{0,0.30f,0.105f},{0.29f,0.245f,0.24f},glaseado,0.17f);
    for (float lado : {-1.0f,1.0f}) {
        const glm::vec3 ojo(lado*0.165f,0.642f,0.295f);
        volumen(mesh,ojo,{0.137f,0.131f,0.075f},color,0.21f);
        volumen(mesh,ojo+glm::vec3(0,0,0.049f),{0.091f,0.089f,0.031f},sombra,0.15f);
        for (int i=0; i<8; ++i) {
            const float a=glm::two_pi<float>()*i/8;
            const glm::vec3 p=ojo+glm::vec3(std::cos(a)*0.120f,std::sin(a)*0.113f,0.038f);
            const glm::vec3 d(std::cos(a)*0.026f,std::sin(a)*0.026f,0);
            const glm::vec3 perp(-d.y,d.x,0);
            quad(mesh,p-d,p+perp,p+d,p-perp,glm::vec3(0.99f,0.74f,0.30f));
        }
        const glm::vec3 mejilla(lado*0.235f,0.447f,0.300f);
        florOfrenda(mesh,mejilla,0.066f,color,id*9,true);
    }
    triangle(mesh,{0,0.556f,0.354f},{-0.052f,0.464f,0.362f},{0.052f,0.464f,0.362f},sombra);
    quad(mesh,{-0.171f,0.265f,0.311f},{0.171f,0.265f,0.311f},
        {0.171f,0.336f,0.331f},{-0.171f,0.336f,0.331f},sombra);
    for (int i=0; i<6; ++i) {
        const float x=-0.164f+0.055f*i;
        box(mesh,{x,0.271f,0.312f},{x+0.048f,0.334f,0.342f},azucar);
    }
    florOfrenda(mesh,{0,0.87f,0.302f},0.089f,color,id*3+7,true);
    const float c=std::cos(giro), s=std::sin(giro);
    for (size_t i=inicio; i<mesh.size(); ++i) {
        const auto p=mesh[i].position;
        mesh[i].position=base+alto*glm::vec3(c*p.x+s*p.z,p.y,-s*p.x+c*p.z);
    }
}

void canastaAlimentos(Mesh& mesh,glm::vec3 base,float escala,int id)
{
    const glm::vec3 mimbre(0.68f,0.45f,0.22f), dorado(0.89f,0.59f,0.28f);
    anillo(mesh,base,0,0.21f*escala,0.24f*escala,0.36f*escala,mimbre,12,11.0f);
    anillo(mesh,base,0.22f*escala,0.37f*escala,0.25f*escala,0.37f*escala,dorado,12,11.0f);
    anillo(mesh,base,0.251f*escala,0.35f*escala,0.251f*escala,0.0f,{0.64f,0.29f,0.24f},12);
    for (int i=0; i<10; ++i) {
        const float a=glm::two_pi<float>()*i/10;
        const float radio=(i==0 ? 0.06f : 0.23f)*escala;
        const auto p=base+glm::vec3(radio*std::cos(a),0.295f*escala,radio*std::sin(a));
        if (id%3==0) {
            const glm::vec3 pan(0.71f,0.40f,0.16f), glaseado(0.88f,0.67f,0.41f);
            volumen(mesh,p,glm::vec3(0.096f,0.059f,0.092f)*escala,pan,a);
            volumen(mesh,p+glm::vec3(0,0.054f*escala,0),glm::vec3(0.067f,0.018f,0.026f)*escala,glaseado,a);
            volumen(mesh,p+glm::vec3(0,0.056f*escala,0),glm::vec3(0.025f,0.020f,0.061f)*escala,glaseado,a);
            volumen(mesh,p+glm::vec3(0,0.069f*escala,0),glm::vec3(0.024f)*escala,glaseado,a);
        } else {
            const std::array<glm::vec3,3> fruta{{{0.98f,0.54f,0.10f},{0.75f,0.17f,0.11f},{0.55f,0.63f,0.16f}}};
            volumen(mesh,p+glm::vec3(0,0.023f*escala*semilla(i,id),0),
                glm::vec3(0.076f,0.072f,0.076f)*escala,fruta[(i+id)%fruta.size()],a);
        }
    }
}

void dibujarMaceton(Mesh& mesh,glm::vec2 p,int id,bool arbolModelado)
{
    const auto base=punto(p,terreno(p));
    const glm::vec3 barro=id%2==0 ? glm::vec3(0.63f,0.28f,0.17f) : glm::vec3(0.20f,0.48f,0.41f);
    anillo(mesh,base,-0.11f,0.32f,0.51f,0.454f,barro);
    anillo(mesh,base,0.43f,0.466f,0.54f,0.466f,barro*1.22f);
    anillo(mesh,base,0.54f,0.466f,0.54f,0.415f,barro*0.91f);
    anillo(mesh,base,0.49f,0.42f,0.49f,0.0f,{0.23f,0.16f,0.08f});
    for (int i=0; i<17; ++i) {
        const float a=glm::two_pi<float>()*i/17;
        const float h=(arbolModelado ? 0.22f : 0.57f)+0.20f*semilla(i,id);
        const glm::vec3 direccion(std::cos(a),0,std::sin(a)), lado(-direccion.z,0,direccion.x);
        const auto pie=base+glm::vec3(0,0.49f,0)+direccion*0.11f;
        const auto hombro=pie+direccion*(h*0.46f)+glm::vec3(0,h*0.64f,0);
        const auto punta=pie+direccion*(h*0.85f)+glm::vec3(0,h,0);
        const glm::vec3 verde=glm::mix(glm::vec3(0.16f,0.39f,0.14f),glm::vec3(0.38f,0.56f,0.21f),semilla(i,id+33));
        triangle(mesh,pie,hombro-lado*0.083f,punta,verde*0.77f,14.0f);
        triangle(mesh,pie,punta,hombro+lado*0.083f,verde,14.0f);
        if (i%3==0) florOfrenda(mesh,punta,0.091f,{0.98f,0.53f,0.075f},i+id);
    }
}

void completarPlaza(Mesh& mesh,const std::vector<ConjuntoOfrenda>& conjuntos,bool modelosCargados)
{
    const auto o=punto(ofrenda,terreno(ofrenda));
    const glm::vec3 naranja(1.0f,0.42f,0.06f), oro(0.99f,0.69f,0.12f), rosa(0.88f,0.16f,0.42f);
    const auto piso = [](glm::vec2 p,float altura=0.024f) { return punto(p,terreno(p)+altura); };
    for (size_t i=0; i<nivelesOfrenda.size(); ++i) {
        const auto& n=nivelesOfrenda[i];
        for (float lado : {-1.0f,1.0f}) {
            calaverita(mesh,o+glm::vec3(lado*(n.ancho-0.72f),n.altura,n.frente-0.71f),
                0.39f+0.033f*i,lado*-0.24f,static_cast<int>(i)*2+(lado<0 ? 0 : 1));
            canastaAlimentos(mesh,o+glm::vec3(lado*(0.87f+0.16f*i),n.altura,n.frente-0.61f),
                0.64f+0.055f*i,static_cast<int>(i)*2+(lado<0 ? 4 : 7));
        }
    }
    const auto anclas=papelPlaza();
    for (size_t i=0; i<anclas.size(); ++i) dibujarPapel(mesh,anclas[i],90+static_cast<int>(i),i==0 ? 4.72f : 3.85f);
    for (size_t i=0; i<macetonesPlaza.size(); ++i) {
        const bool arbol=i==0 || i==1 || i==4 || i==5;
        dibujarMaceton(mesh,macetonesPlaza[i],static_cast<int>(i),arbol && modelosCargados);
    }
    for (size_t id=0; id<conjuntos.size(); ++id) {
        const auto& c=conjuntos[id];
        const int seed=static_cast<int>(id)*73;
        const bool grande=c.radio>0.60f;
        const glm::vec3 tierra=grande ? glm::vec3(0.27f,0.16f,0.11f) : glm::vec3(0.34f,0.21f,0.13f);
        const int divisiones=grande ? 84 : 24;
        for (int i=0; i<divisiones; ++i) {
            const float avance=static_cast<float>(i)/divisiones;
            const auto a=contornoOfrenda(c,avance,0.91f), b=contornoOfrenda(c,(i+1.0f)/divisiones,0.91f);
            triangle(mesh,piso(c.centro,0.039f),piso(a,0.039f),piso(b,0.039f),tierra*(0.87f+0.23f*semilla(i,seed)));
            if (grande) {
                const auto borde=contornoOfrenda(c,avance,0.84f);
                const auto interno=contornoOfrenda(c,avance,0.60f);
                florOfrenda(mesh,piso(borde,0.087f),0.062f+0.028f*semilla(i,seed+9),i%5==0 ? rosa : naranja,seed+i);
                if (i%2==0) florOfrenda(mesh,piso(interno,0.095f),0.075f,i%4==0 ? oro : naranja,seed-i);
            }
        }
        const int cantidad=grande ? 162 : 29;
        for (int i=0; i<cantidad; ++i) {
            const float t=semilla(i,seed+84), r=0.09f+0.70f*semilla(i+39,seed);
            const auto p=contornoOfrenda(c,t,r);
            const glm::vec2 dir(std::cos(t*33.0f),std::sin(t*33.0f)), lado(-dir.y,dir.x);
            quad(mesh,piso(p-dir*0.040f,0.052f),piso(p+lado*0.025f,0.052f),
                piso(p+dir*0.046f,0.058f),piso(p-lado*0.028f,0.052f),i%7==0 ? rosa : (i%3==0 ? oro : naranja),8.0f);
        }
        const auto centro=piso(c.centro,0.05f);
        if (grande) {
            const float altura=id<2 ? 0.56f : 0.39f;
            canastaAlimentos(mesh,piso(c.centro+glm::vec2(-c.radio*0.28f,-0.17f),0.05f),0.72f,static_cast<int>(id));
            for (int j=0; j<16; ++j) {
                const float a=j*2.40f;
                const glm::vec2 p=c.centro+glm::vec2(c.radio*0.26f,0.03f)
                    +glm::vec2(std::cos(a),std::sin(a))*(0.08f+0.27f*semilla(j,seed));
                florOfrenda(mesh,piso(p,0.12f+0.18f*semilla(j+6,seed)),0.092f,j%4==0 ? rosa : oro,seed+j);
            }
            calaverita(mesh,piso(c.centro+glm::vec2(0,0.23f),0.085f),altura,id%2==0 ? 0.21f : -0.21f,static_cast<int>(id)+31);
        } else {
            for (int j=0; j<9; ++j) {
                const float a=j*2.40f;
                const auto p=c.centro+glm::vec2(std::cos(a),std::sin(a))*(c.radio*0.47f);
                florOfrenda(mesh,piso(p,0.09f+0.06f*semilla(j,seed)),0.078f,j%4==0 ? rosa : naranja,seed+j);
            }
            if (id%7==0) calaverita(mesh,centro,0.25f,0.20f,static_cast<int>(id)+55);
        }
    }
}

std::vector<Vivienda> distribuirViviendas()
{
    std::vector<Huella> corredores;
    const auto reservarCalle = [&](const auto& calle, float ancho) {
        for (size_t i=1; i<calle.size(); ++i) {
            const glm::vec2 segment = calle[i]-calle[i-1];
            const float length = glm::length(segment);
            corredores.push_back(huellaEn((calle[i]+calle[i-1])*0.5f,segment/length,
                {ancho,length*0.5f+ancho*0.55f}));
        }
    };
    reservarCalle(callePrincipal,2.19f);
    reservarCalle(callejonOeste,1.32f);
    reservarCalle(callejonEste,1.32f);

    std::vector<Vivienda> casas;
    const auto colocar = [&](glm::vec2 centro, glm::vec2 frente, glm::vec2 size, int clave, int fila) {
        if (terrenoBase(centro)<0.25f) return;
        for (const Bosque& bosque : bosques) {
            if (glm::length((centro-bosque.centro)/(bosque.radio+glm::vec2(0.6f)))<1.0f) return;
        }
        const Huella huella = huellaEn(centro,frente,size*0.5f);
        Huella techo = huella;
        techo.mitad += glm::vec2(0.145f);
        for (float side : {-1.0f,1.0f}) {
            for (float end : {-1.0f,1.0f}) {
                const glm::vec2 corner = centro+side*techo.mitad.x*techo.lateral+end*techo.mitad.y*frente;
                if (distanciaPlaza(corner)<1.06f || terrenoBase(corner)<0.08f) return;
            }
        }
        for (const Huella& calle : corredores) {
            if (seCruzan(calle,techo)) return;
        }
        for (const glm::vec2 farol : faroles) {
            if (contiene(techo,farol,0.32f)) return;
        }
        for (const Vivienda& casa : casas) {
            Huella vecino = casa.huella;
            vecino.mitad += glm::vec2(0.145f);
            if (seCruzan(vecino,techo)) return;
        }
        const float height = 1.70f+semilla(clave+53,fila)*1.52f;
        const float yaw = std::atan2(frente.x,frente.y);
        const auto palette = static_cast<size_t>(semilla(clave,fila+81)*acabadosVivienda);
        casas.push_back({huella,height,yaw,palette,semilla(clave-19,fila+19)>0.89f,
            static_cast<int>(casas.size())});
    };

    // Las fachadas continuas fijan primero el borde de cada calle.
    const auto formarFachadas = [&](const auto& calle, float ancho, int sector) {
        for (size_t i=1; i<calle.size(); ++i) {
            const glm::vec2 segment = calle[i]-calle[i-1];
            const float length = glm::length(segment);
            const glm::vec2 tangent = segment/length;
            const glm::vec2 normal(-tangent.y,tangent.x);
            for (int side : {-1,1}) {
                const int fila = sector+static_cast<int>(i)*side;
                float offset = 0.35f;
                for (int lot=0; offset+2.0f<length; ++lot) {
                    const float width = std::min(2.15f+1.25f*semilla(lot,fila),length-offset-0.22f);
                    const float depth = 2.05f+1.32f*semilla(lot+27,fila);
                    const float setback = ancho+0.22f+0.18f*semilla(lot+51,fila);
                    const glm::vec2 centro = calle[i-1]+tangent*(offset+width*0.5f)+normal*static_cast<float>(side)*(setback+depth*0.5f);
                    colocar(centro,-normal*static_cast<float>(side),{width,depth},lot,fila);
                    offset += width+0.34f;
                }
            }
        }
    };
    formarFachadas(callePrincipal,2.19f,100);
    formarFachadas(callejonOeste,1.32f,200);
    formarFachadas(callejonEste,1.32f,300);

    // Las hileras interiores se curvan alrededor de la plaza y bajan hacia la costa.
    float z = -44.0f;
    for (int fila=0; z<41.0f; ++fila) {
        const float rowDepth = 2.45f+0.76f*semilla(7,fila);
        float x = -64.0f+2.6f*semilla(9,fila);
        for (int lot=0; x<64.0f; ++lot) {
            const float width = 1.85f+1.32f*semilla(lot,fila+8);
            const float depth = rowDepth-0.48f*semilla(lot+16,fila);
            const float center = x+width*0.5f;
            const float phase = center*0.11f+z*0.073f;
            const float wave = 0.77f*std::sin(phase)+0.00064f*center*center;
            const float angle = -std::atan(0.0847f*std::cos(phase)+0.00128f*center);
            colocar({center,z+wave},{std::sin(angle),std::cos(angle)},{width,depth},lot,fila);
            x += width+0.37f;
        }
        z += rowDepth+0.40f;
    }

    // Lotes angostos en remates y esquinas que dejan las hileras.
    for (int fila=-23; fila<=23; ++fila) {
        for (int lot=-35; lot<=35; ++lot) {
            const float phase = semilla(lot+17,fila+50);
            const glm::vec2 centro(1.76f*lot+0.53f*(phase-0.5f),
                1.76f*fila+0.50f*(semilla(lot-39,fila)-0.5f));
            const float angle = 0.17f*std::sin(centro.x*0.11f+centro.y*0.09f);
            colocar(centro,{std::sin(angle),std::cos(angle)},{1.43f+0.45f*phase,1.66f+0.50f*phase},lot,fila+400);
        }
    }
    // Despejar el círculo de veladoras sin que el generador vuelva a ocupar ese lote.
    const auto& circuloEntrada = conjuntosPlaza.front();
    casas.erase(std::remove_if(casas.begin(),casas.end(),[&](const Vivienda& casa) {
        const auto& h = casa.huella;
        const auto delta = circuloEntrada.centro-h.centro;
        const glm::vec2 distancia = glm::max(glm::vec2(0.0f),
            glm::abs(glm::vec2(glm::dot(delta,h.lateral),glm::dot(delta,h.frente)))
            -h.mitad-glm::vec2(0.145f));
        return glm::length(distancia)<circuloEntrada.radio;
    }),casas.end());
    return casas;
}

InstanciasModelos ubicarModelos(const std::vector<Vivienda>& casas,const DecoracionIsla& decoracion)
{
    InstanciasModelos resultado;
    const auto casaCercana = [&](glm::vec2 objetivo, size_t excluir) {
        size_t best = casas.size();
        float distance = 1000.0f;
        for (size_t i=0; i<casas.size(); ++i) {
            if (i==excluir) continue;
            const float candidate = glm::distance(objetivo,casas[i].huella.centro);
            if (candidate<distance) {
                distance = candidate;
                best = i;
            }
        }
        return best;
    };
    const size_t rustica = casaCercana({-3.5f,34.1f},casas.size());
    const size_t hoiAn = casaCercana({3.5f,31.5f},rustica);
    for (size_t i=0; i<casas.size(); ++i) {
        const auto& vivienda = casas[i];
        const auto& h = vivienda.huella;
        const float seed = semilla(vivienda.indiceLote,413);
        TipoModelo tipo = CasaAntigua;
        if (i==rustica) tipo = CasaRustica;
        else if (i==hoiAn) tipo = CasaHoiAn;
        else if (seed<0.15f) tipo = RusticaCompacta;
        else if (seed<0.33f) tipo = HoiAnCompacta;
        const float altura = vivienda.altura+(tipo==CasaAntigua ? 0.30f : 0.58f);
        resultado[tipo].push_back({punto(h.centro,terreno(h.centro)+0.06f),
            {2.0f*h.mitad.x,altura,2.0f*h.mitad.y},{std::cos(vivienda.giro),std::sin(vivienda.giro)}});
    }
    const auto colocarArce = [&](glm::vec2 p, float height, float yaw, TipoModelo tipo) {
        resultado[tipo].push_back({punto(p,terreno(p)-0.12f),
            {height*0.80f,height,height*0.74f},{std::cos(yaw),std::sin(yaw)}});
    };
    for (size_t zone=0; zone<bosques.size(); ++zone) {
        const auto& b = bosques[zone];
        for (int fila=-4; fila<=4; ++fila) {
            for (int col=-4; col<=4; ++col) {
                const float seed = semilla(col+static_cast<int>(zone)*17,fila);
                const glm::vec2 offset(2.2f*col+0.89f*(std::abs(fila)%2)+0.55f*(seed-0.5f),
                    2.05f*fila+0.63f*(semilla(fila,col+13)-0.5f));
                if (glm::length(offset/b.radio)>0.97f) continue;
                const glm::vec2 p = b.centro+offset;
                if (terreno(p)<0.15f || std::any_of(casas.begin(),casas.end(),[p](const Vivienda& v) {
                    return contiene(v.huella,p,0.62f);
                })) continue;
                const float height = 3.65f+2.0f*seed;
                const float yaw = glm::two_pi<float>()*semilla(fila+29,col+31);
                const auto tipo = seed<0.52f ? ArceUno : ArceDos;
                colocarArce(p,height,yaw,tipo);
            }
        }
    }
    for (size_t i=0; i<arbolesPlaza.size(); ++i) {
        colocarArce(arbolesPlaza[i],4.0f+0.6f*semilla(static_cast<int>(i),74),
            1.4f*static_cast<float>(i),i%2==0 ? ArceUno : ArceDos);
    }
    for (size_t i : {0u,1u,4u,5u}) {
        const TipoModelo tipo=i%2==0 ? ArceUno : ArceDos;
        colocarArce(macetonesPlaza[i],2.45f+0.43f*semilla(static_cast<int>(i),17),i*1.24f,tipo);
        resultado[tipo].back().posicion.y+=0.61f;
    }
    for (size_t i=0; i<decoracion.arboles.size(); ++i) {
        const auto& p=decoracion.arboles[i];
        const float seed=semilla(static_cast<int>(i),53);
        colocarArce({p.x,p.z},2.9f+1.95f*seed,glm::two_pi<float>()*seed,seed<0.53f ? ArceUno : ArceDos);
    }
    for (size_t i=0; i<decoracion.rincones.size(); ++i) {
        const auto& r=decoracion.rincones[i];
        if (r.uso!=UsoRincon::Jardineras || r.radio<1.12f) continue;
        const glm::vec2 centro(r.centro.x,r.centro.z);
        const glm::vec2 p=centro-r.frente*0.15f;
        colocarArce(p,1.65f+0.58f*semilla(static_cast<int>(i),42),i*0.81f,i%2==0 ? ArceUno : ArceDos);
        resultado[i%2==0 ? ArceUno : ArceDos].back().posicion.y+=0.30f;
    }
    for (size_t i=0; i<2; ++i) {
        const auto& a=atraques[i];
        resultado[Lancha].push_back({{a.x,-0.24f,a.z},{a.ancho,1.49f,a.largo},{std::cos(a.giro),std::sin(a.giro)}});
    }
    constexpr float giroLago=-0.42f;
    resultado[LanchaConRemos].push_back({{24.5f,-0.25f,51.6f},{6.26f,1.79f,7.18f},{std::cos(giroLago),std::sin(giroLago)}});
    return resultado;
}

const glm::vec2 origenLuz(-72.0f,-52.0f), extensionLuz(144.0f,116.0f);

GLuint crearMapaLuz(const DecoracionIsla& decoracion,const std::vector<VeladoraOfrenda>& veladoras)
{
    constexpr int ancho=288, alto=232;
    const glm::vec2 paso=extensionLuz/glm::vec2(ancho,alto);
    std::vector<float> mapa(ancho*alto*2);
    for (int z=0; z<alto; ++z) {
        for (int x=0; x<ancho; ++x) {
            const glm::vec2 p=origenLuz+(glm::vec2(x,z)+0.5f)*paso;
            mapa[(z*ancho+x)*2+1]=terrenoBase(p);
        }
    }
    const auto sumar = [&](glm::vec3 centro,float alcance,float fuerza) {
        const glm::vec2 posicion(centro.x,centro.z);
        const glm::vec2 celda=(posicion-origenLuz)/paso;
        const glm::vec2 margen=glm::vec2(alcance)/paso;
        const int izquierda=std::max(0,static_cast<int>(std::floor(celda.x-margen.x)));
        const int derecha=std::min(ancho-1,static_cast<int>(std::ceil(celda.x+margen.x)));
        const int abajo=std::max(0,static_cast<int>(std::floor(celda.y-margen.y)));
        const int arriba=std::min(alto-1,static_cast<int>(std::ceil(celda.y+margen.y)));
        for (int z=abajo; z<=arriba; ++z) {
            for (int x=izquierda; x<=derecha; ++x) {
                const glm::vec2 p=origenLuz+(glm::vec2(x,z)+0.5f)*paso;
                const glm::vec2 delta=p-posicion;
                const size_t indice=(z*ancho+x)*2;
                const float y=std::max(0.0f,centro.y-mapa[indice+1]-0.80f);
                const float peso=std::max(0.0f,1.0f-(glm::dot(delta,delta)+0.28f*y*y)/(alcance*alcance));
                mapa[indice]+=fuerza*peso*peso;
            }
        }
    };
    for (const auto& farol : decoracion.faroles) sumar(farol.base+glm::vec3(0,farol.altura-0.32f,0),4.75f,0.49f);
    for (const auto& luz : decoracion.lucesFachada) sumar(luz.centro,2.75f,0.32f);
    for (const auto& luz : decoracion.lucesRibera) sumar(luz,2.7f,0.51f);
    for (const auto& extremos : decoracion.tendederos) {
        for (float t : {0.18f,0.50f,0.82f}) sumar(glm::mix(extremos[0],extremos[1],t)+glm::vec3(0,3.23f,0),2.7f,0.18f);
    }
    for (const auto& r : decoracion.rincones) {
        sumar(r.centro+glm::vec3(0,1.22f,0),3.3f,esPuesto(r.uso) ? 0.66f : 0.27f);
    }
    for (const auto& arco : arcosFestivos) {
        for (float lado : {-1.0f,1.0f}) {
            const auto p=arco.centro+glm::vec2(lado*arco.mitad,0);
            sumar(punto(p,alturaCaminata(p)+1.28f),2.1f,0.38f);
        }
    }
    for (const auto& vela : veladoras) {
        sumar(vela.base+glm::vec3(0,vela.altura+0.08f,0),1.28f,vela.vaso ? 0.092f : 0.14f);
    }
    for (const auto& anclas : papelPlaza()) {
        for (float t : {0.20f,0.50f,0.80f}) {
            sumar(glm::mix(anclas[0],anclas[1],t)+glm::vec3(0,3.90f,0),3.35f,0.16f);
        }
    }
    GLuint textura=0;
    glGenTextures(1,&textura);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,textura);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RG16F,ancho,alto,0,GL_RG,GL_FLOAT,mapa.data());
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glActiveTexture(GL_TEXTURE0);
    return textura;
}

void plantarPasto(Mesh& mesh,const std::vector<Vivienda>& casas,const DecoracionIsla& tramo,
    const std::vector<ConjuntoOfrenda>& conjuntos)
{
    const glm::vec3 broteOscuro(0.13f,0.32f,0.071f), broteClaro(0.37f,0.56f,0.17f);
    constexpr float separacion=0.44f;
    for (int fila=-96; fila<=96; ++fila) {
        for (int col=-137; col<=137; ++col) {
            const float seed=semilla(col+85,fila);
            if (seed<0.15f) continue;
            const glm::vec2 p(separacion*(col+0.81f*(seed-0.5f)),
                separacion*(fila+0.83f*(semilla(fila,col-52)-0.5f)));
            const float y=terrenoBase(p);
            if (y<0.47f || distanciaPlaza(p)<1.09f
                || glm::distance(p,puntoMasCercano(p,callePrincipal))<2.37f
                || glm::distance(p,puntoMasCercano(p,callejonOeste))<1.53f
                || glm::distance(p,puntoMasCercano(p,callejonEste))<1.53f) continue;
            if (std::any_of(tramo.rincones.begin(),tramo.rincones.end(),[p](const Rincon& r) {
                return glm::distance(p,glm::vec2(r.centro.x,r.centro.z))<r.radio+0.12f;
            })) continue;
            if (std::any_of(conjuntos.begin(),conjuntos.end(),[p](const ConjuntoOfrenda& c) {
                return glm::distance(p,c.centro)<c.radio+0.15f;
            })) continue;
            const bool planta=seed>0.966f;
            if (std::any_of(casas.begin(),casas.end(),[p,planta](const Vivienda& v) {
                return contiene(v.huella,p,planta ? 0.43f : 0.10f);
            })) continue;
            if (std::any_of(tramo.macetas.begin(),tramo.macetas.end(),[p](glm::vec3 maceta) {
                return glm::distance(p,glm::vec2(maceta.x,maceta.z))<0.28f;
            })) continue;
            const glm::vec3 centro=punto(p,terreno(p)-0.028f);
            const int hojas=planta ? 11 : 6;
            for (int i=0; i<hojas; ++i) {
                const float s=semilla(i+col*17,fila+91);
                const float a=glm::two_pi<float>()*(seed+i*0.382f+s*0.13f);
                const glm::vec3 eje(std::cos(a),0,std::sin(a)), lado(-eje.z,0,eje.x);
                const float alto=(planta ? 0.32f : 0.10f)+(planta ? 0.31f : 0.18f)*s;
                const float ancho=planta ? 0.071f+0.049f*s : 0.011f+0.011f*s;
                const glm::vec3 raiz=centro+eje*(0.013f+0.045f*s);
                const glm::vec3 mitad=raiz+eje*alto*0.30f+glm::vec3(0,alto*0.65f,0);
                const glm::vec3 punta=raiz+eje*alto*(planta ? 0.72f : 0.48f)+glm::vec3(0,alto,0);
                const glm::vec3 color=glm::mix(broteOscuro,broteClaro,0.20f+0.80f*s);
                quad(mesh,raiz-lado*ancho*0.65f,raiz+lado*ancho*0.65f,
                    mitad+lado*ancho,mitad-lado*ancho,color*0.83f,14.0f);
                triangle(mesh,mitad-lado*ancho,mitad+lado*ancho,punta,color,14.0f);
                if (planta) {
                    triangle(mesh,mitad-lado*ancho,mitad+glm::vec3(0,0.022f,0),
                        punta,color*1.12f,14.0f);
                }
            }
        }
    }
}

Mesh generarMapa(const std::vector<Vivienda>& casas, const InstanciasModelos& instancias,
    const DecoracionIsla& tramo,const std::vector<ConjuntoOfrenda>& conjuntos,
    const std::vector<VeladoraOfrenda>& veladoras,bool modelosCargados)
{
    Mesh mesh;
    quad(mesh, {-850,0,-850}, {-850,0,850}, {850,0,850}, {850,0,-850}, {0.24f,0.38f,0.31f},1.0f);
    montanas(mesh,520,52,1.9f,{0.29f,0.44f,0.47f});
    montanas(mesh,407,36,0.7f,{0.32f,0.46f,0.39f});
    constexpr int columnas = 220, filas = 158;
    constexpr float paso = 0.64f;
    const auto ground = [&](glm::vec2 p) {
        const float h = terreno(p);
        const float mainDistance = glm::distance(p,puntoMasCercano(p,callePrincipal));
        const float alleyDistance = std::min(glm::distance(p,puntoMasCercano(p,callejonOeste)),
            glm::distance(p,puntoMasCercano(p,callejonEste)));
        const float main = 1.0f-glm::smoothstep(1.80f,2.32f,mainDistance);
        const float alley = 1.0f-glm::smoothstep(0.92f,1.45f,alleyDistance);
        const float square = 1.0f-glm::smoothstep(0.92f,1.03f,distanciaPlaza(p));
        float patio=0.0f;
        for (const auto& r : tramo.rincones) {
            const glm::vec2 centro(r.centro.x,r.centro.z);
            if (enMuelle(centro)) continue;
            patio=std::max(patio,1.0f-glm::smoothstep(r.radio*0.71f,r.radio,glm::distance(p,centro)));
        }
        const float stone = std::max({main,alley,square,patio});
        const glm::vec3 coast(0.49f,0.43f,0.29f);
        const float mottling=0.5f+0.30f*std::sin(p.x*0.64f+p.y*0.49f)*std::sin(p.y*0.81f-p.x*0.28f);
        const glm::vec3 verde=glm::mix(glm::vec3(0.18f,0.34f,0.102f),glm::vec3(0.33f,0.50f,0.17f),mottling);
        const glm::vec3 soil=glm::mix(coast,verde,glm::smoothstep(0.18f,0.94f,h));
        const glm::vec3 paving = glm::mix(glm::vec3(0.51f,0.49f,0.45f),
            glm::vec3(0.63f,0.59f,0.52f),std::max({main*0.61f,square,patio}));
        return Vertex{punto(p,h),glm::mix(soil,paving,stone),0.0f,stone};
    };
    std::vector<Vertex> grid;
    grid.reserve((columnas+1)*(filas+1));
    for (int z=0; z<=filas; ++z) {
        for (int x=0; x<=columnas; ++x) {
            grid.push_back(ground({(x-columnas/2)*paso,(z-filas/2)*paso}));
        }
    }
    for (int z=0; z<filas; ++z) {
        for (int x=0; x<columnas; ++x) {
            const size_t index = z*(columnas+1)+x;
            Vertex a=grid[index], b=grid[index+columnas+1], c=grid[index+columnas+2], d=grid[index+1];
            if (std::max({a.position.y,b.position.y,c.position.y,d.position.y}) < -0.1f) continue;
            const glm::vec3 normal=glm::normalize(glm::cross(b.position-a.position,d.position-a.position));
            const float shade=0.66f+0.32f*std::max(0.0f,glm::dot(normal,glm::normalize(glm::vec3(-0.6f,1,0.55f))))
                + 0.04f*semilla(x,z);
            a.color*=shade;
            b.color*=shade;
            c.color*=shade;
            d.color*=shade;
            mesh.insert(mesh.end(),{a,b,c,a,c,d});
        }
    }

    const std::array<glm::vec3,acabadosVivienda> walls{{{0.86f,0.77f,0.63f},{0.78f,0.68f,0.54f},
        {0.81f,0.75f,0.68f},{0.70f,0.63f,0.51f},{0.74f,0.64f,0.54f}}};
    const std::array<glm::vec3,acabadosVivienda> roofs{{{0.79f,0.37f,0.19f},{0.67f,0.30f,0.18f},
        {0.82f,0.45f,0.25f},{0.59f,0.28f,0.20f},{0.73f,0.44f,0.28f}}};
    for (const Vivienda& vivienda : casas) {
        const auto& h = vivienda.huella;
        if (modelosCargados) {
            const auto start = mesh.size();
            const auto base = terreno(h.centro);
            box(mesh,{-h.mitad.x,-1.35f,-h.mitad.y},{h.mitad.x,0.062f,h.mitad.y},{0.50f,0.43f,0.34f});
            for (size_t j=start; j<mesh.size(); ++j) {
                const auto p = mesh[j].position;
                mesh[j].position = punto(h.centro+p.x*h.lateral+p.z*h.frente,base+p.y);
            }
            continue;
        }
        casa(mesh,h.centro,2.0f*h.mitad.x,2.0f*h.mitad.y,vivienda.altura,vivienda.giro,
            walls[vivienda.acabado],roofs[vivienda.acabado],vivienda.azotea);
    }
    for (int i=0; i<48; ++i) {
        const float angle = glm::two_pi<float>()*i/48;
        const glm::vec2 radius(std::cos(angle)*58.5f,std::sin(angle)*41.4f);
        for (float scale : {0.96f,0.91f,0.86f}) {
            const glm::vec2 p = scale*radius;
            if (terreno(p)<0.14f || glm::distance(p,puntoMasCercano(p,callePrincipal))<4.0f) continue;
            if (std::any_of(tramo.arboles.begin(),tramo.arboles.end(),[p](glm::vec3 tree) {
                return glm::distance(p,glm::vec2(tree.x,tree.z))<1.8f;
            })) continue;
            const bool occupied = std::any_of(casas.begin(),casas.end(),[p](const Vivienda& vivienda) {
                return contiene(vivienda.huella,p,1.05f);
            });
            if (occupied) continue;
            const float seed = semilla(i,44);
            arbol(mesh,p,1.65f+1.03f*seed,seed);
            break;
        }
    }
    if (!modelosCargados) {
        for (const auto tipo : {ArceUno,ArceDos}) {
            for (const auto& tree : instancias[tipo]) {
                arbol(mesh,{tree.posicion.x,tree.posicion.z},tree.escala.y*0.8f,0.67f);
            }
        }
    }
    embarcadero(mesh,modelosCargados);
    decorarTablones(mesh);
    dibujarArcosFestivos(mesh);
    monumento(mesh);
    prepararOfrenda(mesh,veladoras);
    completarPlaza(mesh,conjuntos,modelosCargados);
    plantarPasto(mesh,casas,tramo,conjuntos);
    for (const auto& f : tramo.faroles) dibujarFarol(mesh,f);
    for (const auto& luz : tramo.lucesRibera) dibujarLuzRibera(mesh,luz);
    for (const auto& luz : tramo.lucesFachada) dibujarLuzFachada(mesh,luz);
    for (size_t i=0; i<tramo.tendederos.size(); ++i) dibujarPapel(mesh,tramo.tendederos[i],static_cast<int>(i));
    for (size_t i=0; i<tramo.macetas.size(); ++i) dibujarMaceta(mesh,tramo.macetas[i],static_cast<int>(i));
    for (size_t i=0; i<tramo.rincones.size(); ++i) {
        const auto& r=tramo.rincones[i];
        dibujarRincon(mesh,r,static_cast<int>(i));
        if (!esPuesto(r.uso) && r.uso!=UsoRincon::Catrina) {
            const glm::vec2 centro(r.centro.x,r.centro.z);
            const glm::vec2 lateral(r.frente.y,-r.frente.x);
            const glm::vec2 p=centro+lateral*(r.radio*0.56f)-r.frente*(r.radio*0.36f);
            dibujarLuzRibera(mesh,punto(p,alturaCaminata(p)+0.77f));
        }
    }
    return mesh;
}

struct PuntoProcesion {
    glm::vec2 posicion;
    float distancia;
};

constexpr float separacionProcesion=1.42f;
constexpr size_t cantidadPeregrinos=9;
constexpr float velocidadProcesion=1.34f;
constexpr float aceleracionProcesion=1.25f;

using RutaProcesion=std::vector<PuntoProcesion>;

float inicioPeregrino(size_t id)
{
    return 0.42f+separacionProcesion*static_cast<float>(cantidadPeregrinos-1-id);
}

RutaProcesion trazarProcesion(const std::vector<glm::vec2>& vertices)
{
    RutaProcesion ruta{{vertices.front(),0}};
    const auto agregar=[&](glm::vec2 p) {
        const float avance=glm::distance(p,ruta.back().posicion);
        if (avance>0.001f) ruta.push_back({p,ruta.back().distancia+avance});
    };
    for (size_t i=1; i+1<vertices.size(); ++i) {
        const auto cruce=vertices[i];
        const auto previo=vertices[i-1]-cruce;
        const auto siguiente=vertices[i+1]-cruce;
        const float radio=std::min({0.96f,0.38f*glm::length(previo),0.38f*glm::length(siguiente)});
        const auto antes=cruce+radio*glm::normalize(previo);
        const auto despues=cruce+radio*glm::normalize(siguiente);
        agregar(antes);
        for (int paso=1; paso<=16; ++paso) {
            const float t=paso/16.0f;
            agregar(glm::mix(glm::mix(antes,cruce,t),glm::mix(cruce,despues,t),t));
        }
    }
    agregar(vertices.back());
    return ruta;
}

std::array<RutaProcesion,cantidadPeregrinos> prepararProcesion()
{
    const std::vector<glm::vec2> tramoComun{{0,49.5f},callePrincipal[0],callePrincipal[1],callejonOeste[0],
        callejonOeste[1],callejonOeste[2],callejonOeste[3],callejonOeste[4]};
    // El grupo se abre antes de los círculos de veladoras. Los primeros tres siguen por el arco.
    const std::array<std::vector<glm::vec2>,cantidadPeregrinos> llegadas{{
        {{0,5.08f},{0,-2.54f}},
        {{0,5.08f},{0,-1.22f}},
        {{0,5.08f},{0,0.09f}},
        {{0.50f,4.96f},{3.75f,4.30f}},
        {{-4.40f,5.91f},{-3.21f,4.66f}},
        {{0,5.08f},{1.60f,3.57f}},
        {{-2.35f,5.50f},{-1.74f,4.35f}},
        {{0.50f,4.96f},{3.30f,5.76f}},
        {{0,5.08f},{0,3.10f},{-1.08f,3.05f}}
    }};
    std::array<RutaProcesion,cantidadPeregrinos> rutas;
    for (size_t i=0; i<rutas.size(); ++i) {
        auto vertices=tramoComun;
        vertices.insert(vertices.end(),llegadas[i].begin(),llegadas[i].end());
        rutas[i]=trazarProcesion(vertices);
    }
    return rutas;
}

glm::vec2 puntoProcesion(const RutaProcesion& ruta,float distancia)
{
    distancia=glm::clamp(distancia,0.0f,ruta.back().distancia);
    const auto siguiente=std::lower_bound(ruta.begin()+1,ruta.end(),distancia,
        [](const PuntoProcesion& p,float d) { return p.distancia<d; });
    const auto& a=*(siguiente-1);
    const auto& b=*siguiente;
    return glm::mix(a.posicion,b.posicion,(distancia-a.distancia)/(b.distancia-a.distancia));
}

struct AvanceProcesion {
    float distancia;
    float velocidad;
};

float duracionProcesion(float largo)
{
    return largo/velocidadProcesion+velocidadProcesion/aceleracionProcesion;
}

AvanceProcesion avanceProcesion(float segundos,const RutaProcesion& ruta,size_t id)
{
    const float largo=ruta.back().distancia-inicioPeregrino(id);
    const float rampa=velocidadProcesion/aceleracionProcesion;
    const float metrosRampa=0.5f*velocidadProcesion*rampa;
    const float duracion=duracionProcesion(largo);
    const float t=glm::clamp(segundos,0.0f,duracion);
    if (t<rampa) return {0.5f*aceleracionProcesion*t*t,aceleracionProcesion*t};
    if (t<duracion-rampa) return {metrosRampa+velocidadProcesion*(t-rampa),velocidadProcesion};
    const float restante=duracion-t;
    return {largo-0.5f*aceleracionProcesion*restante*restante,aceleracionProcesion*restante};
}

struct PosturaPersona {
    float paso=0, marcha=0, alturaAsiento=0.50f;
    bool ajustarSuelo=false, sentado=false, sombrero=false, mandil=false;
    glm::vec3 izquierda{-0.26f,0.82f,0.02f}, derecha{0.26f,0.82f,0.02f};
};

struct ManosPersona { glm::vec3 izquierda,derecha; };

float tallaPersona(size_t id)
{
    return 0.90f+0.10f*semilla(static_cast<int>(id),48);
}

ManosPersona dibujarPersona(Mesh& mesh,glm::vec3 base,glm::vec2 frente,size_t id,const PosturaPersona& pose)
{
    const glm::vec2 lateral(frente.y,-frente.x);
    const float paso=pose.paso;
    const float intensidad=pose.marcha;
    const float balance=std::sin(glm::two_pi<float>()*paso)*intensidad;
    const float subida=0.037f*balance*balance;
    const float escala=tallaPersona(id);
    const auto alMundo=[&](glm::vec3 v) {
        return base+escala*punto(v.x*lateral+v.z*frente,v.y);
    };
    const size_t inicio=mesh.size();
    const std::array<glm::vec3,cantidadPeregrinos> camisas{{
        {0.64f,0.17f,0.23f},{0.23f,0.52f,0.47f},{0.53f,0.34f,0.66f},
        {0.72f,0.53f,0.22f},{0.31f,0.42f,0.65f},{0.70f,0.35f,0.37f},
        {0.48f,0.54f,0.31f},{0.79f,0.69f,0.54f},{0.34f,0.42f,0.56f}
    }};
    const std::array<glm::vec3,3> pieles{{{0.74f,0.50f,0.33f},{0.62f,0.37f,0.24f},{0.82f,0.57f,0.40f}}};
    const glm::vec3 camisa=camisas[id%camisas.size()], piel=pieles[id%pieles.size()], pelo(0.14f,0.095f,0.083f);
    const glm::vec3 pantalon(0.20f,0.25f,0.33f), zapato(0.22f,0.14f,0.12f);
    const glm::vec3 torso(0.024f*balance,subida+(pose.sentado ? pose.alturaAsiento-0.85f : 0.0f),0);
    box(mesh,torso+glm::vec3(-0.15f,0.78f,-0.115f),torso+glm::vec3(0.15f,0.97f,0.11f),pantalon);
    volumen(mesh,torso+glm::vec3(0,1.11f,0),{0.217f,0.266f,0.137f},camisa);
    if (pose.mandil) {
        const glm::vec3 manta(0.88f,0.76f,0.57f);
        quad(mesh,torso+glm::vec3(-0.159f,0.82f,0.132f),torso+glm::vec3(0.159f,0.82f,0.132f),
            torso+glm::vec3(0.116f,1.22f,0.146f),torso+glm::vec3(-0.116f,1.22f,0.146f),manta);
        box(mesh,torso+glm::vec3(-0.114f,0.94f,0.141f),torso+glm::vec3(0.114f,1.00f,0.153f),manta*0.72f);
    }
    anillo(mesh,torso,1.28f,0.105f,1.34f,0.067f,{0.91f,0.65f,0.28f},10);
    varilla(mesh,torso+glm::vec3(0,1.30f,0),torso+glm::vec3(0,1.46f,0),0.048f,piel);
    volumen(mesh,torso+glm::vec3(0,1.505f,0.013f),{0.122f,0.157f,0.117f},piel);
    volumen(mesh,torso+glm::vec3(0,1.569f,-0.035f),{0.127f,0.11f,0.102f},pelo);
    if (id%3==1) volumen(mesh,torso+glm::vec3(0,1.50f,-0.132f),{0.081f,0.075f,0.066f},pelo);
    if (id%3==2 || pose.sombrero) {
        const glm::vec3 palma(0.73f,0.49f,0.24f);
        anillo(mesh,torso,1.60f,0.217f,1.623f,0.222f,palma,16);
        anillo(mesh,torso,1.623f,0.222f,1.646f,0.127f,palma,16);
        anillo(mesh,torso,1.633f,0.128f,1.664f,0.121f,{0.36f,0.19f,0.12f},14);
        anillo(mesh,torso,1.664f,0.121f,1.733f,0.102f,palma,14);
        anillo(mesh,torso,1.733f,0.102f,1.747f,0.0f,palma,14);
    }
    for (float lado : {-1.0f,1.0f}) {
        const glm::vec3 ojo=torso+glm::vec3(lado*0.042f,1.522f,0.117f);
        box(mesh,ojo-glm::vec3(0.012f,0.009f,0.006f),ojo+glm::vec3(0.012f,0.009f,0.006f),pelo);
        const float fase=paso+(lado>0 ? 0.50f : 0.0f);
        const float t=fase-std::floor(fase);
        const float vuelo=glm::clamp((t-0.55f)/0.45f,0.0f,1.0f);
        const float z=0.306f*(t<0.55f ? 1.0f-2.0f*t/0.55f : -1.0f+2.0f*glm::smoothstep(0.0f,1.0f,vuelo));
        glm::vec3 pie(lado*0.103f,0.0f,(pose.sentado ? 0.47f : 0.04f)+z*intensidad);
        const auto apoyo=alMundo(pie);
        pie.y=0.13f*std::sin(glm::pi<float>()*vuelo)*intensidad;
        if (pose.ajustarSuelo) pie.y+=(alturaCaminata({apoyo.x,apoyo.z})-base.y)/escala;
        const glm::vec3 tobillo=pie+glm::vec3(0,0.081f,-0.037f);
        const glm::vec3 cadera=torso+glm::vec3(lado*0.098f,0.854f,0);
        const glm::vec3 rodilla=pose.sentado ? glm::vec3(lado*0.105f,pose.alturaAsiento-0.03f,0.38f)
            : glm::mix(cadera,tobillo,0.54f)+glm::vec3(0,0,0.031f+0.096f*vuelo*(1-vuelo)*intensidad);
        varilla(mesh,cadera,rodilla,0.069f,pantalon);
        varilla(mesh,rodilla,tobillo,0.056f,pantalon*0.91f);
        box(mesh,pie+glm::vec3(-0.072f,0.012f,-0.101f),pie+glm::vec3(0.072f,0.092f,0.127f),zapato);
    }
    const glm::vec3 manoLibre=torso+pose.izquierda;
    const glm::vec3 manoDerecha=torso+pose.derecha;
    const glm::vec3 hombroLibre=torso+glm::vec3(-0.179f,1.25f,0);
    const glm::vec3 hombroVela=torso+glm::vec3(0.179f,1.25f,0);
    const glm::vec3 codoLibre=glm::mix(hombroLibre,manoLibre,0.60f)+glm::vec3(-0.030f,-0.035f,-0.043f);
    const glm::vec3 codoVela=glm::mix(hombroVela,manoDerecha,0.60f)+glm::vec3(0.035f,-0.080f,-0.060f);
    varilla(mesh,hombroLibre,codoLibre,0.059f,camisa);
    varilla(mesh,hombroVela,codoVela,0.059f,camisa);
    varilla(mesh,codoLibre,manoLibre,0.044f,piel);
    varilla(mesh,codoVela,manoDerecha,0.044f,piel);
    volumen(mesh,manoLibre,{0.049f,0.062f,0.050f},piel);
    volumen(mesh,manoDerecha,{0.060f,0.043f,0.062f},piel);
    for (size_t i=inicio; i<mesh.size(); ++i) mesh[i].position=alMundo(mesh[i].position);
    return {alMundo(manoLibre),alMundo(manoDerecha)};
}


glm::vec3 dibujarPeregrino(Mesh& mesh,const RutaProcesion& ruta,float segundos,size_t id)
{
    const auto avance=avanceProcesion(segundos,ruta,id);
    const float distancia=inicioPeregrino(id)+avance.distancia;
    const auto p=puntoProcesion(ruta,distancia);
    const auto rumbo=puntoProcesion(ruta,distancia+0.13f)-puntoProcesion(ruta,distancia-0.13f);
    const auto alAltar=ofrenda-p;
    const float llegada=glm::smoothstep(0.0f,0.85f,
        segundos-duracionProcesion(ruta.back().distancia-inicioPeregrino(id)));
    const float giro=std::atan2(rumbo.y,rumbo.x);
    const float orientacion=giro+llegada*std::remainder(std::atan2(alAltar.y,alAltar.x)-giro,glm::two_pi<float>());
    const glm::vec2 frente(std::cos(orientacion),std::sin(orientacion));
    PosturaPersona pose;
    pose.paso=avance.distancia/1.12f+0.23f*id;
    pose.marcha=glm::smoothstep(0.0f,0.84f,avance.velocidad);
    pose.ajustarSuelo=true;
    const float balance=std::sin(glm::two_pi<float>()*pose.paso)*pose.marcha;
    pose.izquierda.z=-0.175f*balance;
    pose.derecha={0.254f,1.07f,0.284f+0.038f*balance};
    const auto manos=dibujarPersona(mesh,punto(p,alturaCaminata(p)),frente,id,pose);
    const auto vela=manos.derecha+glm::vec3(0,0.024f,0);
    dibujarVeladora(mesh,{vela,0.158f,true},static_cast<int>(id)+201);
    return vela+glm::vec3(0,0.217f,0);
}


constexpr size_t cantidadLanchas=2;

struct EstadoLancha {
    glm::vec2 centro, frente;
    float velocidad, avance, fase, remada, balanceo, cabeceo, altura;
};

EstadoLancha estadoLancha(size_t id,float segundos,float reloj)
{
    const auto& atraque=atraques[id+2];
    const float lado=atraque.x<0 ? -1.0f : 1.0f;
    const float duracion=id==0 ? 23.4f : 25.6f;
    const float demora=id==0 ? 0.0f : 3.0f;
    const float t=glm::clamp((segundos-demora)/duracion,0.0f,1.0f);
    const float avance=t*t*(3.0f-2.0f*t);
    const float inicioZ=id==0 ? 87.0f : 94.0f;
    const float z=glm::mix(inicioZ,atraque.z,avance);
    const float fuera=glm::clamp((z-57.4f)/(inicioZ-57.4f),0.0f,1.0f);
    const float desvio=id==0 ? 5.7f : 9.1f;
    const float x=atraque.x+lado*(0.27f+desvio*fuera*fuera*(3.0f-2.0f*fuera));
    const float giro=-lado*desvio*6.0f*fuera*(1.0f-fuera)/(inicioZ-57.4f);
    const float velocidad=6.0f*t*(1.0f-t)*(inicioZ-atraque.z)/duracion;
    const float fuerza=glm::smoothstep(54.3f,60.2f,z)*glm::smoothstep(0.0f,0.84f,velocidad);
    const float fase=glm::two_pi<float>()*(segundos/1.62f+0.34f*id);
    return {{x,z},glm::normalize(glm::vec2(giro,-1)),velocidad,avance,fase,fuerza,
        0.018f*std::sin(1.93f*reloj+id)+fuerza*0.023f*std::cos(fase),
        0.009f*std::sin(1.22f*reloj+1.9f*id),
        0.043f*std::sin(1.81f*reloj+2.0f*id)};
}

glm::vec3 puntoLancha(const EstadoLancha& lancha,glm::vec3 p)
{
    const glm::vec2 lateral(lancha.frente.y,-lancha.frente.x);
    const float x=p.x*std::cos(lancha.balanceo)-p.y*std::sin(lancha.balanceo);
    const float y=p.y*std::cos(lancha.balanceo)+p.x*std::sin(lancha.balanceo);
    return punto(lancha.centro+lateral*x+lancha.frente*p.z,
        y+lancha.altura+p.z*std::sin(lancha.cabeceo));
}

const std::array<glm::vec2,9> seccionesCasco{{
    {0.40f,-2.77f},{0.79f,-2.48f},{1.02f,-1.72f},{1.09f,-0.72f},{1.10f,0.37f},
    {0.99f,1.37f},{0.76f,2.10f},{0.42f,2.65f},{0.06f,3.02f}
}};

float alturaBorda(float z)
{
    return 0.53f+0.16f*glm::smoothstep(1.43f,3.03f,z)+0.075f*glm::smoothstep(2.04f,2.77f,-z);
}

void cascoLancha(Mesh& mesh,size_t id)
{
    const glm::vec3 madera(0.47f,0.24f,0.13f), claro(0.67f,0.39f,0.20f);
    const glm::vec3 pintura=id==0 ? glm::vec3(0.26f,0.50f,0.45f) : glm::vec3(0.66f,0.24f,0.17f);
    const auto tabla=[&](glm::vec2 perfil,float lado,float altura) {
        return glm::vec3(lado*perfil.x*(0.67f+0.33f*altura),
            glm::mix(-0.23f,alturaBorda(perfil.y),altura),perfil.y);
    };
    for (size_t i=1; i<seccionesCasco.size(); ++i) {
        const auto a=seccionesCasco[i-1], b=seccionesCasco[i];
        for (float lado : {-1.0f,1.0f}) {
            quad(mesh,tabla(a,lado,0),tabla(b,lado,0),tabla(b,lado,0.55f),tabla(a,lado,0.55f),madera);
            quad(mesh,tabla(a,lado,0.55f),tabla(b,lado,0.55f),tabla(b,lado,0.88f),tabla(a,lado,0.88f),pintura);
            quad(mesh,tabla(a,lado,0.88f),tabla(b,lado,0.88f),tabla(b,lado,1),tabla(a,lado,1),claro);
            varilla(mesh,tabla(a,lado,1),tabla(b,lado,1),0.036f,claro*1.12f);
        }
        const float tono=0.85f+0.19f*semilla(static_cast<int>(i),19);
        quad(mesh,{-a.x*0.87f,0.17f,a.y},{a.x*0.87f,0.17f,a.y},
            {b.x*0.87f,0.17f,b.y},{-b.x*0.87f,0.17f,b.y},claro*tono);
    }
    for (const auto& extremo : {seccionesCasco.front(),seccionesCasco.back()}) {
        quad(mesh,tabla(extremo,-1,0),tabla(extremo,1,0),tabla(extremo,1,1),tabla(extremo,-1,1),madera);
    }
    for (float z : {-1.02f,0.82f}) {
        box(mesh,{-0.88f,0.42f,z-0.13f},{0.88f,0.53f,z+0.15f},claro);
        for (float x : {-0.61f,0.61f}) box(mesh,{x-0.048f,0.17f,z-0.08f},{x+0.048f,0.42f,z+0.08f},madera);
    }
    for (float x : {-0.91f,0.91f}) {
        varilla(mesh,{x,0.46f,-0.66f},{x,0.69f,-0.66f},0.027f,{0.34f,0.26f,0.19f});
    }
}

void decorarLancha(Mesh& mesh,size_t id)
{
    const glm::vec3 naranja(1.0f,0.48f,0.085f), oro(1.0f,0.68f,0.13f);
    for (size_t i=1; i<seccionesCasco.size(); ++i) {
        const auto a=seccionesCasco[i-1], b=seccionesCasco[i];
        for (float lado : {-1.0f,1.0f}) {
            for (int j=0; j<4; ++j) {
                const auto p=glm::mix(a,b,(j+0.30f)/4.0f);
                florOfrenda(mesh,{lado*p.x,alturaBorda(p.y)+0.018f,p.y},0.104f,
                    j==1 ? oro : naranja,static_cast<int>(i*13+j+id));
            }
        }
    }
    for (float z : {-2.10f,-1.57f,0.09f,0.44f,1.46f,1.91f,2.28f}) {
        const float ancho=z>2.1f ? 0.40f : z>1.45f ? 0.65f : 0.75f;
        for (float lado : {-1.0f,1.0f}) {
            const glm::vec3 p(lado*ancho,0.48f,z);
            box(mesh,p+glm::vec3(-0.106f,-0.045f,-0.11f),p+glm::vec3(0.106f,0,0.11f),{0.57f,0.34f,0.18f});
            box(mesh,p+glm::vec3(-0.028f,-0.31f,-0.026f),p+glm::vec3(0.028f,-0.045f,0.026f),{0.48f,0.27f,0.14f});
            const float h=0.15f+0.07f*semilla(static_cast<int>(z*70),static_cast<int>(id));
            dibujarVeladora(mesh,{p,h,true},static_cast<int>(z*70+270+id));
        }
    }
    anillo(mesh,{0,0.17f,2.24f},0.0f,0.33f,0.16f,0.45f,{0.62f,0.39f,0.19f},12);
    anillo(mesh,{0,0.17f,2.24f},0.16f,0.45f,0.16f,0.0f,{0.53f,0.33f,0.16f},12);
    for (int i=0; i<13; ++i) {
        const float a=2.4f*i;
        florOfrenda(mesh,{0.38f*std::cos(a),0.35f+0.11f*semilla(i,33),2.31f+0.38f*std::sin(a)},
            0.134f,i%4==0 ? oro : naranja,i+41);
    }
    const size_t inicio=mesh.size();
    dibujarCatrina(mesh,static_cast<int>(id)+1);
    for (size_t i=inicio; i<mesh.size(); ++i) {
        auto& p=mesh[i].position;
        p.y=(p.y<0.88f ? p.y*0.52f : p.y-0.4224f)*0.94f+0.21f;
        p.x*=0.88f;
        p.z=p.z*0.94f+0.86f;
    }
}

void dibujarEstela(Mesh& mesh,const EstadoLancha& e)
{
    const float fuerza=glm::smoothstep(0.16f,1.7f,e.velocidad);
    if (fuerza<0.01f) return;
    const glm::vec2 lateral(e.frente.y,-e.frente.x);
    for (float lado : {-1.0f,1.0f}) {
        for (int i=0; i<15; ++i) {
            const auto borde=[&](float t) {
                return e.centro-e.frente*(2.45f+(2.3f+4.6f*fuerza)*t)
                    +lateral*lado*(0.34f+(0.80f+2.9f*fuerza)*t);
            };
            const float t=i/15.0f;
            const auto a=borde(t), b=borde((i+1.0f)/15.0f);
            const auto ancho=lateral*(0.072f+0.19f*t);
            const float espuma=0.49f*fuerza*(1.0f-t)*(0.70f+0.30f*semilla(i,83));
            quad(mesh,punto(a-ancho,0.021f),punto(b-ancho,0.021f),punto(b+ancho,0.021f),
                punto(a+ancho,0.021f),{espuma,0,0},15.0f);
        }
    }
}

void dibujarLancha(Mesh& mesh,const Mesh& decorada,const EstadoLancha& e,size_t id)
{
    const size_t inicio=mesh.size();
    mesh.insert(mesh.end(),decorada.begin(),decorada.end());
    const float barrido=glm::mix(-1.58f,0.61f*std::sin(e.fase),e.remada);
    const float inclinacion=glm::mix(-0.04f,-0.20f-0.47f*std::cos(e.fase),e.remada);
    const glm::vec3 asiento(0,0.19f,-0.91f);
    const auto ejeRemo=[&](float lado) {
        return glm::vec3(lado*std::cos(barrido)*std::cos(inclinacion),std::sin(inclinacion),
            std::sin(barrido)*std::cos(inclinacion));
    };
    const auto chumacera=[](float lado) { return glm::vec3(lado*0.94f,0.655f,-0.66f); };
    PosturaPersona remero;
    remero.sentado=true;
    remero.sombrero=true;
    const float escala=tallaPersona(id+12);
    remero.alturaAsiento=0.35f/escala;
    const auto agarre=[&](float lado) {
        const auto mango=chumacera(lado)-0.67f*ejeRemo(lado);
        const auto pose=(mango-asiento)/escala+glm::vec3(0,0.85f-remero.alturaAsiento,0);
        return glm::mix(glm::vec3(lado*0.21f,0.93f,0.35f),pose,glm::smoothstep(0.20f,0.76f,e.remada));
    };
    remero.izquierda=agarre(-1.0f);
    remero.derecha=agarre(1.0f);
    dibujarPersona(mesh,asiento,{0,1},id+12,remero);
    const glm::vec3 madera(0.70f,0.43f,0.22f);
    std::array<glm::vec3,2> palas{};
    for (size_t i=0; i<palas.size(); ++i) {
        const float lado=i==0 ? -1.0f : 1.0f;
        const auto eje=ejeRemo(lado);
        const auto pivote=chumacera(lado);
        const auto mango=pivote-0.75f*eje;
        const auto pala=pivote+1.27f*eje;
        const auto ancho=glm::normalize(glm::cross(eje,glm::vec3(0,1,0)))*0.115f;
        varilla(mesh,mango,pala-eje*0.27f,0.032f,madera);
        quad(mesh,pala-eje*0.35f-ancho*0.48f,pala-ancho,pala+ancho,pala-eje*0.35f+ancho*0.48f,madera*1.14f);
        quad(mesh,pala+glm::vec3(0,0.028f,0)-ancho,pala-ancho,pala+ancho,pala+glm::vec3(0,0.028f,0)+ancho,madera*0.71f);
        palas[i]=puntoLancha(e,pala);
    }
    for (size_t i=inicio; i<mesh.size(); ++i) mesh[i].position=puntoLancha(e,mesh[i].position);
    dibujarEstela(mesh,e);
    for (const auto& pala : palas) {
        const float fuerza=e.remada*(1.0f-glm::smoothstep(-0.13f,0.055f,pala.y));
        if (fuerza<0.03f) continue;
        const glm::vec3 centro(pala.x,0.023f,pala.z);
        anillo(mesh,centro,0,0.14f+0.10f*fuerza,0,0.33f+0.19f*fuerza,{0.42f*fuerza,0,0},16,15.0f);
    }
    if (e.avance>=0.9999f) {
        const float lado=e.centro.x<0 ? -1.0f : 1.0f;
        for (float z : {-1.33f,1.42f}) {
            const glm::vec3 poste(lado*5.28f,1.02f,z<0 ? 48.2f : 46.05f);
            const auto amarre=puntoLancha(e,{lado*0.92f,0.54f,z});
            const auto cuerda=[&](float t) {
                return glm::mix(amarre,poste,t)-glm::vec3(0,0.33f*t*(1.0f-t),0);
            };
            for (int j=0; j<7; ++j) varilla(mesh,cuerda(j/7.0f),cuerda((j+1)/7.0f),0.014f,{0.62f,0.45f,0.28f});
        }
    }
}

enum class Actividad { Vender, Conversar, Descansar };
struct Habitante {
    glm::vec3 base;
    glm::vec2 frente;
    Actividad actividad;
    float asiento=0.0f;
};

std::vector<Habitante> distribuirHabitantes(const std::vector<Vivienda>& casas,const DecoracionIsla& tramo,
    const std::vector<ConjuntoOfrenda>& conjuntos,const std::array<RutaProcesion,cantidadPeregrinos>& rutas)
{
    std::vector<Habitante> gente;
    const auto libre=[&](glm::vec2 p,const Rincon* propio) {
        if (alturaCaminata(p)<0.22f || ocupaPlaza(p,0.40f) || pieDeArco(p,0.34f)) return false;
        if (std::abs(p.x)<5.1f && p.y> -9.75f && p.y< -2.87f) return false;
        const auto cerca=[p](glm::vec3 q,float margen) { return glm::distance(p,glm::vec2(q.x,q.z))<margen; };
        for (const auto& casa : casas) if (contiene(casa.huella,p,0.37f)) return false;
        for (const auto& r : tramo.rincones) if (&r!=propio && cerca(r.centro,r.radio+0.30f)) return false;
        for (const auto& f : tramo.faroles) if (cerca(f.base,0.48f)) return false;
        for (const auto& m : tramo.macetas) if (cerca(m,0.60f)) return false;
        for (const auto& a : tramo.arboles) if (cerca(a,0.57f)) return false;
        for (const auto& a : arbolesPlaza) if (glm::distance(a,p)<0.59f) return false;
        for (const auto& c : tramo.tendederos) if (cerca(c[0],0.43f) || cerca(c[1],0.43f)) return false;
        for (const auto& c : papelPlaza()) if (cerca(c[0],0.43f) || cerca(c[1],0.43f)) return false;
        for (const auto& c : conjuntos) if (glm::distance(c.centro,p)<c.radio+0.34f) return false;
        for (const auto& h : gente) if (cerca(h.base,0.82f)) return false;
        for (const auto& ruta : rutas) {
            for (size_t i=1; i<ruta.size(); ++i) {
                const auto a=ruta[i-1].posicion;
                const auto delta=ruta[i].posicion-a;
                const float t=glm::clamp(glm::dot(p-a,delta)/glm::dot(delta,delta),0.0f,1.0f);
                if (glm::distance(p,a+delta*t)<0.86f) return false;
            }
        }
        if (enMuelle(p)) {
            return p.y<finMuelle-0.36f && std::abs(p.x)<(p.y<inicioPlataforma ? 2.38f : 4.92f);
        }
        return glm::distance(p,puntoMasCercano(p,callePrincipal))>1.12f
            && glm::distance(p,puntoMasCercano(p,callejonOeste))>0.85f
            && glm::distance(p,puntoMasCercano(p,callejonEste))>0.85f;
    };
    for (const auto& r : tramo.rincones) {
        const glm::vec2 centro(r.centro.x,r.centro.z), lateral(r.frente.y,-r.frente.x);
        if (esPuesto(r.uso)) {
            const float escala=std::min(1.0f,r.radio/1.20f);
            const bool tacos=r.uso==UsoRincon::Tacos;
            const std::array<glm::vec2,5> candidatos{{
                {0.08f,-0.61f*escala-0.28f},{0.74f*escala+0.29f,-0.21f},
                {-0.74f*escala-0.29f,-0.21f},{0.66f*escala+0.32f,-0.55f},
                {-0.66f*escala-0.32f,-0.55f}
            }};
            for (size_t i=tacos ? 0 : 1; i<candidatos.size(); ++i) {
                const auto q=candidatos[i];
                const auto p=centro+lateral*q.x+r.frente*q.y;
                if (!libre(p,&r)) continue;
                const auto frente=glm::normalize(centro+r.frente*0.28f-p);
                gente.push_back({punto(p,alturaCaminata(p)),frente,Actividad::Vender});
                break;
            }
            for (float lado : {-1.0f,1.0f}) {
                const auto p=centro+r.frente*(0.48f*escala+0.53f)+lateral*(lado*0.38f);
                if (libre(p,&r)) gente.push_back({punto(p,alturaCaminata(p)),glm::normalize(centro-p),Actividad::Conversar});
            }
        }
        if (r.uso==UsoRincon::Descanso) {
            const float escala=glm::clamp(r.radio/1.56f,0.58f,1.16f);
            const auto p=centro+r.frente*(-0.48f*escala);
            const float suelo=alturaCaminata(p+r.frente*0.40f);
            if (libre(p,&r)) gente.push_back({punto(p,suelo),r.frente,Actividad::Descansar,r.centro.y+0.45f*escala-suelo});
            const auto visita=centro+lateral*0.33f+r.frente*(r.radio*0.47f+0.61f);
            if (libre(visita,&r)) gente.push_back({punto(visita,alturaCaminata(visita)),glm::normalize(p-visita),Actividad::Conversar});
        }
        if (r.uso==UsoRincon::Jardineras) {
            const auto medio=centro+r.frente*(0.73f*r.radio+0.66f);
            const auto a=medio-lateral*0.46f, b=medio+lateral*0.46f;
            if (libre(a,&r) && libre(b,&r)) {
                gente.push_back({punto(a,alturaCaminata(a)),lateral,Actividad::Conversar});
                gente.push_back({punto(b,alturaCaminata(b)),-lateral,Actividad::Conversar});
            }
        }
    }
    return gente;
}

void dibujarHabitante(Mesh& mesh,const Habitante& persona,size_t id,float reloj)
{
    PosturaPersona pose;
    const bool vende=persona.actividad==Actividad::Vender;
    pose.mandil=vende;
    pose.sentado=persona.actividad==Actividad::Descansar;
    pose.ajustarSuelo=!pose.sentado;
    pose.alturaAsiento=glm::clamp(persona.asiento/tallaPersona(id),0.28f,0.57f);
    const float gesto=std::sin(reloj*(vende ? 2.3f : 1.6f)+0.87f*id);
    if (pose.sentado) {
        pose.izquierda={-0.14f,0.97f,0.39f};
        pose.derecha={0.14f,0.97f,0.39f};
    } else {
        pose.izquierda={-0.23f,0.92f+(vende ? 0.09f*gesto : 0.0f),vende ? 0.27f : 0.10f};
        pose.derecha={0.20f,1.05f+0.095f*gesto,0.27f+0.065f*gesto};
    }
    const auto manos=dibujarPersona(mesh,persona.base,persona.frente,id,pose);
    if (persona.actividad==Actividad::Conversar && id%3==0) {
        anillo(mesh,manos.derecha,0.018f,0.049f,0.125f,0.069f,{0.83f,0.70f,0.50f},10);
        anillo(mesh,manos.derecha,0.123f,0.069f,0.126f,0.0f,{0.49f,0.31f,0.20f},10);
    }
}

const glm::vec3 vistaAerea(3,90,111);
const glm::vec3 miradaAerea(0,0,0);
constexpr float inclinacionAerea = -34.0f;
const glm::vec3 vistaMuelle(0, alturaMuelle + alturaOjos, finMuelle-0.8f);

struct Vista {
    Camera camera{miradaAerea, glm::vec3(0,1,0), -91.6f, inclinacionAerea};
    glm::vec3 position = vistaAerea;
    bool vuelo = true;
    bool noche = true;
    bool primerMouse = true;
    bool seguirProcesion = false;
    bool seguirLanchas = false;
    double inicioProcesion = -1;
    double inicioLanchas = -1;
    double lastX = 0;
    double lastY = 0;
};

void mirar(GLFWwindow* window, double x, double y)
{
    auto& view = *static_cast<Vista*>(glfwGetWindowUserPointer(window));
    if (!view.primerMouse && !view.seguirProcesion && !view.seguirLanchas) {
        view.camera.ProcessMouseMovement(static_cast<float>(x - view.lastX), static_cast<float>(view.lastY - y));
    }
    view.primerMouse = false;
    view.lastX = x;
    view.lastY = y;
}

void teclas(GLFWwindow* window, int key, int, int action, int)
{
    if (action != GLFW_PRESS) return;
    auto& view = *static_cast<Vista*>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, GLFW_TRUE);
    if (key == GLFW_KEY_P) {
        view.inicioProcesion=glfwGetTime();
        view.seguirProcesion=true;
        view.seguirLanchas=false;
        view.vuelo=true;
        view.primerMouse=true;
    }
    if (key == GLFW_KEY_L) {
        view.inicioLanchas=glfwGetTime();
        view.seguirLanchas=true;
        view.seguirProcesion=false;
        view.vuelo=true;
        view.primerMouse=true;
    }
    if ((key>=GLFW_KEY_1 && key<=GLFW_KEY_9) || key==GLFW_KEY_G
        || key==GLFW_KEY_W || key==GLFW_KEY_A || key==GLFW_KEY_S || key==GLFW_KEY_D) {
        if (view.seguirProcesion || view.seguirLanchas) view.primerMouse=true;
        view.seguirProcesion=false;
        view.seguirLanchas=false;
    }
    if (key == GLFW_KEY_G) view.vuelo = !view.vuelo;
    if (key == GLFW_KEY_N) view.noche = !view.noche;
    if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9) view.primerMouse = true;
    if (key == GLFW_KEY_1) {
        view.position = vistaAerea;
        view.camera = Camera(miradaAerea, glm::vec3(0,1,0), -91.6f, inclinacionAerea);
        view.vuelo = true;
    }
    if (key == GLFW_KEY_2) {
        view.position = vistaMuelle;
        view.camera = Camera();
        view.vuelo = false;
    }
    if (key == GLFW_KEY_3) {
        const glm::vec2 entrada = ofrenda + glm::vec2(1.32f,12.68f);
        view.position = punto(entrada, terreno(entrada) + alturaOjos);
        view.camera = Camera(miradaAerea, glm::vec3(0,1,0), -96.0f, 15.0f);
        view.vuelo = false;
    }
    if (key == GLFW_KEY_4) {
        const glm::vec2 entrada = glm::mix(callejonOeste[2],callejonOeste[3],0.27f);
        view.position = punto(entrada, terreno(entrada) + alturaOjos);
        view.camera = Camera(miradaAerea, glm::vec3(0,1,0), -63.0f, 2.0f);
        view.vuelo = false;
    }
    if (key == GLFW_KEY_5) {
        const glm::vec2 entrada(0.0f,37.42f);
        view.position = punto(entrada,alturaCaminata(entrada)+alturaOjos);
        view.camera = Camera(miradaAerea,glm::vec3(0,1,0),-106.0f,5.0f);
        view.vuelo = false;
    }
    if (key == GLFW_KEY_6) {
        view.position = {6.1f,4.75f,72.2f};
        view.camera = Camera(miradaAerea,glm::vec3(0,1,0),-102.0f,4.0f);
        view.vuelo = true;
    }
    if (key == GLFW_KEY_7) {
        const glm::vec2 entrada=glm::mix(callejonEste[1],callejonEste[2],0.23f);
        view.position=punto(entrada,alturaCaminata(entrada)+alturaOjos);
        view.camera=Camera(miradaAerea,glm::vec3(0,1,0),-26.0f,2.0f);
        view.vuelo=false;
    }
    if (key == GLFW_KEY_8) {
        const glm::vec2 entrada=glm::mix(callePrincipal[4],callePrincipal[5],0.22f);
        view.position=punto(entrada,alturaCaminata(entrada)+alturaOjos);
        view.camera=Camera(miradaAerea,glm::vec3(0,1,0),-86.0f,6.0f);
        view.vuelo=false;
    }
    if (key == GLFW_KEY_9) {
        view.position={7.35f,terreno(plaza)+7.55f,12.20f};
        view.camera=Camera(miradaAerea,glm::vec3(0,1,0),-114.0f,-17.8f);
        view.vuelo=true;
    }
}

void mover(GLFWwindow* window, Vista& view, const std::vector<Vivienda>& casas,
    const DecoracionIsla& tramo,const std::vector<ConjuntoOfrenda>& conjuntos,float dt)
{
    const glm::vec3 front = view.camera.GetFront();
    const glm::vec3 flat = glm::normalize(glm::vec3(front.x, 0, front.z));
    const glm::vec3 right(-flat.z, 0, flat.x);
    const auto pressed = [window](int key) { return glfwGetKey(window, key) == GLFW_PRESS ? 1.0f : 0.0f; };
    const glm::vec3 direction = (view.vuelo ? front : flat) * (pressed(GLFW_KEY_W) - pressed(GLFW_KEY_S))
        + right * (pressed(GLFW_KEY_D) - pressed(GLFW_KEY_A));
    glm::vec3 next = view.position;
    if (glm::length(direction) > 0.0f) {
        next += glm::normalize(direction) * dt * (view.vuelo ? 16.0f : 5.5f);
    }
    if (view.vuelo) {
        view.position = next;
        return;
    }

    glm::vec2 p(next.x, next.z);
    float floor = alturaCaminata(p);
    const bool pared = std::any_of(casas.begin(),casas.end(),[p](const Vivienda& vivienda) {
        return contiene(vivienda.huella,p,0.18f);
    });
    const auto frente=p-ofrenda;
    const auto& nivel=nivelesOfrenda.front();
    const bool altar=std::abs(frente.x)<nivel.ancho+0.17f
        && frente.y>fondoOfrenda-0.17f && frente.y<nivel.frente+0.17f;
    const bool arreglo=ocupaPlaza(p,0.11f) || std::any_of(conjuntos.begin()+conjuntosPlaza.size(),
        conjuntos.end(),[p](const ConjuntoOfrenda& c) { return glm::distance(c.centro,p)<c.radio+0.10f; });
    const bool puesto=std::any_of(tramo.rincones.begin(),tramo.rincones.end(),[p](const Rincon& r) {
        return (esPuesto(r.uso) || r.uso==UsoRincon::Catrina)
            && glm::distance(p,glm::vec2(r.centro.x,r.centro.z))<r.radio+0.10f;
    });
    if (floor < 0.03f || pared || altar || arreglo || puesto || pieDeArco(p,0.16f)) {
        p = {view.position.x, view.position.z};
        floor = alturaCaminata(p);
    }
    view.position = floor >= 0.03f ? punto(p, floor + alturaOjos) : vistaMuelle;
}

void encuadrarProcesion(Vista& view,const RutaProcesion& ruta,float segundos)
{
    const float guia=inicioPeregrino(0)+avanceProcesion(segundos,ruta,0).distancia;
    constexpr float distanciaCamara=5.82f;
    const auto p=puntoProcesion(ruta,guia-distanciaCamara);
    const auto objetivo=puntoProcesion(ruta,guia-0.84f);
    const auto frente=glm::normalize(objetivo-p);
    const glm::vec2 lateral(frente.y,-frente.x);
    const float restante=ruta.back().distancia-guia;
    const float entrada=1.0f-glm::smoothstep(7.0f,15.0f,restante);
    const auto cerca=punto(p+lateral*0.78f,alturaCaminata(p)+2.28f);
    const auto panoramica=punto({4.0f,16.5f},terreno(plaza)+8.9f);
    view.position=glm::mix(cerca,panoramica,entrada)+glm::vec3(0,2.6f*entrada*(1.0f-entrada),0);
    const auto focoPlaza=punto({0,-1.25f},terreno(plaza)+1.25f);
    const auto dir=glm::mix(punto(objetivo,alturaCaminata(objetivo)+0.80f),focoPlaza,entrada)-view.position;
    view.camera=Camera(miradaAerea,glm::vec3(0,1,0),glm::degrees(std::atan2(dir.z,dir.x)),
        glm::degrees(std::atan2(dir.y,glm::length(glm::vec2(dir.x,dir.z)))));
}

void encuadrarLanchas(Vista& view,const std::array<EstadoLancha,cantidadLanchas>& lanchas,float segundos)
{
    const auto barco=lanchas.front().centro;
    const auto segundo=lanchas.back().centro;
    const float abrir=glm::smoothstep(7.0f,15.0f,segundos);
    const float frenteMuelle=glm::smoothstep(17.0f,26.0f,segundos);
    const glm::vec3 vistaAmbas(0.6f,8.8f,std::max(63.4f,segundo.y+13.7f));
    view.position=glm::mix(punto(barco+glm::vec2(-3.6f,10.8f),4.55f),vistaAmbas,abrir);
    const float centroLago=(barco.y+segundo.y)*0.5f-6.0f;
    const auto foco=glm::mix(punto(barco+glm::vec2(2.4f,-5.0f),0.72f),
        glm::vec3(0,1.2f,glm::mix(centroLago,46.1f,frenteMuelle)),abrir);
    const auto dir=foco-view.position;
    view.camera=Camera(miradaAerea,glm::vec3(0,1,0),glm::degrees(std::atan2(dir.z,dir.x)),
        glm::degrees(std::atan2(dir.y,glm::length(glm::vec2(dir.x,dir.z)))));
}
} // namespace

int main()
{
    if (!glfwInit()) {
        std::cerr << "No se pudo iniciar GLFW.\n";
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    GLFWwindow* window = glfwCreateWindow(1200, 800, "Janitzio | 1 Isla  2 Muelle  3 Plaza  4 Oeste  5 Calle  6 Lago  7 Este  8 Subida  9 Ofrenda | P Procesion  L Lanchas  N Dia/Noche", nullptr, nullptr);
    if (!window) {
        std::cerr << "No se pudo abrir una ventana con OpenGL 3.3.\n";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "No se pudo iniciar GLEW.\n";
        glfwTerminate();
        return 1;
    }
    glGetError(); // GLEW puede consultar una extensión del perfil de compatibilidad.
    glfwSwapInterval(1);
    Vista view;
    glfwSetWindowUserPointer(window, &view);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mirar);
    glfwSetKeyCallback(window, teclas);
    glEnable(GL_DEPTH_TEST);

    Shader shader("Shader/mapa.vs", "Shader/mapa.frag");
    GLint linked = GL_FALSE;
    glGetProgramiv(shader.Program, GL_LINK_STATUS, &linked);
    if (!linked) {
        glfwTerminate();
        return 1;
    }
    const auto casas = distribuirViviendas();
    const auto tramo = prepararDecoracion(casas);
    const auto instancias = ubicarModelos(casas,tramo);
    ModelosJanitzio modelos;
    const bool modelosCargados = modelos.cargar(instancias);
    if (modelosCargados) {
        const size_t antiguas = instancias[CasaAntigua].size();
        const size_t rusticas = instancias[CasaRustica].size()+instancias[RusticaCompacta].size();
        const size_t hoi = instancias[CasaHoiAn].size()+instancias[HoiAnCompacta].size();
        std::cout << "Viviendas importadas: " << antiguas+rusticas+hoi << " de " << casas.size()
            << " (antiguas: " << antiguas << ", rusticas: " << rusticas << ", Hoi An: " << hoi << ')' << std::endl;
    }
    std::cout << "Decoracion de la isla: " << tramo.faroles.size() << " faroles, " << tramo.tendederos.size()
        << " tendederos, " << tramo.macetas.size() << " macetas, " << tramo.lucesRibera.size()
        << " luces de ribera, " << tramo.lucesFachada.size() << " de fachada y " << tramo.arboles.size()
        << " arces nuevos." << std::endl;
    std::cout << "Muelle ampliado y " << tramo.rincones.size() << " espacios decorados."
        << " Lanchas animadas: " << cantidadLanchas
        << '.' << std::endl;
    const auto conjuntos=distribuirConjuntos(casas,tramo);
    const auto veladoras=colocarVeladoras(conjuntos);
    std::cout << "Ofrenda: " << nivelesOfrenda.size() << " niveles, " << conjuntosPlaza.size()
        << " figuras en la plaza, " << conjuntos.size()-conjuntosPlaza.size()
        << " arreglos en el pueblo y " << veladoras.size() << " veladoras." << std::endl;
    const GLuint mapaLuz=crearMapaLuz(tramo,veladoras);
    const Mesh mesh = generarMapa(casas,instancias,tramo,conjuntos,veladoras,modelosCargados);
    std::array<GLuint,2> vaos{}, vbos{};
    glGenVertexArrays(static_cast<GLsizei>(vaos.size()),vaos.data());
    glGenBuffers(static_cast<GLsizei>(vbos.size()),vbos.data());
    for (size_t i=0; i<vaos.size(); ++i) {
        glBindVertexArray(vaos[i]);
        glBindBuffer(GL_ARRAY_BUFFER,vbos[i]);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<const void*>(offsetof(Vertex,color)));
        glVertexAttribPointer(2,1,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<const void*>(offsetof(Vertex,material)));
        glVertexAttribPointer(3,1,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<const void*>(offsetof(Vertex,piedra)));
        glVertexAttribPointer(10,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<const void*>(offsetof(Vertex,papelUV)));
        for (GLuint atributo : {0,1,2,3,10}) glEnableVertexAttribArray(atributo);
    }
    glBindBuffer(GL_ARRAY_BUFFER,vbos[0]);
    glBufferData(GL_ARRAY_BUFFER,mesh.size()*sizeof(Vertex),mesh.data(),GL_STATIC_DRAW);

    const auto rutasProcesion=prepararProcesion();
    const auto habitantes=distribuirHabitantes(casas,tramo,conjuntos,rutasProcesion);
    std::cout << "Habitantes en los puestos y la plaza: " << habitantes.size() << '.' << std::endl;
    std::array<Mesh,cantidadLanchas> lanchasDecoradas;
    for (size_t i=0; i<lanchasDecoradas.size(); ++i) {
        cascoLancha(lanchasDecoradas[i],i);
        decorarLancha(lanchasDecoradas[i],i);
    }
    Mesh dinamicos;
    dinamicos.reserve(1900*(cantidadPeregrinos+habitantes.size())+24000);

    shader.Use();
    const GLint transform = glGetUniformLocation(shader.Program, "transform");
    const GLint inverseTransform = glGetUniformLocation(shader.Program, "inverseTransform");
    const GLint eye = glGetUniformLocation(shader.Program, "eye");
    const GLint night = glGetUniformLocation(shader.Program, "night");
    const GLint sky = glGetUniformLocation(shader.Program, "sky");
    const GLint imported = glGetUniformLocation(shader.Program,"importedModel");
    const GLint lucesVeladoras=glGetUniformLocation(shader.Program,"candleLights[0]");
    const GLint relojLago=glGetUniformLocation(shader.Program,"lakeTime");
    glUniform1i(glGetUniformLocation(shader.Program,"modelTextures"),0);
    glUniform1i(glGetUniformLocation(shader.Program,"windowLayer"),modelos.capaVentanas());
    glUniform1i(glGetUniformLocation(shader.Program,"lightMap"),1);
    glUniform2fv(glGetUniformLocation(shader.Program,"lightOrigin"),1,glm::value_ptr(origenLuz));
    glUniform2fv(glGetUniformLocation(shader.Program,"lightExtent"),1,glm::value_ptr(extensionLuz));
    const glm::vec3 altar = punto(ofrenda, terreno(ofrenda) + 1.15f);
    glUniform3fv(glGetUniformLocation(shader.Program, "altar"), 1, glm::value_ptr(altar));
    const glm::vec3 statue = punto(cima, terreno(cima) + 3.8f);
    glUniform3fv(glGetUniformLocation(shader.Program, "statue"), 1, glm::value_ptr(statue));
    std::array<glm::vec4,maxReflejos> reflejos{};
    for (size_t i=0; i<2; ++i) {
        const auto& farol=tramo.faroles[i];
        reflejos[i]=glm::vec4(farol.base+glm::vec3(0,farol.altura-0.32f,0),1.12f);
    }
    auto lucesCosteras=tramo.lucesRibera;
    constexpr size_t reflejosNuevos=2*cantidadLanchas;
    constexpr size_t inicioCosta=2+reflejosNuevos;
    const size_t cantidadCosta=std::min(lucesCosteras.size(),maxReflejos-inicioCosta);
    const GLint reflectiveLights=glGetUniformLocation(shader.Program,"reflectionSources[0]");
    glUniform1i(glGetUniformLocation(shader.Program,"reflectionCount"),static_cast<GLint>(cantidadCosta+inicioCosta));
    double last = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        const double now = glfwGetTime();
        const float segundos=view.inicioProcesion<0 ? 0.0f : static_cast<float>(now-view.inicioProcesion);
        const float tiempoLanchas=view.inicioLanchas<0 ? 0.0f : static_cast<float>(now-view.inicioLanchas);
        const float reloj=static_cast<float>(now);
        std::array<EstadoLancha,cantidadLanchas> lanchas;
        for (size_t i=0; i<lanchas.size(); ++i) lanchas[i]=estadoLancha(i,tiempoLanchas,reloj);
        if (view.seguirProcesion) encuadrarProcesion(view,rutasProcesion.front(),segundos);
        else if (view.seguirLanchas) encuadrarLanchas(view,lanchas,tiempoLanchas);
        else mover(window,view,casas,tramo,conjuntos,static_cast<float>(std::min(now-last,0.05)));
        last = now;
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0) {
            glfwWaitEvents();
            continue;
        }
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const float aspect = static_cast<float>(width)/height;
        const float fov = view.vuelo ? glm::clamp(2.0f*std::atan(std::tan(glm::radians(35.0f))/aspect),
            glm::radians(55.0f),glm::radians(100.0f)) : glm::radians(47.0f);
        const glm::mat4 projection = glm::perspective(fov,aspect,0.1f,1100.0f);
        const glm::mat4 camera = glm::lookAt(view.position, view.position + view.camera.GetFront(), glm::vec3(0,1,0));
        const glm::mat4 vp = projection * camera;
        const glm::mat4 inverse = glm::inverse(vp);
        glUniformMatrix4fv(transform, 1, GL_FALSE, glm::value_ptr(vp));
        glUniformMatrix4fv(inverseTransform, 1, GL_FALSE, glm::value_ptr(inverse));
        glUniform3fv(eye, 1, glm::value_ptr(view.position));
        glUniform1i(night, view.noche);
        glUniform1f(relojLago,reloj);
        dinamicos.clear();
        std::array<glm::vec3,cantidadPeregrinos+reflejosNuevos> llamas{};
        for (size_t i=0; i<cantidadPeregrinos; ++i) {
            llamas[i]=dibujarPeregrino(dinamicos,rutasProcesion[i],segundos,i);
        }
        for (size_t i=0; i<lanchas.size(); ++i) {
            dibujarLancha(dinamicos,lanchasDecoradas[i],lanchas[i],i);
            for (size_t j=0; j<2; ++j) {
                const auto llama=puntoLancha(lanchas[i],{0,0.79f,j==0 ? -1.60f : 1.59f});
                llamas[cantidadPeregrinos+i*2+j]=llama;
                reflejos[2+i*2+j]=glm::vec4(llama,1.22f);
            }
        }
        for (size_t i=0; i<habitantes.size(); ++i) {
            if (glm::distance(view.position,habitantes[i].base)<65.0f) {
                dibujarHabitante(dinamicos,habitantes[i],i+25,reloj);
            }
        }
        glUniform3fv(lucesVeladoras,static_cast<GLsizei>(llamas.size()),glm::value_ptr(llamas.front()));
        if (view.noche) {
            const auto distance = [&](glm::vec3 p) {
                const glm::vec2 delta(p.x-view.position.x,p.z-view.position.z);
                return glm::dot(delta,delta);
            };
            std::partial_sort(lucesCosteras.begin(),lucesCosteras.begin()+cantidadCosta,lucesCosteras.end(),
                [&](glm::vec3 a,glm::vec3 b) { return distance(a)<distance(b); });
            for (size_t i=0; i<cantidadCosta; ++i) reflejos[i+inicioCosta]=glm::vec4(lucesCosteras[i],0.59f);
            glUniform4fv(reflectiveLights,static_cast<GLsizei>(cantidadCosta+inicioCosta),glm::value_ptr(reflejos[0]));
        }
        glBindVertexArray(vaos[0]);
        glUniform1i(imported,GL_FALSE);
        glDisable(GL_DEPTH_TEST);
        glUniform1i(sky, GL_TRUE);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glEnable(GL_DEPTH_TEST);
        glUniform1i(sky, GL_FALSE);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mesh.size()));
        glBindVertexArray(vaos[1]);
        glBindBuffer(GL_ARRAY_BUFFER,vbos[1]);
        glBufferData(GL_ARRAY_BUFFER,dinamicos.size()*sizeof(Vertex),dinamicos.data(),GL_STREAM_DRAW);
        glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(dinamicos.size()));
        if (modelosCargados) {
            glUniform1i(imported,GL_TRUE);
            modelos.dibujar();
        }
        glfwSwapBuffers(window);
    }
    glDeleteBuffers(static_cast<GLsizei>(vbos.size()),vbos.data());
    glDeleteTextures(1,&mapaLuz);
    glDeleteVertexArrays(static_cast<GLsizei>(vaos.size()),vaos.data());
    glDeleteProgram(shader.Program);
    modelos.liberar();
    glfwTerminate();
}
