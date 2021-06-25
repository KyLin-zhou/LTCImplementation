#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "LTC_data.h"

#include <iostream>

constexpr float PI = 3.14159265359;

constexpr int g_width = 1024;
constexpr int g_height = 768;
constexpr char window_name[] = "Real-Time Polygonal-Light Shading with Linearly Transformed Cosines";

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = g_width / 2.0f;
float lastY = g_height / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct rectLight{
	glm::vec3 center;
	glm::vec3 dir_width;
	glm::vec3 dir_height;
	float half_width;
	float half_height;
	glm::vec3 color;
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		camera.ProcessKeyboard(FORWARD, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		camera.ProcessKeyboard(LEFT, deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		camera.ProcessKeyboard(RIGHT, deltaTime);
	}
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

uint32_t sphereVAO = 0;
uint32_t sphereIndexCount;
void renderSphere()
{
	if (sphereVAO == 0) {
		glGenVertexArrays(1, &sphereVAO);

		uint32_t sphereVBO, sphereEBO;
		glGenBuffers(1, &sphereVBO);
		glGenBuffers(1, &sphereEBO);

		std::vector<glm::vec3> positions;
		std::vector<glm::vec2> uv;
		std::vector<glm::vec3> normals;
		std::vector<uint32_t> indices;

		const uint32_t X_SEGMENTS = 128;
		const uint32_t Y_SEGMENTS = 128;

		for (uint32_t y = 0; y <= Y_SEGMENTS; ++y) {
			for (uint32_t x = 0; x <= X_SEGMENTS; ++x) {
				float xSegment = static_cast<float>(x) / static_cast<float>(X_SEGMENTS);
				float ySegment = static_cast<float>(y) / static_cast<float>(Y_SEGMENTS);
				float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
				float yPos = std::cos(ySegment * PI);
				float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

				positions.push_back(glm::vec3(xPos, yPos, zPos));
				uv.push_back(glm::vec2(xSegment, ySegment));
				normals.push_back(glm::vec3(xPos, yPos, zPos));
			}
		}

		bool oddRow = false;
		for (uint32_t y = 0; y < Y_SEGMENTS; ++y) {
			if (!oddRow) {
				for (uint32_t x = 0; x <= X_SEGMENTS; ++x) {
					indices.push_back(y * (X_SEGMENTS + 1) + x);
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				}
			}
			else {
				for (int x = X_SEGMENTS; x >= 0; --x) {
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
					indices.push_back(y * (X_SEGMENTS + 1) + x);
				}
			}
			oddRow = !oddRow;
		}
		sphereIndexCount = indices.size();

		std::vector<float> data;
		for (size_t i = 0; i < positions.size(); ++i) {
			data.push_back(positions[i].x);
			data.push_back(positions[i].y);
			data.push_back(positions[i].z);
			if (uv.size() > 0) {
				data.push_back(uv[i].x);
				data.push_back(uv[i].y);
			}
			if (normals.size() > 0) {
				data.push_back(normals[i].x);
				data.push_back(normals[i].y);
				data.push_back(normals[i].z);
			}
		}

		glBindVertexArray(sphereVAO);
		glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
		glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), &indices[0], GL_STATIC_DRAW);
		float stride = (3 + 2 + 3) * sizeof(float);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
	}

	glBindVertexArray(sphereVAO);
	glDrawElements(GL_TRIANGLE_STRIP, sphereIndexCount, GL_UNSIGNED_INT, 0);
}

