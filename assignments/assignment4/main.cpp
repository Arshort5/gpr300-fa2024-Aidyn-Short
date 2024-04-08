#include <stdio.h>
#include <math.h>
#include <ew/shader.h>
#include <ew/model.h>
#include <ew/camera.h>
#include <ew/transform.h>
#include <ew/texture.h>

#include <ew/external/glad.h>
#include <ew/cameraController.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

struct Node;

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
GLFWwindow* initWindow(const char* title, int width, int height);
void drawUI();
void resetCamera(ew::Camera* camera, ew::CameraController* controller);
void SolveFKRecursive(Node& node);



struct Node {
	ew::Transform localTransform;
	glm::mat4 globalTransform = localTransform.modelMatrix();
	Node* parent = NULL;
	std::vector<Node*> children;
};

void SolveFKRecursive(Node& node){
	if (node.parent == NULL)
	{
		node.globalTransform = node.localTransform.modelMatrix();
	}	
	else {
		node.globalTransform =  node.parent->globalTransform * node.localTransform.modelMatrix();
	}
	for each (Node* child in node.children)
	{
		SolveFKRecursive(*child);
	}
}

struct Material {
	float Ka = 1.0;
	float Kd = 0.5;
	float Ks = 0.5;
	float Shininess = 128;
}material;


//Global state
int screenWidth = 1080;
int screenHeight = 720;
float prevFrameTime;
float deltaTime;


ew::CameraController cameraController;
ew::Camera camera;


int main() {


	


	GLFWwindow* window = initWindow("Assignment 0", screenWidth, screenHeight);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);


	ew::Shader shader = ew::Shader("assets/lit.vert", "assets/lit.frag");

	ew::Model monkeyModel = ew::Model("assets/suzanne.obj");

	Node pivotPoint;

	Node body;
	

	pivotPoint.children.push_back(&body);
	body.parent = &pivotPoint;

	Node armLeft;


	armLeft.localTransform.scale = glm::vec3(.5f, .5f, .5f);

	Node armRight;
	armRight.localTransform.scale = glm::vec3(.5f, .5f, .5f);

	Node head;
	head.localTransform.scale = glm::vec3(.5f, .5f, .5f);

	body.children.push_back(&armLeft);
	armLeft.parent = &body;

	body.children.push_back(&armRight);
	armRight.parent = &body;

	body.children.push_back(&head);
	head.parent = &body;

	Node elbowLeft;
	Node elbowRight;

	
	elbowLeft.localTransform.scale = glm::vec3(.75f, .75f, .75f);

	elbowRight.localTransform.scale = glm::vec3(.75f, .75f, .75f);

	armLeft.children.push_back(&elbowLeft);
	armRight.children.push_back(&elbowRight);
	elbowLeft.parent = &armLeft;
	elbowRight.parent = &armRight;

	Node wristLeft;
	Node wristRight;
	
	elbowLeft.children.push_back(&wristLeft);
	elbowRight.children.push_back(&wristRight);
	wristLeft.parent = &elbowLeft;
	wristRight.parent = &elbowRight;


	wristLeft.localTransform.scale = glm::vec3(.75f, .75f, .75f);
	wristRight.localTransform.scale = glm::vec3(.75f, .75f, .75f);

	GLuint brickTexture = ew::loadTexture("assets/brick_color.jpg");


	camera.position = glm::vec3(0.0f, 0.0f, 5.0f);
	camera.target = glm::vec3(0.0f, 0.0f, 0.0f); //Look at the center of the scene
	camera.aspectRatio = (float)screenWidth / screenHeight;
	camera.fov = 60.0f; //Vertical field of view, in degrees

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK); //Back face culling
	glEnable(GL_DEPTH_TEST); //Depth testing


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, brickTexture);


	shader.use();
	shader.setInt("_MainTex", 0);


	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		float time = (float)glfwGetTime();
		deltaTime = time - prevFrameTime;
		prevFrameTime = time;


		//RENDER
		glClearColor(0.6f,0.8f,0.92f,1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		pivotPoint.localTransform.rotation = glm::vec3(0, sin(time / 200) * 360,0);

		body.localTransform.position = glm::vec3(-7, sin(time) *5, 0);



		head.localTransform.position =  glm::vec3(0, 1.2 + sin(time)/4, .25f);
		armLeft.localTransform.position = glm::vec3(-1.5 - sin(time *2) / 4, 0, 0);
		armRight.localTransform.position = glm::vec3(1.5 + sin(time *2) / 4, 0, 0);
		elbowLeft.localTransform.position = glm::vec3(-1 - sin(time * 2) / 4, 0, 0);
		elbowRight.localTransform.position = glm::vec3(1 + sin(time * 2) / 4, 0, 0);
		wristLeft.localTransform.position = glm::vec3(-.75, 1 + sin(time * 2) / 4, 0);
		wristRight.localTransform.position = glm::vec3(.75, 1 + sin(time * 2) / 4, 0);

		armLeft.localTransform.rotation = glm::vec3(sin(time / 200) * 360,0 , 0);
		armRight.localTransform.rotation = glm::vec3(sin(time / 200) * 360, 0, 0);


		SolveFKRecursive(pivotPoint);

		shader.use();
		shader.setMat4("_Model", body.globalTransform);
		shader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
		shader.setVec3("_EyePos", camera.position);
		shader.setFloat("_Material.Ka", material.Ka);
		shader.setFloat("_Material.Kd", material.Kd);
		shader.setFloat("_Material.Ks", material.Ks);
		shader.setFloat("_Material.Shininess", material.Shininess);



		monkeyModel.draw();
		shader.setMat4("_Model", armLeft.globalTransform);

		monkeyModel.draw();
		shader.setMat4("_Model", armRight.globalTransform);

		monkeyModel.draw();
		shader.setMat4("_Model", elbowLeft.globalTransform);

		monkeyModel.draw();
		shader.setMat4("_Model", elbowRight.globalTransform);

		monkeyModel.draw();
		shader.setMat4("_Model", wristLeft.globalTransform);

		monkeyModel.draw();
		shader.setMat4("_Model", wristRight.globalTransform);

		monkeyModel.draw();

		shader.setMat4("_Model", head.globalTransform);
		monkeyModel.draw();


		cameraController.move(window, &camera, deltaTime);
		drawUI();


	

		

		glfwSwapBuffers(window);
	}
	printf("Shutting down...");
}

