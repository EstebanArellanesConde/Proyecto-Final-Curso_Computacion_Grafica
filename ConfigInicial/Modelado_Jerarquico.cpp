/*
    Autor: Arellanes Conde Esteban
    Pr�ctica #04
    CGeIHC (L+) - Grupo: 05
    #cta: 319322743
    Fecha: 14/09/2026

    BRAZO ROB�TICO

    Caracter�sticas:
    - Modelado jer�rquico
    - Matrices temporales
    - Restricciones articulares
    - Brazo completo
    - Mano de 5 dedos
    - 4 dedos con 3 falanges (movimiento sincronizado, cierran hacia el frente)
    - Pulgar con 2 falanges (control independiente)

    CONTROLES
    ---------------------------------------------
    Escena:   A/D = X   UP/DOWN = Y   W/S = Z   LEFT/RIGHT = rotar
    Hombro:   R / F
    Codo:     G / T
    Mu�eca:   H / Y
    4 dedos:  Falange 1: J (cerrar) / U (abrir)
              Falange 2: I (cerrar) / K (abrir)
              Falange 3: O (cerrar) / L (abrir)
    Pulgar:   Falange 1: Z (cerrar) / X (abrir)
              Falange 2: C (cerrar) / V (abrir)
    ESC = salir
*/


#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"


// ============================================================
// PROTOTIPOS
// ============================================================

void Inputs(GLFWwindow* window);

float ClampAngle(float value, float minValue, float maxValue);

void DrawCube(
    Shader& shader,
    GLuint VAO,
    GLint modelLoc,
    GLint colorLoc,
    const glm::mat4& model,
    const glm::vec3& color
);

void DrawFinger(
    Shader& shader,
    GLuint VAO,
    GLint modelLoc,
    GLint colorLoc,
    const glm::mat4& baseMatrix,
    const glm::vec3& baseOffset,
    const glm::vec3& axis,
    int numPhalanges,
    const float angles[],
    const float lengths[],
    const float thickness[],
    const glm::vec3 colors[]
);


// ============================================================
// CONFIGURACI�N DE LA VENTANA
// ============================================================

const GLint WIDTH = 1200;
const GLint HEIGHT = 800;


// ============================================================
// MOVIMIENTO DE LA ESCENA
// ============================================================

float movX = 0.0f;
float movY = 0.0f;
float movZ = -11.0f;

float rot = 0.0f;


// ============================================================
// ARTICULACIONES DEL BRAZO
// ============================================================

float hombro = 0.0f;
float codo = 35.0f;
float muneca = 0.0f;


// ============================================================
// ARTICULACIONES DE LOS 4 DEDOS (�ndice, medio, anular, me�ique)
//
// Estos tres valores controlan las falanges correspondientes
// de los cuatro dedos al mismo tiempo.
// ============================================================

float falange1 = 0.0f;
float falange2 = 0.0f;
float falange3 = 0.0f;


// ============================================================
// ARTICULACIONES DEL PULGAR (INDEPENDIENTE, SOLO 2 FALANGES)
// ============================================================

float pulgar1 = 0.0f;
float pulgar2 = 0.0f;


// ============================================================
// EJES DE FLEXI�N
//
// Los 4 dedos largos cierran hacia el frente (+Z):
//   una rotaci�n positiva sobre +Y lleva +X hacia -Z,
//   por eso se usa -Y para cerrar hacia +Z.
//   (Si cerraran hacia atr�s, cambiar a (0, 1, 0)).
//
// El pulgar est� del lado +Y y cierra cruzando la palma hacia -Y
// (hacia los dem�s dedos): eje -Z.
// ============================================================

const glm::vec3 EJE_DEDOS(0.0f, -1.0f, 0.0f);
const glm::vec3 EJE_PULGAR(0.0f, 0.0f, -1.0f);


// ============================================================
// RESTRICCIONES DEL BRAZO
// ============================================================

const float HOMBRO_MIN = -90.0f;
const float HOMBRO_MAX = 90.0f;

const float CODO_MIN = 0.0f;
const float CODO_MAX = 135.0f;

const float MUNECA_MIN = -70.0f;
const float MUNECA_MAX = 70.0f;


// ============================================================
// RESTRICCIONES DE LOS 4 DEDOS
// ============================================================

const float FALANGE1_MIN = 0.0f;
const float FALANGE1_MAX = 90.0f;

const float FALANGE2_MIN = 0.0f;
const float FALANGE2_MAX = 100.0f;

const float FALANGE3_MIN = 0.0f;
const float FALANGE3_MAX = 90.0f;


// ============================================================
// RESTRICCIONES DEL PULGAR
// ============================================================