uint32_t planeVAO = 0;
uint32_t planeIndexCount = 0;
void renderPlane()
{
	if (planeVAO == 0) {
		glGenVertexArrays(1, &planeVAO);

		uint32_t planeVBO, planeEBO;
		glGenBuffers(1, &planeVBO);
		glGenBuffers(1, &planeEBO);

		std::vector<glm::vec3> positions;
		std::vector<glm::vec2> uv;
		std::vector<glm::vec3> normals;
		std::vector<uint32_t> indices;

		const uint32_t X_SEGMENTS = 128;
		const uint32_t Y_SEGMENTS = 128;

		for (uint32_t y = 0; y <= Y_SEGMENTS; ++y) {
			for (uint32_t x = 0; x <= X_SEGMENTS; ++x) {
				float xSegment = static_cast<float>(x) / static_cast<float>(X_SEGMENTS);
				float ySegment = static_cast<float>(y) / static_cast<float>(Y_SEGMENTS);
				float xPos = -5.0f + xSegment * 10.0f;
				float yPos = 0.0f;
				float zPos = -5.0f + ySegment * 10.0f;

				positions.push_back(glm::vec3(xPos, yPos, zPos));
				uv.push_back(glm::vec2(xSegment, ySegment));
				normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
			}
		}

		bool oddRow = false;
		for (uint32_t y = 0; y < Y_SEGMENTS; ++y) {
			if (!oddRow) {
				for (uint32_t x = 0; x <= X_SEGMENTS; ++x) {
					indices.push_back(y * (X_SEGMENTS + 1) + x);
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				}
			}
			else {
				for (int x = X_SEGMENTS; x >= 0; --x) {
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
					indices.push_back(y * (X_SEGMENTS + 1) + x);
				}
			}
			oddRow = !oddRow;
		}
		planeIndexCount = indices.size();

		std::vector<float> data;
		for (size_t i = 0; i < positions.size(); ++i) {
			data.push_back(positions[i].x);
			data.push_back(positions[i].y);
			data.push_back(positions[i].z);
			if (uv.size() > 0) {
				data.push_back(uv[i].x);
				data.push_back(uv[i].y);
			}
			if (normals.size() > 0) {
				data.push_back(normals[i].x);
				data.push_back(normals[i].y);
				data.push_back(normals[i].z);
			}
		}

		glBindVertexArray(planeVAO);
		glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
		glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), &indices[0], GL_STATIC_DRAW);
		float stride = (3 + 2 + 3) * sizeof(float);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
	}

	glBindVertexArray(planeVAO);
	glDrawElements(GL_TRIANGLE_STRIP, planeIndexCount, GL_UNSIGNED_INT, 0);
}

