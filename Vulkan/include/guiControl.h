#pragma once

#include <vector>

enum SCENE
{
	GROUDON, BISTRO_EXTERIOR, BISTRO_INTERIOR, BISTRO, SUN_TEMPLE, ZERO_DAY, EMERALD_SQUARE, CORRIDOR, SCENE_NUM
};

enum MSAA
{
	x1, x2, x4, x8, MSAA_NUM
};

enum TRAJ_CLAMP
{
	TRAJ_REVERT, TRAJ_LOOP, TRAJ_CLAMP_NUM
};

enum TRAJ_INTERPOLATION
{
	TRAJ_LINEAR, TRAJ_COSINE, TRAJ_CUBIC, TRAJ_HERMITE, TRAJ_INTERPOL_NUM
};

enum RENDER_STATE
{
	RS_COLOR, RS_G_BUFFER, RS_POSITION, RS_NORMAL, RS_MATERIAL_ID, RS_DEPTH, RS_BRIGHTNESS, RS_NUM
};

enum RENDER_MODE
{
	RM_DIFFUSE_COLOR, RM_SPECULAR_COLOR, RM_RTX_PREPASS, RM_RAY_TRACING, RM_PATH_TRACING, RM_SAMPLE_VALIDATION, RM_PATH_SPACE_FILTERING, RM_ADAPTIVE_PATH_SPACE_FILTERING_1, RM_ADAPTIVE_PATH_SPACE_FILTERING_2, RM_NUM
};

enum BRDF_SAMPLE_STRATEGY
{
	BRDFSS_UNIFROM, BRDFSS_DIFFUSE, BRDFSS_SPECULAR_GGX, BRDFSS_SPECULAR_DIFFUSE, BRDFSS_NUM
};

enum PSF_MODE
{
	PSFM_DEFAULT, PSFM_HASH_MAP_OCCUPATION, PSFM_DEBUG_HASH_CELLS, PSFM_DEBUG_HASH_FINGERPRINTS, PSFM_NUM
};

enum VARIANCE_FUNCTION_TYPE
{
	VFT_ONE_OVER_X_SQUARED, VFT_SMOOTH_LINEAR, VFT_STEP, VFT_LINEAR, VFT_ONE, VFT_ZERO, VFT_NUM
};

enum SCREENSHOT_FILE_TYPE
{
	SFT_PNG, SFT_HDR, SFT_NUM
};

struct GuiVariables
{
	int average_over = 1;
	int referenceIterations = 1000;
	bool useExponentialMovingAverage = false;
	float exponentialMovingAverageAlpha = 0.009f;
	float movement_scale = 10.0f;
	float time = 12.0f;
	float timeDelta = 2.0f;
	float trajSpeed = 0.5f;
	int currentTrajClamp = TRAJ_CLAMP::TRAJ_LOOP;
	int currentTrajInterpolation = TRAJ_INTERPOLATION::TRAJ_CUBIC;
	float trajTension = 0.0f;
	float trajBias = 0.0f;
	bool autoDayCycle = false;
	float cloudTimer = 0.0f;
	float cloudTimerDelta = 2.0f;
	bool autoCloudTimer = true;
	float gamma = 2.2f;
	float exposure = 1.0f;
	int currentScene = SCENE::GROUDON;
	bool normalMapping = true;
	bool hideImGui = true;
	int currentMsaa = MSAA::x1;
	int currentRenderState = RENDER_STATE::RS_COLOR;
	int currentRenderMode = RENDER_MODE::RM_ADAPTIVE_PATH_SPACE_FILTERING_1;
	int sampleStrategyBRDF = BRDF_SAMPLE_STRATEGY::BRDFSS_SPECULAR_DIFFUSE;
	int currentPsfMode = PSF_MODE::PSFM_DEFAULT;
	float gridScale = 0.24f;
	int maxSamples = 100;
	int evictionTime = 100;
	bool showPTComparison = false;
	bool usePrimaryMethods = false;
	int varianceFuncitonUVfHM = VARIANCE_FUNCTION_TYPE::VFT_ONE_OVER_X_SQUARED;
	int varianceFunctionPSR = VARIANCE_FUNCTION_TYPE::VFT_SMOOTH_LINEAR;
	float varianceThresholdsUVfHM[2] = { 0.4f, 1.2f };
	float varianceThresholdsPSR[2] = { 0.4f, 1.2f };
	float varianceConst = 0.6f;
	bool gridJitter = true;
	int pathIterations = 2;
	bool disableFirstBounce = false;
	bool sampleEnvironmentMap = true;
	bool denoise = false;
	bool toneMapping = true;
	bool drawNormalMaps = true;
	bool keepCurrentLines = false;
	bool renderLines = true;
	bool fullscreen = false;
	bool vsync = false;
	bool rtx_enabled = false;
	bool jitter = true;
};

#ifndef SAVE_FILE_CONVERTER


#include "camera.h"