const float PULGAR1_MIN = 0.0f;
const float PULGAR1_MAX = 80.0f;

const float PULGAR2_MIN = 0.0f;
const float PULGAR2_MAX = 90.0f;


// ============================================================
// RELACI�N MEC�NICA DE LAS FALANGES
// ============================================================

const float FALANGE1_FACTOR = 1.00f;
const float FALANGE2_FACTOR = 0.80f;
const float FALANGE3_FACTOR = 0.60f;

const float PULGAR1_FACTOR = 1.00f;
const float PULGAR2_FACTOR = 0.80f;


// ============================================================
// FUNCI�N CLAMP
// ============================================================

float ClampAngle(float value, float minValue, float maxValue)
{
    return glm::clamp(value, minValue, maxValue);
}


// ============================================================
// FUNCI�N PARA DIBUJAR CUBOS
// ============================================================

void DrawCube(
    Shader& shader,
    GLuint VAO,
    GLint modelLoc,
    GLint colorLoc,
    const glm::mat4& model,
    const glm::vec3& color
)
{
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(colorLoc, 1, glm::value_ptr(color));

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}


// ============================================================
// FUNCI�N PARA DIBUJAR UN DEDO (CADENA JER�RQUICA DE FALANGES)
//
// Cada falange:
//   1) rota sobre su articulaci�n (la rotaci�n se hereda a las siguientes)
//   2) se dibuja centrada a media longitud
//   3) la articulaci�n se desplaza al final de la falange
// ============================================================

void DrawFinger(
    Shader& shader,
    GLuint VAO,
    GLint modelLoc,
    GLint colorLoc,
    const glm::mat4& baseMatrix,
    const glm::vec3& baseOffset,
    const glm::vec3& axis,
    int numPhalanges,
    const float angles[],
    const float lengths[],
    const float thickness[],
    const glm::vec3 colors[]
)
{
    glm::mat4 joint = glm::translate(baseMatrix, baseOffset);

    for (int i = 0; i < numPhalanges; i++)
    {
        // Articulaci�n
        joint = glm::rotate(
            joint,
            glm::radians(angles[i]),
            axis
        );

        // Hueso (matriz temporal, no se hereda)
        glm::mat4 bone = glm::translate(
            joint,
            glm::vec3(lengths[i] * 0.5f, 0.0f, 0.0f)
        );

        bone = glm::scale(
            bone,
            glm::vec3(lengths[i], thickness[i], thickness[i])
        );

        DrawCube(shader, VAO, modelLoc, colorLoc, bone, colors[i]);

        // Siguiente articulaci�n
        joint = glm::translate(
            joint,
            glm::vec3(lengths[i], 0.0f, 0.0f)
        );
    }
}


// ============================================================
// MAIN
// ============================================================

