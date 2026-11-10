/*
    ============================================================
    PIRÁMIDE DE KUKULKÁN - VOXEL ART
    ============================================================

    Autor: Arellanes Conde Esteban
    Práctica #04
    CGeIHC (L+) - Grupo: 05
    #cta: 319322743
    Fecha: 11/09/2026

    Basado en:
    - GLFW
    - GLEW
    - GLM
    - OpenGL 3.3

    La escena representa la Pirámide de Kukulkán
    de Chichén Itzá utilizando únicamente cubos.

    Elementos:
    - Terreno
    - Pirámide escalonada
    - Cuatro escalinatas
    - Templo superior
    - Columnas
    - Detalles decorativos
    - Vegetación voxel
*/

#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Shader
#include "Shader.h"


// ============================================================
// CONFIGURACIÓN DE LA VENTANA
// ============================================================

const GLint WIDTH = 1000;
const GLint HEIGHT = 700;


// ============================================================
// FUNCIÓN PRINCIPAL
// ============================================================

int main()
{
    // ========================================================
    // INICIALIZACIÓN DE GLFW
    // ========================================================

    if (!glfwInit())
    {
        std::cout << "Error al inicializar GLFW" << std::endl;
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);


    // ========================================================
    // CREACIÓN DE LA VENTANA
    // ========================================================

    GLFWwindow* window = glfwCreateWindow(
        WIDTH,
        HEIGHT,
        "ESTEBAN ARELLANES CONDE", // Piramide de Kukulkan - Voxel Art
        nullptr,
        nullptr
    );

    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window"
            << std::endl;

        glfwTerminate();

        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);


    // ========================================================
    // INICIALIZACIÓN DE GLEW
    // ========================================================

    glewExperimental = GL_TRUE;

    if (GLEW_OK != glewInit())
    {
        std::cout << "Failed to initialise GLEW"
            << std::endl;

        glfwTerminate();

        return EXIT_FAILURE;
    }


    // ========================================================
    // VIEWPORT
    // ========================================================

    int screenWidth;
    int screenHeight;

    glfwGetFramebufferSize(
        window,
        &screenWidth,
        &screenHeight
    );

    glViewport(
        0,
        0,
        screenWidth,
        screenHeight
    );


    // ========================================================
    // CONFIGURACIÓN DE OPENGL
    // ========================================================

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);

    glBlendFunc(
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA
    );


    // ========================================================
    // SHADER
    // ========================================================

    Shader ourShader(
        "Shader/core.vs",
        "Shader/core.frag"
    );


    // ========================================================
    // VÉRTICES DEL CUBO
    //
    // Cada vértice:
    //
    // X Y Z R G B
    //
    // 36 vértices
    // 12 triángulos
    // 6 caras
    // ========================================================

    float vertices[] =
    {
        // ====================================================
        // FRENTE
        // ====================================================

        -0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 1.0f,

         0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 1.0f,


        // ====================================================
        // ATRÁS
        // ====================================================

        -0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 1.0f,

         0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 1.0f,


        // ====================================================
        // DERECHA
        // ====================================================

         0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 1.0f,

         0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 1.0f,


         // ====================================================
         // IZQUIERDA
         // ====================================================

         -0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
         -0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
         -0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 1.0f,

         -0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
         -0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
         -0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 1.0f,


         // ====================================================
         // ABAJO
         // ====================================================

         -0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
          0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
          0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 1.0f,

          0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
         -0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
         -0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 1.0f,


         // ====================================================
         // ARRIBA
         // ====================================================

         -0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
          0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 1.0f,
          0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 1.0f,

          0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
         -0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 1.0f,
         -0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 1.0f
    };


    // ========================================================
    // VAO Y VBO
    // ========================================================

    GLuint VBO;
    GLuint VAO;

    glGenVertexArrays(
        1,
        &VAO
    );

    glGenBuffers(
        1,
        &VBO
    );


    glBindVertexArray(VAO);

    glBindBuffer(
        GL_ARRAY_BUFFER,
        VBO
    );

    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertices),
        vertices,
        GL_STATIC_DRAW
    );


    // ========================================================
    // ATRIBUTO DE POSICIÓN
    // ========================================================

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        6 * sizeof(GLfloat),
        (GLvoid*)0
    );

    glEnableVertexAttribArray(0);


    // ========================================================
    // ATRIBUTO DE COLOR
    // ========================================================

    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        6 * sizeof(GLfloat),
        (GLvoid*)(3 * sizeof(GLfloat))
    );

    glEnableVertexAttribArray(1);


    glBindBuffer(
        GL_ARRAY_BUFFER,
        0
    );

    glBindVertexArray(0);


    // ========================================================
    // PROYECCIÓN
    // ========================================================

    glm::mat4 projection =
        glm::perspective(
            glm::radians(45.0f),
            (GLfloat)screenWidth /
            (GLfloat)screenHeight,
            0.1f,
            100.0f
        );


    // ========================================================
    // UBICACIONES DE UNIFORMS
    // ========================================================

    GLint modelLoc =
        glGetUniformLocation(
            ourShader.Program,
            "model"
        );

    GLint viewLoc =
        glGetUniformLocation(
            ourShader.Program,
            "view"
        );

    GLint projectionLoc =
        glGetUniformLocation(
            ourShader.Program,
            "projection"
        );

    GLint objectColorLoc =
        glGetUniformLocation(
            ourShader.Program,
            "objectColor"
        );


    // ========================================================
    // PALETA DE COLORES
    // ========================================================

    // Piedra principal
    glm::vec3 piedra =
        glm::vec3(
            0.58f,
            0.55f,
            0.46f
        );

    // Piedra clara
    glm::vec3 piedraClara =
        glm::vec3(
            0.74f,
            0.70f,
            0.58f
        );

    // Piedra iluminada
    glm::vec3 piedraIluminada =
        glm::vec3(
            0.82f,
            0.78f,
            0.66f
        );

    // Piedra oscura
    glm::vec3 piedraOscura =
        glm::vec3(
            0.39f,
            0.38f,
            0.32f
        );

    // Piedra con tono arena
    glm::vec3 piedraArena =
        glm::vec3(
            0.66f,
            0.60f,
            0.46f
        );

    // Césped
    glm::vec3 pasto =
        glm::vec3(
            0.18f,
            0.36f,
            0.13f
        );

    // Césped claro
    glm::vec3 pastoClaro =
        glm::vec3(
            0.29f,
            0.47f,
            0.18f
        );

    // Verde oscuro
    glm::vec3 verdeOscuro =
        glm::vec3(
            0.07f,
            0.20f,
            0.08f
        );

    // Verde medio
    glm::vec3 verdeMedio =
        glm::vec3(
            0.12f,
            0.30f,
            0.10f
        );

    // Madera
    glm::vec3 madera =
        glm::vec3(
            0.28f,
            0.16f,
            0.07f
        );


    // ========================================================
    // FUNCIÓN PARA DIBUJAR UN CUBO
    // ========================================================

    auto dibujarCubo =
        [&](glm::vec3 posicion,
            glm::vec3 escala,
            glm::vec3 color)
        {
            glm::mat4 model =
                glm::mat4(1.0f);

            model =
                glm::translate(
                    model,
                    posicion
                );

            model =
                glm::scale(
                    model,
                    escala
                );

            glUniformMatrix4fv(
                modelLoc,
                1,
                GL_FALSE,
                glm::value_ptr(model)
            );

            glUniform3fv(
                objectColorLoc,
                1,
                glm::value_ptr(color)
            );

            glDrawArrays(
                GL_TRIANGLES,
                0,
                36
            );
        };


    // ========================================================
    // FUNCIÓN PARA DIBUJAR LA ESCALINATA
    // ========================================================

    auto dibujarEscalinata =
        [&](int lado)
        {
            /*
                lado:

                0 = frente
                1 = atrás
                2 = derecha
                3 = izquierda
            */

            const int pasos = 12;

            const float ancho =
                1.5f;

            const float altura =
                0.3f;

            const float profundidad =
                0.55f;


            for (int i = 0; i < pasos; i++)
            {
                float y =
                    0.25f +
                    i * altura;

                float distancia =
                    5.15f -
                    i * 0.42f;


                float largo =
                    profundidad +
                    i * 0.42f;


                glm::vec3 posicion;

                glm::vec3 escala;


                // =================================================
                // FRENTE
                // =================================================

                if (lado == 0)
                {
                    posicion =
                        glm::vec3(
                            0.0f,
                            y,
                            distancia
                        );

                    escala =
                        glm::vec3(
                            ancho,
                            altura,
                            largo
                        );
                }


                // =================================================
                // ATRÁS
                // =================================================

                else if (lado == 1)
                {
                    posicion =
                        glm::vec3(
                            0.0f,
                            y,
                            -distancia
                        );

                    escala =
                        glm::vec3(
                            ancho,
                            altura,
                            largo
                        );
                }


                // =================================================
                // DERECHA
                // =================================================

                else if (lado == 2)
                {
                    posicion =
                        glm::vec3(
                            distancia,
                            y,
                            0.0f
                        );

                    escala =
                        glm::vec3(
                            largo,
                            altura,
                            ancho
                        );
                }


                // =================================================
                // IZQUIERDA
                // =================================================

                else
                {
                    posicion =
                        glm::vec3(
                            -distancia,
                            y,
                            0.0f
                        );

                    escala =
                        glm::vec3(
                            largo,
                            altura,
                            ancho
                        );
                }


                // Alternar ligeramente los tonos
                // para producir aspecto voxel.

                if (i % 3 == 0)
                {
                    dibujarCubo(
                        posicion,
                        escala,
                        piedraIluminada
                    );
                }
                else
                {
                    dibujarCubo(
                        posicion,
                        escala,
                        piedraClara
                    );
                }
            }
        };


    // ========================================================
    // FUNCIÓN PARA VEGETACIÓN VOXEL
    // ========================================================

    auto dibujarArbol =
        [&](glm::vec3 posicion,
            float escala)
        {
            // Tronco

            dibujarCubo(
                posicion +
                glm::vec3(
                    0.0f,
                    1.0f * escala,
                    0.0f
                ),

                glm::vec3(
                    0.35f * escala,
                    2.0f * escala,
                    0.35f * escala
                ),

                madera
            );


            // Copa inferior

            dibujarCubo(
                posicion +
                glm::vec3(
                    0.0f,
                    2.1f * escala,
                    0.0f
                ),

                glm::vec3(
                    1.8f * escala,
                    0.8f * escala,
                    1.8f * escala
                ),

                verdeOscuro
            );


            // Copa superior

            dibujarCubo(
                posicion +
                glm::vec3(
                    0.0f,
                    2.7f * escala,
                    0.0f
                ),

                glm::vec3(
                    1.25f * escala,
                    0.75f * escala,
                    1.25f * escala
                ),

                verdeMedio
            );
        };


    // ========================================================
    // FUNCIÓN PARA DECORACIÓN DE PIEDRA
    // ========================================================

    auto dibujarDetalle =
        [&](float x,
            float y,
            float z,
            float tam)
        {
            dibujarCubo(
                glm::vec3(
                    x,
                    y,
                    z
                ),

                glm::vec3(
                    tam,
                    tam,
                    tam
                ),

                piedraArena
            );
        };


    // ========================================================
    // BUCLE PRINCIPAL
    // ========================================================

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();


        // ====================================================
        // LIMPIAR PANTALLA
        // ====================================================

        glClearColor(
            0.38f,
            0.48f,
            0.54f,
            1.0f
        );

        glClear(
            GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT
        );


        // ====================================================
        // ACTIVAR SHADER
        // ====================================================

        ourShader.Use();


        // ====================================================
        // CÁMARA
        // ====================================================

        glm::mat4 view =
            glm::mat4(1.0f);

        view =
            glm::lookAt(

                // Posición de cámara
                glm::vec3(
                    13.0f,
                    9.5f,
                    15.0f
                ),

                // Punto al que mira
                glm::vec3(
                    0.0f,
                    2.5f,
                    0.0f
                ),

                // Vector arriba
                glm::vec3(
                    0.0f,
                    1.0f,
                    0.0f
                )
            );


        // ====================================================
        // ENVIAR MATRICES
        // ====================================================

        glUniformMatrix4fv(
            viewLoc,
            1,
            GL_FALSE,
            glm::value_ptr(view)
        );

        glUniformMatrix4fv(
            projectionLoc,
            1,
            GL_FALSE,
            glm::value_ptr(projection)
        );


        // ====================================================
        // ACTIVAR VAO
        // ====================================================

        glBindVertexArray(VAO);


        // ====================================================
        // TERRENO
        // ====================================================

        dibujarCubo(

            glm::vec3(
                0.0f,
                -0.55f,
                0.0f
            ),

            glm::vec3(
                24.0f,
                0.7f,
                24.0f
            ),

            pasto
        );


        // ====================================================
        // SEGUNDA CAPA DE TERRENO
        // ====================================================

        dibujarCubo(

            glm::vec3(
                0.0f,
                -0.15f,
                0.0f
            ),

            glm::vec3(
                20.0f,
                0.15f,
                20.0f
            ),

            pastoClaro
        );


        // ====================================================
        // PIRÁMIDE
        //
        // 9 grandes niveles escalonados
        // ====================================================

        const int niveles = 9;

        const float alturaNivel =
            0.48f;

        const float anchoInicial =
            11.5f;

        const float reduccion =
            1.05f;


        for (int nivel = 0;
            nivel < niveles;
            nivel++)
        {
            float ancho =
                anchoInicial -
                nivel * reduccion;


            float y =
                nivel * alturaNivel +
                alturaNivel / 2.0f;


            // ================================================
            // CUERPO PRINCIPAL
            // ================================================

            glm::vec3 colorNivel;


            if (nivel % 4 == 0)
            {
                colorNivel =
                    piedraClara;
            }
            else if (nivel % 4 == 1)
            {
                colorNivel =
                    piedra;
            }
            else if (nivel % 4 == 2)
            {
                colorNivel =
                    piedraArena;
            }
            else
            {
                colorNivel =
                    piedraClara;
            }


            dibujarCubo(

                glm::vec3(
                    0.0f,
                    y,
                    0.0f
                ),

                glm::vec3(
                    ancho,
                    alturaNivel,
                    ancho
                ),

                colorNivel
            );


            // ================================================
            // BORDE FRONTAL
            // ================================================

            dibujarCubo(

                glm::vec3(
                    0.0f,
                    y +
                    alturaNivel * 0.52f,
                    ancho / 2.0f +
                    0.02f
                ),

                glm::vec3(
                    ancho,
                    0.10f,
                    0.16f
                ),

                piedraOscura
            );


            // ================================================
            // BORDE TRASERO
            // ================================================

            dibujarCubo(

                glm::vec3(
                    0.0f,
                    y +
                    alturaNivel * 0.52f,
                    -ancho / 2.0f -
                    0.02f
                ),

                glm::vec3(
                    ancho,
                    0.10f,
                    0.16f
                ),

                piedraOscura
            );


            // ================================================
            // BORDE DERECHO
            // ================================================

            dibujarCubo(

                glm::vec3(
                    ancho / 2.0f +
                    0.02f,
                    y +
                    alturaNivel * 0.52f,
                    0.0f
                ),

                glm::vec3(
                    0.16f,
                    0.10f,
                    ancho
                ),

                piedraOscura
            );


            // ================================================
            // BORDE IZQUIERDO
            // ================================================

            dibujarCubo(

                glm::vec3(
                    -ancho / 2.0f -
                    0.02f,
                    y +
                    alturaNivel * 0.52f,
                    0.0f
                ),

                glm::vec3(
                    0.16f,
                    0.10f,
                    ancho
                ),

                piedraOscura
            );
        }


        // ====================================================
        // CUATRO ESCALINATAS
        // ====================================================

        dibujarEscalinata(0);
        dibujarEscalinata(1);
        dibujarEscalinata(2);
        dibujarEscalinata(3);


        // ====================================================
        // PLATAFORMA SUPERIOR
        // ====================================================

        float temploY =
            niveles * alturaNivel;


        dibujarCubo(

            glm::vec3(
                0.0f,
                temploY,
                0.0f
            ),

            glm::vec3(
                3.15f,
                0.35f,
                3.15f
            ),

            piedraIluminada
        );


        // ====================================================
        // TEMPLO
        // ====================================================

        // Pared izquierda

        dibujarCubo(

            glm::vec3(
                -1.15f,
                temploY + 0.95f,
                0.0f
            ),

            glm::vec3(
                0.35f,
                1.9f,
                2.2f
            ),

            piedra
        );


        // Pared derecha

        dibujarCubo(

            glm::vec3(
                1.15f,
                temploY + 0.95f,
                0.0f
            ),

            glm::vec3(
                0.35f,
                1.9f,
                2.2f
            ),

            piedra
        );


        // Pared trasera

        dibujarCubo(

            glm::vec3(
                0.0f,
                temploY + 0.95f,
                -1.0f
            ),

            glm::vec3(
                2.65f,
                1.9f,
                0.35f
            ),

            piedraOscura
        );


        // ====================================================
        // COLUMNAS
        // ====================================================

        dibujarCubo(

            glm::vec3(
                -0.72f,
                temploY + 0.95f,
                1.15f
            ),

            glm::vec3(
                0.35f,
                1.9f,
                0.35f
            ),

            piedraClara
        );


        dibujarCubo(

            glm::vec3(
                0.72f,
                temploY + 0.95f,
                1.15f
            ),

            glm::vec3(
                0.35f,
                1.9f,
                0.35f
            ),

            piedraClara
        );


        // ====================================================
        // TECHO
        // ====================================================

        dibujarCubo(

            glm::vec3(
                0.0f,
                temploY + 1.95f,
                0.0f
            ),

            glm::vec3(
                3.25f,
                0.38f,
                3.25f
            ),

            piedraIluminada
        );


        // ====================================================
        // REMATE DEL TEMPLO
        // ====================================================

        dibujarCubo(

            glm::vec3(
                0.0f,
                temploY + 2.25f,
                0.0f
            ),

            glm::vec3(
                2.65f,
                0.30f,
                2.65f
            ),

            piedraArena
        );


        // ====================================================
        // PEQUEÑO REMATE SUPERIOR
        // ====================================================

        dibujarCubo(

            glm::vec3(
                0.0f,
                temploY + 2.55f,
                0.0f
            ),

            glm::vec3(
                1.5f,
                0.45f,
                1.5f
            ),

            piedraOscura
        );


        // ====================================================
        // DETALLES DE LOS NIVELES
        // ====================================================

        for (int i = 0; i < 8; i++)
        {
            float ancho =
                anchoInicial -
                i * reduccion;

            float y =
                (i + 1) * alturaNivel +
                0.08f;


            // Frente izquierda

            dibujarDetalle(
                -ancho / 2.0f + 0.45f,
                y,
                ancho / 2.0f + 0.10f,
                0.22f
            );


            // Frente derecha

            dibujarDetalle(
                ancho / 2.0f - 0.45f,
                y,
                ancho / 2.0f + 0.10f,
                0.22f
            );


            // Derecha

            dibujarDetalle(
                ancho / 2.0f + 0.10f,
                y,
                -ancho / 2.0f + 0.45f,
                0.22f
            );


            // Izquierda

            dibujarDetalle(
                -ancho / 2.0f - 0.10f,
                y,
                ancho / 2.0f - 0.45f,
                0.22f
            );
        }


        // ====================================================
        // VEGETACIÓN
        // ====================================================

        dibujarArbol(
            glm::vec3(
                -7.5f,
                0.0f,
                4.5f
            ),
            1.1f
        );


        dibujarArbol(
            glm::vec3(
                7.0f,
                0.0f,
                5.0f
            ),
            0.9f
        );


        dibujarArbol(
            glm::vec3(
                -6.5f,
                0.0f,
                -5.5f
            ),
            0.8f
        );


        dibujarArbol(
            glm::vec3(
                6.0f,
                0.0f,
                -6.0f
            ),
            1.0f
        );


        // ====================================================
        // PEQUEÑOS BLOQUES DE VEGETACIÓN
        // ====================================================

        dibujarCubo(
            glm::vec3(
                -4.8f,
                0.20f,
                7.0f
            ),

            glm::vec3(
                0.7f,
                0.4f,
                0.7f
            ),

            verdeOscuro
        );


        dibujarCubo(
            glm::vec3(
                4.5f,
                0.20f,
                7.5f
            ),

            glm::vec3(
                0.8f,
                0.4f,
                0.8f
            ),

            verdeMedio
        );


        // ====================================================
        // DESACTIVAR VAO
        // ====================================================

        glBindVertexArray(0);


        // ====================================================
        // ACTUALIZAR PANTALLA
        // ====================================================

        glfwSwapBuffers(window);
    }


    // ========================================================
    // LIMPIEZA
    // ========================================================

    glDeleteVertexArrays(
        1,
        &VAO
    );

    glDeleteBuffers(
        1,
        &VBO
    );

    glfwTerminate();


    return EXIT_SUCCESS;
}

