/*
    Autor: Arellanes Conde Esteban
    Practica #09 - Fuentes de luz
    CGeIHC (L+) - Grupo: 05
    FI UNAM Grupo1
    #cta: 319322743

    Escena de la P7 (Warehouse, RedDog, RobotCar, Expo, Silo + 2 extras)
    iluminada con 5 luces puntuales (colores y componentes ambiental /
    difusa / especular distintas) + la luz de la camara (spot azul).
    Se elimino el ciclo dia-noche (sol y luna).

    CONTROLES
      W A S D / flechas : mover camara      Mouse : orientar
      1 .. 5            : encender / apagar cada luz puntual
      6                 : luz de la camara (spot azul)
      7                 : todas las luces puntuales ON / OFF
      M                 : luz 5 orbita la escena (ON/OFF)
      ESC               : salir
*/

// ============================================================
// STD
// ============================================================
#include <iostream>
#include <string>
#include <limits>
#include <cmath>

// ============================================================
// GLEW / GLFW
// ============================================================
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// ============================================================
// PROYECTO
// ============================================================
#include "Shader.h"
#include "Camera.h"
#include "Model.h"

// ============================================================
// GLM
// ============================================================
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ============================================================
// TEXTURAS
// ============================================================
#include "SOIL2/SOIL2.h"

// Solo en UN .cpp del proyecto
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


// ============================================================
// VENTANA
// ============================================================
const GLuint WIDTH = 800;
const GLuint HEIGHT = 600;
int SCREEN_WIDTH;
int SCREEN_HEIGHT;


// ============================================================
// PROTOTIPOS
// ============================================================
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseCallback(GLFWwindow* window, double xPos, double yPos);
void DoMovement();


// ============================================================
// CAMARA / TIEMPO
// ============================================================
Camera camera(glm::vec3(0.0f, 1.5f, 15.0f));
bool keys[1024];
GLfloat lastX = 400.0f;
GLfloat lastY = 300.0f;
bool firstMouse = true;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;


// ============================================================
// LUCES PUNTUALES
// ============================================================
//
// Cada luz tiene componentes ambiental, difusa y especular propias,
// ademas de su propia atenuacion (alcance). Todas son distintas
// entre si y distintas de la luz azul de la camara.
//
struct PointLightData
{
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float constant, linear, quadratic;
    glm::vec3 cubeColor;     // color del cubo que representa la luz
    bool on;
    const char* name;
};

const int NR_POINT_LIGHTS = 5;

PointLightData lights[NR_POINT_LIGHTS] =
{
    // 0 - ROJA: sobre el RobotCar izquierdo. Alcance medio, brillo especular rojizo
    { glm::vec3(-5.0f, 3.0f, -1.5f),
      glm::vec3(0.10f, 0.00f, 0.00f), glm::vec3(1.00f, 0.05f, 0.05f), glm::vec3(1.00f, 0.40f, 0.40f),
      1.0f, 0.09f, 0.032f, glm::vec3(1.0f, 0.1f, 0.1f), true, "Roja" },

      // 1 - VERDE: sobre el RobotCar derecho. Alcance corto, especular blanco
      { glm::vec3(5.0f, 3.0f, -1.5f),
        glm::vec3(0.00f, 0.08f, 0.00f), glm::vec3(0.05f, 1.00f, 0.10f), glm::vec3(1.00f, 1.00f, 1.00f),
        1.0f, 0.14f, 0.070f, glm::vec3(0.1f, 1.0f, 0.2f), true, "Verde" },

        // 2 - AMBAR (calida): sobre el perro. Mucho ambiente, especular suave
        { glm::vec3(0.0f, 3.5f, -2.0f),
          glm::vec3(0.20f, 0.12f, 0.02f), glm::vec3(1.00f, 0.65f, 0.15f), glm::vec3(0.50f, 0.35f, 0.10f),
          1.0f, 0.07f, 0.017f, glm::vec3(1.0f, 0.7f, 0.1f), true, "Ambar" },

          // 3 - MAGENTA: sobre las expos. Sin ambiente, especular muy fuerte
          { glm::vec3(-4.0f, 3.0f, 3.0f),
            glm::vec3(0.00f, 0.00f, 0.00f), glm::vec3(0.90f, 0.10f, 0.90f), glm::vec3(1.00f, 0.80f, 1.00f),
            1.0f, 0.09f, 0.032f, glm::vec3(1.0f, 0.2f, 1.0f), true, "Magenta" },

            // 4 - CIAN: se mueve (orbita) alrededor de la escena. Alcance largo
            { glm::vec3(7.0f, 4.0f, -8.0f),
              glm::vec3(0.00f, 0.08f, 0.10f), glm::vec3(0.10f, 0.90f, 1.00f), glm::vec3(0.30f, 1.00f, 1.00f),
              1.0f, 0.045f, 0.0075f, glm::vec3(0.2f, 1.0f, 1.0f), true, "Cian" }
};