int main()
{
    // ========================================================
    // GLFW
    // ========================================================

    if (!glfwInit())
    {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow* window =
        glfwCreateWindow(
            WIDTH,
            HEIGHT,
            "Brazo Robotico - Esteban Arellanes Conde",
            nullptr,
            nullptr
        );

    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);


    // ========================================================
    // GLEW
    // ========================================================

    glewExperimental = GL_TRUE;

    if (GLEW_OK != glewInit())
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }


    // ========================================================
    // VIEWPORT
    // ========================================================

    int screenWidth;
    int screenHeight;

    glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
    glViewport(0, 0, screenWidth, screenHeight);


    // ========================================================
    // OPENGL
    // ========================================================

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    // ========================================================
    // SHADER
    // ========================================================

    Shader ourShader(
        "Shader/core.vs",
        "Shader/core.frag"
    );


    // ========================================================
    // CUBO
    // ========================================================

    float vertices[] =
    {
        // Frente
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,

         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,

        // Atr�s
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,

         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,

        // Derecha
         0.5f, -0.5f,  0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,

         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,

         // Izquierda
         -0.5f,  0.5f,  0.5f,
         -0.5f,  0.5f, -0.5f,
         -0.5f, -0.5f, -0.5f,

         -0.5f, -0.5f, -0.5f,
         -0.5f, -0.5f,  0.5f,
         -0.5f,  0.5f,  0.5f,

         // Abajo
         -0.5f, -0.5f, -0.5f,
          0.5f, -0.5f, -0.5f,
          0.5f, -0.5f,  0.5f,

          0.5f, -0.5f,  0.5f,
         -0.5f, -0.5f,  0.5f,
         -0.5f, -0.5f, -0.5f,

         // Arriba
         -0.5f,  0.5f, -0.5f,
          0.5f,  0.5f, -0.5f,
          0.5f,  0.5f,  0.5f,

          0.5f,  0.5f,  0.5f,
         -0.5f,  0.5f,  0.5f,
         -0.5f,  0.5f, -0.5f
    };


    // ========================================================
    // VAO / VBO
    // ========================================================

    GLuint VAO;
    GLuint VBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        3 * sizeof(GLfloat),
        (GLvoid*)0
    );

    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    // ========================================================
    // PROYECCI�N
    // ========================================================

    glm::mat4 projection =
        glm::perspective(
            glm::radians(45.0f),
            (GLfloat)screenWidth / (GLfloat)screenHeight,
            0.1f,
            100.0f
        );


    // ========================================================
    // UNIFORMS
    // ========================================================

    GLint modelLoc = glGetUniformLocation(ourShader.Program, "model");
    GLint viewLoc = glGetUniformLocation(ourShader.Program, "view");
    GLint projectionLoc = glGetUniformLocation(ourShader.Program, "projection");
    GLint colorLoc = ourShader.uniformColor;


    // ========================================================
    // DATOS DE LOS DEDOS
    // ========================================================

    // ---------------- PULGAR (2 falanges) -------------------
    const float pulgarLen[2] = { 0.8f, 0.6f };
    const float pulgarThick[2] = { 0.40f, 0.35f };
    const glm::vec3 pulgarColor[2] =
    {
        glm::vec3(0.2f, 0.6f, 0.9f),
        glm::vec3(0.15f, 0.5f, 0.8f)
    };

    // ---------------- �NDICE --------------------------------
    const float indiceLen[3] = { 0.8f, 0.7f, 0.6f };
    const float indiceThick[3] = { 0.32f, 0.30f, 0.28f };
    const glm::vec3 indiceColor[3] =
    {
        glm::vec3(0.1f, 0.7f, 0.9f),
        glm::vec3(0.1f, 0.6f, 0.8f),
        glm::vec3(0.1f, 0.5f, 0.7f)
    };

    // ---------------- MEDIO ---------------------------------
    const float medioLen[3] = { 0.9f, 0.7f, 0.6f };
    const float medioThick[3] = { 0.34f, 0.30f, 0.28f };
    const glm::vec3 medioColor[3] =
    {
        glm::vec3(0.2f, 0.8f, 0.9f),
        glm::vec3(0.1f, 0.65f, 0.8f),
        glm::vec3(0.1f, 0.55f, 0.7f)
    };

    // ---------------- ANULAR --------------------------------
    const float anularLen[3] = { 0.8f, 0.7f, 0.6f };
    const float anularThick[3] = { 0.32f, 0.30f, 0.28f };
    const glm::vec3 anularColor[3] =
    {
        glm::vec3(0.8f, 0.3f, 0.8f),
        glm::vec3(0.7f, 0.2f, 0.7f),
        glm::vec3(0.6f, 0.15f, 0.6f)
    };

    // ---------------- ME�IQUE -------------------------------
    const float meniqueLen[3] = { 0.7f, 0.6f, 0.5f };
    const float meniqueThick[3] = { 0.30f, 0.28f, 0.25f };
    const glm::vec3 meniqueColor[3] =
    {
        glm::vec3(0.9f, 0.3f, 0.7f),
        glm::vec3(0.8f, 0.2f, 0.6f),
        glm::vec3(0.7f, 0.15f, 0.5f)
    };


    // ========================================================
    // LOOP PRINCIPAL
    // ========================================================

    while (!glfwWindowShouldClose(window))
    {
        // ====================================================
        // INPUT
        // ====================================================

        Inputs(window);
        glfwPollEvents();


        // ====================================================
        // LIMPIAR
        // ====================================================

        glClearColor(0.02f, 0.02f, 0.02f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // ====================================================
        // SHADER
        // ====================================================

        ourShader.Use();


        // ====================================================
        // MATRIZ VIEW
        // ====================================================

        glm::mat4 view = glm::mat4(1.0f);

        view = glm::translate(view, glm::vec3(movX, movY, movZ));
        view = glm::rotate(view, glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f));

        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));


        // ====================================================
        // MATRIZ RA�Z
        // ====================================================

        glm::mat4 model = glm::mat4(1.0f);


        // ====================================================
        // HOMBRO
        // ====================================================

        glm::mat4 hombroMatrix = model;

        hombroMatrix = glm::rotate(
            hombroMatrix,
            glm::radians(hombro),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );


        // ====================================================
        // B�CEPS
        // ====================================================

        glm::mat4 bicepsMatrix = hombroMatrix;

        bicepsMatrix = glm::translate(bicepsMatrix, glm::vec3(1.5f, 0.0f, 0.0f));

        glm::mat4 bicepsModel = bicepsMatrix;

        bicepsModel = glm::scale(bicepsModel, glm::vec3(3.0f, 1.0f, 1.0f));

        DrawCube(
            ourShader, VAO, modelLoc, colorLoc,
            bicepsModel,
            glm::vec3(0.1f, 0.8f, 0.2f)
        );


        // ====================================================
        // CODO
        // ====================================================

        glm::mat4 codoMatrix = bicepsMatrix;

        codoMatrix = glm::translate(codoMatrix, glm::vec3(1.5f, 0.0f, 0.0f));

        codoMatrix = glm::rotate(
            codoMatrix,
            glm::radians(codo),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );


        // ====================================================
        // ANTEBRAZO
        // ====================================================

        glm::mat4 antebrazoMatrix = codoMatrix;

        antebrazoMatrix = glm::translate(antebrazoMatrix, glm::vec3(1.0f, 0.0f, 0.0f));

        glm::mat4 antebrazoModel = antebrazoMatrix;

        antebrazoModel = glm::scale(antebrazoModel, glm::vec3(2.0f, 1.0f, 1.0f));

        DrawCube(
            ourShader, VAO, modelLoc, colorLoc,
            antebrazoModel,
            glm::vec3(0.8f, 0.15f, 0.15f)
        );


        // ====================================================
        // MU�ECA
        // ====================================================

        glm::mat4 munecaMatrix = antebrazoMatrix;

        munecaMatrix = glm::translate(munecaMatrix, glm::vec3(1.0f, 0.0f, 0.0f));

        munecaMatrix = glm::rotate(
            munecaMatrix,
            glm::radians(muneca),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );


        // ====================================================
        // PALMA
        // ====================================================

        glm::mat4 palmaMatrix = munecaMatrix;

        palmaMatrix = glm::translate(palmaMatrix, glm::vec3(0.4f, 0.0f, 0.0f));

        glm::mat4 palmaModel = palmaMatrix;

        palmaModel = glm::scale(palmaModel, glm::vec3(0.8f, 2.2f, 1.0f));

        DrawCube(
            ourShader, VAO, modelLoc, colorLoc,
            palmaModel,
            glm::vec3(0.75f, 0.75f, 0.78f)
        );


        // ====================================================
        // PULGAR (2 FALANGES, INDEPENDIENTE)
        // ====================================================

        // Base del pulgar: del lado +Y (junto al �ndice y del mismo lado
        // hacia donde dobla el codo), ligeramente al frente de la palma
        // (z = 0.35) para poder cruzarla sin atravesar los otros dedos.
        glm::mat4 pulgarBase = palmaMatrix;

        pulgarBase = glm::translate(pulgarBase, glm::vec3(0.10f, 1.0f, 0.35f));

        pulgarBase = glm::rotate(
            pulgarBase,
            glm::radians(35.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );

        const float pulgarAngles[2] =
        {
            pulgar1 * PULGAR1_FACTOR,
            pulgar2 * PULGAR2_FACTOR
        };

        DrawFinger(
            ourShader, VAO, modelLoc, colorLoc,
            pulgarBase,
            glm::vec3(0.0f, 0.0f, 0.0f),
            EJE_PULGAR,
            2,
            pulgarAngles,
            pulgarLen,
            pulgarThick,
            pulgarColor
        );


        // ====================================================
        // 4 DEDOS (3 FALANGES, SINCRONIZADOS, CIERRAN AL FRENTE)
        // ====================================================

        const float dedosAngles[3] =
        {
            falange1 * FALANGE1_FACTOR,
            falange2 * FALANGE2_FACTOR,
            falange3 * FALANGE3_FACTOR
        };

        // Los dedos nacen en el borde de la palma (x = 0.4)

        // �NDICE
        DrawFinger(
            ourShader, VAO, modelLoc, colorLoc,
            palmaMatrix,
            glm::vec3(0.4f, 0.80f, 0.0f),
            EJE_DEDOS, 3,
            dedosAngles, indiceLen, indiceThick, indiceColor
        );

        // MEDIO
        DrawFinger(
            ourShader, VAO, modelLoc, colorLoc,
            palmaMatrix,
            glm::vec3(0.4f, 0.25f, 0.0f),
            EJE_DEDOS, 3,
            dedosAngles, medioLen, medioThick, medioColor
        );

        // ANULAR
        DrawFinger(
            ourShader, VAO, modelLoc, colorLoc,
            palmaMatrix,
            glm::vec3(0.4f, -0.30f, 0.0f),
            EJE_DEDOS, 3,
            dedosAngles, anularLen, anularThick, anularColor
        );

        // ME�IQUE
        DrawFinger(
            ourShader, VAO, modelLoc, colorLoc,
            palmaMatrix,
            glm::vec3(0.4f, -0.80f, 0.0f),
            EJE_DEDOS, 3,
            dedosAngles, meniqueLen, meniqueThick, meniqueColor
        );


        // ====================================================
        // FIN DEL FRAME
        // ====================================================

        glfwSwapBuffers(window);
    }


    // ========================================================
    // LIBERAR RECURSOS
    // ========================================================

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    glfwTerminate();

    return EXIT_SUCCESS;
}


