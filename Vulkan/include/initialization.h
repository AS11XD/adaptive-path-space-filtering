#ifndef INITIALIZATION
#define INITIALIZATION
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <cstring>
#include <chrono>
#include <glm/gtc/type_ptr.hpp>
#include "sampledTexture.h"
#include "utils.h"
#include "denoiser.h"
#include "applyReferencePipeline.h"

class Scene;
class SkyBoxScene;

class Window;

class AppResources
{
public:
	bool enable_rtx_extension = false;
	Window* windowObj;
	vk::DescriptorPool imguiPool;

	vk::Queue transferQueue, computeQueue, graphicsQueue, presentQueue;
	uint32_t cQ, gQ, pQ, tQ;
	vk::CommandPool transferCommandPool, computeCommandPool, graphicsCommandPool;
	vk::Extent2D swapChainExtent;

	uint32_t frameCount = 0;
	uint32_t screenShotFrameCount = 0;
	uint32_t screenShotFrameNum = 0;
	int32_t screenShotFileType = 0;
	bool frameTimeProcessing = false;
	bool screenshotProcessing = false;
	bool useRefBeforeScreenshot = false;
	bool useRefNowScreenshot = false;

	vk::RenderPass preRenderPass;
	vk::RenderPass deferredRenderPass;
	vk::RenderPass deferredRenderPassPSF;
	vk::RenderPass imGuiRenderPass;
	std::vector<SampledTexture> swapChainImages;
	SampledTexture colorReference;
	std::vector<SampledTexture> colorImagesHDR;
	uint32_t currentImageIndex = 0;
	vk::Format swapChainFormat;

	SampledTexture color;
	SampledTexture position;
	SampledTexture normal;
	SampledTexture tangent;
	SampledTexture bitangent;
	SampledTexture depth;
	SampledTexture outputMask;
	SampledTexture outputPT;
	SampledTexture outputVariance;
	SampledTexture colorMsaa;
	SampledTexture positionMsaa;
	SampledTexture normalMsaa;
	SampledTexture tangentMsaa;
	SampledTexture bitangentMsaa;
	SampledTexture depthMsaa;

	vk::SampleCountFlagBits msaa;

	Scene* scene = nullptr;
	SkyBoxScene* skyBoxScene;

	Denoiser denoiser;
	ApplyReferencePipeline applyRefPipe;

	vk::Device device;
	vk::PhysicalDevice pDevice;

	std::chrono::steady_clock::time_point dt_timestamp = std::chrono::high_resolution_clock::now();
	float time_delta;

	void draw(Window* _windowObj);
	void initApp(Window* _windowObj, GLFWwindow* window, uint32_t _width, uint32_t _height, bool vsync);
	void initImGui();
	void onWindowResized(GLFWwindow* window, int _width, int _height);
	void onWindowSizeChanged(bool msaaChanged, bool vsync);
	void destroy(bool full = true);
	void recompileShaders();
	void takeScreenShot(uint32_t numFrames, int32_t fileType);
	void setFullscreen(bool fullscreen);
	uint32_t getHashMapSize();
	uint32_t getHashMapOccupation();
	float getAveragePathDepth();
	bool supportsBlit(vk::Format src, vk::ImageTiling srcTiling, vk::Format dst, vk::ImageTiling dstTiling);

private:
	bool first_draw = true;
	bool first_compute = true;
	int width, height;

	uint64_t fenceValue = 0;
	vk::Instance instance;
	vk::DebugUtilsMessengerEXT dbgUtilsMgr;
	vk::PhysicalDeviceProperties2KHR pDeviceProperties;
	vk::PhysicalDeviceSubgroupProperties pDeviceSubgroupProperties;

	vk::QueryPool graphicsQueryPool;

	vk::SurfaceKHR surface;
	vk::SwapchainKHR swapChain, oldSwapChain;

	std::vector<vk::Framebuffer> swapChainPreFramebuffers;
	std::vector<vk::Framebuffer> swapChainDeferredFramebuffers;
	std::vector<vk::Framebuffer> swapChainDeferredFramebuffersPSF;
	std::vector<vk::Framebuffer> swapChainDeferredFramebuffersImGui;

	vk::Semaphore imageAvailableSemaphore, renderingFinishedSemaphore;

	std::vector<vk::CommandBuffer> graphicsCommandBuffers;
	std::vector<vk::CommandBuffer> graphicsCommandBuffers2;

	bool windowResized = false;

	void createInstance(vk::DebugUtilsMessengerEXT& debugUtilsMessenger,
						std::string appName, std::string engineName);

	void createSurface(GLFWwindow* window);
	std::tuple<uint32_t, uint32_t, uint32_t, uint32_t> getComputeTransferGraphicsPresentQueues();

	void selectPhysicalDevice();
	void createLogicalDevice();
	void createCommandPool(vk::CommandPool& commandPool, uint32_t queueIndex);

	void destroyInstance(vk::DebugUtilsMessengerEXT& debugUtilsMessenger);
	void destroyLogicalDevice(vk::Device& device);
	void destroyCommandPool(vk::CommandPool& commandPool);
	void showAvailableQueues(bool diagExt);

	void createTimestampQueryPool(vk::QueryPool& queryPool, uint32_t queryCount);
	void destroyQueryPool(vk::QueryPool& queryPool);

	void createSwapchain(vk::SwapchainKHR& oldSwapChain, uint32_t width, uint32_t height, bool vsync);

	void printDeviceCapabilities();
	void checkSwapChainSupport();

	void createPreRenderPass();
	void createDeferredRenderPass();
	void createImageViews();

	void createAttachments();
	void createAttachment(const std::string& name, SampledTexture& att, vk::Format format, vk::SampleCountFlagBits msaaBits, vk::ImageUsageFlags usageFlags, vk::ImageAspectFlagBits aspectFlags);

	void createFramebuffersPrePass();
	void createFramebuffersDeferred();

	void updateUniformData();
	void updateCommandBuffer(uint32_t i);
	void updateCommandBuffer2(uint32_t i);
	void createCommandBuffers();
};

VKAPI_ATTR VkBool32 VKAPI_CALL
debugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
							VkDebugUtilsMessageTypeFlagsEXT messageTypes,
							VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
							void* /*pUserData*/);
vk::DebugUtilsMessengerCreateInfoEXT makeDebugUtilsMessengerCreateInfoEXT();

vk::SurfaceFormatKHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& surfaceCapabilities, uint32_t width, uint32_t height);
vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR> presentModes, bool vsync);

vk::Format findSupportedFormat(vk::PhysicalDevice& pDevice,
							   const std::vector<vk::Format>& candidates,
							   vk::ImageTiling tiling,
							   vk::FormatFeatureFlags features);

bool checkFormatSupported(vk::PhysicalDevice& pDevice, vk::Format format, vk::FormatFeatureFlags requestedSupport);

vk::Format findDepthFormat(vk::PhysicalDevice& pDevice);

#endif