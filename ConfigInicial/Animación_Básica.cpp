/*
    Autor: Arellanes Conde Esteban
    Practica #10 - Animación Básica
    CGeIHC (L+) - Grupo: 05
    FI UNAM Grupo1
    #cta: 319322743

    Animación:
      - Piso centrado en el origen del mundo.
      - Perro orbitando alrededor del centro del piso.
      - Pelota recorriendo la semicircunferencia OPUESTA a la del perro
        (sentido contrario) sobre una trayectoria tipo silla de montar.
      - Dos contactos por vuelta (0° y 180°): la pelota roza la cabeza,
        viaja pegada a ella durante el brinco y luego se lanza.
      - El punto de contacto (cabeza) y el radio de la pelota se detectan
        automáticamente leyendo los archivos OBJ al iniciar.
      - Cámara inicial elevada y orientada hacia la escena.

    Controles:
      M          Iniciar/detener la animación del perro y los rebotes.
      N          Iniciar/detener la rotación de la pelota.
      W/A/S/D    Mover la cámara.
      Flechas    Mover la cámara.
      Mouse      Orientar la cámara.
      T/G        Mover la luz en X.
      Y/H        Mover la luz en Y.
      U/J        Mover la luz en Z.
      SPACE      Activar/desactivar el efecto de luz.
      ESC        Salir.

    Ajuste fino en vivo (perro detenido en 0°, sin pulsar M):
      Z/X        headLocal.x  -/+
      R/F        headLocal.y  +/-
      C/V        headLocal.z  -/+
      O/L        ballRadius   +/-
      P          Imprime los valores actuales en consola.
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <algorithm>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "Model.h"

// ============================================================
// PROTOTIPOS
// ============================================================

void KeyCallback(GLFWwindow* window, int key, int scancode,
    int action, int mods);

void MouseCallback(GLFWwindow* window, double xPos, double yPos);

void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

void DoMovement();
void Animation();

glm::mat4 DogMatrix(float angleDeg);
glm::vec3 HeadWorld(float angleDeg);
glm::vec3 BallPath(float angleDeg);

// ============================================================
// VENTANA
// ============================================================

const unsigned int WIDTH = 1280;
const unsigned int HEIGHT = 800;

int SCREEN_WIDTH = WIDTH;
int SCREEN_HEIGHT = HEIGHT;

// ============================================================
// CÁMARA
// ============================================================

// Posición inicial: vista frontal elevada.
Camera camera(glm::vec3(0.0f, 2.8f, 8.0f));

float lastX = WIDTH * 0.5f;
float lastY = HEIGHT * 0.5f;

bool firstMouse = true;
bool keys[1024] = { false };

// ============================================================
// LUCES
// ============================================================

glm::vec3 pointLightPositions[] = {
    glm::vec3(0.0f, 4.0f, 2.0f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 0.0f, 0.0f)
};

glm::vec3 Light1(0.3f, 0.3f, 0.3f);
bool active = true;

// ============================================================
// CONFIGURACIÓN DE LOS MODELOS
// ============================================================

// El piso debe tener su centro geométrico en (0,0,0).
const glm::vec3 FLOOR_OFFSET(0.0f, 0.0f, 0.0f);
const float FLOOR_SCALE = 1.0f;

// Escala global del perro.
const float DOG_SCALE = 1.0f;

// Corrección adicional del origen local del perro.
const glm::vec3 DOG_OFFSET(0.0f, 0.0f, 0.0f);

// Punto de contacto sobre la cabeza, en coordenadas LOCALES del OBJ del perro
// (antes de DOG_SCALE). Se detecta automáticamente al iniciar (punto más alto
// del perro); estos valores solo se usan si el OBJ no se puede leer.
glm::vec3 headLocal(0.0f, 1.0f, 0.0f);

// Escala de la pelota.
const float BALL_SCALE = 1.0f;

// Desplazamiento que centra la esfera en su origen. Se calcula automáticamente
// a partir del OBJ de la pelota.
glm::vec3 ballOffset(0.0f, 0.0f, 0.0f);

// Radio de la pelota YA ESCALADO (unidades de mundo). Se calcula automáticamente
// a partir del OBJ de la pelota.
float ballRadius = 0.25f;

// Qué tanto se encaja la pelota en la cabeza, como fracción del radio.
// 0.0 = justo tangente, 0.08 = roce leve visible, 0.15 = más encajada.
const float CONTACT_OVERLAP = 0.08f;

// ============================================================
// ANIMACIÓN DEL PERRO
// ============================================================

bool AnimDog = false;

float dogRot = 0.0f;       // Ángulo de avance, en grados.
float dogSpeed = 55.0f;    // Grados por segundo (negativo = sentido contrario).
float dogRadius = 1.5f;    // Radio de la órbita.

// Orientación del perro (prueba 0, 90, -90, 180 según tu OBJ).
float dogFace = 180.0f;

// Brinco del perro en cada contacto (0° y 180°).
const float JUMP_WINDOW = 30.0f;   // Grados alrededor del contacto en el aire.
const float MAX_JUMP = 0.8f;       // Altura máxima del brinco.

// ============================================================
// ANIMACIÓN DE LA PELOTA
// ============================================================

bool AnimBall = false;

float rotBall = 0.0f;

// Altura máxima del arco medida sobre el contacto.
float bounceHeight = 1.8f;

// Fracción del semicírculo opuesto que recorre la pelota (1 = semicírculo completo).
float ballSwayFactor = 1.0f;

// true  -> la pelota va por el lado OPUESTO al perro (sentido contrario).
// false -> la pelota va por el mismo lado que el perro.
bool ballOppositeSide = true;

// Curvatura adicional tipo paraboloide hiperbólico (silla de montar).
float saddleK = 0.20f;

// Ventana (grados) alrededor de cada contacto en la que la pelota
// viaja pegada a la cabeza antes de lanzarse.
const float CONTACT_WINDOW = 12.0f;

// Tiempo
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ============================================================
// UTILIDADES
// ============================================================

const float PI = 3.14159265358979323846f;

float WrapAngle(float angle)
{
    angle = std::fmod(angle, 360.0f);

    if (angle < 0.0f)
        angle += 360.0f;

    return angle;
}

// ------------------------------------------------------------
// Lectura de los vértices de un OBJ para obtener sus dimensiones.
// ------------------------------------------------------------

struct ObjInfo
{
    bool ok = false;
    glm::vec3 minV = glm::vec3(0.0f);
    glm::vec3 maxV = glm::vec3(0.0f);
    glm::vec3 topCenter = glm::vec3(0.0f);   // centro (XZ) de la zona más alta, Y = altura máxima
};

ObjInfo ReadObjInfo(const char* path)
{
    ObjInfo info;

    std::ifstream file(path);

    if (!file)
        return info;

    std::vector<glm::vec3> verts;
    std::string line;

    while (std::getline(file, line))
    {
        // Solo líneas "v x y z" (no vt ni vn).
        if (line.size() < 3 || line[0] != 'v' || line[1] != ' ')
            continue;

        std::istringstream ss(line.substr(2));

        float x, y, z;

        if (ss >> x >> y >> z)
            verts.push_back(glm::vec3(x, y, z));
    }

    if (verts.empty())
        return info;

    info.minV = verts[0];
    info.maxV = verts[0];

    for (const glm::vec3& v : verts)
    {
        info.minV = glm::min(info.minV, v);
        info.maxV = glm::max(info.maxV, v);
    }

    // Centro de la zona más alta (5% superior de la altura): la parte
    // superior de la cabeza.
    float height = info.maxV.y - info.minV.y;
    float threshold = info.maxV.y - 0.05f * height;

    glm::vec3 sum(0.0f);
    int count = 0;

    for (const glm::vec3& v : verts)
    {
        if (v.y >= threshold)
        {
            sum += v;
            count++;
        }
    }

    if (count > 0)
    {
        sum /= static_cast<float>(count);
        info.topCenter = glm::vec3(sum.x, info.maxV.y, sum.z);
    }
    else
    {
        info.topCenter = info.maxV;
    }

    info.ok = true;
    return info;
}

// ============================================================
// FUNCIÓN PRINCIPAL
// ============================================================

int main()
{
    if (!glfwInit())
    {
        std::cerr << "Error: no se pudo inicializar GLFW.\n";
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(
        WIDTH,
        HEIGHT,
        "ESTEBAN ARELLANES CONDE",
        nullptr,
        nullptr
    );

    if (!window)
    {
        std::cerr << "Error: no se pudo crear la ventana.\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

    glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Error: no se pudo inicializar GLEW.\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }

    // Limpiar el error de OpenGL que algunas versiones de GLEW generan.
    glGetError();

    glEnable(GL_DEPTH_TEST);

    // La cámara inicia apuntando ligeramente hacia abajo.
    camera.ProcessMouseMovement(0.0f, -18.0f);

    // Captura el cursor para rotar la cámara con el mouse.
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // --------------------------------------------------------
    // SHADERS
    // --------------------------------------------------------

    Shader lightingShader(
        "Shader/lighting.vs",
        "Shader/lighting.frag"
    );

    // --------------------------------------------------------
    // CARGA DE MODELOS
    // --------------------------------------------------------

    Model Dog((char*)"Models/RedDog.obj");
    Model Piso((char*)"Models/piso.obj");
    Model Ball((char*)"Models/ball.obj");

    // --------------------------------------------------------
    // AUTO-CALIBRACIÓN DEL CONTACTO A PARTIR DE LOS OBJ
    // --------------------------------------------------------

    ObjInfo dogInfo = ReadObjInfo("Models/RedDog.obj");

    if (dogInfo.ok)
    {
        // Parte más alta del perro = cabeza/orejas.
        headLocal = dogInfo.topCenter;

        std::printf(
            "[Perro] alto = %.3f | min = (%.3f, %.3f, %.3f) | max = (%.3f, %.3f, %.3f)\n",
            dogInfo.maxV.y - dogInfo.minV.y,
            dogInfo.minV.x, dogInfo.minV.y, dogInfo.minV.z,
            dogInfo.maxV.x, dogInfo.maxV.y, dogInfo.maxV.z
        );
    }
    else
    {
        std::printf("[Perro] No se pudo leer Models/RedDog.obj; se usa headLocal manual.\n");
    }

    ObjInfo ballInfo = ReadObjInfo("Models/ball.obj");

    if (ballInfo.ok)
    {
        glm::vec3 center = 0.5f * (ballInfo.minV + ballInfo.maxV);

        // Centra la esfera en su origen y obtiene su radio real.
        ballOffset = -center;
        ballRadius = 0.5f * (ballInfo.maxV.y - ballInfo.minV.y) * BALL_SCALE;

        std::printf(
            "[Pelota] radio = %.3f | centro original = (%.3f, %.3f, %.3f)\n",
            ballRadius, center.x, center.y, center.z
        );
    }
    else
    {
        std::printf("[Pelota] No se pudo leer Models/ball.obj; se usa ballRadius manual.\n");
    }

    std::printf(
        "headLocal(%.3ff, %.3ff, %.3ff)  ballRadius = %.3ff\n",
        headLocal.x, headLocal.y, headLocal.z, ballRadius
    );

    lightingShader.Use();

    glUniform1i(
        glGetUniformLocation(lightingShader.Program, "Material.difuse"),
        0
    );

    glUniform1i(
        glGetUniformLocation(lightingShader.Program, "Material.specular"),
        1
    );

    // --------------------------------------------------------
    // BUCLE PRINCIPAL
    // --------------------------------------------------------

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());

        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Evita saltos de animación si la aplicación se pausa.
        deltaTime = std::min(deltaTime, 0.05f);

        glfwPollEvents();

        DoMovement();
        Animation();

        glClearColor(0.10f, 0.10f, 0.13f, 1.0f);

        glClear(
            GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT
        );

        lightingShader.Use();

        // ----------------------------------------------------
        // CÁMARA Y PROYECCIÓN
        // ----------------------------------------------------

        glm::mat4 view = camera.GetViewMatrix();

        float aspect = 1.0f;

        if (SCREEN_HEIGHT > 0)
        {
            aspect =
                static_cast<float>(SCREEN_WIDTH) /
                static_cast<float>(SCREEN_HEIGHT);
        }

        glm::mat4 projection = glm::perspective(
            glm::radians(camera.GetZoom()),
            aspect,
            0.1f,
            100.0f
        );

        GLint modelLoc = glGetUniformLocation(
            lightingShader.Program, "model"
        );

        GLint viewLoc = glGetUniformLocation(
            lightingShader.Program, "view"
        );

        GLint projLoc = glGetUniformLocation(
            lightingShader.Program, "projection"
        );

        glUniformMatrix4fv(
            viewLoc, 1, GL_FALSE, glm::value_ptr(view)
        );

        glUniformMatrix4fv(
            projLoc, 1, GL_FALSE, glm::value_ptr(projection)
        );

        glm::vec3 cameraPosition = camera.GetPosition();

        glUniform3f(
            glGetUniformLocation(lightingShader.Program, "viewPos"),
            cameraPosition.x,
            cameraPosition.y,
            cameraPosition.z
        );

        // ----------------------------------------------------
        // ILUMINACIÓN DIRECCIONAL
        // ----------------------------------------------------

        glUniform3f(
            glGetUniformLocation(
                lightingShader.Program, "dirLight.direction"
            ),
            -0.3f, -1.0f, -0.3f
        );

        glUniform3f(
            glGetUniformLocation(
                lightingShader.Program, "dirLight.ambient"
            ),
            0.35f, 0.35f, 0.35f
        );

        glUniform3f(
            glGetUniformLocation(
                lightingShader.Program, "dirLight.diffuse"
            ),
            0.75f, 0.75f, 0.75f
        );

        glUniform3f(
            glGetUniformLocation(
                lightingShader.Program, "dirLight.specular"
            ),
            0.35f, 0.35f, 0.35f
        );

        // ----------------------------------------------------
        // LUZ PUNTUAL
        // ----------------------------------------------------

        glUniform3f(
            glGetUniformLocation(
                lightingShader.Program, "pointLights[0].position"
            ),
            pointLightPositions[0].x,
            pointLightPositions[0].y,
            pointLightPositions[0].z
        );

        glUniform3f(
            glGetUniformLocation(
                lightingShader.Program, "pointLights[0].ambient"
            ),
            Light1.x, Light1.y, Light1.z
        );

        glUniform3f(
            glGetUniformLocation(
                lightingShader.Program, "pointLights[0].diffuse"
            ),
            Light1.x, Light1.y, Light1.z
        );

        glUniform3f(
            glGetUniformLocation(
                lightingShader.Program, "pointLights[0].specular"
            ),
            1.0f, 1.0f, 1.0f
        );

        glUniform1f(
            glGetUniformLocation(
                lightingShader.Program, "pointLights[0].constant"
            ),
            1.0f
        );

        glUniform1f(
            glGetUniformLocation(
                lightingShader.Program, "pointLights[0].linear"
            ),
            0.045f
        );

        glUniform1f(
            glGetUniformLocation(
                lightingShader.Program, "pointLights[0].quadratic"
            ),
            0.015f
        );

        // ----------------------------------------------------
        // SPOTLIGHT DE LA CÁMARA
        // ----------------------------------------------------

        glUniform3f(
            glGetUniformLocation(
                lightingShader.Program, "spotLight.position"
            ),
            cameraPosition.x,
            cameraPosition.y,
            cameraPosition.z
        );

        glm::vec3 cameraFront = camera.GetFront();

        glUniform3f(
            glGetUniformLocation(
                lightingShader.Program, "spotLight.direction"
            ),
            cameraFront.x,
            cameraFront.y,
            cameraFront.z
        );

        glUniform3f(
            glGetUniformLocation(
                lightingShader.Program, "spotLight.ambient"
            ),
            0.08f, 0.08f, 0.08f
        );

        glUniform3f(
            glGetUniformLocation(
                lightingShader.Program, "spotLight.diffuse"
            ),
            0.25f, 0.25f, 0.25f
        );

        glUniform3f(
            glGetUniformLocation(
                lightingShader.Program, "spotLight.specular"
            ),
            0.2f, 0.2f, 0.2f
        );

        glUniform1f(
            glGetUniformLocation(
                lightingShader.Program, "spotLight.constant"
            ),
            1.0f
        );

        glUniform1f(
            glGetUniformLocation(
                lightingShader.Program, "spotLight.linear"
            ),
            0.09f
        );

        glUniform1f(
            glGetUniformLocation(
                lightingShader.Program, "spotLight.quadratic"
            ),
            0.032f
        );

        glUniform1f(
            glGetUniformLocation(
                lightingShader.Program, "spotLight.cutOff"
            ),
            glm::cos(glm::radians(12.5f))
        );

        glUniform1f(
            glGetUniformLocation(
                lightingShader.Program, "spotLight.outerCutOff"
            ),
            glm::cos(glm::radians(17.5f))
        );

        glUniform1f(
            glGetUniformLocation(
                lightingShader.Program, "material.shininess"
            ),
            32.0f
        );

        // ----------------------------------------------------
        // PISO: CENTRO DEL SISTEMA DE COORDENADAS
        // ----------------------------------------------------

        glm::mat4 floorModel(1.0f);

        floorModel = glm::translate(
            floorModel, FLOOR_OFFSET
        );

        floorModel = glm::scale(
            floorModel, glm::vec3(FLOOR_SCALE)
        );

        glUniformMatrix4fv(
            modelLoc, 1, GL_FALSE,
            glm::value_ptr(floorModel)
        );

        Piso.Draw(lightingShader);

        // ----------------------------------------------------
        // PERRO
        // ----------------------------------------------------

        glm::mat4 dogModel = DogMatrix(dogRot);

        glUniformMatrix4fv(
            modelLoc, 1, GL_FALSE,
            glm::value_ptr(dogModel)
        );

        Dog.Draw(lightingShader);

        // ----------------------------------------------------
        // PELOTA
        // ----------------------------------------------------

        glm::vec3 ballPosition = BallPath(dogRot);

        glm::mat4 ballModel(1.0f);

        // Primero se traslada al punto de la trayectoria.
        ballModel = glm::translate(
            ballModel, ballPosition
        );

        // Después se aplica la rotación y la escala local.
        ballModel = glm::rotate(
            ballModel,
            glm::radians(rotBall),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        ballModel = glm::scale(
            ballModel, glm::vec3(BALL_SCALE)
        );

        // Centra la esfera en su origen (calculado desde el OBJ).
        ballModel = glm::translate(
            ballModel, ballOffset
        );

        glUniformMatrix4fv(
            modelLoc, 1, GL_FALSE,
            glm::value_ptr(ballModel)
        );

        Ball.Draw(lightingShader);

        glfwSwapBuffers(window);
    }

    glfwTerminate();

    return EXIT_SUCCESS;
}

// ============================================================
// MATRIZ DEL PERRO
// ============================================================

glm::mat4 DogMatrix(float angleDeg)
{
    glm::mat4 model(1.0f);

    // Brinco: el contacto ocurre en los múltiplos exactos de 180°.
    float nearestContact = std::round(angleDeg / 180.0f) * 180.0f;
    float diff = std::abs(angleDeg - nearestContact);

    float jumpHeight = 0.0f;

    if (diff < JUMP_WINDOW)
    {
        // 0 en el centro del salto, 1 en el borde.
        float tJump = diff / JUMP_WINDOW;

        // Parábola invertida: ascenso y descenso suaves.
        jumpHeight = MAX_JUMP * (1.0f - (tJump * tJump));
    }

    // Traslación vertical ANTES de la rotación orbital (eje Y del mundo).
    model = glm::translate(model, glm::vec3(0.0f, jumpHeight, 0.0f));

    // 1. Rotación orbital alrededor del centro del piso.
    model = glm::rotate(
        model,
        glm::radians(angleDeg),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    // 2. Distancia entre el centro del piso y el perro.
    model = glm::translate(
        model,
        glm::vec3(dogRadius, 0.0f, 0.0f)
    );

    // 3. Orientación local del perro.
    model = glm::rotate(
        model,
        glm::radians(dogFace),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    // 4. Corrección del origen y escala del modelo.
    model = glm::translate(
        model, DOG_OFFSET
    );

    model = glm::scale(
        model, glm::vec3(DOG_SCALE)
    );

    return model;
}

// ============================================================
// POSICIÓN MUNDIAL DE LA CABEZA
// ============================================================

glm::vec3 HeadWorld(float angleDeg)
{
    // Se usa la MISMA matriz que dibuja al perro, de modo que el punto
    // de contacto sigue su órbita, orientación, escala y brinco.
    glm::vec4 localHead(headLocal, 1.0f);

    glm::vec4 worldHead =
        DogMatrix(angleDeg) * localHead;

    return glm::vec3(worldHead);
}

// ============================================================
// TRAYECTORIA DE LA PELOTA
// ============================================================

glm::vec3 BallPath(float angleDeg)
{
    /*
        Cada arco comprende 180° de la órbita del perro.

        t = 0   : contacto con la cabeza (la pelota roza la parte alta).
        t = 0.5 : punto medio del salto.
        t = 1   : siguiente contacto.

        Horizontal (plano XZ):
            avance a lo largo de la cuerda   u(t) = (1 - cos(pi t)) / 2
            desviación lateral               w(t) = A sin(pi t)
          Con A = mitad de la cuerda, la pelota describe una semicircunferencia
          en el lado OPUESTO al arco del perro: sentido contrario.

        Vertical:
            h(t) = H (1 - cos(2 pi t)) / 2  +  k sin(2 pi t) sin(pi t)
          El segundo término es la curvatura de silla (paraboloide hiperbólico).
          Ambos términos valen 0 en los contactos.

        Contacto:
            El centro de la pelota queda a  ballRadius * (1 - CONTACT_OVERLAP)
            sobre el punto más alto de la cabeza: la esfera roza la cabeza.
            A +-CONTACT_WINDOW grados de cada contacto la pelota se mezcla con la
            cabeza real (que brinca), así viajan pegadas y luego se separan.
    */

    float segment = std::floor(angleDeg / 180.0f);

    float angleStart = segment * 180.0f;
    float angleEnd = angleStart + 180.0f;

    float t = glm::clamp((angleDeg - angleStart) / 180.0f, 0.0f, 1.0f);

    // Centro de la pelota sobre la cabeza, con un leve encaje para el roce.
    glm::vec3 up(0.0f, ballRadius * (1.0f - CONTACT_OVERLAP), 0.0f);

    glm::vec3 P0 = HeadWorld(angleStart) + up;
    glm::vec3 P1 = HeadWorld(angleEnd) + up;

    glm::vec3 chord = P1 - P0;
    chord.y = 0.0f;

    float chordLength = glm::length(chord);

    glm::vec3 position = glm::mix(P0, P1, t);

    if (chordLength > 0.0001f)
    {
        glm::vec3 direction = chord / chordLength;

        // Perpendicular a la cuerda en el plano XZ.
        glm::vec3 perpendicular(-direction.z, 0.0f, direction.x);

        if (ballOppositeSide)
        {
            // Lado donde queda el arco del perro: se invierte para ir por el opuesto.
            glm::vec3 dogMid = HeadWorld(angleStart + 90.0f);
            glm::vec3 chordMid = 0.5f * (P0 + P1);

            glm::vec3 toDog = dogMid - chordMid;
            toDog.y = 0.0f;

            if (glm::dot(perpendicular, toDog) > 0.0f)
                perpendicular = -perpendicular;
        }

        // Avance suave a lo largo de la cuerda.
        float u = 0.5f * (1.0f - std::cos(PI * t));

        float lateral =
            ballSwayFactor * 0.5f * chordLength * std::sin(PI * t);

        position = P0 + direction * (chordLength * u)
            + perpendicular * lateral;

        // Altura: cero en ambos contactos.
        position.y = glm::mix(P0.y, P1.y, t)
            + 0.5f * bounceHeight * (1.0f - std::cos(2.0f * PI * t))
            + saddleK * std::sin(2.0f * PI * t) * std::sin(PI * t);
    }
    else
    {
        position.y += 0.5f * bounceHeight * (1.0f - std::cos(2.0f * PI * t));
    }

    // ------------------------------------------------------------
    // CONTACTO: cerca de 0° y 180° la pelota viaja pegada a la cabeza.
    // ------------------------------------------------------------
    float nearestContact = std::round(angleDeg / 180.0f) * 180.0f;
    float diff = std::abs(angleDeg - nearestContact);

    if (diff < CONTACT_WINDOW)
    {
        float x = diff / CONTACT_WINDOW;

        // Peso 1 en el contacto, 0 al borde de la ventana (smoothstep).
        float weight = 1.0f - x * x * (3.0f - 2.0f * x);

        glm::vec3 rider = HeadWorld(angleDeg) + up;

        position = glm::mix(position, rider, weight);
    }

    return position;
}

