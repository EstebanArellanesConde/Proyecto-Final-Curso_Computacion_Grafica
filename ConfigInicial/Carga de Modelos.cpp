/*
    Autor: Arellanes Conde Esteban
    Practica #07
    CGeIHC (L+) - Grupo: 05
    FI UNAM Grupo1
    #cta: 319322743
    Fecha: 02/10/2026
*/

// ============================================================
// STD
// ============================================================

#include <iostream>
#include <string>
#include <limits>

// ============================================================
// GLEW
// ============================================================

#include <GL/glew.h>

// ============================================================
// GLFW
// ============================================================

#include <GLFW/glfw3.h>

// ============================================================
// OPENGL / PROYECTO
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

// STB_IMAGE_IMPLEMENTATION debe definirse en UN solo .cpp del
// proyecto (aquí), antes de incluir stb_image.h. Model.h solo
// incluye la declaración, sin volver a definir esto, para no
// duplicar símbolos al enlazar.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// ============================================================
// PROPIEDADES
// ============================================================

const GLuint WIDTH = 800;
const GLuint HEIGHT = 600;

int SCREEN_WIDTH;
int SCREEN_HEIGHT;

// ============================================================
// PROTOTIPOS
// ============================================================

void KeyCallback(
    GLFWwindow* window,
    int key,
    int scancode,
    int action,
    int mode
);

void MouseCallback(
    GLFWwindow* window,
    double xPos,
    double yPos
);

void DoMovement();


// ============================================================
// CÁMARA
// ============================================================

Camera camera(glm::vec3(0.0f, 1.5f, 15.0f));

bool keys[1024];

GLfloat lastX = 400.0f;
GLfloat lastY = 300.0f;

bool firstMouse = true;


// ============================================================
// TIEMPO
// ============================================================

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;


// ============================================================
// ALINEACIÓN AL PISO (BOUNDING BOX)
// ============================================================
//
// PROBLEMA QUE RESUELVE:
//
// El origen (0,0,0) local de un modelo casi nunca coincide
// con su base. Si el pivote está en el centro del mesh, al
// colocarlo en Y = 0.15f, por ejemplo, la mitad del modelo
// queda por debajo del piso (esto es justo lo que se ve en
// las capturas: el perro "enterrado", solo la cabeza afuera).
//
// SOLUCIÓN:
//
// En lugar de adivinar el offset en Y a mano, calculamos la
// Y mínima real de los vértices del modelo (en espacio local)
// y usamos eso para desplazarlo exactamente lo necesario para
// que su base quede sobre el piso (floorY).
//
// NOTA IMPORTANTE:
//
// Esta función asume la estructura típica del loader de
// LearnOpenGL:
//
//   class Model  { public: std::vector<Mesh> meshes; ... };
//   class Mesh   { public: std::vector<Vertex> vertices; ... };
//   struct Vertex{ glm::vec3 Position; ... };
//
// Si tu Mesh.h / Model.h usa otros nombres de miembros
// (por ejemplo "Vertices" con mayúscula, o "Pos" en vez de
// "Position"), solo ajusta la línea marcada abajo.
//
// También asume que las rotaciones que aplicas son sobre el
// eje Y (como en tu código, rotationAxis = (0,1,0)), ya que
// una rotación alrededor de Y no cambia la componente Y de
// los vértices, así que el cálculo sigue siendo válido.
//
// ============================================================

void GetModelYBounds(
    Model& model,
    float& outMinY,
    float& outMaxY
)
{
    outMinY = std::numeric_limits<float>::max();
    outMaxY = std::numeric_limits<float>::lowest();

    for (Mesh& mesh : model.meshes)
    {
        for (Vertex& vertex : mesh.vertices)
        {
            // <-- Ajusta "vertex.Position.y" si tu Vertex usa otro nombre
            float y = vertex.Position.y;

            if (y < outMinY) outMinY = y;
            if (y > outMaxY) outMaxY = y;
        }
    }
}

// Devuelve la Y de traslación necesaria para que la base del
// modelo (ya escalado por scaleY) quede exactamente sobre
// floorY. Súmalo (o úsalo directo) como componente Y de la
// posición con la que llamas a DrawModel.
float AlignToFloorY(
    Model& model,
    float scaleY,
    float floorY = 0.0f
)
{
    float minY, maxY;

    GetModelYBounds(model, minY, maxY);

    return floorY - (minY * scaleY);
}