#include <map>
#include <queue>
#include <imgui.h>
#include <imgui_orient.h>
#include <implot-master/implot.h>
#include <imGuiFileDialog/ImGuiFileDialog.h>
#include <imGuiFileDialog/ImGuiFileDialogConfig.h>

#include "plotData.h"
#include "plotDataContainer.h"
#include "trajectory.h"
#include "sampledTexture.h"
#include "light.h"

struct GuiControlWindow
{
	bool open;
	bool closable;
	std::function<uint32_t(AppResources*, uint32_t&)> body;
};

struct PlotResult
{
	std::vector<float> xs;
	std::vector<float> ys;
};

struct GuiAdvancedScreenshotPreset
{
	std::string savefilePath;
	std::string trajectoryPath;
	int32_t framesWaited;
	int32_t startAtFrame;
	int32_t renderMode;
	bool renderReference;
	int32_t referenceIterations;
	bool useExponentialMovingAverage;
	float exponentialMovingAverageAlpha;
};

struct GuiAdvancedGraphSavePreset
{
	std::string savefilePath;
	std::string trajectoryPath;
	int32_t renderMode;
};

class AppResources;

class GuiControl
{
public:
	enum
	{
		MSAA_CHANGED = 1,
		HIDE_IMGUI_SCREENSHOT = 2
	};

	const char* sceneNames[SCENE::SCENE_NUM] = { "Groudon", "Bistro Exterior", "Bistro Interior", "Bistro", "Sun Temple", "Zero Day", "Emerald Square", "Corridor" };
	const char* msaaNames[MSAA::MSAA_NUM] = { "x1", "x2", "x4", "x8" };
	const char* renderStateNames[RENDER_STATE::RS_NUM] = { "Color", "G-Buffer", "Position", "Normal", "Material Id", "Depth", "Brightness" };
	const char* renderModesNames[RENDER_MODE::RM_NUM] = { "Diffuse Color", "Specular Color","RTX-Prepass", "Ray Tracing", "Path Tracing", "Sample Validation", "Path Space Filtering", "Adaptive Path Space Filtering Primary", "Adaptive Path Graphs" };
	const char* brdfSampleStrategyNames[BRDF_SAMPLE_STRATEGY::BRDFSS_NUM] = { "Uniform", "Diffuse", "Specular GGX", "Diffuse and Specular GGX" };
	const char* psfModeNames[PSF_MODE::PSFM_NUM] = { "Default", "Hash-Map-Occupation", "Debug-Hash-Cells", "Debug-Hash-Fingerprint" };
	const char* trajClampNames[TRAJ_CLAMP::TRAJ_CLAMP_NUM] = { "Revert", "Loop" };
	const char* trajInterpolNames[TRAJ_INTERPOLATION::TRAJ_INTERPOL_NUM] = { "Linear", "Cosine", "Cubic", "Hermite" };
	const char* varianceFunctionTypeNames[VARIANCE_FUNCTION_TYPE::VFT_NUM] = { "One Over X Squared", "Smooth Linear", "Step", "Linear", "One", "Zero" };
	const char* screenshotFileTypeNames[SCREENSHOT_FILE_TYPE::SFT_NUM] = { "Portable Network Graphic", "High Dynamic Range" };

	bool applyPythonPipeline = false;
	std::string pythonRectPathCSV = "";
	bool advancedScreenshot = false;
	bool advancedGraphSave = false;
	std::vector<std::string> pythonPipelineImageNames;
	bool dialogOpen = false;
	bool referenceMode = false;
	int currentReferenceIteration = 0;
	bool trajectoryMode = false;
	bool trajectoryRecordMode = false;

	GuiControl(float fov, float width, float height) : camera(fov, width, height)
	{
	}

	GuiVariables var;
	std::vector<LightObj>* lights;
	PlotDataContainer<float> graphicsTimings = {
		{ "Sky Box Generation", PlotData<float>() },
		{ "Build RT Acceleration Structures", PlotData<float>() },
		{ "Hash Map Eviction", PlotData<float>() },
		{ "G-Buffer Prepass", PlotData<float>() },
		{ "Deferred RT Pass", PlotData<float>() },
		{ "PSF Pass", PlotData<float>() },
		{ "Denoiser", PlotData<float>() },
		{ "ImGui", PlotData<float>() }
	};

	Camera camera;
	std::map<std::string, GuiControlWindow> windows = {
		{
			"Main",
			{
				true,
				false
			}
		},
		{
			"Screenshot Tool",
			{
				true,
				true
			}
		},
		{
			"A-PSF",
			{
				var.currentRenderMode == RM_ADAPTIVE_PATH_SPACE_FILTERING_1 ||
				var.currentRenderMode == RM_ADAPTIVE_PATH_SPACE_FILTERING_2,
				true
			}
		}
	};

	std::array<ImU32, 3> varianceThresholdColors = {
		0xff95517a,
		0xff7556ef,
		0xff00a6ff
	};