bool cameraSpot = true;      // 6: luz de la camara (spot azul)
bool orbitLight5 = false;    // M: la luz cian orbita la escena
float orbitAngle = 0.0f;
const glm::vec3 PIVOT(0.0f, 0.0f, -2.0f);
const float ORBIT_R = 10.0f;
const float ORBIT_H = 4.0f;


// ============================================================
// ALINEACION AL PISO (BOUNDING BOX) - de la P7
// ============================================================

void GetModelYBounds(Model& model, float& outMinY, float& outMaxY)
{
    outMinY = std::numeric_limits<float>::max();
    outMaxY = std::numeric_limits<float>::lowest();

    for (Mesh& mesh : model.meshes)
    {
        for (Vertex& vertex : mesh.vertices)
        {
            float y = vertex.Position.y;   // <-- ajustar si tu Vertex usa otro nombre
            if (y < outMinY) outMinY = y;
            if (y > outMaxY) outMaxY = y;
        }
    }
}

float AlignToFloorY(Model& model, float scaleY, float floorY = 0.0f)
{
    float minY, maxY;
    GetModelYBounds(model, minY, maxY);

    // Si el modelo no cargo (sin mallas) no desplazamos nada
    if (minY > maxY)
    {
        std::cout << "[Align] AVISO: modelo sin vertices (revisa la ruta)." << std::endl;
        return floorY;
    }

    return floorY - (minY * scaleY);
}


// ============================================================
// DIBUJAR UN MODELO
// ============================================================

void DrawModel(
    Model& modelObject,
    Shader& shader,
    const glm::vec3& position,
    const glm::vec3& scale,
    const glm::vec3& rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f),
    float rotationDegrees = 0.0f
)
{
    glm::mat4 model = glm::mat4(1.0f);

    model = glm::translate(model, position);

    if (rotationDegrees != 0.0f)
        model = glm::rotate(model, glm::radians(rotationDegrees), rotationAxis);

    model = glm::scale(model, scale);

    glUniformMatrix4fv(
        glGetUniformLocation(shader.Program, "model"),
        1, GL_FALSE, glm::value_ptr(model)
    );

    modelObject.Draw(shader);
}

void GLFWErrorCallback(int error, const char* description)
{
    std::cerr << "[GLFW ERROR] " << error
        << ": " << description << std::endl;
}

// ============================================================
// MAIN
// ============================================================

