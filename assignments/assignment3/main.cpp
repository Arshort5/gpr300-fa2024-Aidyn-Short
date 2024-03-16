#include <stdio.h>
#include <math.h>
#include <ew/shader.h>
#include <ew/model.h>
#include <ew/camera.h>
#include <ew/transform.h>
#include <ew/texture.h>

#include <ew/procGen.h>

#include <ew/external/glad.h>
#include <ew/cameraController.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <Aidyn/Framebuffer.h>


void framebufferSizeCallback(GLFWwindow* window, int width, int height);
GLFWwindow* initWindow(const char* title, int width, int height);
void drawUI();
void resetCamera(ew::Camera* camera, ew::CameraController* controller);
void renderScene(const ew::Shader& shader);

float lightDirection[] = { 0,0,0 };
float fustrumHeight = 10;
float near_plane = 1.0f, far_plane = 20.0f;
float minBias = 0.005f;
float maxBias = 0.015f;
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


ew::Camera lightCamera;

aidyn::Framebuffer shadowBuffer;


aidyn::Framebuffer GBuffer;







int main() {
	GLFWwindow* window = initWindow("Assignment 0", screenWidth, screenHeight);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	shadowBuffer = aidyn::createShadowBuffer(screenWidth, screenHeight);

	GBuffer = aidyn::createGBuffer(screenWidth, screenHeight);




	ew::Shader shader = ew::Shader("assets/lit.vert", "assets/lit.frag");

	//ew::Shader postProcessShader = ew::Shader("assets/nothingPost.vert", "assets/nothingPost.frag");

	ew::Shader depthShader = ew::Shader("assets/depthOnly.vert", "assets/depthOnly.frag");
	
	ew::Shader geometryShader = ew::Shader("assets/lit.vert", "assets/geometryPass.frag");



	GLuint brickTexture = ew::loadTexture("assets/brick_color.jpg");


	camera.position = glm::vec3(0.0f, 0.0f, 5.0f);
	camera.target = glm::vec3(0.0f, 0.0f, 0.0f); //Look at the center of the scene
	camera.aspectRatio = (float)screenWidth / screenHeight;
	camera.fov = 60.0f; //Vertical field of view, in degrees

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK); //Back face culling
	glEnable(GL_DEPTH_TEST); //Depth testing


	glActiveTexture(GL_TEXTURE0);


	shader.use();
	shader.setInt("_MainTex", 0);




	float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
		// positions   // texCoords
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,

		-1.0f,  1.0f,  0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f
	};


	unsigned int quadVAO, quadVBO;
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));








	lightCamera.orthographic = true;
	lightCamera.aspectRatio = 1;
	lightCamera.target = glm::vec3(0, 0, 0);


	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		float time = (float)glfwGetTime();
		deltaTime = time - prevFrameTime;
		prevFrameTime = time;


		lightCamera.orthoHeight = fustrumHeight;
		lightCamera.position = glm::vec3(lightDirection[0], lightDirection[1] + 4, lightDirection[2]);

		lightCamera.farPlane = far_plane;
		lightCamera.nearPlane = near_plane;
		cameraController.move(window, &camera, deltaTime);


	
		


		//Calculations
		glm::mat4 lightProjection, lightView, lightSpaceMatrix;


		lightProjection = glm::ortho(-lightCamera.orthoHeight, lightCamera.orthoHeight, -lightCamera.orthoHeight, lightCamera.orthoHeight, near_plane, far_plane);
		lightView = lightCamera.viewMatrix();
		lightSpaceMatrix = lightProjection * lightView;

		glCullFace(GL_FRONT);
		depthShader.use();
		depthShader.setMat4("_ViewProjection", lightSpaceMatrix);


		//RENDER

		glViewport(0, 0, shadowBuffer.width, shadowBuffer.height);
		glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer.fbo);
	

		glClear(GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, brickTexture);

		//GBuffer render

		glBindFramebuffer(GL_FRAMEBUFFER, GBuffer.fbo);
		glViewport(0, 0, GBuffer.width, GBuffer.height);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		geometryShader.use();
		depthShader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
		geometryShader.setInt("_MainTex", 0);
		renderScene(geometryShader);






		//normal render
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glViewport(0, 0, screenWidth, screenHeight);
		glClearColor(0.6f, 0.8f, 0.92f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		

		shader.use();
		shader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
		shader.setVec3("_EyePos", camera.position);
		shader.setFloat("_Material.Ka", material.Ka);
		shader.setFloat("_Material.Kd", material.Kd);
		shader.setFloat("_Material.Ks", material.Ks);
		shader.setVec3("_LightDirection", glm::normalize(lightCamera.target - lightCamera.position));
		shader.setFloat("_Material.Shininess", material.Shininess);
		shader.setMat4("_LightViewProj", lightSpaceMatrix);
		shader.setFloat("minBias", minBias);
		shader.setFloat("maxBias", maxBias);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, shadowBuffer.depthBuffer);
		shader.setInt("_ShadowMap", 1);

		renderScene(shader);

		

		



	

		drawUI();	
		glfwSwapBuffers(window);
	}
	printf("Shutting down...");
}


void renderScene(const ew::Shader &shader) 
{
	ew::Model monkeyModel = ew::Model("assets/suzanne.obj");


	ew::Mesh planeMesh = ew::Mesh(ew::createPlane(10, 10, 1));
	ew::Transform transform;

	shader.setMat4("_Model", transform.modelMatrix());
	monkeyModel.draw();
	transform.position = glm::vec3(0, -1, 0);
	shader.setMat4("_Model", transform.modelMatrix());
	planeMesh.draw();



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

	if (ImGui::CollapsingHeader("Light:"))
	{
		ImGui::DragFloat3("Direction", lightDirection, .01f, -2, 2);
		ImGui::SliderFloat("Shadow Cam Size", &fustrumHeight, 1.0f, 100.0f);
		ImGui::SliderFloat("Shadow Min Bias",&minBias, 0.0001f, 0.1f);
		ImGui::SliderFloat("Shadow Max Bias", &maxBias, 0.0001f, 0.1f);
	}

	if (ImGui::CollapsingHeader("Material")) {
		ImGui::SliderFloat("AmbientK", &material.Ka, 0.0f, 1.0f);
		ImGui::SliderFloat("DiffuseK", &material.Kd, 0.0f, 1.0f);
		ImGui::SliderFloat("SpecularK", &material.Ks, 0.0f, 1.0f);
		ImGui::SliderFloat("Shininess", &material.Shininess, 2.0f, 1024.0f);
	}



	ImGui::End();




	ImGui::Begin("GBuffers"); {
		ImVec2 texSize = ImVec2(GBuffer.width / 4, GBuffer.height / 4);
		for (size_t i = 0; i < 3; i++)
		{
			ImGui::Image((ImTextureID)GBuffer.colorBuffer[i], texSize, ImVec2(0, 1), ImVec2(1, 0));
		}
		ImGui::End();

	}



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

