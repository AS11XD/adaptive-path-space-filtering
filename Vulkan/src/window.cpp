#include "window.h"
#include "guiControl.h"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

std::vector<bool> Window::pressedKeys(NUM_MAPPED_KEYS, false);
std::vector<bool> Window::windowStates(NUM_WINDOW_STATES, false);
glm::vec2 Window::mouse_pos(0.0f);
glm::vec2 Window::mouseScroll(0.0f);

int Window::getKeyIndex(int key)
{
	switch (key)
	{
		case GLFW_KEY_W:
			return KEY_W;
		case GLFW_KEY_A:
			return KEY_A;
		case GLFW_KEY_S:
			return KEY_S;
		case GLFW_KEY_D:
			return KEY_D;
		case GLFW_KEY_Q:
			return KEY_Q;
		case GLFW_KEY_E:
			return KEY_E;
		case GLFW_KEY_G:
			// show/hide the gui
			return KEY_G;
		default:
			// unmapped key
			return -1;
	}
}

void Window::onKeyPress(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	int keyIndex = getKeyIndex(key);
	// if we need to hold the key, also toggle bool on release, otherwise don't
	if (action == GLFW_PRESS || (action == GLFW_RELEASE && keyIndex < HOLDING_KEYS))
	{
		if (keyIndex != -1) pressedKeys[keyIndex] = !pressedKeys[keyIndex];
	}
}

void Window::setCatchedMouseMode(bool activate, GLFWwindow* window)
{
	if (activate)
	{
		if (!windowStates[MOUSE_CATCHED])
		{
			if (glfwRawMouseMotionSupported())
			{
				windowStates[MOUSE_CATCHED] = true;
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
				double mouse_x_pos, mouse_y_pos;
				glfwGetCursorPos(window, &mouse_x_pos, &mouse_y_pos);
				mouse_pos = glm::vec2(mouse_x_pos, mouse_y_pos);
			}
			else
			{
				std::cerr << "Raw mouse motion not supported!" << std::endl;
			}
		}
	}
	else
	{
		if (windowStates[MOUSE_CATCHED])
		{
			windowStates[MOUSE_CATCHED] = false;
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}
}

void Window::onMouseClick(GLFWwindow* window, int button, int action, int mods)
{
	if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		pressedKeys[KEY_MOUSE_RIGHT] = true;
		setCatchedMouseMode(true, window);
	}
	else if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		pressedKeys[KEY_MOUSE_RIGHT] = false;
		setCatchedMouseMode(false, window);
	}
	if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT)
	{
		pressedKeys[KEY_MOUSE_LEFT] = true;
	}
	else if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_LEFT)
	{
		pressedKeys[KEY_MOUSE_LEFT] = false;
	}
}

void Window::onMouseScroll(GLFWwindow* window, double xoffset, double yoffset)
{
	if (glm::abs(xoffset) > 0.001f)
	{
		mouseScroll.x = xoffset;
		pressedKeys[KEY_MOUSE_WHEEL_SCROLL] = true;
	}
	if (glm::abs(yoffset) > 0.001f)
	{
		mouseScroll.y = yoffset;
		pressedKeys[KEY_MOUSE_WHEEL_SCROLL] = true;
	}
}

void Window::updateCamera()
{
	float time_delta = app.time_delta;

	if (!gui_control.referenceMode)
	{
		if (gui_control.trajectoryMode)
		{
			gui_control.flyAlongTrajectory(std::clamp(time_delta, 0.0f, 1.0f));
		}
		if (!gui_control.dialogOpen && !app.screenshotProcessing)
		{
			if (pressedKeys[KEY_W])
			{
				gui_control.camera.moveFront(time_delta * gui_control.var.movement_scale);
			}
			if (pressedKeys[KEY_A])
			{
				gui_control.camera.moveSideways(-time_delta * gui_control.var.movement_scale);
			}
			if (pressedKeys[KEY_S])
			{
				gui_control.camera.moveFront(-time_delta * gui_control.var.movement_scale);
			}
			if (pressedKeys[KEY_D])
			{
				gui_control.camera.moveSideways(time_delta * gui_control.var.movement_scale);
			}
			if (pressedKeys[KEY_Q])
			{
				gui_control.camera.moveDown(time_delta * gui_control.var.movement_scale);
			}
			if (pressedKeys[KEY_E])
			{
				gui_control.camera.moveDown(-time_delta * gui_control.var.movement_scale);
			}
			if (gui_control.trajectoryMode && (pressedKeys[KEY_W] || pressedKeys[KEY_A] || pressedKeys[KEY_S] || pressedKeys[KEY_D]
				|| pressedKeys[KEY_Q] || pressedKeys[KEY_E]))
			{
				gui_control.trajectoryMode = false;
			}
			if (windowStates[MOUSE_CATCHED] && !gui_control.trajectoryMode)
			{
				double mouse_x_pos, mouse_y_pos;
				glfwGetCursorPos(window, &mouse_x_pos, &mouse_y_pos);
				gui_control.camera.onMouseMove(mouse_pos.x - mouse_x_pos, mouse_pos.y - mouse_y_pos);
				mouse_pos = glm::vec2(mouse_x_pos, mouse_y_pos);
			}
			if (pressedKeys[KEY_MOUSE_WHEEL_SCROLL])
			{
				gui_control.var.movement_scale += 0.5f * mouseScroll.y;
				gui_control.var.movement_scale = glm::max(0.0f, gui_control.var.movement_scale);
				pressedKeys[KEY_MOUSE_WHEEL_SCROLL] = false;
			}
		}
	}
}

