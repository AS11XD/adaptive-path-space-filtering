#ifndef SAVE_FILE_CONVERTER
#include "guiControl.h"

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <glm/gtx/transform.hpp>
#include <imgui.h>
#include <iconFontCppHeaders/IconsFontAwesome6.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include "initialization.h"
#include "scene.h"
#include "skyBoxScene.h"

//#define A_DEBUG
#ifdef A_DEBUG
struct AllocationMetrics
{
	uint32_t allocated = 0;
	uint32_t freed = 0;
	void printCurrentUsage() { std::cout << (allocated - freed) << " bytes used\n"; }
};

static AllocationMetrics allocMetrics;

void* operator new (size_t size)
{
	allocMetrics.allocated += size;
	return malloc(size);
}

void operator delete(void* memory, size_t size)
{
	allocMetrics.freed += size;
	free(memory);
}

void operator delete[](void* memory, size_t size)
{
	allocMetrics.freed += size;
	free(memory);
}

#endif // A_DEBUG

uint32_t GuiControl::addTimer(AppResources* app)
{
	uint32_t res = 0;
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
				ImGui::GetIO().Framerate);

	float renderTime = 0.0f;
	for (auto& it : graphicsTimings.container)
	{
		renderTime += it.second.averageData(var.average_over);
	}

	if (ImGui::TreeNode("graphicsTimings", "Graphics %.3f ms/frame", renderTime))
	{
		for (auto& it : graphicsTimings.container)
		{
			ImGui::Text((it.first + " %.3f ms/frame").c_str(), it.second.oldAverageData());
		}

		if (ImGui::TreeNode("renderTimingsPlot", "Plot"))
		{
			if (ImPlot::BeginPlot("Render Time"))
			{
				ImPlot::SetupAxes("Frame", "Time [ms]", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
				for (auto& it : graphicsTimings.container)
				{
					ImPlot::PlotLine((it.first + " Time [ms]").c_str(), it.second.getDataPointer(), it.second.getMaxSize());
				}
				ImPlot::EndPlot();
			}
			ImGui::TreePop();
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("allTimingsPlot", "Plot"))
	{
		std::vector<std::pair<std::string, PlotData<float>>> plotdataSums(graphicsTimings.container.size());
		int i = 0;
		for (auto& it : graphicsTimings.container)
		{
			if (i == 0)
				plotdataSums[i] = std::pair<std::string, PlotData<float>>(it.first, it.second);
			else
				plotdataSums[i] = std::pair<std::string, PlotData<float>>(it.first, plotdataSums[i - 1].second + it.second);
			++i;
		}

		if (ImPlot::BeginPlot("Frame Time"))
		{
			ImPlot::SetupAxes("Frame", "Time [ms]", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

			for (int j = plotdataSums.size() - 1; j >= 0; --j)
			{
				ImPlot::PlotShaded(plotdataSums[j].first.c_str(), plotdataSums[j].second.getDataPointer(), plotdataSums[j].second.getMaxSize());
			}
			ImPlot::EndPlot();
		}

		ImGui::Checkbox("Advanced Graph Save", &advancedGraphSave);
		if (advancedGraphSave)
		{
			ImGui::Separator();
			ImGui::Indent();

			ImGui::Combo("Render Mode", &agsp.renderMode, renderModesNames, RENDER_MODE::RM_NUM);

			if (agsp.savefilePath == "")
			{
				if (ImGui::Button("Load Save File##2"))
				{
					ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey-load-ss", "load from file", ".sav", "./sav/");
					dialogOpen = true;
				}
				// display
				if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey-load-ss"))
				{
					// action if OK
					if (ImGuiFileDialog::Instance()->IsOk())
					{
						std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
						filePathName.erase(std::remove(filePathName.begin(), filePathName.end(), '\0'), filePathName.end());
						// action
						agsp.savefilePath = filePathName;
					}

					dialogOpen = false;
					// close
					ImGuiFileDialog::Instance()->Close();
				}
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.0f, 0.0f, 1.0f));
				if (ImGui::Button(ICON_FA_XMARK "##5"))
				{
					agsp.savefilePath = "";
				}
				ImGui::PopStyleColor(3);
				ImGui::SameLine();
				ImGui::Text(agsp.savefilePath.c_str());
			}

			if (agsp.trajectoryPath == "")
			{
				if (ImGui::Button("Load Trajectory##2"))
				{
					ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey-load-traj-ss", "load from file", ".traj", "./traj/");
					dialogOpen = true;
				}
				// display
				if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey-load-traj-ss"))
				{
					// action if OK
					if (ImGuiFileDialog::Instance()->IsOk())
					{
						std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
						filePathName.erase(std::remove(filePathName.begin(), filePathName.end(), '\0'), filePathName.end());
						// action
						agsp.trajectoryPath = filePathName;
					}

					dialogOpen = false;
					// close
					ImGuiFileDialog::Instance()->Close();
				}
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.0f, 0.0f, 1.0f));
				if (ImGui::Button(ICON_FA_XMARK "##6"))
				{
					agsp.trajectoryPath = "";
				}
				ImGui::PopStyleColor(3);
				ImGui::SameLine();
				ImGui::Text(agsp.trajectoryPath.c_str());
			}

			ImGui::Unindent();
			ImGui::Separator();

			if (ImGui::Button("save##4"))
			{
				graphicsTimings.reset();
				if (agsp.savefilePath != "")
				{
					loadFromFile(app, res, agsp.savefilePath);
				}
				var.currentRenderMode = agsp.renderMode;

				if (var.currentRenderMode != RM_ADAPTIVE_PATH_SPACE_FILTERING_1 && var.currentRenderMode != RM_ADAPTIVE_PATH_SPACE_FILTERING_2)
				{
					windows["A-PSF"].open = false;
				}
				else
				{
					windows["A-PSF"].open = true;
				}
				if (agsp.trajectoryPath != "")
				{
					loadTrajectoryFromFile(app, agsp.trajectoryPath);
				}
				else
				{
					trajectoryMode = false;
				}
				app->frameCount = 0;
				app->frameTimeProcessing = true;
			}
		}
		else
		{
			saveGraph();
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("timerControl", "Timer Control"))
	{
		var.average_over = std::max(var.average_over, 1);
		std::string frames = (var.average_over > 1) ? " frames" : " frame";
		ImGui::InputInt(("Average over " + std::to_string(var.average_over) + frames).c_str(), &var.average_over, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue);
		ImGui::TreePop();
	}

	return res;
}