int main()
{
    glfwSetErrorCallback(GLFWErrorCallback);
    if (!glfwInit())
    {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(
        WIDTH, HEIGHT, "ESTEBAN ARELLANES CONDE", nullptr, nullptr);

    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);

    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCursorPosCallback(window, MouseCallback);

    glewExperimental = GL_TRUE;
    if (GLEW_OK != glewInit())
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ---------------- SHADERS ----------------
    Shader lightingShader("Shader/lightsP9.vs", "Shader/lightsP9.frag");
    Shader lampShader("Shader/lamp.vs", "Shader/lamp.frag");

    // ---------------- MODELOS ----------------
    Model dog((char*)"Models/RedDog.obj");
    Model robot_car((char*)"Models/car_project.obj");
    Model wood_grill_expo((char*)"Models/expo.obj");
    Model industrial_silo((char*)"Models/Industrial Silo_6_obj.obj");
    Model warehouse((char*)"Models/Warehouse.obj");
    Model extra1((char*)"Models/extra1.obj");   // <-- tus rutas
    Model extra2((char*)"Models/extra2.obj");

    const float dogScale = 3.5f;
    const float carScale = 3.0f;
    const float expoScale = 0.125f;
    const float siloScale = 0.45f;
    const float extra1Scale = 1.0f;
    const float extra2Scale = 1.0f;
    const float floorY = 0.0f;

    float dogY = AlignToFloorY(dog, dogScale, floorY);
    float carY = AlignToFloorY(robot_car, carScale, floorY);
    float expoY = AlignToFloorY(wood_grill_expo, expoScale, floorY);
    float siloY = AlignToFloorY(industrial_silo, siloScale, floorY);
    float extra1Y = AlignToFloorY(extra1, extra1Scale, floorY);
    float extra2Y = AlignToFloorY(extra2, extra2Scale, floorY);

    // ---------------- CUBO PARA MOSTRAR LAS LUCES ----------------
    float cubeVerts[] = {
        -0.5f, -0.5f, -0.5f,   0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,  -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,   0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,  -0.5f,  0.5f,  0.5f
    };
    GLuint cubeIdx[] = {
        0,1,2, 2,3,0,  4,5,6, 6,7,4,  0,4,7, 7,3,0,
        1,5,6, 6,2,1,  0,1,5, 5,4,0,  3,2,6, 6,7,3
    };

    GLuint lampVAO, lampVBO, lampEBO;
    glGenVertexArrays(1, &lampVAO);
    glGenBuffers(1, &lampVBO);
    glGenBuffers(1, &lampEBO);
    glBindVertexArray(lampVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lampVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lampEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIdx), cubeIdx, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    glm::mat4 projection = glm::perspective(
        camera.GetZoom(),
        (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT,
        0.1f, 1000.0f);

    // ========================================================
    // GAME LOOP
    // ========================================================
    while (!glfwWindowShouldClose(window))
    {
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();
        DoMovement();

        // Luz 5 (cian) orbitando
        if (orbitLight5)
        {
            orbitAngle += 40.0f * deltaTime;
            float a = glm::radians(orbitAngle);
            lights[4].position = glm::vec3(PIVOT.x + ORBIT_R * cos(a),
                ORBIT_H,
                PIVOT.z + ORBIT_R * sin(a));
        }

        glClearColor(0.01f, 0.01f, 0.02f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.GetViewMatrix();

        // ----------------------------------------------------
        // ESCENA CON LUCES
        // ----------------------------------------------------
        lightingShader.Use();
        GLuint sp = lightingShader.Program;

        glUniformMatrix4fv(glGetUniformLocation(sp, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(sp, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniform3f(glGetUniformLocation(sp, "viewPos"),
            camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z);
        glUniform3f(glGetUniformLocation(sp, "globalAmbient"), 0.03f, 0.03f, 0.04f);

        // 5 luces puntuales
        for (int i = 0; i < NR_POINT_LIGHTS; i++)
        {
            std::string p = "pointLights[" + std::to_string(i) + "].";
            const PointLightData& L = lights[i];

            glUniform3fv(glGetUniformLocation(sp, (p + "position").c_str()), 1, glm::value_ptr(L.position));
            glUniform3fv(glGetUniformLocation(sp, (p + "ambient").c_str()), 1, glm::value_ptr(L.ambient));
            glUniform3fv(glGetUniformLocation(sp, (p + "diffuse").c_str()), 1, glm::value_ptr(L.diffuse));
            glUniform3fv(glGetUniformLocation(sp, (p + "specular").c_str()), 1, glm::value_ptr(L.specular));
            glUniform1f(glGetUniformLocation(sp, (p + "constant").c_str()), L.constant);
            glUniform1f(glGetUniformLocation(sp, (p + "linear").c_str()), L.linear);
            glUniform1f(glGetUniformLocation(sp, (p + "quadratic").c_str()), L.quadratic);
            glUniform1i(glGetUniformLocation(sp, (p + "enabled").c_str()), L.on ? 1 : 0);
        }

        // Luz de la camara (spot azul, como en el previo)
        glUniform3f(glGetUniformLocation(sp, "spotLight.position"), camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z);
        glUniform3f(glGetUniformLocation(sp, "spotLight.direction"), camera.GetFront().x, camera.GetFront().y, camera.GetFront().z);
        glUniform3f(glGetUniformLocation(sp, "spotLight.ambient"), 0.2f, 0.2f, 0.8f);
        glUniform3f(glGetUniformLocation(sp, "spotLight.diffuse"), 0.2f, 0.2f, 0.8f);
        glUniform3f(glGetUniformLocation(sp, "spotLight.specular"), 0.0f, 0.0f, 0.0f);
        glUniform1f(glGetUniformLocation(sp, "spotLight.constant"), 1.0f);
        glUniform1f(glGetUniformLocation(sp, "spotLight.linear"), 0.3f);
        glUniform1f(glGetUniformLocation(sp, "spotLight.quadratic"), 0.7f);
        glUniform1f(glGetUniformLocation(sp, "spotLight.cutOff"), glm::cos(glm::radians(12.0f)));
        glUniform1f(glGetUniformLocation(sp, "spotLight.outerCutOff"), glm::cos(glm::radians(18.0f)));
        glUniform1i(glGetUniformLocation(sp, "spotLight.enabled"), cameraSpot ? 1 : 0);

        // Warehouse
        DrawModel(warehouse, lightingShader, glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(1.0f));

        // RedDog
        DrawModel(dog, lightingShader, glm::vec3(0.0f, dogY, -2.0f), glm::vec3(dogScale));

        // RobotCar #1 y #2
        DrawModel(robot_car, lightingShader, glm::vec3(5.0f, carY, -1.5f), glm::vec3(carScale),
            glm::vec3(0.0f, 1.0f, 0.0f), 180.0f);
        DrawModel(robot_car, lightingShader, glm::vec3(-5.0f, carY, -1.5f), glm::vec3(carScale),
            glm::vec3(0.0f, 1.0f, 0.0f), 0.0f);

        // Expo #1 y #2
        DrawModel(wood_grill_expo, lightingShader, glm::vec3(-7.0f, expoY, 3.0f), glm::vec3(expoScale));
        DrawModel(wood_grill_expo, lightingShader, glm::vec3(1.5f, expoY, 3.0f), glm::vec3(expoScale));

        // Silo
        DrawModel(industrial_silo, lightingShader, glm::vec3(7.0f, siloY, -8.0f), glm::vec3(siloScale));

        // Extras
        DrawModel(extra1, lightingShader, glm::vec3(-9.0f, extra1Y, -6.0f), glm::vec3(extra1Scale));
        DrawModel(extra2, lightingShader, glm::vec3(9.0f, extra2Y, 2.0f), glm::vec3(extra2Scale));

        // ----------------------------------------------------
        // CUBOS QUE REPRESENTAN LAS 5 LUCES
        // ----------------------------------------------------
        lampShader.Use();
        GLuint lp = lampShader.Program;
        glUniformMatrix4fv(glGetUniformLocation(lp, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(lp, "view"), 1, GL_FALSE, glm::value_ptr(view));

        glBindVertexArray(lampVAO);
        for (int i = 0; i < NR_POINT_LIGHTS; i++)
        {
            glm::vec3 c = lights[i].on ? lights[i].cubeColor : lights[i].cubeColor * 0.1f;
            glm::mat4 m = glm::translate(glm::mat4(1.0f), lights[i].position);
            m = glm::scale(m, glm::vec3(0.3f));
            glUniformMatrix4fv(glGetUniformLocation(lp, "model"), 1, GL_FALSE, glm::value_ptr(m));
            glUniform3fv(glGetUniformLocation(lp, "lampColor"), 1, glm::value_ptr(c));
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &lampVAO);
    glDeleteBuffers(1, &lampVBO);
    glDeleteBuffers(1, &lampEBO);

    glfwTerminate();
    return 0;
}


// ============================================================
// MOVIMIENTO (continuo)
// ============================================================

void DoMovement()
{
    if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])    camera.ProcessKeyboard(FORWARD, deltaTime);
    if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN])  camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])  camera.ProcessKeyboard(LEFT, deltaTime);
    if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT]) camera.ProcessKeyboard(RIGHT, deltaTime);
}