void resetCamera(ew::Camera* camera, ew::CameraController* controller) {
	camera->position = glm::vec3(0, 0, 5.0f);
	camera->target = glm::vec3(0);
	controller->yaw = controller->pitch = 0;

}





void drawUI() {
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Settings");
	if (ImGui::Button("Reset Camera")) {
		resetCamera(&camera, &cameraController);
	}


	if (ImGui::CollapsingHeader("Material")) {
		ImGui::SliderFloat("AmbientK", &material.Ka, 0.0f, 1.0f);
		ImGui::SliderFloat("DiffuseK", &material.Kd, 0.0f, 1.0f);
		ImGui::SliderFloat("SpecularK", &material.Ks, 0.0f, 1.0f);
		ImGui::SliderFloat("Shininess", &material.Shininess, 2.0f, 1024.0f);
	}

	ImGui::Text("Add Controls Here!");

	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	screenWidth = width;
	screenHeight = height;
}

/// <summary>
/// Initializes GLFW, GLAD, and IMGUI
/// </summary>
/// <param name="title">Window title</param>
/// <param name="width">Window width</param>
/// <param name="height">Window height</param>
/// <returns>Returns window handle on success or null on fail</returns>
GLFWwindow* initWindow(const char* title, int width, int height) {
	printf("Initializing...");
	if (!glfwInit()) {
		printf("GLFW failed to init!");
		return nullptr;
	}

	GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (window == NULL) {
		printf("GLFW failed to create window");
		return nullptr;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGL(glfwGetProcAddress)) {
		printf("GLAD Failed to load GL headers");
		return nullptr;
	}

	//Initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	return window;
}

