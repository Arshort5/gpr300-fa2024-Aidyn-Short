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
#include <iostream>

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


struct PointLight
{
	glm::vec3 position;
	float radius;
	glm::vec4 color;
};

const int MAX_POINT_LIGHTS = 64;
PointLight pointLights[MAX_POINT_LIGHTS];

struct Framebuffer {
	unsigned int fbo;
	unsigned int colorBuffers[3];
	unsigned int depthBuffer;
	unsigned int width;
	unsigned int height;
}framebuffer;

Framebuffer createGBuffer(unsigned int width, unsigned int height) {
	Framebuffer framebuffer;
	framebuffer.width = width;
	framebuffer.height = height;

	glCreateFramebuffers(1, &framebuffer.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);

	int formats[3] = {
		GL_RGB32F, //0 = World Position 
		GL_RGB16F, //1 = World Normal
		GL_RGB16F  //2 = Albedo
	};
	//Create 3 color textures
	for (size_t i = 0; i < 3; i++)
	{
		glGenTextures(1, &framebuffer.colorBuffers[i]);
		glBindTexture(GL_TEXTURE_2D, framebuffer.colorBuffers[i]);
		glTexStorage2D(GL_TEXTURE_2D, 1, formats[i], width, height);
		//Clamp to border so we don't wrap when sampling for post processing
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		//Attach each texture to a different slot.
	//GL_COLOR_ATTACHMENT0 + 1 = GL_COLOR_ATTACHMENT1, etc
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, framebuffer.colorBuffers[i], 0);
	}
	//Explicitly tell OpenGL which color attachments we will draw to
	const GLenum drawBuffers[3] = {
			GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2
	};
	glDrawBuffers(3, drawBuffers);
	//TODO: Add texture2D depth buffer

	glGenRenderbuffers(1, &framebuffer.depthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, framebuffer.depthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, framebuffer.depthBuffer);
	//TODO: Check for completeness

	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
	{
		printf("Not complete");
	}


	//Clean up global state

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return framebuffer;
}

Framebuffer GBuffer;