// ============================================================
// MOVIMIENTO DE LA CÁMARA, LA LUZ Y EL AJUSTE FINO
// ============================================================

void DoMovement()
{
    if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])
        camera.ProcessKeyboard(FORWARD, deltaTime);

    if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN])
        camera.ProcessKeyboard(BACKWARD, deltaTime);

    if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])
        camera.ProcessKeyboard(LEFT, deltaTime);

    if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT])
        camera.ProcessKeyboard(RIGHT, deltaTime);

    const float lightSpeed = 2.0f * deltaTime;

    if (keys[GLFW_KEY_T])
        pointLightPositions[0].x += lightSpeed;

    if (keys[GLFW_KEY_G])
        pointLightPositions[0].x -= lightSpeed;

    if (keys[GLFW_KEY_Y])
        pointLightPositions[0].y += lightSpeed;

    if (keys[GLFW_KEY_H])
        pointLightPositions[0].y -= lightSpeed;

    if (keys[GLFW_KEY_U])
        pointLightPositions[0].z -= lightSpeed;

    if (keys[GLFW_KEY_J])
        pointLightPositions[0].z += lightSpeed;

    // Ajuste fino del punto de contacto y del radio de la pelota.
    // Velocidad proporcional al radio para que funcione con modelos pequeños.
    const float tune = 0.6f * std::max(ballRadius, 0.05f) * deltaTime;

    if (keys[GLFW_KEY_Z]) headLocal.x -= tune;
    if (keys[GLFW_KEY_X]) headLocal.x += tune;
    if (keys[GLFW_KEY_R]) headLocal.y += tune;
    if (keys[GLFW_KEY_F]) headLocal.y -= tune;
    if (keys[GLFW_KEY_C]) headLocal.z -= tune;
    if (keys[GLFW_KEY_V]) headLocal.z += tune;

    if (keys[GLFW_KEY_O]) ballRadius += tune;
    if (keys[GLFW_KEY_L]) ballRadius = std::max(0.0f, ballRadius - tune);
}

