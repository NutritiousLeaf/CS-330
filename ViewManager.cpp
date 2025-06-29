///////////////////////////////////////////////////////////////////////////////
// viewmanager.h
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>    

// declaration of the global variables and defines
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	// camera object used for viewing and interacting with
	// the 3D scene
	Camera* g_pCamera = nullptr;

	// these variables are used for mouse movement processing
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// time between current frame and last frame
	float gDeltaTime = 0.0f; 
	float gLastFrame = 0.0f;
	float gCameraSpeed = 5.0f;  // default movement speed adjusted by scroll

	// the following variable is false when orthographic projection
	// is off and true when it is on
	bool bOrthographicProjection = false;
}

/***********************************************************
 *  ViewManager()
 *
 *  The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(
	ShaderManager *pShaderManager)
{
	// initialize the member variables
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;
	g_pCamera = new Camera();
	// default camera view parameters
	g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
	g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 80;
}

/***********************************************************
 *  ~ViewManager()
 *
 *  The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
	// free up allocated memory
	m_pShaderManager = NULL;
	m_pWindow = NULL;
	if (NULL != g_pCamera)
	{
		delete g_pCamera;
		g_pCamera = NULL;
	}
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	// try to create the displayed OpenGL window
	window = glfwCreateWindow(
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		windowTitle,
		NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	// tell GLFW to capture all mouse events
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// this callback is used to receive mouse moving events
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);
	glfwSetScrollCallback(window, &ViewManager::Scroll_Callback);

	// enable blending for supporting tranparent rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;

	return(window);
}

/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse is moved within the active GLFW display window.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	if (gFirstMouse)
	{
		gLastX = xMousePos;
		gLastY = yMousePos;
		gFirstMouse = false;
	}

	float xoffset = xMousePos - gLastX;
	float yoffset = gLastY - yMousePos; // reversed since y-coordinates range from bottom to top
	gLastX = xMousePos;
	gLastY = yMousePos;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	g_pCamera->Yaw += xoffset;
	g_pCamera->Pitch += yoffset;

	// Clamp pitch
	if (g_pCamera->Pitch > 89.0f)
		g_pCamera->Pitch = 89.0f;
	if (g_pCamera->Pitch < -89.0f)
		g_pCamera->Pitch = -89.0f;

	// Recalculate camera front
	glm::vec3 front;
	front.x = cos(glm::radians(g_pCamera->Yaw)) * cos(glm::radians(g_pCamera->Pitch));
	front.y = sin(glm::radians(g_pCamera->Pitch));
	front.z = sin(glm::radians(g_pCamera->Yaw)) * cos(glm::radians(g_pCamera->Pitch));
	g_pCamera->Front = glm::normalize(front);
}

void ViewManager::Scroll_Callback(GLFWwindow* window, double xoffset, double yoffset)
{
	gCameraSpeed += (float)yoffset;
	if (gCameraSpeed < 1.0f)
		gCameraSpeed = 1.0f;
	if (gCameraSpeed > 20.0f)
		gCameraSpeed = 20.0f;

	std::cout << "Camera speed: " << gCameraSpeed << std::endl;

}

/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  This method is called to process any keyboard events
 *  that may be waiting in the event queue.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	// close the window if the escape key has been pressed
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}

	// --- Camera movement controls ---
	float cameraSpeed = gCameraSpeed * gDeltaTime;

	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
		g_pCamera->Position += cameraSpeed * g_pCamera->Front;
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
		g_pCamera->Position -= cameraSpeed * g_pCamera->Front;
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
		g_pCamera->Position -= glm::normalize(glm::cross(g_pCamera->Front, g_pCamera->Up)) * cameraSpeed;
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
		g_pCamera->Position += glm::normalize(glm::cross(g_pCamera->Front, g_pCamera->Up)) * cameraSpeed;
	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
		g_pCamera->Position += cameraSpeed * g_pCamera->Up;
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
		g_pCamera->Position -= cameraSpeed * g_pCamera->Up;

	// --- Projection mode toggle (P = perspective, O = orthographic) ---
	static bool pPressed = false, oPressed = false;

	if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS && !pPressed)
	{
		bOrthographicProjection = false;
		pPressed = true;

		// Reset camera for perspective view
		g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
		g_pCamera->Yaw = -90.0f;
		g_pCamera->Pitch = -20.0f;

		// Recalculate Front vector
		glm::vec3 front;
		front.x = cos(glm::radians(g_pCamera->Yaw)) * cos(glm::radians(g_pCamera->Pitch));
		front.y = sin(glm::radians(g_pCamera->Pitch));
		front.z = sin(glm::radians(g_pCamera->Yaw)) * cos(glm::radians(g_pCamera->Pitch));
		g_pCamera->Front = glm::normalize(front);
	}

	if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_RELEASE)
		pPressed = false;

	if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS && !oPressed)
	{
		bOrthographicProjection = true;
		oPressed = true;

		// Reset camera for orthographic front view
		g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
		g_pCamera->Yaw = -90.0f;
		g_pCamera->Pitch = -20.0f;

		// Recalculate Front vector
		glm::vec3 front;
		front.x = cos(glm::radians(g_pCamera->Yaw)) * cos(glm::radians(g_pCamera->Pitch));
		front.y = sin(glm::radians(g_pCamera->Pitch));
		front.z = sin(glm::radians(g_pCamera->Yaw)) * cos(glm::radians(g_pCamera->Pitch));
		g_pCamera->Front = glm::normalize(front);
	}

	if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_RELEASE)
		oPressed = false;

	// --- Camera preset views for extra credit (1 = front, 2 = top, 3 = side) ---
	static bool viewKeyPressed[3] = { false, false, false };

	if (glfwGetKey(m_pWindow, GLFW_KEY_1) == GLFW_PRESS && !viewKeyPressed[0])
	{
		bOrthographicProjection = true;
		viewKeyPressed[0] = true;

		// Front orthographic view
		g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
		g_pCamera->Yaw = -90.0f;
		g_pCamera->Pitch = -20.0f;

		glm::vec3 front;
		front.x = cos(glm::radians(g_pCamera->Yaw)) * cos(glm::radians(g_pCamera->Pitch));
		front.y = sin(glm::radians(g_pCamera->Pitch));
		front.z = sin(glm::radians(g_pCamera->Yaw)) * cos(glm::radians(g_pCamera->Pitch));
		g_pCamera->Front = glm::normalize(front);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_1) == GLFW_RELEASE)
		viewKeyPressed[0] = false;

	if (glfwGetKey(m_pWindow, GLFW_KEY_2) == GLFW_PRESS && !viewKeyPressed[1])
	{
		bOrthographicProjection = true;
		viewKeyPressed[1] = true;

		// Top-down orthographic view
		g_pCamera->Position = glm::vec3(0.0f, 15.0f, 0.0f);
		g_pCamera->Yaw = -90.0f;
		g_pCamera->Pitch = -89.9f;

		glm::vec3 front;
		front.x = cos(glm::radians(g_pCamera->Yaw)) * cos(glm::radians(g_pCamera->Pitch));
		front.y = sin(glm::radians(g_pCamera->Pitch));
		front.z = sin(glm::radians(g_pCamera->Yaw)) * cos(glm::radians(g_pCamera->Pitch));
		g_pCamera->Front = glm::normalize(front);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_2) == GLFW_RELEASE)
		viewKeyPressed[1] = false;

	if (glfwGetKey(m_pWindow, GLFW_KEY_3) == GLFW_PRESS && !viewKeyPressed[2])
	{
		bOrthographicProjection = true;
		viewKeyPressed[2] = true;

		// Side orthographic view (right-facing)
		g_pCamera->Position = glm::vec3(15.0f, 5.0f, 0.0f);
		g_pCamera->Yaw = -180.0f;
		g_pCamera->Pitch = -20.0f;

		glm::vec3 front;
		front.x = cos(glm::radians(g_pCamera->Yaw)) * cos(glm::radians(g_pCamera->Pitch));
		front.y = sin(glm::radians(g_pCamera->Pitch));
		front.z = sin(glm::radians(g_pCamera->Yaw)) * cos(glm::radians(g_pCamera->Pitch));
		g_pCamera->Front = glm::normalize(front);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_3) == GLFW_RELEASE)
		viewKeyPressed[2] = false;

}

/***********************************************************
 *  PrepareSceneView()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	// per-frame timing
	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// process any keyboard events that may be waiting in the 
	// event queue
	ProcessKeyboardEvents();

	// get the current view matrix from the camera
	view = g_pCamera->GetViewMatrix();

	// define the current projection matrix
	if (!bOrthographicProjection)
	{
		projection = glm::perspective(glm::radians(g_pCamera->Zoom),
			(GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT,
			0.1f, 100.0f);
	}
	else
	{
		float orthoSize = 10.0f;
		float aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
		projection = glm::ortho(-orthoSize * aspect, orthoSize * aspect,
			-orthoSize, orthoSize,
			0.1f, 100.0f);
	}

	// if the shader manager object is valid
	if (NULL != m_pShaderManager)
	{
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ViewName, view);
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
		// set the view position of the camera into the shader for proper rendering
		m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
	}
}