// ============================================================
// KEY CALLBACK (toggles: solo en PRESS)
// ============================================================

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (GLFW_KEY_ESCAPE == key && GLFW_PRESS == action)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)        keys[key] = true;
        else if (action == GLFW_RELEASE) keys[key] = false;
    }

    if (action != GLFW_PRESS) return;

    if (key >= GLFW_KEY_1 && key <= GLFW_KEY_5)
    {
        int i = key - GLFW_KEY_1;
        lights[i].on = !lights[i].on;
        std::cout << "[Luz " << (i + 1) << " - " << lights[i].name << "] "
            << (lights[i].on ? "ON" : "OFF") << std::endl;
        return;
    }

    switch (key)
    {
    case GLFW_KEY_6:
        cameraSpot = !cameraSpot;
        std::cout << "[Luz camara azul] " << (cameraSpot ? "ON" : "OFF") << std::endl;
        break;

    case GLFW_KEY_7:
    {
        bool anyOn = false;
        for (int i = 0; i < NR_POINT_LIGHTS; i++) anyOn |= lights[i].on;
        for (int i = 0; i < NR_POINT_LIGHTS; i++) lights[i].on = !anyOn;
        std::cout << "[Luces puntuales] " << (!anyOn ? "TODAS ON" : "TODAS OFF") << std::endl;
        break;
    }

    case GLFW_KEY_M:
        orbitLight5 = !orbitLight5;
        std::cout << "[Luz 5] Orbita: " << (orbitLight5 ? "ON" : "OFF") << std::endl;
        break;
    }
}


// ============================================================
// MOUSE CALLBACK
// ============================================================

void MouseCallback(GLFWwindow* window, double xPos, double yPos)
{
    if (firstMouse)
    {
        lastX = xPos;
        lastY = yPos;
        firstMouse = false;
    }

    GLfloat xOffset = xPos - lastX;
    GLfloat yOffset = lastY - yPos;

    lastX = xPos;
    lastY = yPos;

    camera.ProcessMouseMovement(xOffset, yOffset);
}