// ============================================================
// TECLADO
// ============================================================

void KeyCallback(
    GLFWwindow* window,
    int key,
    int scancode,
    int action,
    int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
        return;
    }

    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    }

    if (action != GLFW_PRESS)
        return;

    switch (key)
    {
    case GLFW_KEY_SPACE:
        active = !active;

        if (active)
            Light1 = glm::vec3(0.3f, 0.3f, 0.3f);
        else
            Light1 = glm::vec3(0.0f);

        break;

    case GLFW_KEY_M:
        AnimDog = !AnimDog;
        break;

    case GLFW_KEY_N:
        AnimBall = !AnimBall;
        break;

    case GLFW_KEY_P:
        std::printf(
            "headLocal(%.3ff, %.3ff, %.3ff)  ballRadius = %.3ff\n",
            headLocal.x, headLocal.y, headLocal.z, ballRadius
        );
        break;

    default:
        break;
    }
}

// ============================================================
// ACTUALIZACIÓN DE LA ANIMACIÓN
// ============================================================

void Animation()
{
    // El ángulo orbital determina tanto la posición del perro
    // como la fase de los saltos de la pelota.
    if (AnimDog)
    {
        dogRot = WrapAngle(
            dogRot + dogSpeed * deltaTime
        );
    }

    // La rotación de la pelota es independiente del rebote.
    if (AnimBall)
    {
        rotBall = WrapAngle(
            rotBall + 180.0f * deltaTime
        );
    }
}

// ============================================================
// RATÓN
// ============================================================

void MouseCallback(
    GLFWwindow* window,
    double xPos,
    double yPos)
{
    if (firstMouse)
    {
        lastX = static_cast<float>(xPos);
        lastY = static_cast<float>(yPos);
        firstMouse = false;
    }

    float xOffset =
        static_cast<float>(xPos) - lastX;

    float yOffset =
        lastY - static_cast<float>(yPos);

    lastX = static_cast<float>(xPos);
    lastY = static_cast<float>(yPos);

    camera.ProcessMouseMovement(
        xOffset, yOffset
    );
}

// ============================================================
// REDIMENSIONAMIENTO DE LA VENTANA
// ============================================================

void FramebufferSizeCallback(
    GLFWwindow* window,
    int width,
    int height)
{
    SCREEN_WIDTH = width;
    SCREEN_HEIGHT = height;

    glViewport(0, 0, width, height);
}