void GuiControl::addTrajectoryControl(AppResources* app)
{
	if (!trajectoryRecordMode && ImGui::Button("Record"))
	{
		trajectoryRecordMode = true;
		ct.points.clear();
		ct.points.push_back({ camera.getPosition(), camera.getLookAt() });
		std::cout << "current camera position and direction added to trajectory\n";
	}

	if (trajectoryRecordMode)
	{
		if (ImGui::Button("Add Current Position"))
		{
			ct.points.push_back({ camera.getPosition(), camera.getLookAt() });
			std::cout << "Current camera position and direction added to trajectory\n";
		}

		ImGui::SameLine();

		ImGui::Text("Recorded: %i", ct.points.size());

		ImGui::SameLine();

		if (ImGui::Button("Cancel"))
		{
			trajectoryRecordMode = false;
		}

		ImGui::SameLine();

		if (ct.points.size() > 1)
		{
			saveTrajectory(app);
		}
	}
	ImGui::SameLine();

	loadTrajectory(app);

	if (trajectoryMode)
	{
		if (ImGui::Combo("Clamp Mode", &var.currentTrajClamp, trajClampNames, TRAJ_CLAMP::TRAJ_CLAMP_NUM))
		{
			camera.trajectoryIdx = 0.0f;
			camera.trajectoryReverse = false;
		}
		if (ImGui::Combo("Interpolation Mode", &var.currentTrajInterpolation, trajInterpolNames, TRAJ_INTERPOLATION::TRAJ_INTERPOL_NUM))
		{
			camera.trajectoryIdx = 0.0f;
			camera.trajectoryReverse = false;
		}
		ImGui::SliderFloat("Speed", &var.trajSpeed, -10.0f, 10.0f);
		if (var.trajSpeed != 0.0f && ImGui::Button("Pause"))
		{
			var.trajSpeed = 0.0f;
		}
		if (var.trajSpeed == 0.0f && ImGui::Button("Resume"))
		{
			var.trajSpeed = 0.5f;
		}
		if (var.currentTrajInterpolation == TRAJ_INTERPOLATION::TRAJ_HERMITE)
		{
			ImGui::SliderFloat("Tension", &var.trajTension, 0.0f, 10.0f);
			ImGui::SliderFloat("Bias", &var.trajBias, 0.0f, 10.0f);
		}
	}
}