int main()
{
	// ---------------------- rect light source configuration ----------------------
	rectLight rect;
	rect.center = glm::vec3(0.0f, 0.0f, 10.0f);
	rect.dir_width = glm::vec3(1.0f, 0.0f, 0.0f);
	rect.dir_height = glm::vec3(0.0f, 1.0f, 0.0f);
	rect.half_height = 2.0f;
	rect.half_width = 2.0f;
	rect.color = glm::vec3(150.0f, 150.0f, 150.0f);

	glfwInit();

	GLFWwindow* window = glfwCreateWindow(g_width, g_height, window_name, NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initalize GLAD" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);

	uint32_t LTC_texture1, LTC_texture2; 
	// LTC_texture1 : LTC Inv_M
	// LTC_texture2 : LTC Frensel Approximation

	glGenTextures(1, &LTC_texture1);
	glBindTexture(GL_TEXTURE_2D, LTC_texture1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, LTC::textureSize, LTC::textureSize, 0, GL_RGBA, GL_FLOAT, LTC::data1);
	glGenerateMipmap(GL_TEXTURE_2D);

	glGenTextures(1, &LTC_texture2);
	glBindTexture(GL_TEXTURE_2D, LTC_texture2);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, LTC::textureSize, LTC::textureSize, 0, GL_RGBA, GL_FLOAT, LTC::data2);
	glGenerateMipmap(GL_TEXTURE_2D);

	Shader LTCShader("../LTCShadingVert.glsl", "../LTCShadingFrag.glsl");
	Shader lightShader("../lightVert.glsl", "../lightFrag.glsl");

	// ---------------------- Shader Configuration ----------------------

	LTCShader.use();
	LTCShader.setInt("LTC_texture1", 0);
	LTCShader.setInt("LTC_texture2", 1);
	LTCShader.setVec3("rect.center", rect.center);
	LTCShader.setVec3("rect.dir_width", rect.dir_width);
	LTCShader.setVec3("rect.dir_height", rect.dir_height);
	LTCShader.setFloat("rect.half_width", rect.half_width);
	LTCShader.setFloat("rect.half_height", rect.half_height);
	LTCShader.setVec3("rect.color", rect.color);

	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), static_cast<float>(g_width)
		/ static_cast<float>(g_height), 0.1f, 100.0f);
	LTCShader.setMat4("projection", projection);

	// ---------------------- rect light source position ----------------------

	glm::vec3 p0 = rect.center + rect.dir_width * rect.half_width + rect.dir_height * rect.half_height;
	glm::vec3 p1 = rect.center - rect.dir_width * rect.half_width + rect.dir_height * rect.half_height;
	glm::vec3 p2 = rect.center - rect.dir_width * rect.half_width - rect.dir_height * rect.half_height;
	glm::vec3 p3 = rect.center + rect.dir_width * rect.half_width - rect.dir_height * rect.half_height;

	float lightVertices[] = {
		p0.x, p0.y, p0.z,
	    p1.x, p1.y, p1.z,
		p2.x, p2.y, p2.z,
		p3.x, p3.y, p3.z,
	};

	uint32_t lightIndices[] = {
		0, 1, 3,
		1, 2, 3
	};
	uint32_t lightVAO, lightVBO, lightEBO;

	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);

	glGenBuffers(1, &lightVBO);
	glGenBuffers(1, &lightEBO);

	glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(lightVertices), lightVertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lightEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(lightIndices), lightIndices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	int nrRows = 5;
	int nrColumns = 7;
	float spacing = 2.5;

	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 view = camera.GetViewMatrix();

		LTCShader.use();
		LTCShader.setMat4("view", view);
		LTCShader.setVec3("camPos", camera.Position);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, LTC_texture1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, LTC_texture2);

		// ---------------------- rendering sphere ----------------------

		LTCShader.setVec3("albedo", 0.5f, 0.0f, 0.0f);
		LTCShader.setFloat("ao", 1.0f);

		glm::mat4 model = glm::mat4(1.0f);
		for (int row = 0; row < nrRows; ++row) {
			LTCShader.setFloat("metallic", static_cast<float>(row) / static_cast<float>(nrRows));
			for (int col = 0; col < nrColumns; ++col) {
				LTCShader.setFloat("roughness", 0.6f * static_cast<float>(col) / static_cast<float>(nrColumns));
				model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(
					static_cast<float>(col - (nrColumns / 2)) * spacing,
					static_cast<float>(row - (nrRows / 2)) * spacing + 4.0f,
					0.0f
				));
				LTCShader.setMat4("model", model);
				renderSphere();
			}
		}

		// ---------------------- rendering plane ----------------------

		LTCShader.setVec3("albedo", 0.2f, 0.2f, 0.5f);
		LTCShader.setFloat("ao", 1.0f);
		LTCShader.setFloat("metallic", 0.2f);
		LTCShader.setFloat("roughness", 0.0f);

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, -2.0f, 10.0f));
		LTCShader.setMat4("model", model);
		renderPlane();

		// ---------------------- rendering rect light source ---------------------- 

		lightShader.use();

		lightShader.setMat4("projection", projection);
		lightShader.setMat4("view", view);
		model = glm::mat4(1.0f);
		lightShader.setMat4("model", model);

		glBindVertexArray(lightVAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &sphereVAO);
	glDeleteVertexArrays(1, &lightVAO);
	glDeleteBuffers(1, &lightVBO);
	glDeleteBuffers(1, &lightEBO);

	glfwTerminate();

	return 0;
}