GLFWmonitor* Window::get_current_monitor()
{
	int nmonitors, i;
	int wx, wy, ww, wh;
	int mx, my, mw, mh;
	int overlap, bestoverlap;
	GLFWmonitor* bestmonitor;
	GLFWmonitor** monitors;
	const GLFWvidmode* mode;

	bestoverlap = 0;
	bestmonitor = NULL;

	glfwGetWindowPos(window, &wx, &wy);
	glfwGetWindowSize(window, &ww, &wh);
	monitors = glfwGetMonitors(&nmonitors);

	for (i = 0; i < nmonitors; i++)
	{
		mode = glfwGetVideoMode(monitors[i]);
		glfwGetMonitorPos(monitors[i], &mx, &my);
		mw = mode->width;
		mh = mode->height;

		overlap =
			std::max(0, std::min(wx + ww, mx + mw) - std::max(wx, mx)) *
			std::max(0, std::min(wy + wh, my + mh) - std::max(wy, my));

		if (bestoverlap < overlap)
		{
			bestoverlap = overlap;
			bestmonitor = monitors[i];
		}
	}

	return bestmonitor;
}

Window::Window(const std::string& name, int sizeX, int sizeY) : gui_control(70, sizeX, sizeY)
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(sizeX, sizeY, name.c_str(), nullptr, nullptr);
	// glfwSetWindowSizeCallback(window, TriangleApplication::onWindowResized);
	glfwSetKeyCallback(window, onKeyPress);
	glfwSetMouseButtonCallback(window, onMouseClick);
	glfwSetScrollCallback(window, onMouseScroll);

	monitor = glfwGetPrimaryMonitor();

	setFullscreen(gui_control.var.fullscreen);

	app.initApp(this, window, sizeX, sizeY, gui_control.var.vsync);

	gui_control.var.rtx_enabled = app.enable_rtx_extension;
}

Window::~Window()
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Window::ImGuiLayout()
{
	if (!pressedKeys[KEY_G] && !hide_imgui_screenshot)
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		if (!pressedKeys[KEY_MOUSE_RIGHT])
		{
			setCatchedMouseMode(false, window);
		}
		uint32_t changed = gui_control.layout(&app);
		if (changed & GuiControl::MSAA_CHANGED)
		{
			return;
		}
		if (changed & GuiControl::HIDE_IMGUI_SCREENSHOT)
		{
			app.time_delta = 0.002f;
			hide_imgui_screenshot = true;
		}
		ImGui::Render();
	}
	else
	{
		setCatchedMouseMode(true, window);
		if (hide_imgui_screenshot)
		{
			if (app.screenShotFrameNum > 0)
			{
				if (!app.screenshotProcessing)
				{
					app.takeScreenShot(app.screenShotFrameNum, app.screenShotFileType);
				}
			}
			else
			{
				app.takeScreenShot(app.screenShotFrameNum, app.screenShotFileType);
			}
		}
	}
}

void Window::run()
{
	app.dt_timestamp = std::chrono::high_resolution_clock::now();
	while (!glfwWindowShouldClose(window))
	{
		std::chrono::steady_clock::time_point temp_timestamp = std::chrono::high_resolution_clock::now();
		app.time_delta = (std::chrono::duration<float, std::milli>(temp_timestamp - app.dt_timestamp)).count() / 1000.0f;
		app.dt_timestamp = temp_timestamp;
		ImGuiLayout();
		app.time_delta = (app.screenshotProcessing || app.frameTimeProcessing) ? 0.002f : app.time_delta;
		app.draw(this);
		glfwPollEvents();
		updateCamera();
	}
}

void Window::cleanup()
{
	app.device.waitIdle();
	ImGui_ImplGlfw_Shutdown();
	ImGui_ImplVulkan_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();
	app.destroy();
}

bool Window::isFullscreen()
{
	return glfwGetWindowMonitor(window) != nullptr;
}

void Window::setFullscreen(bool fullscreen)
{
	if (isFullscreen() == fullscreen)
		return;

	if (fullscreen)
	{
		// backup window position and window size
		glfwGetWindowPos(window, &windowPosition[0], &windowPosition[1]);
		glfwGetWindowSize(window, &windowSize[0], &windowSize[1]);

		// get resolution of monitor
		monitor = get_current_monitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);

		gui_control.camera.updateScreenSize(mode->width, mode->height);
		// switch to full screen
		glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
	}
	else
	{
		// restore last window size and position
		glfwSetWindowMonitor(window, nullptr, windowPosition[0], windowPosition[1], windowSize[0], windowSize[1], 0);
	}
}