void GuiControl::addLights()
{
	if (lights)
	{
		if (ImGui::TreeNode("lights", "Light Control"))
		{
			for (auto& light : *lights)
			{
				bool tmp = light.light.on;
				ImGui::Checkbox(light.name.c_str(), &tmp);
				light.light.on = tmp;
				if (tmp)
				{
					if (light.name != "Sun" && light.name != "Moon" && light.name != "Torch")
					{
						ImGui::SameLine();
						ImGui::SliderFloat((light.name + " Intensity").c_str(), &light.light.intensity, 0.0f, 1000.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
					}
					else if (light.name == "Sun")
					{
						ImGui::SameLine();
						ImGui::SliderFloat((light.name + " Intensity").c_str(), &Sun::intensity, 0.0f, 1000.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
					}
					else if (light.name == "Moon")
					{
						ImGui::SameLine();
						ImGui::SliderFloat((light.name + " Intensity").c_str(), &Moon::intensity, 0.0f, 1000.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
					}
					else
					{
						ImGui::SameLine();
						ImGui::SliderFloat((light.name + " Intensity").c_str(), &Torch::intensity, 0.0f, 1000.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
					}
				}
			}
			if (ImGui::Button("All on"))
			{
				for (auto& light : *lights)
				{
					light.light.on = true;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("All off"))
			{
				for (auto& light : *lights)
				{
					light.light.on = false;
				}
			}
			ImGui::TreePop();
		}
	}
}

PlotResult GuiControl::getUVfHMPlot(size_t sampleCount, float xStart, float xEnd)
{
	PlotResult pr;
	pr.xs.resize(sampleCount);
	pr.ys.resize(sampleCount);
	float xStep = (xEnd - xStart) / (sampleCount - 1);
	for (size_t i = 0; i < sampleCount; ++i)
	{
		float xVal = xStart + i * xStep;
		pr.xs[i] = xVal;
		switch (var.varianceFuncitonUVfHM)
		{
			case VFT_ONE_OVER_X_SQUARED:
				pr.ys[i] = oneOverXSqFunction(xVal, xStart, xEnd, var.varianceThresholdsUVfHM);
				break;
			case VFT_SMOOTH_LINEAR:
				pr.ys[i] = smoothLinearFunction(xVal, xStart, xEnd, var.varianceThresholdsUVfHM);
				break;
			case VFT_STEP:
				pr.ys[i] = stepFunction(xVal, xStart, xEnd, var.varianceThresholdsUVfHM);
				break;
			case VFT_LINEAR:
				pr.ys[i] = linearFunction(xVal, xStart, xEnd, var.varianceThresholdsUVfHM);
				break;
			case VFT_ONE:
				pr.ys[i] = oneFunction(xVal, xStart, xEnd, var.varianceThresholdsUVfHM);
				break;
			case VFT_ZERO:
				pr.ys[i] = zeroFunction(xVal, xStart, xEnd, var.varianceThresholdsUVfHM);
				break;
			default:
				break;
		}
	}
	return pr;
}

float GuiControl::oneOverXSqFunction(float x, float xStart, float xEnd, float varianceThreshold[2])
{
	float xVal = (x - xStart) / (xEnd - xStart);
	xVal -= ((varianceThreshold[1] + varianceThreshold[0]) * 0.5f) / (xEnd - xStart);
	float scal = (varianceThreshold[1] - varianceThreshold[0]) / (xEnd - xStart);
	xVal /= scal * 2.0f;
	float y = -(1.0f - scal * 0.2f) / (xVal * xVal * 16.0f + 1.0f) + 1.0f;
	y = std::min(y, 1.0f);
	y = std::max(y, 0.0f);
	return y;
}

PlotResult GuiControl::getPSRPlot(size_t sampleCount, float xStart, float xEnd)
{
	PlotResult pr;
	pr.xs.resize(sampleCount);
	pr.ys.resize(sampleCount);
	float xStep = (xEnd - xStart) / (sampleCount - 1);
	for (size_t i = 0; i < sampleCount; ++i)
	{
		float xVal = xStart + i * xStep;
		pr.xs[i] = xVal;
		switch (var.varianceFunctionPSR)
		{
			case VFT_ONE_OVER_X_SQUARED:
				pr.ys[i] = oneOverXSqFunction(xVal, xStart, xEnd, var.varianceThresholdsPSR);
				break;
			case VFT_SMOOTH_LINEAR:
				pr.ys[i] = smoothLinearFunction(xVal, xStart, xEnd, var.varianceThresholdsPSR);
				break;
			case VFT_STEP:
				pr.ys[i] = stepFunction(xVal, xStart, xEnd, var.varianceThresholdsPSR);
				break;
			case VFT_LINEAR:
				pr.ys[i] = linearFunction(xVal, xStart, xEnd, var.varianceThresholdsPSR);
				break;
			case VFT_ONE:
				pr.ys[i] = oneFunction(xVal, xStart, xEnd, var.varianceThresholdsPSR);
				break;
			case VFT_ZERO:
				pr.ys[i] = zeroFunction(xVal, xStart, xEnd, var.varianceThresholdsPSR);
				break;
			default:
				break;
		}
	}
	return pr;
}

float GuiControl::smoothstep(float edge0, float edge1, float x)
{
	float t = (x - edge0) / (edge1 - edge0);
	t = std::min(t, 1.0f);
	t = std::max(t, 0.0f);
	return t * t * (3.0f - 2.0f * t);
}

float GuiControl::smoothLinearFunction(float x, float xStart, float xEnd, float varianceThreshold[2])
{
	float xVal = (x - xStart) / (xEnd - xStart);
	float scal = (varianceThreshold[1] - varianceThreshold[0]) / (xEnd - xStart);
	float mid = (varianceThreshold[1] + varianceThreshold[0]) * 0.5f / (xEnd - xStart);
	float y = smoothstep(0.0f, (1.0f - scal * 0.2f), (xVal - mid) / (scal * 5.0f) + (1.0f - scal * 0.2f) * 0.5f);
	y = std::min(y, 1.0f);
	y = std::max(y, 0.0f);
	return y;
}

float GuiControl::stepFunction(float x, float xStart, float xEnd, float varianceThreshold[2])
{
	return (x >= varianceThreshold[0] && x <= varianceThreshold[1]) ? 0.0f : 1.0f;
}

float GuiControl::linearFunction(float x, float xStart, float xEnd, float varianceThreshold[2])
{
	return std::max(0.0f, std::min(1.0f, (x - varianceThreshold[0]) / (varianceThreshold[1] - varianceThreshold[0])));
}

float GuiControl::oneFunction(float x, float xStart, float xEnd, float varianceThreshold[2])
{
	return 1.0f;
}

float GuiControl::zeroFunction(float x, float xStart, float xEnd, float varianceThreshold[2])
{
	return 0.0f;
}

void GuiControl::flyAlongTrajectory(float dt)
{
#ifdef A_DEBUG
	allocMetrics.printCurrentUsage();
#endif // A_DEBUG
	if (ct.points.size() > 1)
	{
		moveAlongTrajectory(camera, ct, var.currentTrajClamp, var.currentTrajInterpolation, dt, var.trajSpeed, var.trajTension, var.trajBias);
	}
}

ImTextureID GuiControl::AddTexture(AppResources* app, SampledTexture* texture)
{
	return ImGui_ImplVulkan_AddTexture(texture->sampler, texture->imageView, VK_IMAGE_LAYOUT_GENERAL);
}

uint32_t GuiControl::takeScreenShot(AppResources* app, uint32_t framesWaited, int32_t fileType)
{
	if (!var.hideImGui)
	{
		app->takeScreenShot(framesWaited, fileType);
		return 0;
	}
	else
	{
		app->screenShotFrameNum = framesWaited;
		app->screenShotFileType = fileType;
		ImGui::End();
		return (uint32_t)HIDE_IMGUI_SCREENSHOT;
	}
}

void GuiControl::setScene(AppResources* app, SampledTexture* renderedCubemap)
{
#ifdef A_DEBUG
	allocMetrics.printCurrentUsage();
#endif // A_DEBUG

	if (app->scene)
	{
		lights = nullptr;
		app->scene->cleanUp(app, true);
		delete app->scene;
	}
	switch (var.currentScene)
	{
		case SCENE::GROUDON:
			app->scene = new Scene("res/obj/groudon/Groudon.obj", app, this, renderedCubemap, var.normalMapping, glm::scale(glm::vec3(0.02f)));
			break;

		case SCENE::BISTRO_EXTERIOR:
			app->scene = new Scene("res/fbx/Bistro_v5/BistroExterior.fbx", app, this, renderedCubemap, var.normalMapping, glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f), true);
			break;

		case SCENE::BISTRO_INTERIOR:
			app->scene = new Scene("res/fbx/Bistro_v5/BistroInterior.fbx", app, this, renderedCubemap, var.normalMapping, glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f), true);
			break;

		case SCENE::BISTRO:
			app->scene = new Scene(std::vector<std::string>({ "res/fbx/Bistro_v5/BistroExterior.fbx", "res/fbx/Bistro_v5/BistroInterior.fbx" }),
								   app, this, renderedCubemap, var.normalMapping, glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f), true);
			break;

		case SCENE::SUN_TEMPLE:
			app->scene = new Scene("res/fbx/SunTemple_v4/SunTemple.fbx", app, this, renderedCubemap, var.normalMapping);
			break;

		case SCENE::ZERO_DAY:
			app->scene = new Scene("res/fbx/ZeroDay_v1/MEASURE_ONE.fbx", app, this, renderedCubemap, var.normalMapping);
			break;

		case SCENE::EMERALD_SQUARE:
			app->scene = new Scene("res/fbx/EmeraldSquare_v4_1/EmeraldSquare_Day.fbx", app, this, renderedCubemap, var.normalMapping);
			break;

		case SCENE::CORRIDOR:
			app->scene = new Scene("res/fbx/Corridor/corridor.fbx", app, this, renderedCubemap, var.normalMapping);
			break;

		default:
			break;
	}

#ifdef A_DEBUG
	allocMetrics.printCurrentUsage();
#endif // A_DEBUG
}

void GuiControl::reloadShader(AppResources* app)
{
	app->scene->cleanUp(app, false);
	app->skyBoxScene->cleanUp(app, false);
	app->applyRefPipe.cleanUpPipeline(app, true);
	// recompile shaders
	app->recompileShaders();
	app->scene->createPipelines(app);
	app->skyBoxScene->createPipelines(app);
	app->denoiser.reCreatePipelines(app);
	app->applyRefPipe.generatePipeline(app, true);
	initTextures(app);
}

void GuiControl::setMSAA(AppResources* app)
{
	switch (var.currentMsaa)
	{
		case MSAA::x1:
			app->msaa = vk::SampleCountFlagBits::e1;
			break;
		case MSAA::x2:
			app->msaa = vk::SampleCountFlagBits::e2;
			break;
		case MSAA::x4:
			app->msaa = vk::SampleCountFlagBits::e4;
			break;
		case MSAA::x8:
			app->msaa = vk::SampleCountFlagBits::e8;
			break;
		default:
			break;
	}
}

void GuiControl::initTextures(AppResources* app)
{
	varianceImg = AddTexture(app, &app->outputVariance);
	ptImg = AddTexture(app, &app->outputPT);
	maskImg = AddTexture(app, &app->outputMask);
	brightnessImg = AddTexture(app, &app->scene->brightness);
}

void GuiControl::save(AppResources* app)
{
	if (ImGui::Button("save##1"))
	{
		time_t curr_time = time(0);
		struct tm newtime;
		char date_string[100];

		time(&curr_time);
#ifdef _WIN64
		localtime_s(&newtime, &curr_time);
#elif _WIN32
		localtime_s(curr_time, &curr_time);
#elif __linux__
		newtime = *localtime(&curr_time);
#endif
		strftime(date_string, 50, "%B-%d-%Y_%H-%M-%S", &newtime);

		ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey-save", "save to file", ".sav", "./sav/" + std::string(sceneNames[var.currentScene]) + "_" + std::string(date_string) + ".sav");

		dialogOpen = true;
	}
	// display
	if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey-save"))
	{
		// action if OK
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
			filePathName.erase(std::remove(filePathName.begin(), filePathName.end(), '\0'), filePathName.end());
			// action
			std::fstream outFile(filePathName, std::ios::out | std::ios::binary);
			outFile.write((char*)&var, sizeof(var));
			outFile.seekg(sizeof(var), std::ios::beg);
			outFile.write((char*)&camera, sizeof(camera));
			outFile.close();
			std::cout << "saved to " << filePathName << "\n";
		}

		dialogOpen = false;
		// close
		ImGuiFileDialog::Instance()->Close();
	}
}

void GuiControl::saveAsp(AppResources* app, GuiAdvancedScreenshotPreset& asp)
{
	if (ImGui::Button("save##2"))
	{
		time_t curr_time = time(0);
		struct tm newtime;
		char date_string[100];

		time(&curr_time);
#ifdef _WIN64
		localtime_s(&newtime, &curr_time);
#elif _WIN32
		localtime_s(curr_time, &curr_time);
#elif __linux__
		newtime = *localtime(&curr_time);
#endif
		strftime(date_string, 50, "%B-%d-%Y_%H-%M-%S", &newtime);

		ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey-save-asp", "save to file", ".asp", "./asp/" + std::string(renderModesNames[asp.renderMode]) + "_" + std::string(date_string) + ".asp");

		dialogOpen = true;
	}
	// display
	if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey-save-asp"))
	{
		// action if OK
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
			filePathName.erase(std::remove(filePathName.begin(), filePathName.end(), '\0'), filePathName.end());
			// action
			std::fstream outFile(filePathName, std::ios::out | std::ios::binary);
			std::streamoff offset = 0;
			writeToOpenedFile(outFile, offset, asp.savefilePath);
			writeToOpenedFile(outFile, offset, asp.trajectoryPath);
			outFile.seekg(offset, std::ios::beg);
			outFile.write((char*)&asp + sizeof(asp.savefilePath) + sizeof(asp.trajectoryPath), sizeof(asp) - sizeof(asp.savefilePath) - sizeof(asp.trajectoryPath));
			outFile.close();
			std::cout << "saved to " << filePathName << "\n";
		}

		dialogOpen = false;
		// close
		ImGuiFileDialog::Instance()->Close();
	}
}