	std::array<ImU32, 5> varianceColors = {
		0xff318f48,
		0xff3fa7a5,
		0xff6abdf4,
		0xff5080eb,
		0xff5b42de
	};

	std::array<ImU32, 11> iterColors = {
		0xff030000,
		0xff390b16,
		0xff670941,
		0xff6e176a,
		0xff672593,
		0xff5437bb,
		0xff3950dc,
		0xff1977f3,
		0xff0aa4fb,
		0xff45d7f5,
		0xffa4fefc
	};

	std::array<ImU32, 11> iterValues = {
		0,
		1,
		2,
		3,
		4,
		5,
		6,
		7,
		8,
		9,
		10
	};

	std::array<float, 5> varianceValues = {
		0.0f,
		0.4f,
		0.8f,
		1.2f,
		1.6f
	};

	void initTextures(AppResources* app);
	void setScene(AppResources* app, SampledTexture* renderedCubemap);
	void setMSAA(AppResources* app);
	void reloadShader(AppResources* app);
	void flyAlongTrajectory(float dt);
	uint32_t layout(AppResources* app);
	void mainLayout(AppResources* app);
	void psfLayout(AppResources* app);
	void sstLayout(AppResources* app);
	uint32_t calculateHashMapSize(AppResources* app, uint32_t maxHashMapSize);
	CameraTrajectory ct;
	void saveGraphToFile(PlotDataContainer<float>& plotDataContainer, const std::string& path);
	bool aspQueueEmpty() { return aspQueue.size() <= 0; }
private:
	GuiAdvancedGraphSavePreset agsp = {
		"",
		"",
		var.currentRenderMode
	};
	std::vector<GuiAdvancedScreenshotPreset> asps = {
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
		}
	};
	std::queue<GuiAdvancedScreenshotPreset*> aspQueue;
	PlotDataContainer<float> graphicsTimingsTemp;

	int32_t screenshotFileType = SCREENSHOT_FILE_TYPE::SFT_PNG;
	ImTextureID varianceImg;
	ImTextureID ptImg;
	ImTextureID maskImg;
	ImTextureID brightnessImg;
	ImTextureID AddTexture(AppResources* app, SampledTexture* texture);
	uint32_t takeScreenShot(AppResources* app, uint32_t framesWaited, int32_t fileType);
	void save(AppResources* app);
	void saveTrajectory(AppResources* app);
	void saveAsp(AppResources* app, GuiAdvancedScreenshotPreset& asp);
	void saveGraph();
	void load(AppResources* app, uint32_t& changed);
	void loadFromFile(AppResources* app, uint32_t& changed, std::string filePathName);
	void loadTrajectory(AppResources* app);
	void loadTrajectoryFromFile(AppResources* app, std::string filePathName);
	void loadAsp(AppResources* app);
	void loadAspFromFile(AppResources* app, GuiAdvancedScreenshotPreset& asp, std::string filePathName);
	uint32_t addTimer(AppResources* app);
	void addTrajectoryControl(AppResources* app);
	void addLights();
	PlotResult getUVfHMPlot(size_t sampleCount, float xStart, float xEnd);
	float oneOverXSqFunction(float x, float xStart, float xEnd, float varianceThreshold[2]);
	PlotResult getPSRPlot(size_t sampleCount, float xStart, float xEnd);
	float smoothstep(float edge0, float edge1, float x);
	float smoothLinearFunction(float x, float xStart, float xEnd, float varianceThreshold[2]);
	float stepFunction(float x, float xStart, float xEnd, float varianceThreshold[2]);
	float linearFunction(float x, float xStart, float xEnd, float varianceThreshold[2]);
	float oneFunction(float x, float xStart, float xEnd, float varianceThreshold[2]);
	float zeroFunction(float x, float xStart, float xEnd, float varianceThreshold[2]);
};

template<typename T>
void writeToOpenedFile(std::fstream& file, std::streamoff& offset, std::vector<T>& data)
{
	size_t size = data.size();
	file.seekg(offset, std::ios::beg);
	file.write((char*)&size, sizeof(size));
	offset += sizeof(size);
	file.seekg(offset, std::ios::beg);
	if (size != 0)
	{
		file.write((char*)data.data(), sizeof(T) * data.size());
		offset += sizeof(T) * data.size();
	}
}

template<typename T>
void readFromOpenedFile(std::fstream& file, std::streamoff& offset, std::vector<T>& data)
{
	size_t size = 0;
	file.seekg(offset, std::ios::beg);
	file.read((char*)&size, sizeof(size));
	offset += sizeof(size);
	data = std::vector<T>();
	if (size != 0)
	{
		file.seekg(offset, std::ios::beg);
		data.resize(size);
		file.read((char*)(data.data()), size * sizeof(T));
		offset += size * sizeof(T);
	}
}

void writeToOpenedFile(std::fstream& file, std::streamoff& offset, std::string& data);

void readFromOpenedFile(std::fstream& file, std::streamoff& offset, std::string& data);

#endif // !SAVE_FILE_CONVERTER
