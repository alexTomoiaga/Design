#define GLEW_STATIC
#define TINYOBJLOADER_IMPLEMENTATION

#include <iostream>
#include "glm/glm.hpp"//core glm functionality
#include "glm/gtc/matrix_transform.hpp"//glm extension for generating common transformation matrices
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "GLEW/glew.h"
#include "GLFW/glfw3.h"
#include <string>
#include "Shader.hpp"
#include "Camera.hpp"
#include "SkyBox.hpp"
#include "Model3D.hpp"
#include "Mesh.hpp"

int glWindowWidth = 1920;
int glWindowHeight = 1080;
GLFWwindow* glWindow = NULL;
int retina_width, retina_height;

std::vector<const GLchar*> faces;
std::vector<const GLchar*> skybox_face;
gps::SkyBox mySkyBox;

glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;
glm::mat3 lightDirMatrix;
GLuint modelLoc;
GLuint viewLoc;
GLuint projectionLoc;
GLuint normalMatrixLoc;
GLuint lightDirMatrixLoc;

glm::vec3 v2 = glm::vec3(7.0f, -1.85f, -32.8f);
glm::vec3 lightDir;
glm::vec3 lightColor;
GLuint lightDirLoc;
GLuint lightColorLoc;

const GLuint SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;

bool pressedKeys[1024];
bool firstMouse = true;
bool jump = false;
GLfloat angleHorse;
GLfloat angleChicken = 2.5f;
GLfloat lightAngle;
GLfloat move = 0;
GLfloat down = -1.85f;
GLfloat forward = -32.8f;
GLfloat moveChicken = 2.0f;
GLfloat angleOfChicken;
GLfloat galeataH = -7.5f;


gps::Camera myCamera(glm::vec3(0.0f, 1.0f, 2.5f), glm::vec3(0.0f, 0.0f, 0.0f)); //Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget)
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
GLfloat cameraSpeed = 0.01f;
GLfloat horseSpeed = 0.02f;
GLfloat chickenSpeed = 0.01f;
GLfloat lastX = 320, lastY = 240;
GLfloat yaw, pitch;

gps::Model3D myModel;
gps::Model3D myDragon;
gps::Model3D ground;
gps::Model3D lightCube;
gps::Model3D myScene;
gps::Model3D myHorse;
gps::Model3D myChicken;
gps::Model3D myChicken1;
gps::Model3D galeata;


gps::Shader myCustomShader;
gps::Shader lightShader;
gps::Shader depthMapShader;
gps::Shader skyboxShader;

GLuint shadowMapFBO;
GLuint depthMapTexture;



GLenum glCheckError_(const char *file, int line) {
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
			case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
			case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
			case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
			case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
			case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
			case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
			case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}

#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);
	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);
	myCustomShader.useShaderProgram();
	//set projection matrix
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	//send matrix data to shader
	GLint projLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	//set Viewport transform
	glViewport(0, 0, retina_width, retina_height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (key >= 0 && key < 1024) {
		if (action == GLFW_PRESS)
			pressedKeys[key] = true;
		else if (action == GLFW_RELEASE)
			pressedKeys[key] = false;
	}
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}
	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;
	float sensitivity = 0.05;
	xoffset *= sensitivity;
	yoffset *= sensitivity;
	yaw += xoffset;
	pitch += yoffset;
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;
	cameraFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront.y = sin(glm::radians(pitch));
	cameraFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(cameraFront);
}

void processMovement() {

	if (pressedKeys[GLFW_KEY_W])

		cameraPos += cameraSpeed * cameraFront;
	
	if (pressedKeys[GLFW_KEY_UP])
		if (move < 31.98f)
			move += horseSpeed;

	if (pressedKeys[GLFW_KEY_DOWN])
		if(move >- 11.98f)
		move -= horseSpeed;

	if (pressedKeys[GLFW_KEY_LEFT]) {
		angleHorse += 0.1f;
		if (angleHorse > 360.0f)
			angleHorse -= 360.0f;
	}

	if (pressedKeys[GLFW_KEY_RIGHT]) {
		angleHorse -= 0.1f;
		if (angleHorse < 0.0f)
			angleHorse += 360.0f;
	}

	if (pressedKeys[GLFW_KEY_S])
		cameraPos -= cameraSpeed * cameraFront;

	if (pressedKeys[GLFW_KEY_A])
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

	if (pressedKeys[GLFW_KEY_D])
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

	if (pressedKeys[GLFW_KEY_I]) {
		if(moveChicken < 4.0f)
		moveChicken += 0.1f;
	}

	if (pressedKeys[GLFW_KEY_K]) {
		if(moveChicken > 0.0f)
		moveChicken -= 0.1f;
	}

	if (pressedKeys[GLFW_KEY_Z]) {
		if (galeataH < -7.0f)
			galeataH += 0.1f;
	}

	if (pressedKeys[GLFW_KEY_X]) {
		if (galeataH > -8.0f)
			galeataH -= 0.1f;
	}

	if (pressedKeys[GLFW_KEY_L]) {
		if (angleChicken < 5.0f)
			angleChicken += 0.1f;
	}

	if (pressedKeys[GLFW_KEY_J]) {
		if (angleChicken > 0.1f)
			angleChicken -= 0.1f;
	}


	if (pressedKeys[GLFW_KEY_N]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	if (pressedKeys[GLFW_KEY_V]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_SMOOTH);
	}

	if (pressedKeys[GLFW_KEY_B]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	}

	if (pressedKeys[GLFW_KEY_M]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}



	if (pressedKeys[GLFW_KEY_Q]) {
		lightAngle += 0.3f;
		if (lightAngle > 360.0f)
			lightAngle -= 360.0f;
		glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
		myCustomShader.useShaderProgram();
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDirTr));
	}

	if (pressedKeys[GLFW_KEY_E]) {
		lightAngle -= 0.3f; 
		if (lightAngle < 0.0f)
			lightAngle += 360.0f;
		glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
		myCustomShader.useShaderProgram();
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDirTr));
	}	
}