void GuiControl::saveGraphToFile(PlotDataContainer<float>& plotDataContainer, const std::string& path)
{
	std::fstream outFile(path, std::ios::out);
	for (auto& it : plotDataContainer.container)
	{
		outFile << it.first << "\n";

		for (size_t i = 0; i < it.second.getCurrentSize(); ++i)
		{
			outFile << it.second.getDataPointer()[i];
			if (i < it.second.getCurrentSize() - 1)
				outFile << ",";
		}
		outFile << "\n";
	}

	outFile.close();
}

void GuiControl::saveGraph()
{
	if (ImGui::Button("save##4"))
	{
		graphicsTimingsTemp = PlotDataContainer(graphicsTimings);

		time_t curr_time = time(0);
		struct tm newtime;
		char date_string[100];

		time(&curr_time);
#ifdef _WIN64
		localtime_s(&newtime, &curr_time);
#elif _WIN32
		localtime_s(curr_time, &curr_time);
#elif __linux__
		newtime = *localtime(&curr_time);
#endif
		strftime(date_string, 50, "%B-%d-%Y_%H-%M-%S", &newtime);

		ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey-save-csv", "save to file", ".csv", "./csv/" + std::string(date_string) + ".csv");

		dialogOpen = true;
	}
	// display
	if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey-save-csv"))
	{
		// action if OK
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
			filePathName.erase(std::remove(filePathName.begin(), filePathName.end(), '\0'), filePathName.end());
			// action
			saveGraphToFile(graphicsTimingsTemp, filePathName);
			std::cout << "saved to " << filePathName << "\n";
		}

		dialogOpen = false;
		// close
		ImGuiFileDialog::Instance()->Close();
	}
}

void GuiControl::saveTrajectory(AppResources* app)
{
	if (ImGui::Button("save##3"))
	{
		time_t curr_time = time(0);
		struct tm newtime;
		char date_string[100];

		time(&curr_time);
#ifdef _WIN64
		localtime_s(&newtime, &curr_time);
#elif _WIN32
		localtime_s(curr_time, &curr_time);
#elif __linux__
		newtime = *localtime(&curr_time);
#endif
		strftime(date_string, 50, "%B-%d-%Y_%H-%M-%S", &newtime);

		ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey-save-traj", "save to file", ".traj", "./traj/" + std::string(date_string) + ".traj");

		dialogOpen = true;
	}
	// display
	if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey-save-traj"))
	{
		// action if OK
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
			filePathName.erase(std::remove(filePathName.begin(), filePathName.end(), '\0'), filePathName.end());
			// action
			std::fstream outFile(filePathName, std::ios::out | std::ios::binary);
			std::streamoff offset = 0;
			writeToOpenedFile(outFile, offset, ct.points);
			outFile.close();
			trajectoryRecordMode = false;
			std::cout << "trajectory saved to " << filePathName << "\n";
			camera.trajectoryIdx = 0.0f;
			camera.trajectoryReverse = false;
			trajectoryRecordMode = false;
			trajectoryMode = true;
		}

		dialogOpen = false;
		// close
		ImGuiFileDialog::Instance()->Close();
	}
}

void GuiControl::load(AppResources* app, uint32_t& changed)
{
	if (ImGui::Button("load##1"))
	{
		ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey-load", "load from file", ".sav", "./sav/");
		dialogOpen = true;
	}
	// display
	if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey-load"))
	{
		// action if OK
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
			filePathName.erase(std::remove(filePathName.begin(), filePathName.end(), '\0'), filePathName.end());
			// action
			loadFromFile(app, changed, filePathName);
		}

		dialogOpen = false;
		// close
		ImGuiFileDialog::Instance()->Close();
	}
}

void GuiControl::loadFromFile(AppResources* app, uint32_t& changed, std::string filePathName)
{
	GuiVariables oldVar = var;

	std::fstream inFile(filePathName, std::ios::in | std::ios::binary);
	inFile.read((char*)&var, sizeof(var));
	inFile.seekg(sizeof(var), std::ios::beg);
	inFile.read((char*)&camera, sizeof(camera));
	inFile.close();

	if (oldVar.currentScene != var.currentScene || oldVar.normalMapping != var.normalMapping)
	{
		setScene(app, &app->skyBoxScene->renderedCubemap);
		initTextures(app);
	}

	app->scene->clearHashMap(app);

	if (oldVar.currentMsaa != var.currentMsaa)
	{
		setMSAA(app);
		app->device.waitIdle();
		ImGui_ImplGlfw_Shutdown();
		ImGui_ImplVulkan_Shutdown();
		ImPlot::DestroyContext();
		ImGui::DestroyContext();
		app->onWindowSizeChanged(true, var.vsync);
		changed |= MSAA_CHANGED;
	}

	if (oldVar.fullscreen != var.fullscreen)
	{
		app->setFullscreen(var.fullscreen);
	}

	if (oldVar.vsync != var.vsync)
	{
		app->device.waitIdle();
		ImGui_ImplGlfw_Shutdown();
		ImGui_ImplVulkan_Shutdown();
		ImPlot::DestroyContext();
		ImGui::DestroyContext();
		app->onWindowSizeChanged(true, var.vsync);
		changed |= MSAA_CHANGED;
	}

	if (oldVar.currentRenderMode != var.currentRenderMode)
	{
		if (var.currentRenderMode == RM_ADAPTIVE_PATH_SPACE_FILTERING_1 || var.currentRenderMode == RM_ADAPTIVE_PATH_SPACE_FILTERING_2)
		{
			windows["A-PSF"].open = true;
		}
		else
		{
			windows["A-PSF"].open = false;
		}
	}

	camera.updateScreenSize(app->swapChainExtent.width, app->swapChainExtent.height);

	std::cout << "loaded from " << filePathName << "\n";
}

void GuiControl::loadTrajectory(AppResources* app)
{
	if (ImGui::Button("load##2"))
	{
		ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey-load-traj", "load from file", ".traj", "./traj/");
		dialogOpen = true;
	}
	// display
	if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey-load-traj"))
	{
		// action if OK
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
			filePathName.erase(std::remove(filePathName.begin(), filePathName.end(), '\0'), filePathName.end());
			// action

			loadTrajectoryFromFile(app, filePathName);
		}

		dialogOpen = false;
		// close
		ImGuiFileDialog::Instance()->Close();
	}
}