// ============================================================
// FUNCIÓN AUXILIAR PARA DIBUJAR UN MODELO
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

    // --------------------------------------------------------
    // TRASLACIÓN
    // --------------------------------------------------------

    model = glm::translate(
        model,
        position
    );

    // --------------------------------------------------------
    // ROTACIÓN
    // --------------------------------------------------------

    if (rotationDegrees != 0.0f)
    {
        model = glm::rotate(
            model,
            glm::radians(rotationDegrees),
            rotationAxis
        );
    }

    // --------------------------------------------------------
    // ESCALA
    // --------------------------------------------------------

    model = glm::scale(
        model,
        scale
    );

    // --------------------------------------------------------
    // ENVIAR MATRIZ AL SHADER
    // --------------------------------------------------------

    glUniformMatrix4fv(
        glGetUniformLocation(
            shader.Program,
            "model"
        ),
        1,
        GL_FALSE,
        glm::value_ptr(model)
    );

    // --------------------------------------------------------
    // DIBUJAR
    // --------------------------------------------------------

    modelObject.Draw(shader);
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
        std::cout
            << "Failed to initialize GLFW"
            << std::endl;

        return EXIT_FAILURE;
    }

    glfwWindowHint(
        GLFW_CONTEXT_VERSION_MAJOR,
        3
    );

    glfwWindowHint(
        GLFW_CONTEXT_VERSION_MINOR,
        3
    );

    glfwWindowHint(
        GLFW_OPENGL_PROFILE,
        GLFW_OPENGL_CORE_PROFILE
    );

    glfwWindowHint(
        GLFW_OPENGL_FORWARD_COMPAT,
        GL_TRUE
    );

    glfwWindowHint(
        GLFW_RESIZABLE,
        GL_FALSE
    );


    // ========================================================
    // CREAR VENTANA
    // ========================================================

    GLFWwindow* window =
        glfwCreateWindow(
            WIDTH,
            HEIGHT,
            "ESTEBAN ARELLANES CONDE",
            nullptr,
            nullptr
        );

    if (window == nullptr)
    {
        std::cout
            << "Failed to create GLFW window"
            << std::endl;

        glfwTerminate();

        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);


    // ========================================================
    // FRAMEBUFFER
    // ========================================================

    glfwGetFramebufferSize(
        window,
        &SCREEN_WIDTH,
        &SCREEN_HEIGHT
    );


    // ========================================================
    // CALLBACKS
    // ========================================================

    glfwSetKeyCallback(
        window,
        KeyCallback
    );

    glfwSetCursorPosCallback(
        window,
        MouseCallback
    );

    // Si quieres controlar la cámara con mouse:
    //
    // glfwSetInputMode(
    //     window,
    //     GLFW_CURSOR,
    //     GLFW_CURSOR_DISABLED
    // );


    // ========================================================
    // GLEW
    // ========================================================

    glewExperimental = GL_TRUE;

    if (GLEW_OK != glewInit())
    {
        std::cout
            << "Failed to initialize GLEW"
            << std::endl;

        glfwTerminate();

        return EXIT_FAILURE;
    }


    // ========================================================
    // VIEWPORT
    // ========================================================

    glViewport(
        0,
        0,
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );


    // ========================================================
    // OPENGL
    // ========================================================

    glEnable(GL_DEPTH_TEST);

    // Transparencia
    glEnable(GL_BLEND);

    glBlendFunc(
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA
    );


    // ========================================================
    // SHADER
    // ========================================================

    Shader shader(
        "Shader/modelLoading.vs",
        "Shader/modelLoading.frag"
    );


    // ========================================================
    // CARGAR MODELOS
    // ========================================================

    // --------------------------------------------------------
    // RED DOG
    // --------------------------------------------------------

    Model dog(
        (char*)"Models/RedDog.obj"
    );


    // --------------------------------------------------------
    // ROBOT CAR
    // --------------------------------------------------------

    Model robot_car(
        (char*)"Models/car_project.obj"
    );


    // --------------------------------------------------------
    // EXPO
    // --------------------------------------------------------

    Model wood_grill_expo(
        (char*)"Models/expo.obj"
    );


    // --------------------------------------------------------
    // SILO
    // --------------------------------------------------------

    Model industrial_silo(
        (char*)"Models/Industrial Silo_6_obj.obj"
    );


    // --------------------------------------------------------
    // WAREHOUSE
    // --------------------------------------------------------

    Model warehouse(
        (char*)"Models/Warehouse.obj"
    );


    // ========================================================
    // ESCALAS DE CADA MODELO
    // ========================================================
    //
    // Se declaran aquí para poder reutilizarlas tanto en el
    // cálculo de alineación al piso como en el dibujo.
    //
    // ========================================================

    const float dogScale = 3.5f;
    const float carScale = 3.0f;
    const float expoScale = 0.125f;
    const float siloScale = 0.45f;

    const float floorY = 0.0f; // Piso del Warehouse (según su geometría, ver comentario abajo)


    // ========================================================
    // ALINEACIÓN AUTOMÁTICA AL PISO
    // ========================================================
    //
    // En vez de valores fijos "a ojo" (0.15f, 0.40f, 0.80f...)
    // calculamos, para cada modelo, cuánto hay que desplazarlo
    // en Y para que su base quede exactamente sobre el piso.
    //
    // ========================================================

    float dogY = AlignToFloorY(dog, dogScale, floorY);
    float carY = AlignToFloorY(robot_car, carScale, floorY);
    float expoY = AlignToFloorY(wood_grill_expo, expoScale, floorY);
    float siloY = AlignToFloorY(industrial_silo, siloScale, floorY);

    // Imprime los offsets calculados para poder verificarlos.
    // Si alguno sale con un valor absurdo (ej. cientos de
    // unidades), es señal de que el nombre del campo en
    // GetModelYBounds no corresponde al de tu Vertex/Mesh.

    std::cout << "[Align] dog.y  = " << dogY << std::endl;
    std::cout << "[Align] car.y  = " << carY << std::endl;
    std::cout << "[Align] expo.y = " << expoY << std::endl;
    std::cout << "[Align] silo.y = " << siloY << std::endl;


    // ========================================================
    // PROYECCIÓN
    // ========================================================

    glm::mat4 projection =
        glm::perspective(
            camera.GetZoom(),
            (float)SCREEN_WIDTH /
            (float)SCREEN_HEIGHT,
            0.1f,
            1000.0f
        );


    // ========================================================
    // GAME LOOP
    // ========================================================

    while (!glfwWindowShouldClose(window))
    {
        // ====================================================
        // TIEMPO
        // ====================================================

        GLfloat currentFrame =
            glfwGetTime();

        deltaTime =
            currentFrame -
            lastFrame;

        lastFrame =
            currentFrame;


        // ====================================================
        // EVENTOS
        // ====================================================

        glfwPollEvents();

        DoMovement();


        // ====================================================
        // LIMPIAR BUFFER
        // ====================================================

        glClearColor(
            0.5f,
            0.5f,
            0.5f,
            1.0f
        );

        glClear(
            GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT
        );


        // ====================================================
        // SHADER
        // ====================================================

        shader.Use();


        // ====================================================
        // VIEW
        // ====================================================

        glm::mat4 view =
            camera.GetViewMatrix();

        glUniformMatrix4fv(
            glGetUniformLocation(
                shader.Program,
                "projection"
            ),
            1,
            GL_FALSE,
            glm::value_ptr(projection)
        );

        glUniformMatrix4fv(
            glGetUniformLocation(
                shader.Program,
                "view"
            ),
            1,
            GL_FALSE,
            glm::value_ptr(view)
        );


        // ====================================================
        // WAREHOUSE
        // ====================================================
        //
        // El Warehouse define el espacio de la escena.
        //
        // Se mantiene en escala 1.
        //
        // Su geometría ya tiene aproximadamente:
        //
        // X = -14.6 ... +14.6
        // Y =  0.0  ... +9.7
        //
        // por lo que funciona como edificio principal y su
        // piso ya coincide con floorY = 0.0f.
        //
        // ====================================================

        DrawModel(
            warehouse,
            shader,

            glm::vec3(
                0.0f,
                0.0f,
                -5.0f
            ),

            glm::vec3(
                1.0f,
                1.0f,
                1.0f
            )
        );


        // ====================================================
        // RED DOG
        // ====================================================
        //
        // El perro es nuestra referencia de escala.
        //
        // Lo colocamos dentro del almacén.
        //
        // dogY se calculó automáticamente a partir del
        // bounding box del modelo, para que su base quede
        // exactamente sobre el piso (ya no se adivina el
        // valor a mano).
        //
        // ====================================================

        DrawModel(
            dog,
            shader,

            glm::vec3(
                0.0f,
                dogY,
                -2.0f
            ),

            glm::vec3(
                dogScale,
                dogScale,
                dogScale
            )
        );


        // ====================================================
        // ROBOT CAR #1
        // ====================================================
        //
        // Primera unidad.
        //
        // ====================================================

        DrawModel(
            robot_car,
            shader,

            glm::vec3(
                5.0f,
                carY,
                -1.5f
            ),

            glm::vec3(
                carScale,
                carScale,
                carScale
            ),

            glm::vec3(
                0.0f,
                1.0f,
                0.0f
            ),

            180.0f
        );


        // ====================================================
        // ROBOT CAR #2
        // ====================================================
        //
        // Segunda unidad.
        //
        // Se coloca en el lado opuesto para aprovechar
        // mejor el espacio del Warehouse.
        //
        // ====================================================

        DrawModel(
            robot_car,
            shader,

            glm::vec3(
                -5.0f,
                carY,
                -1.5f
            ),

            glm::vec3(
                carScale,
                carScale,
                carScale
            ),

            glm::vec3(
                0.0f,
                1.0f,
                0.0f
            ),

            0.0f
        );


        // ====================================================
        // EXPO / TARIMA #1
        // ====================================================
        //
        // Tarima izquierda.
        //
        // Se mantiene pequeña respecto al RobotCar.
        //
        // ====================================================

        DrawModel(
            wood_grill_expo,
            shader,

            glm::vec3(
                -7.0f,
                expoY,
                3.0f
            ),

            glm::vec3(
                expoScale,
                expoScale,
                expoScale
            )
        );


        // ====================================================
        // EXPO / TARIMA #2
        // ====================================================
        //
        // Segunda tarima.
        //
        // ====================================================

        DrawModel(
            wood_grill_expo,
            shader,

            glm::vec3(
                1.5f,
                expoY,
                3.0f
            ),

            glm::vec3(
                expoScale,
                expoScale,
                expoScale
            )
        );


        // ====================================================
        // SILO INDUSTRIAL
        // ====================================================
        //
        // Se coloca al fondo del almacén.
        //
        // siloY ya considera el desfase entre el Y=0 del
        // Silo y el piso del Warehouse (antes se resolvía
        // a mano con 0.80f).
        //
        // ====================================================

        DrawModel(
            industrial_silo,
            shader,

            glm::vec3(
                7.0f,
                siloY,
                -8.0f
            ),

            glm::vec3(
                siloScale,
                siloScale,
                siloScale
            ),

            glm::vec3(
                0.0f,
                1.0f,
                0.0f
            ),

            0.0f
        );


        // ====================================================
        // SWAP
        // ====================================================

        glfwSwapBuffers(window);
    }


    // ========================================================
    // TERMINAR
    // ========================================================

    glfwTerminate();

    return 0;
}