bool initOpenGLWindow() {
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	//for Mac OS X
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Shader Example", NULL, NULL);
	if (!glWindow) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
	glfwMakeContextCurrent(glWindow);

	glfwWindowHint(GLFW_SAMPLES, 4);

	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	glfwSetKeyCallback(glWindow, keyboardCallback);
	glfwSetCursorPosCallback(glWindow, mouseCallback);
    glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	return true;
}

void initOpenGLState() {
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glViewport(0, 0, retina_width, retina_height);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initFBOs() {
	//generate FBO ID
	glGenFramebuffers(1, &shadowMapFBO);

	//create depth texture for FBO
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//attach texture to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix() {
	const GLfloat near_plane = 1.0f, far_plane = 10.0f;
	glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);
	glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
	glm::mat4 lightView = glm::lookAt(lightDirTr, myCamera.getCameraTarget(), glm::vec3(0.0f, 1.0f, 0.0f));

	return lightProjection * lightView;
}

void initModels() {
	//myModel = gps::Model3D("objects/nanosuit/nanosuit.obj", "objects/nanosuit/");
	myDragon = gps::Model3D("objects/Dragon/Dragon.obj", "objects/Dragon/");
	myScene = gps::Model3D("objects/castelier/p1.obj","objects/castelier/");
	myHorse = gps::Model3D("objects/horse/horse.obj", "objects/horse/");
	myChicken = gps::Model3D("objects/Chicken/chicken_01.obj","objects/Chicken/");
	myChicken1 = gps::Model3D("objects/Chicken/chicken_01.obj", "objects/Chicken/");
	ground = gps::Model3D("objects/ground/ground.obj", "objects/ground/");
	lightCube = gps::Model3D("objects/cube/cube.obj", "objects/cube/");
	galeata = gps::Model3D("objects/galeata/galeata.obj", "objects/galeata/");

	skybox_face.push_back("textures/ame_siege/siege_rt.tga");
	skybox_face.push_back("textures/ame_siege/siege_lf.tga");
	skybox_face.push_back("textures/ame_siege/siege_up.tga");
	skybox_face.push_back("textures/ame_siege/siege_dn.tga");
	skybox_face.push_back("textures/ame_siege/siege_bk.tga");
	skybox_face.push_back("textures/ame_siege/siege_ft.tga");
}

void initShaders() {
	myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
	depthMapShader.loadShader("shaders/simpleDepthMap.vert", "shaders/simpleDepthMap.frag");
}

void initUniforms() {
	myCustomShader.useShaderProgram();

	modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
	viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
	normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
	lightDirMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDirMatrix");

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 1.0f, 2.0f);
	lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	mySkyBox.Load(skybox_face);

	skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
	skyboxShader.useShaderProgram();

	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

}

void jumpChicken() {
	if (jump && down >= -5.0f) {
		down -= 0.1f;
		forward += 0.05f;
	}

}

