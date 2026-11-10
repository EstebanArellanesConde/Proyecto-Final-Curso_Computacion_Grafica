/*
	Autor: Arellanes Conde Esteban
	Practica #07
	CGeIHC (L+) - Grupo: 05
	FI UNAM Grupo1
	#cta: 319322743
	Fecha: 02/10/2026
*/

#include <iostream>
#include <cmath>

// GLEW
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// Other Libs
#include "stb_image.h"

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Other includes
#include "Shader.h"
#include "Camera.h"


// Function prototypes
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseCallback(GLFWwindow* window, double xPos, double yPos);
void DoMovement();

// Window dimensions
const GLuint WIDTH = 800, HEIGHT = 600;
int SCREEN_WIDTH, SCREEN_HEIGHT;

// Camera
Camera  camera(glm::vec3(0.0f, 0.0f, 3.0f));
GLfloat lastX = WIDTH / 2.0;
GLfloat lastY = HEIGHT / 2.0;
bool keys[1024];
bool firstMouse = true;

// Light attributes
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

// Deltatime
GLfloat deltaTime = 0.0f;	// Time between current frame and last frame
GLfloat lastFrame = 0.0f;  	// Time of last frame

// The MAIN function, from here we start the application and run the game loop
int main()
{
	// Init GLFW
	glfwInit();
	// Set all the required options for GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	// Create a GLFWwindow object that we can use for GLFW's functions
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "ESTEBAN ARELLANES CONDE", nullptr, nullptr);

	if (nullptr == window)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();

		return EXIT_FAILURE;
	}

	glfwMakeContextCurrent(window);

	glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);

	// Set the required callback functions
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MouseCallback);

	// GLFW Options
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
	glewExperimental = GL_TRUE;
	// Initialize GLEW to setup the OpenGL Function pointers
	if (GLEW_OK != glewInit())
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return EXIT_FAILURE;
	}

	// Define the viewport dimensions
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	// OpenGL options
	glEnable(GL_DEPTH_TEST);


	// Build and compile our shader program
	Shader lampShader("Shader/lamp.vs", "Shader/lamp.frag");

	// ------------------------------------------------------------------
	// Atlas de la textura (cruz de 4x4 celdas, cada celda = 0.25 x 0.25)
	// Con stbi_set_flip_vertically_on_load(true), v=0 es la parte de ABAJO.
	//
	//   fila 0 (arriba) :   .     .    .    .
	//   fila 1          :   .     .   [6]   .      <- azul   (cara superior)
	//   fila 2          :  [1]   [2]  [3]  [4]     <- blanco, rosa, morado, verde
	//   fila 3 (abajo)  :   .     .   [5]   .      <- rojo   (cara inferior)
	//
	// Asignacion de caras:
	//   Frontal (z+) = 3 morado     Derecha   (x+) = 4 verde
	//   Trasera (z-) = 1 blanco     Izquierda (x-) = 2 rosa
	//   Superior(y+) = 6 azul       Inferior  (y-) = 5 rojo
	// ------------------------------------------------------------------

	// Set up vertex data (and buffer(s)) and attribute pointers
	GLfloat vertices[] =
	{
		// Positions            // Colors            // Texture Coords

		// Cara frontal (z+) -> morado (3): u[0.50,0.75] v[0.25,0.50]
		-0.5f, -0.5f,  0.5f,    1.0f, 1.0f, 1.0f,    0.50f, 0.25f,
		 0.5f, -0.5f,  0.5f,    1.0f, 1.0f, 1.0f,    0.75f, 0.25f,
		 0.5f,  0.5f,  0.5f,    1.0f, 1.0f, 1.0f,    0.75f, 0.50f,
		-0.5f,  0.5f,  0.5f,    1.0f, 1.0f, 1.0f,    0.50f, 0.50f,

		// Cara derecha (x+) -> verde (4): u[0.75,1.00] v[0.25,0.50]
		 0.5f, -0.5f,  0.5f,    1.0f, 1.0f, 1.0f,    0.75f, 0.25f,
		 0.5f, -0.5f, -0.5f,    1.0f, 1.0f, 1.0f,    1.00f, 0.25f,
		 0.5f,  0.5f, -0.5f,    1.0f, 1.0f, 1.0f,    1.00f, 0.50f,
		 0.5f,  0.5f,  0.5f,    1.0f, 1.0f, 1.0f,    0.75f, 0.50f,

		 // Cara trasera (z-) -> blanco (1): u[0.00,0.25] v[0.25,0.50]
		  0.5f, -0.5f, -0.5f,    1.0f, 1.0f, 1.0f,    0.00f, 0.25f,
		 -0.5f, -0.5f, -0.5f,    1.0f, 1.0f, 1.0f,    0.25f, 0.25f,
		 -0.5f,  0.5f, -0.5f,    1.0f, 1.0f, 1.0f,    0.25f, 0.50f,
		  0.5f,  0.5f, -0.5f,    1.0f, 1.0f, 1.0f,    0.00f, 0.50f,

		  // Cara izquierda (x-) -> rosa (2): u[0.25,0.50] v[0.25,0.50]
		  -0.5f, -0.5f, -0.5f,    1.0f, 1.0f, 1.0f,    0.25f, 0.25f,
		  -0.5f, -0.5f,  0.5f,    1.0f, 1.0f, 1.0f,    0.50f, 0.25f,
		  -0.5f,  0.5f,  0.5f,    1.0f, 1.0f, 1.0f,    0.50f, 0.50f,
		  -0.5f,  0.5f, -0.5f,    1.0f, 1.0f, 1.0f,    0.25f, 0.50f,

		  // Cara superior (y+) -> azul (6): u[0.50,0.75] v[0.50,0.75]
		  -0.5f,  0.5f,  0.5f,    1.0f, 1.0f, 1.0f,    0.50f, 0.50f,
		   0.5f,  0.5f,  0.5f,    1.0f, 1.0f, 1.0f,    0.75f, 0.50f,
		   0.5f,  0.5f, -0.5f,    1.0f, 1.0f, 1.0f,    0.75f, 0.75f,
		  -0.5f,  0.5f, -0.5f,    1.0f, 1.0f, 1.0f,    0.50f, 0.75f,

		  // Cara inferior (y-) -> rojo (5): u[0.50,0.75] v[0.00,0.25]
		  -0.5f, -0.5f,  0.5f,    1.0f, 1.0f, 1.0f,    0.50f, 0.25f,
		   0.5f, -0.5f,  0.5f,    1.0f, 1.0f, 1.0f,    0.75f, 0.25f,
		   0.5f, -0.5f, -0.5f,    1.0f, 1.0f, 1.0f,    0.75f, 0.00f,
		  -0.5f, -0.5f, -0.5f,    1.0f, 1.0f, 1.0f,    0.50f, 0.00f,
	};

	GLuint indices[] =
	{  // 6 caras x 2 triangulos (4 vertices por cara)
		 0,  1,  2,    0,  2,  3,    // frontal
		 4,  5,  6,    4,  6,  7,    // derecha
		 8,  9, 10,    8, 10, 11,    // trasera
		12, 13, 14,   12, 14, 15,    // izquierda
		16, 17, 18,   16, 18, 19,    // superior
		20, 21, 22,   20, 22, 23     // inferior
	};

	// First, set the container's VAO (and VBO)
	GLuint VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	// Color attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	// Texture Coordinate attribute
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);

	// Load textures
	GLuint texture1;
	glGenTextures(1, &texture1);
	glBindTexture(GL_TEXTURE_2D, texture1);

	// Parametros: CLAMP_TO_EDGE evita que se "cuelen" colores de la celda vecina del atlas
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // MAG no admite filtros mipmap

	int textureWidth, textureHeight, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	// Diffuse map (cambia el nombre por tu textura de dado)
	unsigned char* image = stbi_load("images/Textura ejemplo dado.PNG", &textureWidth, &textureHeight, &nrChannels, 0);

	if (image)
	{
		GLenum format = (nrChannels == 4) ? GL_RGBA : (nrChannels == 3) ? GL_RGB : GL_RED;
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // por si el ancho no es multiplo de 4
		glTexImage2D(GL_TEXTURE_2D, 0, format, textureWidth, textureHeight, 0, format, GL_UNSIGNED_BYTE, image);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(image);
	glBindTexture(GL_TEXTURE_2D, 0);


	// Game loop
	while (!glfwWindowShouldClose(window))
	{
		// Calculate deltatime of current frame
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
		glfwPollEvents();
		DoMovement();

		// Clear the colorbuffer
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		lampShader.Use();
		//// Create camera transformations
		glm::mat4 view;
		view = camera.GetViewMatrix();
		glm::mat4 projection = glm::perspective(camera.GetZoom(), (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.1f, 100.0f);

		// Giro suave del cubo para poder apreciar todas las caras
		glm::mat4 model(1);
		model = glm::rotate(model, (GLfloat)glfwGetTime() * 0.5f, glm::vec3(0.5f, 1.0f, 0.0f));

		// Get the uniform locations
		GLint modelLoc = glGetUniformLocation(lampShader.Program, "model");
		GLint viewLoc = glGetUniformLocation(lampShader.Program, "view");
		GLint projLoc = glGetUniformLocation(lampShader.Program, "projection");

		// Bind diffuse map
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture1);
		// El nombre del sampler debe coincidir con el de tu lamp.frag
		glUniform1i(glGetUniformLocation(lampShader.Program, "texture1"), 0);

		// Set matrices
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

		// Draw the cube: 36 indices (6 caras x 2 triangulos x 3 vertices)
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// Swap the screen buffers
		glfwSwapBuffers(window);
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteTextures(1, &texture1);
	// Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();

	return 0;
}

// Moves/alters the camera positions based on user input
void DoMovement()
{
	// Camera controls
	if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])
	{
		camera.ProcessKeyboard(FORWARD, deltaTime);
	}

	if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN])
	{
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	}

	if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])
	{
		camera.ProcessKeyboard(LEFT, deltaTime);
	}

	if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT])
	{
		camera.ProcessKeyboard(RIGHT, deltaTime);
	}
}

// Is called whenever a key is pressed/released via GLFW
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (GLFW_KEY_ESCAPE == key && GLFW_PRESS == action)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if (key >= 0 && key < 1024)
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

void MouseCallback(GLFWwindow* window, double xPos, double yPos)
{
	if (firstMouse)
	{
		lastX = xPos;
		lastY = yPos;
		firstMouse = false;
	}

	GLfloat xOffset = xPos - lastX;
	GLfloat yOffset = lastY - yPos;  // Reversed since y-coordinates go from bottom to left

	lastX = xPos;
	lastY = yPos;

	camera.ProcessMouseMovement(xOffset, yOffset);
}