// ============================================================
// MOVIMIENTO
// ============================================================

void DoMovement()
{
    // --------------------------------------------------------
    // ADELANTE
    // --------------------------------------------------------

    if (
        keys[GLFW_KEY_W] ||
        keys[GLFW_KEY_UP]
        )
    {
        camera.ProcessKeyboard(
            FORWARD,
            deltaTime
        );
    }


    // --------------------------------------------------------
    // ATRÁS
    // --------------------------------------------------------

    if (
        keys[GLFW_KEY_S] ||
        keys[GLFW_KEY_DOWN]
        )
    {
        camera.ProcessKeyboard(
            BACKWARD,
            deltaTime
        );
    }


    // --------------------------------------------------------
    // IZQUIERDA
    // --------------------------------------------------------

    if (
        keys[GLFW_KEY_A] ||
        keys[GLFW_KEY_LEFT]
        )
    {
        camera.ProcessKeyboard(
            LEFT,
            deltaTime
        );
    }


    // --------------------------------------------------------
    // DERECHA
    // --------------------------------------------------------

    if (
        keys[GLFW_KEY_D] ||
        keys[GLFW_KEY_RIGHT]
        )
    {
        camera.ProcessKeyboard(
            RIGHT,
            deltaTime
        );
    }
}


// ============================================================
// KEY CALLBACK
// ============================================================