// ============================================================
// INPUTS
// ============================================================

static bool Key(GLFWwindow* window, int key)
{
    return glfwGetKey(window, key) == GLFW_PRESS;
}

void Inputs(GLFWwindow* window)
{
    const float VEL_MOV = 0.08f;
    const float VEL_ANG = 0.18f;

    // ESC
    if (Key(window, GLFW_KEY_ESCAPE))
        glfwSetWindowShouldClose(window, true);


    // ---------------- MOVIMIENTO DE LA ESCENA ----------------

    if (Key(window, GLFW_KEY_D))    movX += VEL_MOV;
    if (Key(window, GLFW_KEY_A))    movX -= VEL_MOV;

    if (Key(window, GLFW_KEY_UP))   movY += VEL_MOV;
    if (Key(window, GLFW_KEY_DOWN)) movY -= VEL_MOV;

    if (Key(window, GLFW_KEY_W))    movZ -= VEL_MOV;
    if (Key(window, GLFW_KEY_S))    movZ += VEL_MOV;

    if (Key(window, GLFW_KEY_RIGHT)) rot += VEL_ANG;
    if (Key(window, GLFW_KEY_LEFT))  rot -= VEL_ANG;


    // ---------------- BRAZO ----------------------------------

    // Hombro: R / F
    if (Key(window, GLFW_KEY_R)) hombro += VEL_ANG;
    if (Key(window, GLFW_KEY_F)) hombro -= VEL_ANG;
    hombro = ClampAngle(hombro, HOMBRO_MIN, HOMBRO_MAX);

    // Codo: G / T
    if (Key(window, GLFW_KEY_G)) codo += VEL_ANG;
    if (Key(window, GLFW_KEY_T)) codo -= VEL_ANG;
    codo = ClampAngle(codo, CODO_MIN, CODO_MAX);

    // Mu�eca: H / Y
    if (Key(window, GLFW_KEY_H)) muneca += VEL_ANG;
    if (Key(window, GLFW_KEY_Y)) muneca -= VEL_ANG;
    muneca = ClampAngle(muneca, MUNECA_MIN, MUNECA_MAX);


    // ---------------- 4 DEDOS (sincronizados) ----------------

    // Falange 1: J = cerrar / U = abrir
    if (Key(window, GLFW_KEY_J)) falange1 += VEL_ANG;
    if (Key(window, GLFW_KEY_U)) falange1 -= VEL_ANG;
    falange1 = ClampAngle(falange1, FALANGE1_MIN, FALANGE1_MAX);

    // Falange 2: I = cerrar / K = abrir
    if (Key(window, GLFW_KEY_I)) falange2 += VEL_ANG;
    if (Key(window, GLFW_KEY_K)) falange2 -= VEL_ANG;
    falange2 = ClampAngle(falange2, FALANGE2_MIN, FALANGE2_MAX);

    // Falange 3: O = cerrar / L = abrir
    if (Key(window, GLFW_KEY_O)) falange3 += VEL_ANG;
    if (Key(window, GLFW_KEY_L)) falange3 -= VEL_ANG;
    falange3 = ClampAngle(falange3, FALANGE3_MIN, FALANGE3_MAX);


    // ---------------- PULGAR (independiente) -----------------

    // Falange 1: Z = cerrar / X = abrir
    if (Key(window, GLFW_KEY_Z)) pulgar1 += VEL_ANG;
    if (Key(window, GLFW_KEY_X)) pulgar1 -= VEL_ANG;
    pulgar1 = ClampAngle(pulgar1, PULGAR1_MIN, PULGAR1_MAX);

    // Falange 2: C = cerrar / V = abrir
    if (Key(window, GLFW_KEY_C)) pulgar2 += VEL_ANG;
    if (Key(window, GLFW_KEY_V)) pulgar2 -= VEL_ANG;
    pulgar2 = ClampAngle(pulgar2, PULGAR2_MIN, PULGAR2_MAX);

}