void GuiControl::loadTrajectoryFromFile(AppResources* app, std::string filePathName)
{
	std::fstream inFile(filePathName, std::ios::in | std::ios::binary);
	std::streamoff offset = 0;
	readFromOpenedFile(inFile, offset, ct.points);
	inFile.close();

	camera.trajectoryIdx = 0.0f;
	camera.trajectoryReverse = false;
	trajectoryRecordMode = false;
	trajectoryMode = true;

	std::cout << "trajectory loaded from " << filePathName << "\n";
}

void GuiControl::loadAsp(AppResources* app)
{
	if (ImGui::Button("load##3"))
	{
		ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey-load-asp", "load from file", ".asp", "./asp/", 100);
		dialogOpen = true;
	}
	// display
	if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey-load-asp"))
	{
		// action if OK
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			auto selection = ImGuiFileDialog::Instance()->GetSelection(); // multiselection
			if (selection.size() > 1)
			{
				asps.clear();
			}
			for (auto& s : selection)
			{
				std::string filePathName = s.second;
				filePathName.erase(std::remove(filePathName.begin(), filePathName.end(), '\0'), filePathName.end());
				GuiAdvancedScreenshotPreset asp;
				loadAspFromFile(app, asp, filePathName);
				asps.emplace_back(std::move(asp));
			}
		}

		dialogOpen = false;
		// close
		ImGuiFileDialog::Instance()->Close();
	}
}

void GuiControl::loadAspFromFile(AppResources* app, GuiAdvancedScreenshotPreset& asp, std::string filePathName)
{
	std::fstream inFile(filePathName, std::ios::in | std::ios::binary);
	std::streamoff offset = 0;
	readFromOpenedFile(inFile, offset, asp.savefilePath);
	readFromOpenedFile(inFile, offset, asp.trajectoryPath);
	inFile.seekg(offset, std::ios::beg);
	inFile.read((char*)&asp + sizeof(asp.savefilePath) + sizeof(asp.trajectoryPath), sizeof(asp) - sizeof(asp.savefilePath) - sizeof(asp.trajectoryPath));
	inFile.close();

	std::cout << "advanced screenshot preset loaded from " << filePathName << "\n";
}

uint32_t GuiControl::layout(AppResources* app)
{
	uint32_t changed = 0x0;

	ImGui::DockSpaceOverViewport(NULL, ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingInCentralNode);
	mainLayout(app);
	psfLayout(app);
	sstLayout(app);

	for (auto& gw : windows)
	{
		if (gw.second.open)
		{
			if (gw.second.closable)
				ImGui::Begin(gw.first.c_str(), &gw.second.open);
			else
				ImGui::Begin(gw.first.c_str());

			changed |= gw.second.body(app, changed);
			if (changed != 0x0)
				return changed;
			ImGui::End();
		}
	}
	return changed;
}

