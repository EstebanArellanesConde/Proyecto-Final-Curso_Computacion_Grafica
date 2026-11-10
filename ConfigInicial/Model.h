#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// ============================================================
// CARGA DE TEXTURAS: stb_image en vez de SOIL2
// ============================================================
//
// SOIL2 es una librería vieja y poco mantenida, con bugs
// conocidos al decodificar ciertos JPEGs (progresivos, CMYK,
// etc.) que pueden corromper memoria en vez de fallar limpio.
// Eso encaja con el crash que aparece solo al usar varios
// archivos .jpg reales distintos (con un solo archivo repetido
// el caché de texturas_loaded nunca vuelve a decodificar nada).
//
// stb_image es más moderna y maneja mucho mejor esos casos.
//
// IMPORTANTE: en exactamente UN .cpp de tu proyecto (usa
// main.cpp) debe existir, ANTES de incluir stb_image.h:
//
//     #define STB_IMAGE_IMPLEMENTATION
//     #include "stb_image.h"
//
// Aquí en Model.h solo se incluye la declaración (sin el
// #define), porque la implementación ya se compiló una vez
// en main.cpp. Si el #define se pone en más de un .cpp,
// obtendrás errores de "symbol already defined" al enlazar.
//
// ============================================================

#include "stb_image.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"
#include "Shader.h"

using namespace std;


/*
====================================================================
TEXTURE LOADING FUNCTION
====================================================================
*/

GLint TextureFromFile(const char* path, string directory);


/*
====================================================================
MODEL CLASS
====================================================================
*/

class Model
{
public:

    /*
    Constructor
    */
    Model(GLchar* path)
    {
        this->loadModel(path);
    }


    /*
    Draw all meshes belonging to the model.
    */
    void Draw(Shader shader)
    {
        for (GLuint i = 0; i < this->meshes.size(); i++)
        {
            this->meshes[i].Draw(shader);
        }
    }


    /*
    Draw only the opaque meshes (opacity >= 1.0).
    Call this first, with blending disabled.
    */
    void DrawOpaque(Shader shader)
    {
        for (GLuint i = 0; i < this->meshes.size(); i++)
        {
            if (this->meshes[i].opacity >= 1.0f)
            {
                this->meshes[i].Draw(shader);
            }
        }
    }


    /*
    Draw only the transparent meshes (opacity < 1.0).
    Call this last, with blending enabled.
    */
    void DrawTransparent(Shader shader)
    {
        for (GLuint i = 0; i < this->meshes.size(); i++)
        {
            if (this->meshes[i].opacity < 1.0f)
            {
                this->meshes[i].Draw(shader);
            }
        }
    }


    /*
    Meshes belonging to the model.

    Público para poder recorrer sus vértices desde main.cpp
    (por ejemplo, para calcular el bounding box y alinear el
    modelo al piso).
    */
    vector<Mesh> meshes;


private:

    /*
    ================================================================
    MODEL DATA
    ================================================================
    */

    string directory;

    /*
    Stores textures already loaded so we don't load
    the same texture multiple times.
    */
    vector<Texture> textures_loaded;


    /*
    ================================================================
    LOAD MODEL
    ================================================================
    */

    void loadModel(string path)
    {
        Assimp::Importer importer;

        const aiScene* scene =
            importer.ReadFile(
                path,
                aiProcess_Triangulate |
                aiProcess_FlipUVs |
                aiProcess_GenSmoothNormals
            );


        /*
        Check for errors.
        */
        if (
            !scene ||
            scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
            !scene->mRootNode
            )
        {
            cout
                << "ERROR::ASSIMP:: "
                << importer.GetErrorString()
                << endl;

            return;
        }


        /*
        Retrieve directory containing the model.
        */
        this->directory =
            path.substr(
                0,
                path.find_last_of("/\\")
            );


        cout << endl;
        cout << "============================================" << endl;
        cout << "MODEL LOADED" << endl;
        cout << "============================================" << endl;

        cout << "Model: " << path << endl;
        cout << "Directory: " << this->directory << endl;

        cout
            << "Meshes: "
            << scene->mNumMeshes
            << endl;

        cout
            << "Materials: "
            << scene->mNumMaterials
            << endl;

        cout << "============================================" << endl;


        /*
        Process root node recursively.
        */
        this->processNode(
            scene->mRootNode,
            scene
        );
    }


    /*
    ================================================================
    PROCESS NODE
    ================================================================
    */

    void processNode(
        aiNode* node,
        const aiScene* scene
    )
    {
        /*
        Process meshes belonging to this node.
        */
        for (GLuint i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh =
                scene->mMeshes[
                    node->mMeshes[i]
                ];

            this->meshes.push_back(
                this->processMesh(
                    mesh,
                    scene
                )
            );
        }


        /*
        Process children recursively.
        */
        for (GLuint i = 0; i < node->mNumChildren; i++)
        {
            this->processNode(
                node->mChildren[i],
                scene
            );
        }
    }