int main() {
	GLFWwindow* window = initWindow("Assignment 0", screenWidth, screenHeight);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	shadowBuffer = aidyn::createShadowBuffer(screenWidth, screenHeight);

	GBuffer = createGBuffer(screenWidth, screenHeight);

	aidyn::Framebuffer frameBuffer = aidyn::createFramebuffer(screenWidth, screenHeight,GL_RGB16F);


	ew::Shader shader = ew::Shader("assets/lit.vert", "assets/lit.frag");

	ew::Shader depthShader = ew::Shader("assets/depthOnly.vert", "assets/depthOnly.frag");
	
	ew::Shader geometryShader = ew::Shader("assets/lit.vert", "assets/geometryPass.frag");

	ew::Shader defferedShader = ew::Shader("assets/defferedLit.vert", "assets/defferedLit.frag");

	ew::Mesh sphereMesh = ew::Mesh(ew::createSphere(1.0f, 8));

	ew::Shader lightOrbShader = ew::Shader("assets/lightOrb.vert", "assets/lightOrb.frag");

	GLuint brickTexture = ew::loadTexture("assets/brick_color.jpg");

	int index = 0;
	for (int x = 0; x < 8; x++)
	{
		for (int y = 0; y < 8; y++)
		{
			pointLights[index].position = glm::vec3(x * 6.25 - 25, 1, y * 6.25 - 25);
			pointLights[index].radius = 3.0;
			pointLights[index].color = glm::vec4(rand() % 2, rand() % 2, rand() % 2, 1);

			index++;
		}
	}

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

	unsigned int dummyVAO;
	glCreateVertexArrays(1, &dummyVAO);

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


		//RENDER Shadows 

		glViewport(0, 0, shadowBuffer.width, shadowBuffer.height);
		glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer.fbo);
	

		glClear(GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, brickTexture);

		renderScene(depthShader);




		//GBuffer render


		glBindFramebuffer(GL_FRAMEBUFFER, GBuffer.fbo);
		glViewport(0, 0, GBuffer.width, GBuffer.height);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glCullFace(GL_BACK);

		geometryShader.use();
		geometryShader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, brickTexture);


		geometryShader.setInt("_MainTex", 0);
		renderScene(geometryShader);





		//Lighting render

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, screenWidth, screenHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		

		defferedShader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, GBuffer.colorBuffers[0]);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, GBuffer.colorBuffers[1]);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, GBuffer.colorBuffers[2]);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, shadowBuffer.depthBuffer);

		defferedShader.setInt("gPosition", 0);
		defferedShader.setInt("gNormal", 1);
		defferedShader.setInt("gAlbedoSpec", 2);
		defferedShader.setInt("_ShadowMap", 3);

		//defferedShader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
		defferedShader.setVec3("_EyePos", camera.position);
		defferedShader.setFloat("_Material.Ka", material.Ka);
		defferedShader.setFloat("_Material.Kd", material.Kd);
		defferedShader.setFloat("_Material.Ks", material.Ks);
		defferedShader.setVec3("_LightDirection", glm::normalize(lightCamera.target - lightCamera.position));
		defferedShader.setFloat("_Material.Shininess", material.Shininess);
		defferedShader.setMat4("_LightViewProj", lightSpaceMatrix);
		defferedShader.setFloat("minBias", minBias);
		defferedShader.setFloat("maxBias", maxBias);

		for (int i = 0; i < MAX_POINT_LIGHTS; i++) {
			//Creates prefix "_PointLights[0]." etc
			std::string prefix = "_PointLights[" + std::to_string(i) + "].";
			defferedShader.setVec3(prefix + "position", pointLights[i].position);
			defferedShader.setFloat(prefix + "radius", pointLights[i].radius);
			defferedShader.setVec4(prefix + "color", pointLights[i].color);
		}



		glBindVertexArray(dummyVAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glBindVertexArray(0);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, GBuffer.fbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(
			0, 0, screenWidth, screenHeight, 0, 0, screenWidth, screenHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST
		);

		lightOrbShader.use();
		lightOrbShader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
		for (int i = 0; i < MAX_POINT_LIGHTS; i++)
		{
			glm::mat4 m = glm::mat4(1.0f);
			m = glm::translate(m, pointLights[i].position);
			m = glm::scale(m, glm::vec3(0.2f)); //Whatever radius you want

			lightOrbShader.setMat4("_Model", m);
			lightOrbShader.setVec3("_Color", pointLights[i].color);
			sphereMesh.draw();
		}


		//renderScene(defferedShader);
		
/*
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
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, brickTexture);
		renderScene(shader);

		
		

		
		*/


	

		drawUI();	
		glfwSwapBuffers(window);
	}
	printf("Shutting down...");
}


void renderScene(const ew::Shader &shader) 
{
	ew::Mesh boxMesh = ew::Mesh(ew::createCube(1));


	ew::Mesh planeMesh = ew::Mesh(ew::createPlane(50, 50, 5));
	ew::Transform cubeTransform;
	cubeTransform.position = glm::vec3(0, -1, 0);
	shader.setMat4("_Model", cubeTransform.modelMatrix());
	planeMesh.draw();
	
	for (int i = -6; i < 6; i++)
	{
		for (int j = -6; j < 6; j++)
		{
			cubeTransform.position = glm::vec3(i * 4, 1, j * 4);
			shader.setMat4("_Model", cubeTransform.modelMatrix());
			boxMesh.draw();

		}
	}



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
			ImGui::Image((ImTextureID)GBuffer.colorBuffers[i], texSize, ImVec2(0, 1), ImVec2(1, 0));
		}
		ImGui::Image((ImTextureID)shadowBuffer.depthBuffer, texSize, ImVec2(0, 1), ImVec2(1, 0));
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