void KeyCallback(
    GLFWwindow* window,
    int key,
    int scancode,
    int action,
    int mode
)
{
    // --------------------------------------------------------
    // ESCAPE
    // --------------------------------------------------------

    if (
        GLFW_KEY_ESCAPE == key &&
        GLFW_PRESS == action
        )
    {
        glfwSetWindowShouldClose(
            window,
            GL_TRUE
        );
    }


    // --------------------------------------------------------
    // TECLAS
    // --------------------------------------------------------

    if (
        key >= 0 &&
        key < 1024
        )
    {
        if (action == GLFW_PRESS)
        {
            keys[key] = true;
        }
        else if (action == GLFW_RELEASE)
        {
            keys[key] = false;
        }
    }
}


// ============================================================
// MOUSE CALLBACK
// ============================================================

void MouseCallback(
    GLFWwindow* window,
    double xPos,
    double yPos
)
{
    // --------------------------------------------------------
    // PRIMER MOVIMIENTO
    // --------------------------------------------------------

    if (firstMouse)
    {
        lastX = xPos;
        lastY = yPos;

        firstMouse = false;
    }


    // --------------------------------------------------------
    // OFFSET
    // --------------------------------------------------------

    GLfloat xOffset =
        xPos - lastX;

    GLfloat yOffset =
        lastY - yPos;


    // --------------------------------------------------------
    // ACTUALIZAR POSICIÓN
    // --------------------------------------------------------

    lastX = xPos;
    lastY = yPos;


    // --------------------------------------------------------
    // CÁMARA
    // --------------------------------------------------------

    camera.ProcessMouseMovement(
        xOffset,
        yOffset
    );
}