    /*
    ================================================================
    PROCESS MESH
    ================================================================
    */

    Mesh processMesh(
        aiMesh* mesh,
        const aiScene* scene
    )
    {
        /*
        Data to fill.
        */
        vector<Vertex> vertices;
        vector<GLuint> indices;
        vector<Texture> textures;


        /*
        Default material color.
        */
        glm::vec3 diffuseColor(
            1.0f,
            1.0f,
            1.0f
        );

        /*
        Default opacity (opaque).
        */
        float opacity = 1.0f;


        /*
        ============================================================
        VERTICES
        ============================================================
        */

        for (GLuint i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;


            /*
            --------------------------------------------------------
            POSITION
            --------------------------------------------------------
            */

            vertex.Position.x =
                mesh->mVertices[i].x;

            vertex.Position.y =
                mesh->mVertices[i].y;

            vertex.Position.z =
                mesh->mVertices[i].z;


            /*
            --------------------------------------------------------
            NORMAL
            --------------------------------------------------------
            */

            if (mesh->HasNormals())
            {
                vertex.Normal.x =
                    mesh->mNormals[i].x;

                vertex.Normal.y =
                    mesh->mNormals[i].y;

                vertex.Normal.z =
                    mesh->mNormals[i].z;
            }
            else
            {
                vertex.Normal =
                    glm::vec3(
                        0.0f,
                        0.0f,
                        0.0f
                    );
            }


            /*
            --------------------------------------------------------
            TEXTURE COORDINATES
            --------------------------------------------------------
            */

            if (mesh->mTextureCoords[0])
            {
                vertex.TexCoords.x =
                    mesh->mTextureCoords[0][i].x;

                vertex.TexCoords.y =
                    mesh->mTextureCoords[0][i].y;
            }
            else
            {
                vertex.TexCoords =
                    glm::vec2(
                        0.0f,
                        0.0f
                    );
            }


            vertices.push_back(vertex);
        }


        /*
        ============================================================
        INDICES
        ============================================================
        */

        for (GLuint i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face =
                mesh->mFaces[i];

            for (
                GLuint j = 0;
                j < face.mNumIndices;
                j++
                )
            {
                indices.push_back(
                    face.mIndices[j]
                );
            }
        }


        /*
        ============================================================
        MATERIAL
        ============================================================
        */

        if (mesh->mMaterialIndex >= 0)
        {
            aiMaterial* material =
                scene->mMaterials[
                    mesh->mMaterialIndex
                ];


            /*
            --------------------------------------------------------
            MATERIAL NAME
            --------------------------------------------------------
            */

            aiString materialName;

            material->Get(
                AI_MATKEY_NAME,
                materialName
            );


            /*
            --------------------------------------------------------
            DIFFUSE COLOR (Kd)
            --------------------------------------------------------

            This is the important part for car_project.mtl.

            Example:

            Kd 0.752931 0.000000 0.003627

            becomes:

            diffuseColor =
                vec3(
                    0.752931,
                    0.0,
                    0.003627
                );
            --------------------------------------------------------
            */

            aiColor3D color;

            if (
                material->Get(
                    AI_MATKEY_COLOR_DIFFUSE,
                    color
                ) == AI_SUCCESS
                )
            {
                diffuseColor =
                    glm::vec3(
                        color.r,
                        color.g,
                        color.b
                    );
            }


            /*
            --------------------------------------------------------
            OPACITY (d)

            Example:  d 0.3  ->  opacity = 0.3
            --------------------------------------------------------
            */

            float d = 1.0f;

            if (
                material->Get(
                    AI_MATKEY_OPACITY,
                    d
                ) == AI_SUCCESS
                )
            {
                opacity = d;
            }


            /*
            --------------------------------------------------------
            DEBUG INFORMATION
            --------------------------------------------------------
            */

            cout
                << "Mesh material: "
                << materialName.C_Str()
                << " | Kd: "
                << diffuseColor.r
                << ", "
                << diffuseColor.g
                << ", "
                << diffuseColor.b
                << " | d: "
                << opacity
                << endl;


            /*
            --------------------------------------------------------
            DIFFUSE TEXTURES
            --------------------------------------------------------

            This will still work for RedDog.mtl if it contains:

            map_Kd Texture_albedo.jpg
            --------------------------------------------------------
            */

            vector<Texture> diffuseMaps =
                this->loadMaterialTextures(
                    material,
                    aiTextureType_DIFFUSE,
                    "texture_diffuse"
                );

            textures.insert(
                textures.end(),
                diffuseMaps.begin(),
                diffuseMaps.end()
            );


            /*
            --------------------------------------------------------
            SPECULAR TEXTURES
            --------------------------------------------------------
            */

            vector<Texture> specularMaps =
                this->loadMaterialTextures(
                    material,
                    aiTextureType_SPECULAR,
                    "texture_specular"
                );

            textures.insert(
                textures.end(),
                specularMaps.begin(),
                specularMaps.end()
            );
        }


        /*
        ============================================================
        CREATE MESH
        ============================================================
        */

        return Mesh(
            vertices,
            indices,
            textures,
            diffuseColor,
            opacity
        );
    }


