/*
    Autor: Arellanes Conde Esteban
    Practica #08 - Ciclo Dia / Noche
    CGeIHC (L+) - Grupo: 05
    FI UNAM Grupo1
    #cta: 319322743

    Escena de la P7 (Warehouse, RedDog, RobotCar, Expo, Silo + 2 extras)
    iluminada por dos fuentes de luz: SOL y LUNA.

    JERARQUIA (igual que el brazo robotico de la P4):

        pivote (centro de la escena, "la tierra")
          |-- rotate(timeOfDay)       <-- una sola rotacion mueve ambos
                |-- translate(+R)  --> SOL
                |-- translate(-R)  --> LUNA   (siempre opuesta al sol)

    CONTROLES
    ---------------------------------------------
    Camara:        W/A/S/D o flechas + mouse
    1              Encender / apagar el SOL
    2              Encender / apagar la LUNA
    N              Pausar / reanudar el ciclo automatico
    B              Cambio automatico de luz en el horizonte ON/OFF
    Z / X          Retroceder / avanzar la hora manualmente
    R              Reiniciar al amanecer
    ESC            Salir
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
// CICLO DIA / NOCHE
// ============================================================
//
// timeOfDay en grados:
//     0   = amanecer  (sol en el horizonte, lado +X)
//     90  = mediodia  (sol arriba)
//     180 = atardecer (sol en el horizonte, lado -X)
//     270 = medianoche (luna arriba)
//
// ============================================================

float timeOfDay = 45.0f;            // hora inicial (manana)
const float CYCLE_SECONDS = 30.0f;  // duracion de un dia completo (para el video)

bool autoCycle = true;   // N: pausa / reanuda
bool autoSwap = true;   // B: apagado/encendido automatico en el horizonte
bool sunSwitch = true;   // 1: interruptor manual del sol
bool moonSwitch = true;   // 2: interruptor manual de la luna

// Pivote ("la tierra"): centro del Warehouse
const glm::vec3 PIVOT(0.0f, 0.0f, -5.0f);
const float ORBIT_R = 25.0f;   // radio de la orbita
const float ORBIT_TILT = 20.0f;   // inclinacion de la orbita (grados)

// Estado previo, solo para imprimir mensajes al hacer el cambio
bool prevSunActive = true;
bool prevMoonActive = false;


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
    // ---------------- GLFW ----------------
    if (!glfwInit())
    {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Temporalmente quitar esto:
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

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
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // ---------------- GLEW ----------------
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
    // scene : modelos con textura + 2 luces (sol y luna)
    // lamp  : cubos del sol y la luna (color plano)
    Shader sceneShader("Shader/scene.vs", "Shader/scene.frag");
    Shader lampShader("Shader/lamp.vs", "Shader/lamp.frag");


    // ---------------- MODELOS (P7) ----------------
    Model dog((char*)"Models/RedDog.obj");
    Model robot_car((char*)"Models/car_project.obj");
    Model wood_grill_expo((char*)"Models/expo.obj");
    Model industrial_silo((char*)"Models/Industrial Silo_6_obj.obj");
    Model warehouse((char*)"Models/Warehouse.obj");

    // ---------------- MODELOS ADICIONALES (2) ----------------
    // >>> CAMBIA ESTAS RUTAS por tus dos modelos nuevos <<<
    Model extra1((char*)"Models/extra1.obj");
    Model extra2((char*)"Models/extra2.obj");


    // ---------------- ESCALAS Y ALINEACION ----------------
    const float dogScale = 3.5f;
    const float carScale = 3.0f;
    const float expoScale = 0.125f;
    const float siloScale = 0.45f;
    const float extra1Scale = 1.0f;   // ajusta
    const float extra2Scale = 1.0f;   // ajusta
    const float floorY = 0.0f;

    float dogY = AlignToFloorY(dog, dogScale, floorY);
    float carY = AlignToFloorY(robot_car, carScale, floorY);
    float expoY = AlignToFloorY(wood_grill_expo, expoScale, floorY);
    float siloY = AlignToFloorY(industrial_silo, siloScale, floorY);
    float extra1Y = AlignToFloorY(extra1, extra1Scale, floorY);
    float extra2Y = AlignToFloorY(extra2, extra2Scale, floorY);


    // ---------------- CUBO PARA SOL / LUNA ----------------
    float cubeVerts[] = {
        -0.5f, -0.5f, -0.5f,   0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,  -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,   0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,  -0.5f,  0.5f,  0.5f
    };

    GLuint cubeIdx[] = {
        0,1,2, 2,3,0,   // atras
        4,5,6, 6,7,4,   // frente
        0,4,7, 7,3,0,   // izquierda
        1,5,6, 6,2,1,   // derecha
        0,1,5, 5,4,0,   // abajo
        3,2,6, 6,7,3    // arriba
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


    // ---------------- PROYECCION ----------------
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

        // ----------------------------------------------------
        // AVANCE DEL TIEMPO
        // ----------------------------------------------------
        if (autoCycle)
            timeOfDay += (360.0f / CYCLE_SECONDS) * deltaTime;

        timeOfDay = std::fmod(timeOfDay, 360.0f);
        if (timeOfDay < 0.0f) timeOfDay += 360.0f;


        // ----------------------------------------------------
        // PIVOTE: sol y luna giran alrededor de "la tierra"
        // ----------------------------------------------------
        glm::mat4 pivotM = glm::translate(glm::mat4(1.0f), PIVOT);
        pivotM = glm::rotate(pivotM, glm::radians(ORBIT_TILT), glm::vec3(1.0f, 0.0f, 0.0f));
        pivotM = glm::rotate(pivotM, glm::radians(timeOfDay), glm::vec3(0.0f, 0.0f, 1.0f));

        // Hijos del pivote (matrices temporales, como en el brazo)
        glm::mat4 sunM = glm::translate(pivotM, glm::vec3(ORBIT_R, 0.0f, 0.0f));
        glm::mat4 moonM = glm::translate(pivotM, glm::vec3(-ORBIT_R, 0.0f, 0.0f));

        glm::vec3 sunPos = glm::vec3(sunM[3]);
        glm::vec3 moonPos = glm::vec3(moonM[3]);

        // Altura normalizada sobre el horizonte (-1 ... 1)
        float sunH = (sunPos.y - PIVOT.y) / ORBIT_R;
        float moonH = (moonPos.y - PIVOT.y) / ORBIT_R;

        // Factores suaves 0..1 (transicion en el horizonte)
        float dayF = glm::smoothstep(-0.10f, 0.25f, sunH);
        float nightF = glm::smoothstep(-0.10f, 0.25f, moonH);


        // ----------------------------------------------------
        // INTERCAMBIO SOL <-> LUNA (encendido / apagado)
        // ----------------------------------------------------
        float sunHorizon = autoSwap ? dayF : 1.0f;
        float moonHorizon = autoSwap ? nightF : 1.0f;

        float sunIntensity = sunSwitch ? 1.2f * sunHorizon : 0.0f;
        float moonIntensity = moonSwitch ? 0.6f * moonHorizon : 0.0f;

        bool sunActive = sunIntensity > 0.0f;
        bool moonActive = moonIntensity > 0.0f;

        if (sunActive != prevSunActive)
        {
            std::cout << "[Luz] SOL " << (sunActive ? "ENCENDIDO" : "APAGADO") << std::endl;
            prevSunActive = sunActive;
        }
        if (moonActive != prevMoonActive)
        {
            std::cout << "[Luz] LUNA " << (moonActive ? "ENCENDIDA" : "APAGADA") << std::endl;
            prevMoonActive = moonActive;
        }


        // ----------------------------------------------------
        // COLORES DEL CIELO Y AMBIENTE (segun la hora)
        // ----------------------------------------------------
        glm::vec3 skyDay(0.53f, 0.81f, 0.92f);
        glm::vec3 skyNight(0.02f, 0.02f, 0.08f);
        glm::vec3 skyColor = glm::mix(skyNight, skyDay, dayF);

        glm::vec3 ambDay(0.35f, 0.35f, 0.35f);
        glm::vec3 ambNight(0.04f, 0.05f, 0.12f);
        glm::vec3 ambient = glm::mix(ambNight, ambDay, dayF);

        // Tinte anaranjado cerca del horizonte (amanecer / atardecer)
        float horizonTint = 1.0f - glm::smoothstep(0.0f, 0.5f, std::fabs(sunH));
        glm::vec3 sunColor = glm::mix(glm::vec3(1.0f, 0.95f, 0.85f),
            glm::vec3(1.0f, 0.55f, 0.25f), horizonTint);
        glm::vec3 moonColor(0.6f, 0.7f, 1.0f);


        // ----------------------------------------------------
        // LIMPIAR
        // ----------------------------------------------------
        glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.GetViewMatrix();


        // ----------------------------------------------------
        // ESCENA (modelos)
        // ----------------------------------------------------
        sceneShader.Use();
        GLuint sp = sceneShader.Program;

        glUniformMatrix4fv(glGetUniformLocation(sp, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(sp, "view"), 1, GL_FALSE, glm::value_ptr(view));

        glUniform3f(glGetUniformLocation(sp, "viewPos"),
            camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z);

        glUniform3fv(glGetUniformLocation(sp, "ambientColor"), 1, glm::value_ptr(ambient));

        glUniform3fv(glGetUniformLocation(sp, "sun.position"), 1, glm::value_ptr(sunPos));
        glUniform3fv(glGetUniformLocation(sp, "sun.color"), 1, glm::value_ptr(sunColor));
        glUniform1f(glGetUniformLocation(sp, "sun.intensity"), sunIntensity);

        glUniform3fv(glGetUniformLocation(sp, "moon.position"), 1, glm::value_ptr(moonPos));
        glUniform3fv(glGetUniformLocation(sp, "moon.color"), 1, glm::value_ptr(moonColor));
        glUniform1f(glGetUniformLocation(sp, "moon.intensity"), moonIntensity);

        // Warehouse
        DrawModel(warehouse, sceneShader,
            glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(1.0f));

        // RedDog
        DrawModel(dog, sceneShader,
            glm::vec3(0.0f, dogY, -2.0f), glm::vec3(dogScale));

        // RobotCar #1 y #2
        DrawModel(robot_car, sceneShader,
            glm::vec3(5.0f, carY, -1.5f), glm::vec3(carScale),
            glm::vec3(0.0f, 1.0f, 0.0f), 180.0f);

        DrawModel(robot_car, sceneShader,
            glm::vec3(-5.0f, carY, -1.5f), glm::vec3(carScale),
            glm::vec3(0.0f, 1.0f, 0.0f), 0.0f);

        // Expo #1 y #2
        DrawModel(wood_grill_expo, sceneShader,
            glm::vec3(-7.0f, expoY, 3.0f), glm::vec3(expoScale));

        DrawModel(wood_grill_expo, sceneShader,
            glm::vec3(1.5f, expoY, 3.0f), glm::vec3(expoScale));

        // Silo
        DrawModel(industrial_silo, sceneShader,
            glm::vec3(7.0f, siloY, -8.0f), glm::vec3(siloScale));

        // Extras (ajusta posiciones a tu gusto)
        DrawModel(extra1, sceneShader,
            glm::vec3(-9.0f, extra1Y, -6.0f), glm::vec3(extra1Scale));

        DrawModel(extra2, sceneShader,
            glm::vec3(9.0f, extra2Y, 2.0f), glm::vec3(extra2Scale));


        // ----------------------------------------------------
        // SOL Y LUNA (cubos)
        // ----------------------------------------------------
        lampShader.Use();
        GLuint lp = lampShader.Program;

        glUniformMatrix4fv(glGetUniformLocation(lp, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(lp, "view"), 1, GL_FALSE, glm::value_ptr(view));

        glBindVertexArray(lampVAO);

        // Sol (solo si esta sobre el horizonte)
        if (sunH > -0.05f)
        {
            glm::vec3 c = sunActive ? glm::vec3(1.0f, 0.9f, 0.3f) : glm::vec3(0.25f, 0.22f, 0.1f);
            glm::mat4 m = glm::scale(sunM, glm::vec3(2.5f));
            glUniformMatrix4fv(glGetUniformLocation(lp, "model"), 1, GL_FALSE, glm::value_ptr(m));
            glUniform3fv(glGetUniformLocation(lp, "lampColor"), 1, glm::value_ptr(c));
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        }

        // Luna
        if (moonH > -0.05f)
        {
            glm::vec3 c = moonActive ? glm::vec3(0.85f, 0.9f, 1.0f) : glm::vec3(0.2f, 0.22f, 0.28f);
            glm::mat4 m = glm::scale(moonM, glm::vec3(1.8f));
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

    // Control manual de la hora (grados por segundo)
    if (keys[GLFW_KEY_X]) timeOfDay += 60.0f * deltaTime;
    if (keys[GLFW_KEY_Z]) timeOfDay -= 60.0f * deltaTime;
}


// ============================================================
// KEY CALLBACK (toggles: solo en PRESS, una vez por pulsacion)
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

    switch (key)
    {
    case GLFW_KEY_1:
        sunSwitch = !sunSwitch;
        std::cout << "[Switch] Sol: " << (sunSwitch ? "ON" : "OFF") << std::endl;
        break;

    case GLFW_KEY_2:
        moonSwitch = !moonSwitch;
        std::cout << "[Switch] Luna: " << (moonSwitch ? "ON" : "OFF") << std::endl;
        break;

    case GLFW_KEY_N:
        autoCycle = !autoCycle;
        std::cout << "[Ciclo] Automatico: " << (autoCycle ? "ON" : "PAUSA") << std::endl;
        break;

    case GLFW_KEY_B:
        autoSwap = !autoSwap;
        std::cout << "[Ciclo] Cambio automatico en horizonte: " << (autoSwap ? "ON" : "OFF") << std::endl;
        break;

    case GLFW_KEY_R:
        timeOfDay = 0.0f;
        std::cout << "[Ciclo] Reiniciado al amanecer" << std::endl;
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