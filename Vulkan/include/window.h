#pragma once
#include <iostream>
#include <string>
#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include "initialization.h"
#include "creation_utils.h"
#include "guiControl.h"

class Window
{
private:
	AppResources app;
	static glm::vec2 mouse_pos;
	static glm::vec2 mouseScroll;
	static int getKeyIndex(int key);
	static void onKeyPress(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void setCatchedMouseMode(bool activate, GLFWwindow* window);
	static void onMouseClick(GLFWwindow* window, int button, int action, int mods);
	static void onMouseScroll(GLFWwindow* window, double xoffset, double yoffset);
	void updateCamera();
	GLFWmonitor* get_current_monitor();
	uint32_t cube_size;
	std::array<int, 2> windowPosition{ 0, 0 };
	std::array<int, 2> windowSize{ 0, 0 };
	float floor_eps = 0.1f;
	float floor_size = 10.0f;
public:
	GLFWwindow* window;
	GLFWmonitor* monitor;
	Window(const std::string& name, int sizeX, int sizeY);
	~Window();
	void run();
	void ImGuiLayout();
	void cleanup();
	bool isFullscreen();
	void setFullscreen(bool fullscreen);
	void getSize(int* width, int* height)
	{
		glfwGetWindowSize(window, width, height);
	}
	GuiControl gui_control;
	enum keyIndex
	{
		KEY_W,
		KEY_A,
		KEY_S,
		KEY_D,
		KEY_Q,
		KEY_E,
		KEY_MOUSE_RIGHT,
		KEY_MOUSE_LEFT,
		KEY_MOUSE_WHEEL_SCROLL,
		// all keys need to set their state to true when they got pressed, but some need to reset their state on release, others do not
		HOLDING_KEYS,
		KEY_G,
		NUM_MAPPED_KEYS
	};
	enum windowStateIndex
	{
		MOUSE_CATCHED,
		NUM_WINDOW_STATES
	};
	// if we create a second window this vector is shared between the two, so every input goes to both windows (we probably will never do that)
	static std::vector<bool> pressedKeys;
	static std::vector<bool> windowStates;
	bool hide_imgui_screenshot = false;
};