void renderScene() {	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	processMovement();

	//render the scene to the depth buffer (first pass)
	depthMapShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"), 1,  GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));	
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	
	
	model = glm::scale(glm::mat4(1.0f), glm::vec3(0.233625f, 0.233625f, -0.233625f)); //create model matrix for myHorse
	model = glm::translate(model, glm::vec3(10.0f, -4.25f, -10.0f)); //create model matrix for myHorse
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, move)); //create model matrix for myHorse
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); //send model matrix to shader
	myHorse.Draw(depthMapShader);


	model = glm::rotate(glm::mat4(1.0f), glm::radians(angleOfChicken), glm::vec3(0, 1, 0)); //create model matrix for myChicken
	model = glm::scale(model, glm::vec3(0.2f, 0.2f, -0.2f)); //create model matrix for myChicken
	model = glm::translate(model, glm::vec3(moveChicken, -5.0f, 0.0f)); //create model matrix for myChicken
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); //send model matrix to shader
	myChicken.Draw(depthMapShader);

	model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0, 1, 0)); //create model matrix for myChicken
	model = glm::scale(model, glm::vec3(0.2f, 0.2f, -0.2f)); //create model matrix for myChicken
	model = glm::translate(model, glm::vec3(7.0f, down, forward)); //create model matrix for myChicken
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); //send model matrix to shader
	myChicken1.Draw(depthMapShader);
	
	model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0, 1, 0)); // //create model matrix for myScene
	model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f)); //create model matrix for myScene
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); //send model matrix to shader
	myScene.Draw(depthMapShader);


	model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0, 1, 0)); // //create model matrix for galeata
	model = glm::scale(model, glm::vec3(0.148106f, 0.148106f, 0.148106f)); //create model matrix for galeata
	model = glm::translate(model, glm::vec3(-35.2f, galeataH, -30.0f)); //create model matrix for galeata
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model)); //send model matrix to shader
	galeata.Draw(depthMapShader);

	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f)); //create model matrix for ground
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),  1, GL_FALSE, glm::value_ptr(model)); //send model matrix to shader
	ground.Draw(depthMapShader);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//render the scene (second pass)
	myCustomShader.useShaderProgram(); 
	//send lightSpace matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"), 1, GL_FALSE, glm::value_ptr(computeLightSpaceTrMatrix()));

	view = glm::lookAt(cameraPos, cameraPos + cameraFront, glm::vec3(0.0f, 1.0f, 0.0f));
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view)); //send view matrix to shader

	lightDirMatrix = glm::mat3(glm::inverseTranspose(view)); //compute light direction transformation matrix
	glUniformMatrix3fv(lightDirMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightDirMatrix)); //send lightDir matrix data to shader

	glViewport(0, 0, retina_width, retina_height);
	myCustomShader.useShaderProgram();
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture); //bind the depth map
	glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);
	
	glm::vec3 v1 = glm::vec3(10.0f, -4.25f, move);


	model = glm::scale(glm::mat4(1.0f), glm::vec3(0.233625f, 0.233625f, -0.233625f)); //create model matrix for myHorse
	model = glm::translate(model, glm::vec3(10.0f, -4.25f, -10.0f)); //create model matrix for myHorse
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, move)); //create model matrix for myHorse
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); //send model matrix data to shader
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model)); //compute normal matrix
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix)); //send normal matrix data to shader
	myHorse.Draw(myCustomShader);
	

	model = glm::rotate(glm::mat4(1.0f), glm::radians(angleOfChicken), glm::vec3(0, 1, 0)); //create model matrix for myChicken
	model = glm::scale(model, glm::vec3(0.2f, 0.2f, -0.2f)); //create model matrix for myChicken
	model = glm::translate(model, glm::vec3(moveChicken, -5.0f, 0.0f)); //create model matrix for myChicken
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); //send model matrix data to shader
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model)); //compute normal matrix
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix)); //send normal matrix data to shader
	myChicken.Draw(myCustomShader);

	model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0, 1, 0)); //create model matrix for myChicken
	model = glm::scale(model, glm::vec3(0.2f, 0.2f, -0.2f)); //create model matrix for myChicken
	model = glm::translate(model, glm::vec3(7.0f, down, forward)); //create model matrix for myChicken
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); //send model matrix data to shader
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model)); //compute normal matrix
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix)); //send normal matrix data to shader
	myChicken1.Draw(myCustomShader);
	
	model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0, 1, 0)); // //create model matrix for myScene
	model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f)); //create model matrix for myScene
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	myScene.Draw(myCustomShader);

	model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0, 1, 0)); // //create model matrix for galeata
	model = glm::scale(model, glm::vec3(0.148106f, 0.148106f, 0.148106f)); //create model matrix for galeata
	model = glm::translate(model, glm::vec3(-35.2f, galeataH, -30.0f)); //create model matrix for galeata
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	galeata.Draw(myCustomShader);

	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f)); //create model matrix for ground
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model)); //send model matrix data to shader
	normalMatrix = glm::mat3(glm::inverseTranspose(view*model)); //create normal matrix
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix)); //send normal matrix data to shader
	ground.Draw(myCustomShader);

	lightShader.useShaderProgram(); //draw a white cube around the light
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	model = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::translate(model, lightDir);
	model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	lightCube.Draw(lightShader);

	mySkyBox.Draw(skyboxShader, view, projection);
}

int main(int argc, const char * argv[]) {
	initOpenGLWindow();
	initOpenGLState();
	initFBOs();
	initModels();
	initShaders();
	initUniforms();	
	glCheckError();
	//glShadeModel(GL_SMOOTH);
	while (!glfwWindowShouldClose(glWindow)) {
		angleOfChicken += angleChicken;
		if (angleOfChicken < 360.0f)
			angleOfChicken += 360.0f;
		if (angleOfChicken > 360.0f)
			angleOfChicken -= 360.0f;
		if (move < -10.0f)
			jump = true;
		jumpChicken();
		renderScene();
		glfwPollEvents();
		glfwSwapBuffers(glWindow);

	}
	glfwTerminate();
	return 0;
}