    /*
    ================================================================
    LOAD MATERIAL TEXTURES
    ================================================================
    */

    vector<Texture> loadMaterialTextures(
        aiMaterial* mat,
        aiTextureType type,
        string typeName
    )
    {
        vector<Texture> textures;


        /*
        Check every texture of this type.
        */
        for (
            GLuint i = 0;
            i < mat->GetTextureCount(type);
            i++
            )
        {
            aiString str;

            mat->GetTexture(
                type,
                i,
                &str
            );


            /*
            Check whether this texture was already loaded.
            */
            GLboolean skip = false;

            for (
                GLuint j = 0;
                j < textures_loaded.size();
                j++
                )
            {
                if (
                    textures_loaded[j].path
                    == str
                    )
                {
                    textures.push_back(
                        textures_loaded[j]
                    );

                    skip = true;

                    break;
                }
            }


            /*
            Load new texture.
            */
            if (!skip)
            {
                Texture texture;


                texture.id =
                    TextureFromFile(
                        str.C_Str(),
                        this->directory
                    );


                /*
                Si TextureFromFile falló (id == 0), no la
                metemos al caché ni a la lista de texturas de
                este mesh: así el material simplemente se
                dibuja con su color Kd (materialDiffuse) en
                vez de con una textura rota, sin tronar el
                programa.
                */
                if (texture.id == 0)
                {
                    continue;
                }


                texture.type =
                    typeName;


                texture.path =
                    str;


                textures.push_back(
                    texture
                );


                textures_loaded.push_back(
                    texture
                );


                cout
                    << "Texture loaded: "
                    << str.C_Str()
                    << endl;
            }
        }


        return textures;
    }
};


/*
====================================================================
TEXTURE FROM FILE (stb_image)
====================================================================
*/

GLint TextureFromFile(
    const char* path,
    string directory
)
{
    /*
    Construct complete filename.
    */
    string filename =
        string(path);


    /*
    If Assimp gives an absolute path,
    don't prepend the model directory.
    */
    if (
        filename.length() > 1 &&
        filename[1] == ':'
        )
    {
        // Already absolute Windows path.
    }
    else
    {
        filename =
            directory +
            "/" +
            filename;
    }


    cout
        << "Loading texture: "
        << filename
        << endl;


    /*
    Generate OpenGL texture ID.
    */
    GLuint textureID;

    glGenTextures(
        1,
        &textureID
    );


    /*
    Load image with stb_image.

    A diferencia de SOIL_load_image (que aquí siempre se
    forzaba a SOIL_LOAD_RGB), pedimos a stbi_load que nos
    diga cuántos canales tiene realmente la imagen (1, 3 o 4)
    para elegir el formato de OpenGL correcto, y así también
    soportar texturas con canal alfa (tu shader ya revisa
    texColor.a para hacer discard).
    */

    int width = 0;
    int height = 0;
    int channels = 0;

    unsigned char* image =
        stbi_load(
            filename.c_str(),
            &width,
            &height,
            &channels,
            0
        );


    /*
    Check image loading.
    */
    if (!image)
    {
        cout
            << "ERROR::TEXTURE::FAILED_TO_LOAD "
            << filename
            << " | stbi reason: "
            << stbi_failure_reason()
            << endl;

        glDeleteTextures(
            1,
            &textureID
        );

        return 0;
    }


    /*
    Elegir formato según el número real de canales.
    */
    GLenum format = GL_RGB;

    if (channels == 1)
    {
        format = GL_RED;
    }
    else if (channels == 3)
    {
        format = GL_RGB;
    }
    else if (channels == 4)
    {
        format = GL_RGBA;
    }


    /*
    Bind texture.
    */
    glBindTexture(
        GL_TEXTURE_2D,
        textureID
    );


    /*
    Filas de la imagen alineadas a 1 byte, por si el ancho
    no es múltiplo de 4 (evita texturas "inclinadas"/corridas
    con ciertos anchos impares).
    */
    glPixelStorei(
        GL_UNPACK_ALIGNMENT,
        1
    );


    /*
    Upload image.
    */
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        format,
        width,
        height,
        0,
        format,
        GL_UNSIGNED_BYTE,
        image
    );


    /*
    Generate mipmaps.
    */
    glGenerateMipmap(
        GL_TEXTURE_2D
    );


    /*
    Texture wrapping.
    */
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_REPEAT
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_REPEAT
    );


    /*
    Texture filtering.
    */
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );


    /*
    Cleanup.
    */
    glBindTexture(
        GL_TEXTURE_2D,
        0
    );

    stbi_image_free(
        image
    );


    return textureID;
}