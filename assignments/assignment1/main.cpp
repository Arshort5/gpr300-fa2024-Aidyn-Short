#include <stdio.h>
#include <math.h>
#include <ew/shader.h>
#include <ew/model.h>
#include <ew/camera.h>
#include <ew/transform.h>
#include <ew/texture.h>

#include<iostream>

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
int activeShader = 0;

ew::CameraController cameraController;
ew::Camera camera;


int main() {
	GLFWwindow* window = initWindow("Assignment 0", screenWidth, screenHeight);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);


	aidyn::Framebuffer framebuffer = aidyn::createFramebuffer(screenWidth, screenHeight, GL_RGB16F);

	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
	{
		printf("Not complete");
	}

	unsigned int dummyVAO;
	glCreateVertexArrays(1, &dummyVAO);


	ew::Shader shader = ew::Shader("assets/lit.vert", "assets/lit.frag");

	ew::Shader postProcessShader = ew::Shader("assets/nothingPost.vert", "assets/nothingPost.frag");

	ew::Shader inversionShader = ew::Shader("assets/inversion.vert", "assets/inversion.frag");

	ew::Shader grayScaleShader = ew::Shader("assets/grayscale.vert", "assets/grayscale.frag");

	ew::Shader edgeDetectShader = ew::Shader("assets/edge.vert", "assets/edge.frag");

	ew::Shader blurShader = ew::Shader("assets/blur.vert", "assets/blur.frag");

	ew::Shader sharpenShader = ew::Shader("assets/sharpen.vert", "assets/sharpen.frag");



	ew::Model monkeyModel = ew::Model("assets/suzanne.obj");

	ew::Transform monkeyTransform;




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












	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		float time = (float)glfwGetTime();
		deltaTime = time - prevFrameTime;
		prevFrameTime = time;



		cameraController.move(window, &camera, deltaTime);

		//RENDER
		
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);
		
		glEnable(GL_DEPTH_TEST);

		glClearColor(0.6f, 0.8f, 0.92f, 1.0f);
		glViewport(0, 0, screenWidth, screenHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);





	
		glBindTexture(GL_TEXTURE_2D, brickTexture);

		monkeyTransform.rotation = glm::rotate(monkeyTransform.rotation, deltaTime, glm::vec3(0.0, 1.0, 0.0));

		shader.use();
		shader.setMat4("_Model", monkeyTransform.modelMatrix());
		shader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
		shader.setVec3("_EyePos", camera.position);
		shader.setFloat("_Material.Ka", material.Ka);
		shader.setFloat("_Material.Kd", material.Kd);
		shader.setFloat("_Material.Ks", material.Ks);
		shader.setFloat("_Material.Shininess", material.Shininess);



		monkeyModel.draw();

		
		


		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDisable(GL_DEPTH_TEST);


		
		glClear(GL_COLOR_BUFFER_BIT );
		

		switch (activeShader)
		{
		case 0:
			postProcessShader.use();
			break;
		case 1:
			inversionShader.use();
			break;
		case 2:
			grayScaleShader.use();
			break;
		case 3:
			blurShader.use();
			break;
		case 4:
			sharpenShader.use();
			break;
		case 5:
			edgeDetectShader.use();
			break;
		default:
			postProcessShader.use();
			break;
		}

		
		glBindVertexArray(quadVAO);
		glBindTexture(GL_TEXTURE_2D, framebuffer.colorBuffer[0]);

		glDrawArrays(GL_TRIANGLES, 0, 6);

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
	
	if (ImGui::CollapsingHeader("Effects"))
	{
		if (ImGui::Button("Default")) {
			activeShader = 0;
			std::cout << activeShader;
		}
		else if (ImGui::Button("Inversion")) {
			activeShader = 1;
			std::cout << activeShader;
		}
		else if (ImGui::Button("Gray scale")) {
			activeShader = 2;
			std::cout << activeShader;
		}
		else if (ImGui::Button("Blur")) {
			activeShader = 3;
			std::cout << activeShader;
		}
		else if (ImGui::Button("Sharpen")) {
			activeShader = 4;
			std::cout << activeShader;
		}
		else if (ImGui::Button("Edge detection")) {
			activeShader = 5;
			std::cout << activeShader;
		}
	}


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