void GuiControl::mainLayout(AppResources* app)
{
	windows["Main"].body = [&](AppResources* app, uint32_t c)
	{
		uint32_t changed = c;
		changed |= addTimer(app);
		if (changed & MSAA_CHANGED)
			return (uint32_t)MSAA_CHANGED;

		ImGui::Separator();

		if (ImGui::CollapsingHeader("Tools"))
		{
			if (ImGui::Button("Open Screenshot Tool"))
			{
				windows["Screenshot Tool"].open = true;
			}

			if (ImGui::Button("Reload Shaders"))
			{
				reloadShader(app);
			}

			if ((referenceMode) ? ImGui::TreeNode("Ground Truth Renderer", "Ground Truth Renderer %i / %i", currentReferenceIteration, var.referenceIterations) : ImGui::TreeNode("Ground Truth Renderer", "Ground Truth Renderer"))
			{
				if (!referenceMode)
				{
					if (ImGui::Button("Render Ground Truth"))
					{
						referenceMode = true;
						currentReferenceIteration = 0;
						app->device.waitIdle();
						ImGui_ImplGlfw_Shutdown();
						ImGui_ImplVulkan_Shutdown();
						ImPlot::DestroyContext();
						ImGui::DestroyContext();
						app->onWindowSizeChanged(true, var.vsync);
						return (uint32_t)MSAA_CHANGED;
					}
					ImGui::SameLine();
					ImGui::InputInt("Iterations", &var.referenceIterations);
				}
				else
				{
					if (ImGui::Button("Stop"))
					{
						referenceMode = false;
						currentReferenceIteration = 0;
						app->device.waitIdle();
						ImGui_ImplGlfw_Shutdown();
						ImGui_ImplVulkan_Shutdown();
						ImPlot::DestroyContext();
						ImGui::DestroyContext();
						app->onWindowSizeChanged(true, var.vsync);
						return (uint32_t)MSAA_CHANGED;
					}
				}

				ImGui::Checkbox("Use Exponential Moving Average", &var.useExponentialMovingAverage);

				if (var.useExponentialMovingAverage)
				{
					ImGui::SameLine();
					ImGui::SliderFloat("Exponential Moving Average Alpha", &var.exponentialMovingAverageAlpha, 0.0f, 1.0f);
				}

				ImGui::TreePop();
			}

			if (ImGui::Checkbox("Fullscreen", &var.fullscreen))
			{
				app->setFullscreen(var.fullscreen);
			}
			ImGui::SameLine();

			if (ImGui::Checkbox("V-Sync", &var.vsync))
			{
				app->device.waitIdle();
				ImGui_ImplGlfw_Shutdown();
				ImGui_ImplVulkan_Shutdown();
				ImPlot::DestroyContext();
				ImGui::DestroyContext();
				app->onWindowSizeChanged(true, var.vsync);
				return (uint32_t)MSAA_CHANGED;
			}

			if (var.fullscreen)
			{
				ImGui::SameLine();
				if (ImGui::Button("Exit Application"))
				{
					app->device.waitIdle();
					ImGui_ImplGlfw_Shutdown();
					ImGui_ImplVulkan_Shutdown();
					ImPlot::DestroyContext();
					ImGui::DestroyContext();
					app->destroy();
					exit(0);
				}
			}
		}

		ImGui::Separator();
		if (ImGui::CollapsingHeader("Scene"))
		{
			save(app);
			ImGui::SameLine();
			load(app, changed);
			if (changed & MSAA_CHANGED)
				return changed;

			if (ImGui::Combo("Scenes", &var.currentScene, sceneNames, SCENE::SCENE_NUM))
			{
				setScene(app, &app->skyBoxScene->renderedCubemap);
				initTextures(app);
			}

			if (ImGui::Checkbox("Load Normal Maps", &var.normalMapping))
			{
				setScene(app, &app->skyBoxScene->renderedCubemap);
				initTextures(app);
			}
		}
		ImGui::Separator();

		if (ImGui::CollapsingHeader("Trajectories"))
		{
			addTrajectoryControl(app);
		}

		ImGui::Separator();

		if (ImGui::CollapsingHeader("Sky Box Settings"))
		{
			ImGui::Checkbox("Auto-Day-Night Cycle", &var.autoDayCycle);
			if (!var.autoDayCycle)
			{
				ImGui::SliderFloat("Skybox Time", &var.time, 0.0f, 24.0f);
			}
			else
			{
				ImGui::SliderFloat("Minutes per Cycle", &var.timeDelta, 0.02f, 20.0f);
			}

			ImGui::Checkbox("Cloud Auto-Time", &var.autoCloudTimer);

			if (!var.autoCloudTimer)
			{
				if (ImGui::Button("<"))
				{
					var.cloudTimer -= 6.0f;
				}
				ImGui::SameLine();
				if (ImGui::Button(">"))
				{
					var.cloudTimer += 6.0f;
				}
			}
			else
			{
				ImGui::SliderFloat("Wind Speed", &var.cloudTimerDelta, 0.0f, 20.0f);
			}
		}

		ImGui::Separator();

		if (ImGui::CollapsingHeader("Render Settings"))
		{
			if (ImGui::Combo("MSAA", &var.currentMsaa, msaaNames, MSAA::MSAA_NUM))
			{
				setMSAA(app);
				app->device.waitIdle();
				ImGui_ImplGlfw_Shutdown();
				ImGui_ImplVulkan_Shutdown();
				ImPlot::DestroyContext();
				ImGui::DestroyContext();
				app->onWindowSizeChanged(true, var.vsync);
				return (uint32_t)MSAA_CHANGED;
			}

			ImGui::Combo("Render State", &var.currentRenderState, renderStateNames, RENDER_STATE::RS_NUM);
			if (ImGui::Combo("Render Mode", &var.currentRenderMode, renderModesNames, RENDER_MODE::RM_NUM))
			{
				if (var.currentRenderMode != RM_ADAPTIVE_PATH_SPACE_FILTERING_1 && var.currentRenderMode != RM_ADAPTIVE_PATH_SPACE_FILTERING_2)
				{
					windows["A-PSF"].open = false;
				}
				else
				{
					windows["A-PSF"].open = true;
				}
			}

			if (var.currentRenderState == RS_COLOR)
			{
				if (ImGui::Checkbox("Denoise", &var.denoise))
				{
					app->device.waitIdle();
					ImGui_ImplGlfw_Shutdown();
					ImGui_ImplVulkan_Shutdown();
					ImPlot::DestroyContext();
					ImGui::DestroyContext();
					app->onWindowSizeChanged(true, var.vsync);
					return (uint32_t)MSAA_CHANGED;
				}
			}

			if (var.currentRenderMode == RM_PATH_TRACING || var.currentRenderMode == RM_PATH_SPACE_FILTERING ||
				var.currentRenderMode == RM_ADAPTIVE_PATH_SPACE_FILTERING_1 || var.currentRenderMode == RM_ADAPTIVE_PATH_SPACE_FILTERING_2 ||
				var.currentRenderMode == RM_RAY_TRACING || var.currentRenderMode == RM_SAMPLE_VALIDATION)
			{
				ImGui::SliderInt("Recursion Depth", &var.pathIterations, 0, 10);
				if (var.currentRenderMode == RM_PATH_TRACING || var.currentRenderMode == RM_PATH_SPACE_FILTERING ||
					var.currentRenderMode == RM_ADAPTIVE_PATH_SPACE_FILTERING_1 || var.currentRenderMode == RM_ADAPTIVE_PATH_SPACE_FILTERING_2 ||
					var.currentRenderMode == RM_SAMPLE_VALIDATION)
				{
					if (var.currentRenderMode == RM_PATH_SPACE_FILTERING || var.currentRenderMode == RM_ADAPTIVE_PATH_SPACE_FILTERING_1 || var.currentRenderMode == RM_ADAPTIVE_PATH_SPACE_FILTERING_2)
					{
						ImGui::Combo("PSF Mode", &var.currentPsfMode, psfModeNames, PSF_MODE::PSFM_NUM);
						ImGui::SliderFloat("Grid Scale", &var.gridScale, 1e-3f, 10.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
						ImGui::SliderInt("Eviction Time", &var.evictionTime, 1, 10000, "%d", ImGuiSliderFlags_Logarithmic);
						ImGui::SliderInt("Max Samples", &var.maxSamples, 1, 10000, "%d", ImGuiSliderFlags_Logarithmic);
						ImGui::Checkbox("Grid Jitter", &var.gridJitter);
						if (var.currentPsfMode == PSFM_HASH_MAP_OCCUPATION)
						{
							float hashMapOccupation = (float)app->getHashMapOccupation() / calculateHashMapSize(app, app->getHashMapSize());
							ImGui::Text("Current Hash Map Occupation: %.3f%%", hashMapOccupation * 100.0f);
						}

						if (var.currentRenderMode == RM_ADAPTIVE_PATH_SPACE_FILTERING_1 || var.currentRenderMode == RM_ADAPTIVE_PATH_SPACE_FILTERING_2)
						{
							if (ImGui::Button("Open Variance Controls"))
							{
								windows["A-PSF"].open = true;
							}
						}
					}
					ImGui::Combo("BRDF Sampling Strategy", &var.sampleStrategyBRDF, brdfSampleStrategyNames, BRDF_SAMPLE_STRATEGY::BRDFSS_NUM);
					ImGui::Checkbox("Disable First Bounce", &var.disableFirstBounce);
					ImGui::Checkbox("Sample Environment Map", &var.sampleEnvironmentMap);
					addLights();
				}
			}
			ImGui::Checkbox("Jitter", &var.jitter);

			ImGui::SliderFloat("Gamma", &var.gamma, 0.5f, 5.0f);
			ImGui::Checkbox("Tone Mapping", &var.toneMapping);
			if (var.toneMapping)
			{
				ImGui::SliderFloat("Exposure", &var.exposure, 1e-3f, 1e3f, "%.3f", ImGuiSliderFlags_Logarithmic);
			}
			if (var.normalMapping)
				ImGui::Checkbox("Use Normal Maps", &var.drawNormalMaps);

			ImGui::Checkbox("Render Line Visualization", &var.renderLines);

			if (var.renderLines)
			{
				ImGui::SameLine();
				ImGui::Checkbox("Keep Current Lines", &var.keepCurrentLines);
			}
		}
		ImGui::Separator();

		if (ImGui::CollapsingHeader("Key Bindings"))
		{
			ImGui::Text("Use 'W', 'A', 'S', 'D', 'Q' and 'E' to navigate.");
			ImGui::Text("Hold the right mouse button to look around.");
			ImGui::Text("Scroll up to increase and down to decrease movement speed.");
			ImGui::Text("Press 'G' to hide/show this GUI.");

			ImGui::SliderFloat("Movement Speed", &var.movement_scale, 0.5f, 50.0f);
		}

		return changed;
	};
}

void GuiControl::psfLayout(AppResources* app)
{
	windows["A-PSF"].body = [&](AppResources* app, uint32_t c)
	{
		uint32_t changed = c;

		ImVec2 imageSize = ImVec2(ImGui::GetWindowWidth(), (ImGui::GetWindowWidth() * app->swapChainExtent.height) / app->swapChainExtent.width);
		ImGui::Image(varianceImg, imageSize);
		ImGui::PushStyleColor(ImGuiCol_Text, 0xffff00ff);
		ImGui::Text("Variance is negative");
		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_Text, 0xffffff00);
		ImGui::Text("Variance is nan");
		ImGui::PopStyleColor();
		ImGui::PushStyleColor(ImGuiCol_Text, 0xffffffff);
		ImGui::Text("Variance is inf");
		ImGui::PopStyleColor();
		ImGui::ColorGradientBar("Variance", varianceColors.size(), varianceColors.data(), varianceValues.data());

		if (var.currentRenderMode == RM_ADAPTIVE_PATH_SPACE_FILTERING_2)
		{
			ImGui::SliderFloat("Variance Constant", &var.varianceConst, 0.0f, 10000.0f, "%.5f", ImGuiSliderFlags_Logarithmic);
			ImGui::Checkbox("Use Primary Methods", &var.usePrimaryMethods);
		}
		if (var.currentRenderMode == RM_ADAPTIVE_PATH_SPACE_FILTERING_1 || var.usePrimaryMethods)
		{
			ImGui::SliderFloatRange("Use Value from HashMap Variance-Thresholds", var.varianceThresholdsUVfHM, varianceThresholdColors.data(), varianceThresholdColors.size() - 1, varianceValues[0], varianceValues[varianceValues.size() - 1]);
			ImGui::SliderFloatRange("Path Survival Rate Variance-Thresholds", var.varianceThresholdsPSR, varianceThresholdColors.data(), varianceThresholdColors.size() - 1, varianceValues[0], varianceValues[varianceValues.size() - 1]);
		}
		if (var.currentRenderMode == RM_ADAPTIVE_PATH_SPACE_FILTERING_1 || var.usePrimaryMethods)
		{
			ImGui::Combo("Use Value from HashMap Function Type", &var.varianceFuncitonUVfHM, varianceFunctionTypeNames, VARIANCE_FUNCTION_TYPE::VFT_NUM);
			ImGui::Combo("Path Survival Rate Function Type", &var.varianceFunctionPSR, varianceFunctionTypeNames, VARIANCE_FUNCTION_TYPE::VFT_NUM);

			if (ImPlot::BeginPlot("Variance Exploitation", ImVec2(-1, 0), ImPlotFlags_AntiAliased))
			{
				ImPlot::SetupAxes("Variance", "Probability");
				ImPlot::SetupAxisLimits(ImAxis_X1, varianceValues[0], varianceValues[varianceValues.size() - 1], ImPlotCond_Always);
				ImPlot::SetupAxisLimits(ImAxis_Y1, -0.1f, 1.1f, ImPlotCond_Always);
				PlotResult& plotUVfHM = getUVfHMPlot(200, varianceValues[0], varianceValues[varianceValues.size() - 1]);
				PlotResult& plotStopPath = getPSRPlot(200, varianceValues[0], varianceValues[varianceValues.size() - 1]);
				ImPlot::PlotLine("Use Value from HashMap", plotUVfHM.xs.data(), plotUVfHM.ys.data(), plotUVfHM.xs.size());
				ImPlot::PlotLine("Path Survival Rate", plotStopPath.xs.data(), plotStopPath.ys.data(), plotStopPath.xs.size());
				ImPlot::EndPlot();
			}
		}
		if (var.currentRenderMode == RM_ADAPTIVE_PATH_SPACE_FILTERING_1)
		{
			ImGui::Checkbox("Show Path Tracing Result", &var.showPTComparison);
			ImGui::Image(maskImg, imageSize);
		}
		ImGui::Image(ptImg, imageSize);
		if (var.currentRenderMode == RM_ADAPTIVE_PATH_SPACE_FILTERING_2)
		{
			ImGui::ColorGradientBar("Iterations", iterColors.size(), iterColors.data(), iterValues.data());
			float averagePathDepth = app->getAveragePathDepth();
			ImGui::Text("Current Average Path Depth: %.3f", averagePathDepth);
		}
		ImGui::Image(brightnessImg, imageSize);
		return changed;
	};
}

