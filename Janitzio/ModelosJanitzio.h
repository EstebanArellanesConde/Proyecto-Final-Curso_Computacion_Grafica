#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "stb_image.h"

enum TipoModelo : size_t {
    CasaAntigua, CasaRustica, CasaHoiAn, ArceUno, ArceDos, RusticaCompacta, HoiAnCompacta,
    Lancha, LanchaConRemos, TotalModelos
};

struct InstanciaModelo {
    glm::vec3 posicion;
    glm::vec3 escala;
    glm::vec2 giro;
};

using InstanciasModelos = std::array<std::vector<InstanciaModelo>, TotalModelos>;

class ModelosJanitzio {
public:
    bool cargar(const InstanciasModelos& instancias)
    {
        std::ifstream file("Assets/modelos.bin", std::ios::binary);
        if (!file) return false;

        char magic[4];
        uint32_t count, size, layers, windows;
        if (!file.read(magic,sizeof(magic)) || std::memcmp(magic,"JTZ3",4)!=0
            || !leer(file,count) || !leer(file,size) || !leer(file,layers) || !leer(file,windows)
            || count!=TotalModelos || size!=512 || layers==0 || layers>64 || windows>=layers) {
            std::cerr << "El paquete de modelos tiene un formato inesperado.\n";
            return false;
        }

        std::array<DatosMalla,TotalModelos> data;
        for (auto& mesh : data) {
            uint32_t vertexCount, indexCount;
            if (!leer(file,vertexCount) || !leer(file,indexCount)
                || vertexCount==0 || vertexCount>500000 || indexCount==0 || indexCount>1500000 || indexCount%3!=0) return false;
            mesh.vertices.resize(vertexCount);
            mesh.indices.resize(indexCount);
            if (!file.read(reinterpret_cast<char*>(mesh.vertices.data()),mesh.vertices.size()*sizeof(VerticeModelo))
                || !file.read(reinterpret_cast<char*>(mesh.indices.data()),mesh.indices.size()*sizeof(uint32_t))
                || *std::max_element(mesh.indices.begin(),mesh.indices.end())>=vertexCount) return false;
        }

        int width, height;
        stbi_uc* pixels = stbi_load("Assets/materiales.png",&width,&height,nullptr,4);
        if (!pixels || width!=static_cast<int>(size) || height!=static_cast<int>(size*layers)) {
            stbi_image_free(pixels);
            std::cerr << "No se pudieron cargar las texturas de las casas y los arces.\n";
            return false;
        }
        glGenTextures(1,&texturas);
        glBindTexture(GL_TEXTURE_2D_ARRAY,texturas);
        glTexImage3D(GL_TEXTURE_2D_ARRAY,0,GL_RGBA8,size,size,layers,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
        stbi_image_free(pixels);
        glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

        for (size_t i=0; i<data.size(); ++i) {
            if (!instancias[i].empty()) preparar(grupos[i],data[i],instancias[i]);
        }
        if (glGetError()!=GL_NO_ERROR) {
            liberar();
            std::cerr << "OpenGL no pudo preparar los modelos.\n";
            return false;
        }
        ventana = static_cast<GLint>(windows);
        return true;
    }

    GLint capaVentanas() const { return ventana; }

    void dibujar() const
    {
        glBindTexture(GL_TEXTURE_2D_ARRAY,texturas);
        for (const auto& g : grupos) {
            if (g.vao==0) continue;
            glBindVertexArray(g.vao);
            glDrawElementsInstanced(GL_TRIANGLES,g.indices,GL_UNSIGNED_INT,nullptr,g.instancias);
        }
    }

    void liberar()
    {
        for (auto& g : grupos) {
            glDeleteBuffers(static_cast<GLsizei>(g.buffers.size()),g.buffers.data());
            glDeleteVertexArrays(1,&g.vao);
            g = {};
        }
        glDeleteTextures(1,&texturas);
        texturas = 0;
        ventana = -1;
    }

private:
    struct VerticeModelo {
        float posicion[3], normal[3], uv[2], material;
    };
    static_assert(sizeof(VerticeModelo)==9*sizeof(float));

    struct DatosMalla {
        std::vector<VerticeModelo> vertices;
        std::vector<uint32_t> indices;
    };
    struct Grupo {
        GLuint vao = 0;
        std::array<GLuint,3> buffers{};
        GLsizei indices = 0;
        GLsizei instancias = 0;
    };

    template<class T>
    static bool leer(std::ifstream& file, T& value)
    {
        return static_cast<bool>(file.read(reinterpret_cast<char*>(&value),sizeof(value)));
    }

    static void atributo(GLuint index, GLint count, GLsizei stride, size_t offset, GLuint divisor = 0)
    {
        glVertexAttribPointer(index,count,GL_FLOAT,GL_FALSE,stride,reinterpret_cast<const void*>(offset));
        glEnableVertexAttribArray(index);
        glVertexAttribDivisor(index,divisor);
    }

    static void preparar(Grupo& group, const DatosMalla& mesh, const std::vector<InstanciaModelo>& instances)
    {
        glGenVertexArrays(1,&group.vao);
        glGenBuffers(static_cast<GLsizei>(group.buffers.size()),group.buffers.data());
        glBindVertexArray(group.vao);
        glBindBuffer(GL_ARRAY_BUFFER,group.buffers[0]);
        glBufferData(GL_ARRAY_BUFFER,mesh.vertices.size()*sizeof(VerticeModelo),mesh.vertices.data(),GL_STATIC_DRAW);
        atributo(0,3,sizeof(VerticeModelo),offsetof(VerticeModelo,posicion));
        atributo(4,3,sizeof(VerticeModelo),offsetof(VerticeModelo,normal));
        atributo(5,2,sizeof(VerticeModelo),offsetof(VerticeModelo,uv));
        atributo(6,1,sizeof(VerticeModelo),offsetof(VerticeModelo,material));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,group.buffers[1]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,mesh.indices.size()*sizeof(uint32_t),mesh.indices.data(),GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER,group.buffers[2]);
        glBufferData(GL_ARRAY_BUFFER,instances.size()*sizeof(InstanciaModelo),instances.data(),GL_STATIC_DRAW);
        atributo(7,3,sizeof(InstanciaModelo),offsetof(InstanciaModelo,posicion),1);
        atributo(8,3,sizeof(InstanciaModelo),offsetof(InstanciaModelo,escala),1);
        atributo(9,2,sizeof(InstanciaModelo),offsetof(InstanciaModelo,giro),1);
        group.indices = static_cast<GLsizei>(mesh.indices.size());
        group.instancias = static_cast<GLsizei>(instances.size());
    }

    std::array<Grupo,TotalModelos> grupos{};
    GLuint texturas = 0;
    GLint ventana = -1;
};
