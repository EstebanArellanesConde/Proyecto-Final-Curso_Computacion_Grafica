#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Shader.h"

using namespace std;

struct Vertex
{
    // Position
    glm::vec3 Position;

    // Normal
    glm::vec3 Normal;

    // Texture Coordinates
    glm::vec2 TexCoords;
};

struct Texture
{
    GLuint id;
    string type;
    aiString path;
};

class Mesh
{
public:

    /* Mesh Data */
    vector<Vertex> vertices;
    vector<GLuint> indices;
    vector<Texture> textures;

    // Diffuse material color obtained from the .mtl file (Kd)
    glm::vec3 diffuseColor;

    // Opacity obtained from the .mtl file (d). 1.0 = opaque, 0.0 = invisible
    float opacity;

    /* Functions */

    // Constructor
    Mesh(
        vector<Vertex> vertices,
        vector<GLuint> indices,
        vector<Texture> textures,
        glm::vec3 diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f),
        float opacity = 1.0f
    )
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;
        this->diffuseColor = diffuseColor;
        this->opacity = opacity;

        // Initialize buffers
        this->setupMesh();
    }

    // Render the mesh
    void Draw(Shader shader)
    {
        /*
        ============================================================
        TEXTURES
        ============================================================
        */

        GLuint diffuseNr = 1;
        GLuint specularNr = 1;

        bool hasDiffuseTexture = false;

        for (GLuint i = 0; i < this->textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);

            stringstream ss;
            string number;

            string name = this->textures[i].type;

            if (name == "texture_diffuse")
            {
                ss << diffuseNr++;
                hasDiffuseTexture = true;
            }
            else if (name == "texture_specular")
            {
                ss << specularNr++;
            }

            number = ss.str();

            /*
            Set sampler to the correct texture unit.
            */
            GLint location =
                glGetUniformLocation(
                    shader.Program,
                    (name + number).c_str()
                );

            if (location != -1)
            {
                glUniform1i(location, i);
            }

            /*
            Bind texture.
            */
            glBindTexture(
                GL_TEXTURE_2D,
                this->textures[i].id
            );
        }

        /*
        ============================================================
        MATERIAL
        ============================================================
        */

        /*
        Tell the fragment shader whether this mesh has a
        diffuse texture or not.
        */
        GLint hasTextureLocation =
            glGetUniformLocation(
                shader.Program,
                "hasTexture"
            );

        if (hasTextureLocation != -1)
        {
            glUniform1i(
                hasTextureLocation,
                hasDiffuseTexture ? 1 : 0
            );
        }

        /*
        Send the Kd color from the .mtl file.
        */
        GLint diffuseColorLocation =
            glGetUniformLocation(
                shader.Program,
                "materialDiffuse"
            );

        if (diffuseColorLocation != -1)
        {
            glUniform3fv(
                diffuseColorLocation,
                1,
                glm::value_ptr(this->diffuseColor)
            );
        }

        /*
        Send the opacity (d) from the .mtl file.
        */
        GLint opacityLocation =
            glGetUniformLocation(
                shader.Program,
                "opacity"
            );

        if (opacityLocation != -1)
        {
            glUniform1f(
                opacityLocation,
                this->opacity
            );
        }

        /*
        Keep compatibility with the old shader.
        */
        GLint shininessLocation =
            glGetUniformLocation(
                shader.Program,
                "material.shininess"
            );

        if (shininessLocation != -1)
        {
            glUniform1f(
                shininessLocation,
                16.0f
            );
        }

        /*
        ============================================================
        DRAW
        ============================================================
        */

        glBindVertexArray(this->VAO);

        glDrawElements(
            GL_TRIANGLES,
            static_cast<GLsizei>(this->indices.size()),
            GL_UNSIGNED_INT,
            0
        );

        glBindVertexArray(0);

        /*
        ============================================================
        CLEANUP TEXTURES
        ============================================================
        */

        for (GLuint i = 0; i < this->textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        // Return to texture unit 0
        glActiveTexture(GL_TEXTURE0);
    }

private:

    /* Render data */
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;

    /* Functions */

    void setupMesh()
    {
        /*
        ============================================================
        CREATE BUFFERS
        ============================================================
        */

        glGenVertexArrays(1, &this->VAO);
        glGenBuffers(1, &this->VBO);
        glGenBuffers(1, &this->EBO);

        glBindVertexArray(this->VAO);

        /*
        ============================================================
        VERTEX BUFFER
        ============================================================
        */

        glBindBuffer(GL_ARRAY_BUFFER, this->VBO);

        glBufferData(
            GL_ARRAY_BUFFER,
            this->vertices.size() * sizeof(Vertex),
            &this->vertices[0],
            GL_STATIC_DRAW
        );

        /*
        ============================================================
        INDEX BUFFER
        ============================================================
        */

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);

        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            this->indices.size() * sizeof(GLuint),
            &this->indices[0],
            GL_STATIC_DRAW
        );

        /*
        ============================================================
        VERTEX ATTRIBUTES
        ============================================================
        */

        // Position
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            (GLvoid*)0
        );

        // Normal
        glEnableVertexAttribArray(1);

        glVertexAttribPointer(
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            (GLvoid*)offsetof(Vertex, Normal)
        );

        // Texture coordinates
        glEnableVertexAttribArray(2);

        glVertexAttribPointer(
            2,
            2,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            (GLvoid*)offsetof(Vertex, TexCoords)
        );

        /*
        Unbind VAO.
        */
        glBindVertexArray(0);
    }
};