void GuiControl::sstLayout(AppResources* app)
{
	windows["Screenshot Tool"].body = [&](AppResources* app, uint32_t c)
	{
		uint32_t changed = c;

		ImGui::Checkbox("Take Advanced Screenshot", &advancedScreenshot);
		if (advancedScreenshot)
		{
			ImGui::Separator();
			ImGui::Indent();

			uint32_t i = 0;
			for (auto& asp : asps)
			{
				std::string treeId = "asp-" + std::to_string(i);
				if (ImGui::TreeNode(treeId.c_str(), "Advanced Screenshot Settings %d", i))
				{
					ImGui::Combo("Render Mode", &asp.renderMode, renderModesNames, RENDER_MODE::RM_NUM);
					ImGui::InputInt("Wait Frames", &asp.framesWaited);
					ImGui::InputInt("Start at Frame Number", &asp.startAtFrame);

					if (asp.savefilePath == "")
					{
						if (ImGui::Button("Load Save File"))
						{
							ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey-load-ss", "load from file", ".sav", "./sav/");
							dialogOpen = true;
						}
						// display
						if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey-load-ss"))
						{
							// action if OK
							if (ImGuiFileDialog::Instance()->IsOk())
							{
								std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
								filePathName.erase(std::remove(filePathName.begin(), filePathName.end(), '\0'), filePathName.end());
								// action
								asp.savefilePath = filePathName;
							}

							dialogOpen = false;
							// close
							ImGuiFileDialog::Instance()->Close();
						}
					}
					else
					{
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.0f, 0.0f, 1.0f));
						if (ImGui::Button(ICON_FA_XMARK "##1"))
						{
							asp.savefilePath = "";
						}
						ImGui::PopStyleColor(3);
						ImGui::SameLine();
						ImGui::Text(asp.savefilePath.c_str());
					}

					if (asp.trajectoryPath == "")
					{
						if (ImGui::Button("Load Trajectory"))
						{
							ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey-load-traj-ss", "load from file", ".traj", "./traj/");
							dialogOpen = true;
						}
						// display
						if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey-load-traj-ss"))
						{
							// action if OK
							if (ImGuiFileDialog::Instance()->IsOk())
							{
								std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
								filePathName.erase(std::remove(filePathName.begin(), filePathName.end(), '\0'), filePathName.end());
								// action
								asp.trajectoryPath = filePathName;
							}

							dialogOpen = false;
							// close
							ImGuiFileDialog::Instance()->Close();
						}
					}
					else
					{
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.0f, 0.0f, 1.0f));
						if (ImGui::Button(ICON_FA_XMARK "##2"))
						{
							asp.trajectoryPath = "";
						}
						ImGui::PopStyleColor(3);
						ImGui::SameLine();
						ImGui::Text(asp.trajectoryPath.c_str());
					}

					ImGui::Checkbox("Render as Reference", &asp.renderReference);
					if (asp.renderReference)
					{
						ImGui::InputInt("Iterations", &asp.referenceIterations);
						ImGui::Checkbox("Use Exponential Moving Average", &asp.useExponentialMovingAverage);

						if (asp.useExponentialMovingAverage)
						{
							ImGui::SameLine();
							ImGui::SliderFloat("Exponential Moving Average Alpha", &asp.exponentialMovingAverageAlpha, 0.0f, 1.0f);
						}
					}

					saveAsp(app, asp);
					if (asps.size() > 1)
					{
						ImGui::SameLine();
						if (ImGui::Button("discard"))
						{
							asps.erase(asps.begin() + i);
						}
					}
					ImGui::TreePop();
				}
				i++;
			}
			ImGui::Separator();
			if (ImGui::Button("new"))
			{
				asps.push_back(
					{
						"",
						"",
						0,
						0,
						var.currentRenderMode,
						false,
						1000,
						false,
						0.009f
					});
			}
			ImGui::SameLine();
			loadAsp(app);

			bool savePath = false;
			for (auto& asp : asps)
			{
				if (asp.savefilePath != "")
				{
					savePath = true;
					break;
				}
			}
			if (savePath)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.0f, 0.0f, 1.0f));
				if (ImGui::Button(ICON_FA_XMARK "##3"))
				{
					for (auto& asp : asps)
						asp.savefilePath = "";
				}
				ImGui::PopStyleColor(3);
				ImGui::SameLine();
			}

			if (ImGui::Button("Load Save File All"))
			{
				ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey-load-ss-all", "load from file", ".sav", "./sav/");
				dialogOpen = true;
			}
			// display
			if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey-load-ss-all"))
			{
				// action if OK
				if (ImGuiFileDialog::Instance()->IsOk())
				{
					std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
					filePathName.erase(std::remove(filePathName.begin(), filePathName.end(), '\0'), filePathName.end());
					// action
					for (auto& asp : asps)
						asp.savefilePath = filePathName;
				}

				dialogOpen = false;
				// close
				ImGuiFileDialog::Instance()->Close();
			}
			bool anytrajPath = false;
			for (auto& asp : asps)
			{
				if (asp.trajectoryPath != "")
				{
					anytrajPath = true;
					break;
				}
			}
			if (anytrajPath)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.0f, 0.0f, 1.0f));
				if (ImGui::Button(ICON_FA_XMARK "##4"))
				{
					for (auto& asp : asps)
						asp.trajectoryPath = "";
				}
				ImGui::PopStyleColor(3);
				ImGui::SameLine();
			}
			if (ImGui::Button("Load Trajectory All"))
			{
				ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey-load-traj-ss-all", "load from file", ".traj", "./traj/");
				dialogOpen = true;
			}
			// display
			if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey-load-traj-ss-all"))
			{
				// action if OK
				if (ImGuiFileDialog::Instance()->IsOk())
				{
					std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
					filePathName.erase(std::remove(filePathName.begin(), filePathName.end(), '\0'), filePathName.end());
					// action
					for (auto& asp : asps)
						asp.trajectoryPath = filePathName;
				}

				dialogOpen = false;
				// close
				ImGuiFileDialog::Instance()->Close();
			}

			if (ImGui::Button("discard all"))
			{
				asps.clear();
				asps.push_back(
					{
						"",
						"",
						0,
						0,
						var.currentRenderMode,
						false,
						1000,
						false,
						0.009f
					});
			}

			ImGui::Unindent();
			ImGui::Separator();
		}

		ImGui::Combo("Format", &screenshotFileType, screenshotFileTypeNames, SCREENSHOT_FILE_TYPE::SFT_NUM);
		ImGui::Checkbox("Apply Python Pipeline Afterwards", &applyPythonPipeline);
		if (applyPythonPipeline)
		{
			ImGui::Separator();
			ImGui::Indent();

			if (pythonRectPathCSV == "")
			{
				if (ImGui::Button("Load Rectangle File"))
				{
					ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey-load-rf", "load from file", ".csv", "../Python/ImageHighlighter/figures/");
					dialogOpen = true;
				}
				// display
				if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey-load-rf"))
				{
					// action if OK
					if (ImGuiFileDialog::Instance()->IsOk())
					{
						std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
						filePathName.erase(std::remove(filePathName.begin(), filePathName.end(), '\0'), filePathName.end());
						// action
						pythonRectPathCSV = filePathName;
					}

					dialogOpen = false;
					// close
					ImGuiFileDialog::Instance()->Close();
				}
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.0f, 0.0f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.0f, 0.0f, 1.0f));
				if (ImGui::Button(ICON_FA_XMARK "##7"))
				{
					pythonRectPathCSV = "";
				}
				ImGui::PopStyleColor(3);
				ImGui::SameLine();
				ImGui::Text(pythonRectPathCSV.c_str());
			}

			ImGui::Unindent();
			ImGui::Separator();
		}

		if (ImGui::Button("Take Screenshot"))
		{
			uint32_t res = changed;
			if (advancedScreenshot)
			{
				for (auto& asp : asps)
					aspQueue.push(&asp);
			}
			else
			{
				res |= takeScreenShot(app, 0, screenshotFileType);
				if (res == HIDE_IMGUI_SCREENSHOT)
					return res;
			}
		}

		ImGui::SameLine();
		ImGui::Checkbox("Hide ImGui", &var.hideImGui);
		bool tmpHideImGui = var.hideImGui;

		if (!referenceMode)
		{
			if (aspQueue.size() > 0 && advancedScreenshot && !app->screenshotProcessing)
			{
				GuiAdvancedScreenshotPreset* asp = aspQueue.front();
				aspQueue.pop();

				uint32_t res = changed;
				app->frameCount = std::max(asp->startAtFrame, 0);
				app->screenShotFrameCount = std::max(asp->startAtFrame, 0);
				if (asp->savefilePath != "")
				{
					loadFromFile(app, res, asp->savefilePath);
					var.hideImGui = tmpHideImGui;
				}
				var.currentRenderMode = asp->renderMode;

				if (var.currentRenderMode != RM_ADAPTIVE_PATH_SPACE_FILTERING_1 && var.currentRenderMode != RM_ADAPTIVE_PATH_SPACE_FILTERING_2)
				{
					windows["A-PSF"].open = false;
				}
				else
				{
					windows["A-PSF"].open = true;
				}
				if (asp->trajectoryPath != "")
				{
					loadTrajectoryFromFile(app, asp->trajectoryPath);
				}
				else
				{
					trajectoryMode = false;
				}

				if (asp->renderReference)
				{
					var.referenceIterations = asp->referenceIterations;
					var.useExponentialMovingAverage = asp->useExponentialMovingAverage;
					if (asp->useExponentialMovingAverage)
						var.exponentialMovingAverageAlpha = asp->exponentialMovingAverageAlpha;
					app->useRefBeforeScreenshot = true;
				}

				res |= takeScreenShot(app, asp->framesWaited, screenshotFileType);
				if (res == HIDE_IMGUI_SCREENSHOT)
					return res;
			}
		}
		else
		{

		}
		return changed;
	};
}

uint32_t GuiControl::calculateHashMapSize(AppResources* app, uint32_t maxHashMapSize)
{
	float gridScaleCompensation = 0.016f / var.gridScale;
	//float iterationCompensation = var.pathIterations;
	float resolutionCompensation = (float)(app->swapChainExtent.width * app->swapChainExtent.height) / (1920.0f * 1080.0f);
	return std::min((uint32_t)floor(maxHashMapSize * gridScaleCompensation * resolutionCompensation), maxHashMapSize);
}

void writeToOpenedFile(std::fstream& file, std::streamoff& offset, std::string& data)
{
	size_t size = data.size();
	file.seekg(offset, std::ios::beg);
	file.write((char*)&size, sizeof(size));
	offset += sizeof(size);
	file.seekg(offset, std::ios::beg);
	if (size != 0)
	{
		file.write((char*)data.data(), sizeof(data[0]) * data.size());
		offset += sizeof(data[0]) * data.size();
	}
}

void readFromOpenedFile(std::fstream& file, std::streamoff& offset, std::string& data)
{
	size_t size = 0;
	file.seekg(offset, std::ios::beg);
	file.read((char*)&size, sizeof(size));
	offset += sizeof(size);
	data = "";
	if (size != 0)
	{
		file.seekg(offset, std::ios::beg);
		data.resize(size);
		file.read((char*)(data.data()), size * sizeof(data[0]));
		offset += size * sizeof(data[0]);
	}
}

#endif // !SAVE_FILE_CONVERTER
