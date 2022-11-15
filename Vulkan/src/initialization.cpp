#include <iostream>
#include <fstream>
#include <cstring>
#include <functional>
#include <optional>
#include <filesystem>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#define NOMINMAX
#define VK_USE_PLATFORM_WIN32_KHR
#define VK_ENABLE_BETA_EXTENSIONS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#include <glm/gtx/transform.hpp>
#include "initialization.h"
#include "utils.h"
#include "window.h"
#include <optional>
#include "guiControl.h"

#include <renderdoc.h>
#include "scene.h"
#include "skyBoxScene.h"
#include <iconFontCppHeaders/IconsFontAwesome6.h>

// here you create the instance and physical / logical device and maybe compute/transfer queues
// also check if device is suitable etc

struct DeviceSelectionCache
{
	uint32_t vendorID;
	uint32_t deviceID;
};

//#define VALIDATION_LAYERS

#ifndef VALIDATION_LAYERS
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {
#ifdef VALIDATION_LAYERS
		"VK_LAYER_KHRONOS_validation"
#endif
};
std::vector<const char*> instanceExtensions = {
#ifdef VALIDATION_LAYERS
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
};

const std::vector<const char*> extensionNames = {
#ifndef VALIDATION_LAYERS

#endif
};

void AppResources::destroy(bool full)
{
	device.freeCommandBuffers(graphicsCommandPool, graphicsCommandBuffers);
	device.freeCommandBuffers(graphicsCommandPool, graphicsCommandBuffers2);

	device.destroyRenderPass(preRenderPass);
	device.destroyRenderPass(deferredRenderPass);
	device.destroyRenderPass(deferredRenderPassPSF);
	device.destroyRenderPass(imGuiRenderPass);

	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		device.destroyImageView(swapChainImages[i].imageView);
		destroySampledTexture(device, colorImagesHDR[i]);
		device.destroyFramebuffer(swapChainPreFramebuffers[i]);
		device.destroyFramebuffer(swapChainDeferredFramebuffers[i]);
		device.destroyFramebuffer(swapChainDeferredFramebuffersPSF[i]);
		device.destroyFramebuffer(swapChainDeferredFramebuffersImGui[i]);
	}
	destroySampledTexture(device, colorReference);

	destroySampledTexture(device, color);
	destroySampledTexture(device, position);
	destroySampledTexture(device, normal);
	destroySampledTexture(device, tangent);
	destroySampledTexture(device, bitangent);
	destroySampledTexture(device, outputMask);
	destroySampledTexture(device, outputPT);
	destroySampledTexture(device, outputVariance);
	destroySampledTexture(device, depth);
	destroySampledTexture(device, colorMsaa);
	destroySampledTexture(device, positionMsaa);
	destroySampledTexture(device, normalMsaa);
	destroySampledTexture(device, tangentMsaa);
	destroySampledTexture(device, bitangentMsaa);
	destroySampledTexture(device, depthMsaa);

	skyBoxScene->cleanUp(this, full);
	if (full)
	{
		delete skyBoxScene;
	}

	scene->cleanUp(this, full);
	if (full)
	{
		delete scene;
	}

	if (full)
	{
		denoiser.cleanup(this);
		applyRefPipe.cleanUpPipeline(this, true);

		device.destroySemaphore(imageAvailableSemaphore);
		device.destroySemaphore(renderingFinishedSemaphore);

		device.destroyQueryPool(graphicsQueryPool);

		device.destroyCommandPool(graphicsCommandPool);
		if (gQ != cQ)
		{
			device.destroyCommandPool(computeCommandPool);
		}
		if (gQ != tQ)
		{
			device.destroyCommandPool(transferCommandPool);
		}

		device.destroyDescriptorPool(imguiPool);

		if (swapChain != vk::SwapchainKHR(VK_NULL_HANDLE))
		{
			device.destroySwapchainKHR(swapChain);
		}

		instance.destroySurfaceKHR(surface);

		device.destroy();

#ifdef VALIDATION_LAYERS
		instance.destroyDebugUtilsMessengerEXT(dbgUtilsMgr);
#endif
		instance.destroy();
	}
}

void AppResources::recompileShaders()
{
	std::string shaderInputFolder = "./shaders";
	std::string shaderOutputFolder = "./build/shaders/";
	for (const auto& entry : std::filesystem::directory_iterator(shaderInputFolder))
	{
		std::string shaderPath = entry.path().string();
		std::string shaderName = "";
		for (int i = shaderPath.size() - 1; i >= 0; i--)
		{
			if (shaderPath.c_str()[i] == '/' || shaderPath.c_str()[i] == '\\')
				break;
			else
				shaderName = shaderPath.c_str()[i] + shaderName;
		}
		if (shaderName.find("_ext") != std::string::npos)
		{
			continue;
		}
		if (shaderName.find(".") == std::string::npos)
		{
			continue;
		}

		std::string vulkanPath = std::getenv("VULKAN_SDK");
		std::string command = vulkanPath + "/Bin/glslc.exe --target-env=vulkan1.2 " + shaderPath + " -o " + shaderOutputFolder + shaderName + ".spv";
		//std::cout << command << "\n";
		system(command.c_str());
	}
}

void AppResources::takeScreenShot(uint32_t numFrames, int32_t fileType)
{
	screenShotFileType = fileType;
	if (useRefBeforeScreenshot)
	{
		if (numFrames > 0)
		{
			screenshotProcessing = true;
			screenShotFrameCount = frameCount + numFrames;
		}
		else
		{
			useRefNowScreenshot = true;
		}
		return;
	}

	if (numFrames == 0)
	{
		saveScreenShot(this, fileType);
		windowObj->hide_imgui_screenshot = false;
	}
	else
	{
		screenshotProcessing = true;
		screenShotFrameCount = frameCount + numFrames;
	}
}

void AppResources::setFullscreen(bool fullscreen)
{
	windowObj->setFullscreen(fullscreen);
}

uint32_t AppResources::getHashMapSize()
{
	return scene->getHashMapSize();
}

uint32_t AppResources::getHashMapOccupation()
{
	return scene->getHashMapOccupation(this);
}

float AppResources::getAveragePathDepth()
{
	return (float)scene->getAveragePathDepth(this) / (width * height);
}

void AppResources::initApp(Window* _windowObj, GLFWwindow* window, uint32_t _width, uint32_t _height, bool vsync)
{
	recompileShaders();

	width = _width;
	height = _height;
	windowObj = _windowObj;

	windowObj->gui_control.setMSAA(this);

	oldSwapChain = VK_NULL_HANDLE;

	createInstance(dbgUtilsMgr, "Master_Thesis_Project", "vulkan-master-thesis-engine");

	createSurface(window);

	selectPhysicalDevice();

	checkSwapChainSupport();

	auto chain = pDevice.getProperties2KHR<vk::PhysicalDeviceProperties2KHR, vk::PhysicalDeviceSubgroupProperties>();
	pDeviceProperties = chain.get<vk::PhysicalDeviceProperties2>();
	pDeviceSubgroupProperties = chain.get<vk::PhysicalDeviceSubgroupProperties>();

	std::tie(cQ, tQ, gQ, pQ) = getComputeTransferGraphicsPresentQueues();
	createLogicalDevice();

	device.getQueue(gQ, 0U, &graphicsQueue);
	createCommandPool(graphicsCommandPool, gQ);

	if (gQ != pQ)
	{
		device.getQueue(pQ, 0U, &presentQueue);
	}
	else
	{
		presentQueue = graphicsQueue;
	}

	if (gQ != cQ)
	{
		device.getQueue(cQ, 0U, &computeQueue);
		createCommandPool(computeCommandPool, cQ);
	}
	else
	{
		computeQueue = graphicsQueue;
		computeCommandPool = graphicsCommandPool;
	}

	if (gQ != tQ)
	{
		device.getQueue(tQ, 0U, &transferQueue);
		createCommandPool(transferCommandPool, tQ);
	}
	else
	{
		transferQueue = graphicsQueue;
		transferCommandPool = graphicsCommandPool;
	}
	imageAvailableSemaphore = device.createSemaphore(vk::SemaphoreCreateInfo(), nullptr);
	renderingFinishedSemaphore = device.createSemaphore(vk::SemaphoreCreateInfo(), nullptr);

	createTimestampQueryPool(graphicsQueryPool, windowObj->gui_control.graphicsTimings.container.size() + 1);

	auto makeBuffer = [this](vk::BufferUsageFlags usage_flags, vk::MemoryPropertyFlags memory_flags,
							 vk::DeviceSize size,
							 std::string name) -> Buffer
	{
		Buffer b;
		createBuffer(pDevice, device, size, usage_flags, memory_flags, name, b.buf, b.mem);
		return b;
	};

	createSwapchain(oldSwapChain, width, height, vsync);

	createPreRenderPass();
	createDeferredRenderPass();

	createImageViews();
	createAttachments();

	skyBoxScene = new SkyBoxScene(this);
	windowObj->gui_control.setScene(this, &skyBoxScene->renderedCubemap);

	createFramebuffersPrePass();
	createFramebuffersDeferred();

	OptixDenoiserOptions options;
	options.guideAlbedo = true;
	options.guideNormal = true;
	denoiser.init(this, options, OPTIX_PIXEL_FORMAT_FLOAT4, true);
	denoiser.allocateBuffers(swapChainExtent, false);
	denoiser.createSemaphore();
	applyRefPipe.generatePipeline(this, true);

	initImGui();
	createCommandBuffers();
}

void AppResources::onWindowResized(GLFWwindow* window, int _width, int _height)
{
	if (_width != 0 && _height != 0)
	{
		windowResized = true;
		width = _width;
		height = _height;
	}
}

void AppResources::onWindowSizeChanged(bool msaaChanged, bool vsync)
{
	windowObj->getSize(&width, &height);
	if (width != 0 && height != 0)
	{
		device.waitIdle();
		destroy(false);
		windowObj->gui_control.camera.updateScreenSize(width, height);
		createSwapchain(oldSwapChain, width, height, vsync);
		createPreRenderPass();
		createDeferredRenderPass();
		createImageViews();
		createAttachments();
		createFramebuffersPrePass();
		createFramebuffersDeferred();

		scene->createPipelines(this);
		skyBoxScene->createPipelines(this);

		denoiser.allocateBuffers(swapChainExtent, true);

		if (msaaChanged)
		{
			device.destroyDescriptorPool(imguiPool);
			initImGui();
		}
		windowObj->gui_control.initTextures(this);

		createCommandBuffers();
		windowResized = false;
	}
}

/*
	This is the function in which errors will go through to be displayed.
*/
VKAPI_ATTR VkBool32 VKAPI_CALL
debugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
							VkDebugUtilsMessageTypeFlagsEXT messageTypes,
							VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
							void* /*pUserData*/)
{
	if (enableValidationLayers)
	{
		if (pCallbackData->messageIdNumber == 648835635)
		{
			// UNASSIGNED-khronos-Validation-debug-build-warning-message
			return VK_FALSE;
		}
		if (pCallbackData->messageIdNumber == 767975156)
		{
			// UNASSIGNED-BestPractices-vkCreateInstance-specialuse-extension
			return VK_FALSE;
		}
	}

	std::cerr << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity)) << ": "
		<< vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageTypes)) << ":\n";
	std::cerr << "\t"
		<< "messageIDName   = <" << pCallbackData->pMessageIdName << ">\n";
	std::cerr << "\t"
		<< "messageIdNumber = " << pCallbackData->messageIdNumber << "\n";
	std::cerr << "\t"
		<< "message         = <" << pCallbackData->pMessage << ">\n";
	if (0 < pCallbackData->queueLabelCount)
	{
		std::cerr << "\t"
			<< "Queue Labels:\n";
		for (uint8_t i = 0; i < pCallbackData->queueLabelCount; i++)
		{
			std::cerr << "\t\t"
				<< "labelName = <" << pCallbackData->pQueueLabels[i].pLabelName << ">\n";
		}
	}
	if (0 < pCallbackData->cmdBufLabelCount)
	{
		std::cerr << "\t"
			<< "CommandBuffer Labels:\n";
		for (uint8_t i = 0; i < pCallbackData->cmdBufLabelCount; i++)
		{
			std::cerr << "\t\t"
				<< "labelName = <" << pCallbackData->pCmdBufLabels[i].pLabelName << ">\n";
		}
	}
	if (0 < pCallbackData->objectCount)
	{
		std::cerr << "\t"
			<< "Objects:\n";
		for (uint8_t i = 0; i < pCallbackData->objectCount; i++)
		{
			std::cerr << "\t\t"
				<< "Object " << i << "\n";
			std::cerr << "\t\t\t"
				<< "objectType   = "
				<< vk::to_string(static_cast<vk::ObjectType>(pCallbackData->pObjects[i].objectType)) << "\n";
			std::cerr << "\t\t\t"
				<< "objectHandle = " << pCallbackData->pObjects[i].objectHandle << "\n";
			if (pCallbackData->pObjects[i].pObjectName)
			{
				std::cerr << "\t\t\t"
					<< "objectName   = <" << pCallbackData->pObjects[i].pObjectName << ">\n";
			}
		}
	}
	return VK_TRUE;
}

/*
	This function fills the structure with flags indicating
	which error messages should go through
*/
vk::DebugUtilsMessengerCreateInfoEXT makeDebugUtilsMessengerCreateInfoEXT()
{

	using SEVERITY = vk::DebugUtilsMessageSeverityFlagBitsEXT; // for readability
	using MESSAGE = vk::DebugUtilsMessageTypeFlagBitsEXT;
	return { {},
			SEVERITY::eWarning | SEVERITY::eError,
			MESSAGE::eGeneral | MESSAGE::ePerformance | MESSAGE::eValidation,
			&debugUtilsMessengerCallback };
}

/*
	The dynamic loader allows us to access many extensions
	Required before creating instance for loading the extension VK_EXT_DEBUG_UTILS_EXTENSION_NAME
*/
void initDynamicLoader()
{
	static vk::DynamicLoader dl;
	static PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>(
		"vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
}

void AppResources::createInstance(vk::DebugUtilsMessengerEXT& debugUtilsMessenger,
								  std::string appName, std::string engineName)
{
	initDynamicLoader();
	vk::ApplicationInfo applicationInfo(appName.c_str(), 1, engineName.c_str(), 1, VK_API_VERSION_1_2);

	unsigned int glfwExtensionCount;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for (size_t i = 0; i < glfwExtensionCount; i++)
	{
		instanceExtensions.push_back(glfwExtensions[i]);
	}
	instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	instanceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
	instanceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
	instanceExtensions.push_back(VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME);

	// ### initialize the InstanceCreateInfo ###
	vk::InstanceCreateInfo instanceCreateInfo( //flags, pAppInfo, layerCount, layerNames, extcount, extNames
											  {}, &applicationInfo,
											  static_cast<uint32_t>(validationLayers.size()), validationLayers.data(),
											  static_cast<uint32_t>(instanceExtensions.size()), instanceExtensions.data());

	// ### DebugInfo: use of StructureChain instead of pNext ###
	// DebugUtils is used to catch errors from the instance
	vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = makeDebugUtilsMessengerCreateInfoEXT();
	// the StructureChain fills the pNext member of the struct in a typesafe way
	// this is only possible with vulkan-hpp, in plain vulkan there is no typechecking
	vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT> chain =
	{ instanceCreateInfo, debugCreateInfo };

	if (!enableValidationLayers)
	{ // for Release mode
		chain.unlink<vk::DebugUtilsMessengerCreateInfoEXT>();
	}

	// create an Instance
	instance = vk::createInstance(chain.get<vk::InstanceCreateInfo>());

	// update the dispatcher to use instance related extensions 
	VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

	if (enableValidationLayers)
	{
		debugUtilsMessenger = instance.createDebugUtilsMessengerEXT(makeDebugUtilsMessengerCreateInfoEXT());
	}
}

void AppResources::createSurface(GLFWwindow* window)
{
	assert(window);
	assert(instance);

	VkSurfaceKHR _surface;
	if (glfwCreateWindowSurface(instance, window, NULL, &_surface) != VK_SUCCESS)
	{
		std::cerr << "failed to create window surface!" << std::endl;
		exit(1);
	}
	surface = _surface;
}

std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>
AppResources::getComputeTransferGraphicsPresentQueues()
{
	uint32_t tq = -1;
	std::optional<uint32_t> otq;
	uint32_t cq = -1;
	std::optional<uint32_t> ocq;
	uint32_t gq = -1;
	std::optional<uint32_t> ogq;
	uint32_t pq = -1;
	std::optional<uint32_t> opq;

	using Chain = vk::StructureChain<vk::QueueFamilyProperties2, vk::QueueFamilyCheckpointPropertiesNV>;
	using QFB = vk::QueueFlagBits;
	auto queueFamilyProperties2 = pDevice.getQueueFamilyProperties2<Chain, std::allocator<Chain>, vk::DispatchLoaderDynamic>();

	for (uint32_t j = 0; j < queueFamilyProperties2.size(); j++)
	{
		vk::QueueFamilyProperties const& properties =
			queueFamilyProperties2[static_cast<size_t>(j)].get<vk::QueueFamilyProperties2>().queueFamilyProperties;

		vk::Bool32 presentSupport = pDevice.getSurfaceSupportKHR(j, surface);

		if (properties.queueFlags & QFB::eCompute)
		{
			if (!(properties.queueFlags & QFB::eGraphics ||
				properties.queueFlags & QFB::eProtected))
			{
				ocq = j;
			} // when a queue supports only compute and not graphics we want to use that
			cq = j;
		}

		if (properties.queueFlags & QFB::eTransfer)
		{
			if (!(properties.queueFlags & QFB::eCompute ||
				properties.queueFlags & QFB::eGraphics ||
				properties.queueFlags & QFB::eProtected))
			{
				otq = j;
			} // when a queue supports only transfer, we want to use this one
			tq = j;
		}

		if (properties.queueFlags & QFB::eGraphics)
		{
			if (!(properties.queueFlags & QFB::eCompute ||
				properties.queueFlags & QFB::eProtected))
			{
				ogq = j;
			} // when a queue supports only graphics and not compute we want to use that
			gq = j;
			if (presentSupport)
			{
				opq = j;
			}
		}

		if (presentSupport)
		{
			pq = j;
		}
	}

	if (otq.has_value())
	{
		tq = otq.value();
	}
	if (ocq.has_value())
	{
		cq = ocq.value();
	}
	if (ogq.has_value())
	{
		gq = ogq.value();
	}
	if (opq.has_value())
	{
		pq = opq.value();
	}

	return std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>(cq, tq, gq, pq);
}

void AppResources::selectPhysicalDevice()
{
	// takes the first one
	std::vector<vk::PhysicalDevice> physDs = instance.enumeratePhysicalDevices();

	const static char* cache_name = "device_selection_cache";
	const static char* recreation_message = "To select a new device, delete the file \"device_selection_cache\" in your working directory before executing the framework.";

	std::ifstream ifile(cache_name, std::ios::binary);
	if (ifile.is_open())
	{
		DeviceSelectionCache cache;
		ifile.read(reinterpret_cast<char*>(&cache), sizeof(cache));
		ifile.close();
		for (auto physD : physDs)
		{
			auto props = physD.getProperties2().properties;
			if (props.vendorID == cache.vendorID && props.deviceID == cache.deviceID)
			{
				std::cout << "Selecting previously selected device: \"" << props.deviceName << "\"" << std::endl;
				std::cout << recreation_message << std::endl;
				pDevice = physD;
				return;
			}
		}
		std::cout << "Previously selected device was not found." << std::endl;
	}
	else
	{
		std::cout << "No previous device selection found." << std::endl;
	}

	std::cout << "Select one of the available devices:" << std::endl;

	for (int i = 0; i < physDs.size(); i++)
	{
		auto props = physDs[i].getProperties2().properties;
		std::cout << i << ")\t" << props.deviceName.data() << std::endl;
	}

	uint32_t i;
	while (true)
	{
		std::cout << "Enter device number: ";
		std::cin >> i;
		if (i < physDs.size()) break;
	}

	auto props = physDs[i].getProperties2().properties;
	DeviceSelectionCache cache;
	cache.vendorID = props.vendorID;
	cache.deviceID = props.deviceID;

	std::ofstream ofile(cache_name, std::ios::out | std::ios::binary);
	ofile.write(reinterpret_cast<const char*>(&cache), sizeof(cache));
	ofile.close();
	std::cout << "Selected device: \"" << props.deviceName.data() << "\"" << std::endl
		<< "This device will be automatically selected in the future." << std::endl
		<< recreation_message << std::endl;

	pDevice = physDs[i];
}

/*
	The logical device holds the queues and will be used in almost every call from now on
*/
void AppResources::createLogicalDevice()
{

	//first get the queues
	uint32_t cQ, tQ, gQ, pQ;
	std::tie(cQ, tQ, gQ, pQ) = getComputeTransferGraphicsPresentQueues();
	std::vector<vk::DeviceQueueCreateInfo> queuesInfo;
	// flags, queueFamily, queueCount, queuePriority
	float prio = 1.f;

	vk::DeviceQueueCreateInfo graphicsInfo({}, gQ, 1U, &prio);
	queuesInfo.push_back(graphicsInfo);

	if (gQ != cQ)
	{
		vk::DeviceQueueCreateInfo computeInfo({}, cQ, 1U, &prio);
		queuesInfo.push_back(computeInfo);
	}

	if (gQ != tQ)
	{
		vk::DeviceQueueCreateInfo transferInfo({}, tQ, 1U, &prio);
		queuesInfo.push_back(transferInfo);
	}

	if (gQ != pQ)
	{
		vk::DeviceQueueCreateInfo presentInfo({}, pQ, 1U, &prio);
		queuesInfo.push_back(presentInfo);
	}
	// {}, queueCreateInfoCount, pQueueCreateInfos, enabledLayerCount, ppEnabledLayerNames, enabledExtensionCount, ppEnabledExtensionNames, pEnabledFeatures

	std::vector extensionNames_(extensionNames);

	auto deviceExtensionProperties = pDevice.enumerateDeviceExtensionProperties();
	bool enable_portability_subset = false;
	bool enable_swapchain_extension = false;
	for (auto ext : deviceExtensionProperties)
	{
		if (strcmp(ext.extensionName.data(), VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME) == 0)
		{
			enable_portability_subset = true;
		}
		if (strcmp(ext.extensionName.data(), VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
		{
			enable_swapchain_extension = true;
		}
		if (strcmp(ext.extensionName.data(), VK_KHR_RAY_QUERY_EXTENSION_NAME) == 0)
		{
			enable_rtx_extension = true;
		}
		if (strcmp(ext.extensionName.data(), VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) == 0)
		{
			enable_rtx_extension = true;
		}
	}

	if (!enable_rtx_extension)
		throw std::exception("unable to find ray query extension");

	if (enable_portability_subset)
	{
		extensionNames_.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
	}

	if (enable_swapchain_extension)
	{
		extensionNames_.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	}
	// Ray tracing related extensions required by this sample
	if (enable_rtx_extension)
	{
		extensionNames_.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
		extensionNames_.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
		extensionNames_.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
		extensionNames_.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
		extensionNames_.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
		extensionNames_.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
	}
	extensionNames_.push_back(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
	extensionNames_.push_back(VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME);
	extensionNames_.push_back(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME);
	extensionNames_.push_back(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
	extensionNames_.push_back(VK_KHR_EXTERNAL_FENCE_WIN32_EXTENSION_NAME);
	extensionNames_.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
	extensionNames_.push_back(VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME);

	vk::DeviceCreateInfo dci({}, queuesInfo.size(), queuesInfo.data(),
							 CAST(validationLayers), validationLayers.data(),
							 CAST(extensionNames_), extensionNames_.data()); // no extension
	auto supportedFeatures =
		pDevice.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceDescriptorIndexingFeaturesEXT>();

	vk::PhysicalDeviceTimelineSemaphoreFeatures physicalDeviceTimelineSemaphoreFeatures = {};
	physicalDeviceTimelineSemaphoreFeatures.timelineSemaphore = VK_TRUE;
	physicalDeviceTimelineSemaphoreFeatures.pNext = &supportedFeatures.get<vk::PhysicalDeviceDescriptorIndexingFeaturesEXT>();

	vk::PhysicalDeviceSynchronization2FeaturesKHR physicalDeviceSynchronization2Features = {};
	physicalDeviceSynchronization2Features.synchronization2 = VK_TRUE;
	physicalDeviceSynchronization2Features.pNext = &physicalDeviceTimelineSemaphoreFeatures;

	vk::PhysicalDeviceShaderAtomicFloatFeaturesEXT physicalDeviceShaderAtomicFloatFeaturesEXT = {};
	physicalDeviceShaderAtomicFloatFeaturesEXT.shaderBufferFloat32AtomicAdd = VK_TRUE;
	physicalDeviceShaderAtomicFloatFeaturesEXT.shaderSharedFloat32AtomicAdd = VK_TRUE;
	physicalDeviceShaderAtomicFloatFeaturesEXT.pNext = &physicalDeviceSynchronization2Features;

	vk::PhysicalDeviceBufferDeviceAddressFeatures deviceBufferDeviceAddressFeatures = {};
	deviceBufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
	deviceBufferDeviceAddressFeatures.pNext = &physicalDeviceShaderAtomicFloatFeaturesEXT;

	vk::PhysicalDeviceRayQueryFeaturesKHR deviceRayQueryFeatures = {};
	deviceRayQueryFeatures.rayQuery = VK_TRUE;
	deviceRayQueryFeatures.pNext = &deviceBufferDeviceAddressFeatures;

	vk::PhysicalDeviceAccelerationStructureFeaturesKHR deviceAccelerationStructureFeatures = {};
	deviceAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
	deviceAccelerationStructureFeatures.pNext = &deviceRayQueryFeatures;

	dci.pNext = &deviceAccelerationStructureFeatures;
	vk::PhysicalDeviceFeatures pdf = {};
	pdf.fillModeNonSolid = VK_TRUE;
	pdf.independentBlend = VK_TRUE;
	pdf.sampleRateShading = VK_TRUE;
	pdf.fragmentStoresAndAtomics = VK_TRUE;
	pdf.shaderStorageImageReadWithoutFormat = VK_TRUE;
	pdf.shaderStorageImageWriteWithoutFormat = VK_TRUE;
	pdf.wideLines = VK_TRUE;
	pdf.geometryShader = VK_TRUE;

	dci.setPEnabledFeatures(&pdf);
	device = pDevice.createDevice(dci);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(device);
	setObjectName(device, device, "This is my lovely device !");
}

void AppResources::createCommandPool(vk::CommandPool& commandPool, uint32_t queueIndex)
{
	vk::CommandPoolCreateInfo cpi(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueIndex);
	commandPool = device.createCommandPool(cpi);
}

void AppResources::destroyInstance(vk::DebugUtilsMessengerEXT& debugUtilsMessenger)
{
#ifdef VALIDATION_LAYERS
	instance.destroyDebugUtilsMessengerEXT(debugUtilsMessenger);
#endif
	instance.destroy();
}

void AppResources::destroyLogicalDevice(vk::Device& device)
{
	device.destroy();
}

void AppResources::destroyCommandPool(vk::CommandPool& commandPool)
{
	device.destroyCommandPool(commandPool);
	commandPool = vk::CommandPool();
}

void AppResources::showAvailableQueues(bool diagExt)
{

	using Chain = vk::StructureChain<vk::QueueFamilyProperties2, vk::QueueFamilyCheckpointPropertiesNV>;
	auto queueFamilyProperties2 = pDevice.getQueueFamilyProperties2<Chain, std::allocator<Chain>, vk::DispatchLoaderDynamic>();

	for (size_t j = 0; j < queueFamilyProperties2.size(); j++)
	{
		std::cout << "\t"
			<< "QueueFamily " << j << "\n";
		vk::QueueFamilyProperties const& properties =
			queueFamilyProperties2[j].get<vk::QueueFamilyProperties2>().queueFamilyProperties;
		std::cout << "\t\t"
			<< "QueueFamilyProperties:\n";
		std::cout << "\t\t\t"
			<< "queueFlags                  = " << vk::to_string(properties.queueFlags) << "\n";
		std::cout << "\t\t\t"
			<< "queueCount                  = " << properties.queueCount << "\n";
		std::cout << "\t\t\t"
			<< "timestampValidBits          = " << properties.timestampValidBits << "\n";
		std::cout << "\t\t\t"
			<< "minImageTransferGranularity = " << properties.minImageTransferGranularity.width << " x "
			<< properties.minImageTransferGranularity.height << " x "
			<< properties.minImageTransferGranularity.depth << "\n";
		std::cout << "\n";

		if (diagExt)
		{
			vk::QueueFamilyCheckpointPropertiesNV const& checkpointProperties =
				queueFamilyProperties2[j].get<vk::QueueFamilyCheckpointPropertiesNV>();
			std::cout << "\t\t"
				<< "CheckPointPropertiesNV:\n";
			std::cout << "\t\t\t"
				<< "checkpointExecutionStageMask  = "
				<< vk::to_string(checkpointProperties.checkpointExecutionStageMask) << "\n";
			std::cout << "\n";
		}
	}
}

void AppResources::createTimestampQueryPool(vk::QueryPool& queryPool, uint32_t queryCount)
{
	vk::QueryPoolCreateInfo createInfo({}, vk::QueryType::eTimestamp, queryCount);
	queryPool = device.createQueryPool(createInfo);
}

void AppResources::destroyQueryPool(vk::QueryPool& queryPool)
{
	device.destroyQueryPool(queryPool);
	queryPool = vk::QueryPool();
}

void AppResources::createSwapchain(vk::SwapchainKHR& oldSwapChain, uint32_t width, uint32_t height, bool vsync)
{
	vk::SurfaceCapabilitiesKHR surfaceCapabilities = pDevice.getSurfaceCapabilitiesKHR(surface);
	auto surfaceFormats = pDevice.getSurfaceFormatsKHR(surface);
	auto presentModes = pDevice.getSurfacePresentModesKHR(surface);

	uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
	if (surfaceCapabilities.maxImageCount != 0 && imageCount > surfaceCapabilities.maxImageCount)
	{
		imageCount = surfaceCapabilities.maxImageCount;
	}

	vk::SurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(surfaceFormats);

	swapChainExtent = chooseSwapExtent(surfaceCapabilities, width, height);

	vk::SurfaceTransformFlagBitsKHR surfaceTransform;
	if (surfaceCapabilities.supportedTransforms &
		vk::SurfaceTransformFlagBitsKHR::eIdentity)
	{
		surfaceTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
	}
	else
	{
		surfaceTransform = surfaceCapabilities.currentTransform;
	}

	vk::PresentModeKHR presentMode = choosePresentMode(presentModes, vsync);

	vk::SwapchainCreateInfoKHR sciKHR = vk::SwapchainCreateInfoKHR();
	sciKHR.surface = surface;
	sciKHR.minImageCount = imageCount;
	sciKHR.imageFormat = surfaceFormat.format;
	sciKHR.imageColorSpace = surfaceFormat.colorSpace;
	sciKHR.imageExtent = swapChainExtent;
	sciKHR.imageArrayLayers = 1;
	sciKHR.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
	sciKHR.imageSharingMode = vk::SharingMode::eExclusive;
	sciKHR.queueFamilyIndexCount = 0;
	sciKHR.pQueueFamilyIndices = nullptr;
	sciKHR.preTransform = surfaceTransform;
	sciKHR.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	sciKHR.presentMode = presentMode;
	sciKHR.clipped = VK_TRUE;
	sciKHR.oldSwapchain = oldSwapChain;

	swapChain = device.createSwapchainKHR(sciKHR);
	if (oldSwapChain != vk::SwapchainKHR(VK_NULL_HANDLE))
	{
		device.destroySwapchainKHR(oldSwapChain);
	}
	oldSwapChain = swapChain;
	swapChainFormat = surfaceFormat.format;

	std::vector<vk::Image> swcImages = device.getSwapchainImagesKHR(swapChain);
	swapChainImages.resize(swcImages.size());
	colorImagesHDR.resize(swcImages.size());
	for (size_t i = 0; i < swapChainImages.size(); ++i)
	{
		swapChainImages[i].image = swcImages[i];
	}
}

vk::SurfaceFormatKHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
	// We can either choose any format
	if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined)
	{
		return { vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
	}

	// Or go with the standard format - if available
	for (const auto& availableSurfaceFormat : availableFormats)
	{
		if (availableSurfaceFormat.format == vk::Format::eR8G8B8A8Unorm)
		{
			return availableSurfaceFormat;
		}
	}

	// Or fall back to the first available one
	return availableFormats[0];
}

vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& surfaceCapabilities, uint32_t width, uint32_t height)
{
	if (surfaceCapabilities.currentExtent.width == -1)
	{
		vk::Extent2D swapChainExtent = {};

		swapChainExtent.width = std::min(std::max(width, surfaceCapabilities.minImageExtent.width),
										 surfaceCapabilities.maxImageExtent.width);
		swapChainExtent.height = std::min(std::max(height, surfaceCapabilities.minImageExtent.height),
										  surfaceCapabilities.maxImageExtent.height);

		return swapChainExtent;
	}
	else
	{
		return surfaceCapabilities.currentExtent;
	}
}

vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR> presentModes, bool vsync)
{
	for (const auto& presentMode : presentModes)
	{
		if (vsync)
		{
			return vk::PresentModeKHR::eFifo;
		}
		else
		{
			if (presentMode == vk::PresentModeKHR::eMailbox)
			{
				return presentMode;
			}
		}
	}

	// If mailbox is unavailable, fall back to FIFO (guaranteed to be available)
	return vk::PresentModeKHR::eFifo;
}

void AppResources::printDeviceCapabilities()
{
	//vk::PhysicalDeviceFeatures features = physicalDevice.getFeatures();
	std::vector<vk::ExtensionProperties> ext = pDevice.enumerateDeviceExtensionProperties();
	std::vector<vk::LayerProperties> layers = pDevice.enumerateDeviceLayerProperties();
	vk::PhysicalDeviceMemoryProperties memoryProperties = pDevice.getMemoryProperties();
	vk::PhysicalDeviceProperties properties = pDevice.getProperties();
	vk::PhysicalDeviceType dt = properties.deviceType;

	std::cout << "====================" << std::endl
		<< "Device Name: " << properties.deviceName << std::endl
		<< "Device ID: " << properties.deviceID << std::endl
		<< "Device Type: " << vk::to_string(properties.deviceType) << std::endl
		<< "Driver Version: " << properties.driverVersion << std::endl
		<< "API Version: " << properties.apiVersion << std::endl
		<< "====================" << std::endl
		<< std::endl;

	bool budgetExt = false;
	bool diagExt = false;
	std::cout << "This device supports the following extensions (" << ext.size() << "): " << std::endl;
	for (vk::ExtensionProperties e : ext)
	{
		std::cout << std::string(e.extensionName.data()) << std::endl;
		if (std::string(e.extensionName.data()) == VK_EXT_MEMORY_BUDGET_EXTENSION_NAME)
		{
			budgetExt = true;
		}
		if (std::string(e.extensionName.data()) == VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME)
		{
			diagExt = true;
		}
	}

	std::cout << "This device supports the following memory types (" << memoryProperties.memoryTypeCount << "): "
		<< std::endl;
	uint32_t c = 0U;
	for (vk::MemoryType e : memoryProperties.memoryTypes)
	{
		if (c > memoryProperties.memoryTypeCount)
		{
			break;
		}

		std::cout << e.heapIndex << "\t ";
		std::cout << vk::to_string(e.propertyFlags) << std::endl;
		c++;
	}
	std::cout << "====================" << std::endl
		<< std::endl;

	if (budgetExt)
	{
		std::cout << "This device has the following heaps (" << memoryProperties.memoryHeapCount << "): " << std::endl;
		c = 0U;
		for (vk::MemoryHeap e : memoryProperties.memoryHeaps)
		{
			if (c > memoryProperties.memoryHeapCount)
			{
				break;
			}

			std::cout << "Size: " << formatSize(e.size) << "\t ";
			std::cout << vk::to_string(e.flags) << std::endl;
			c++;
		}
	}

	std::cout << "====================" << std::endl
		<< std::endl
		<< "This device has the following layers (" << layers.size() << "): " << std::endl;
	for (vk::LayerProperties l : layers)
	{
		std::cout << std::string(l.layerName.data()) << "\t : " << std::string(l.description.data()) << std::endl;
	}
	std::cout << "====================" << std::endl
		<< std::endl;

	showAvailableQueues(diagExt);
}

void AppResources::checkSwapChainSupport()
{
	auto extprop = pDevice.enumerateDeviceExtensionProperties();

	if (extprop.size() == 0)
	{
		std::cerr << "physical device doesn't support any extensions" << std::endl;
		exit(1);
	}

	for (const auto& extension : extprop)
	{
		if (strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
		{
			//std::cout << "physical device supports swap chains" << std::endl;
			return;
		}
	}

	std::cerr << "physical device doesn't support swap chains" << std::endl;
	exit(1);
}

void AppResources::createPreRenderPass()
{
	vk::AttachmentDescription2 colorAttachment = {};
	colorAttachment.format = vk::Format::eR32G32B32A32Sfloat;
	colorAttachment.samples = msaa;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
	colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentDescription2 positionAttachment = {};
	positionAttachment.format = vk::Format::eR32G32B32A32Sfloat;
	positionAttachment.samples = msaa;
	positionAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	positionAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	positionAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	positionAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	positionAttachment.initialLayout = vk::ImageLayout::eUndefined;
	positionAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentDescription2 normalAttachment = {};
	normalAttachment.format = vk::Format::eR32G32B32A32Sfloat;
	normalAttachment.samples = msaa;
	normalAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	normalAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	normalAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	normalAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	normalAttachment.initialLayout = vk::ImageLayout::eUndefined;
	normalAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentDescription2 tangentAttachment = {};
	tangentAttachment.format = vk::Format::eR32G32B32A32Sfloat;
	tangentAttachment.samples = msaa;
	tangentAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	tangentAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	tangentAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	tangentAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	tangentAttachment.initialLayout = vk::ImageLayout::eUndefined;
	tangentAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentDescription2 bitangentAttachment = {};
	bitangentAttachment.format = vk::Format::eR32G32B32A32Sfloat;
	bitangentAttachment.samples = msaa;
	bitangentAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	bitangentAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	bitangentAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	bitangentAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	bitangentAttachment.initialLayout = vk::ImageLayout::eUndefined;
	bitangentAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentDescription2 depthAttachment = {};
	depthAttachment.format = findDepthFormat(pDevice);
	depthAttachment.samples = msaa;
	depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	depthAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
	depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::AttachmentDescription2 colorAttachmentResolve{};
	colorAttachmentResolve.format = vk::Format::eR32G32B32A32Sfloat;
	colorAttachmentResolve.samples = vk::SampleCountFlagBits::e1;
	if (msaa != vk::SampleCountFlagBits::e1)
		colorAttachmentResolve.loadOp = vk::AttachmentLoadOp::eDontCare;
	else
		colorAttachmentResolve.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachmentResolve.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachmentResolve.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachmentResolve.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttachmentResolve.initialLayout = vk::ImageLayout::eUndefined;
	colorAttachmentResolve.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentDescription2 depthAttachmentResolve{};
	depthAttachmentResolve.format = findDepthFormat(pDevice);
	depthAttachmentResolve.samples = vk::SampleCountFlagBits::e1;
	if (msaa != vk::SampleCountFlagBits::e1)
		depthAttachmentResolve.loadOp = vk::AttachmentLoadOp::eDontCare;
	else
		depthAttachmentResolve.loadOp = vk::AttachmentLoadOp::eClear;
	depthAttachmentResolve.storeOp = vk::AttachmentStoreOp::eStore;
	depthAttachmentResolve.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	depthAttachmentResolve.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	depthAttachmentResolve.initialLayout = vk::ImageLayout::eUndefined;
	depthAttachmentResolve.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::AttachmentReference2 colorAttachmentReference = {};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentReference2 positionAttachmentReference = {};
	positionAttachmentReference.attachment = 1;
	positionAttachmentReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentReference2 normalAttachmentReference = {};
	normalAttachmentReference.attachment = 2;
	normalAttachmentReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentReference2 tangentAttachmentReference = {};
	tangentAttachmentReference.attachment = 3;
	tangentAttachmentReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentReference2 bitangentAttachmentReference = {};
	bitangentAttachmentReference.attachment = 4;
	bitangentAttachmentReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentReference2 depthAttachmentReference{};
	depthAttachmentReference.attachment = 5;
	depthAttachmentReference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	vk::AttachmentReference2 colorAttachmentResolveReference{};
	colorAttachmentResolveReference.attachment = 6;
	colorAttachmentResolveReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentReference2 positionAttachmentResolveReference{};
	positionAttachmentResolveReference.attachment = 7;
	positionAttachmentResolveReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentReference2 normalAttachmentResolveReference{};
	normalAttachmentResolveReference.attachment = 8;
	normalAttachmentResolveReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentReference2 tangentAttachmentResolveReference{};
	tangentAttachmentResolveReference.attachment = 9;
	tangentAttachmentResolveReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentReference2 bitangentAttachmentResolveReference{};
	bitangentAttachmentResolveReference.attachment = 10;
	bitangentAttachmentResolveReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentReference2 depthAttachmentResolveReference{};
	depthAttachmentResolveReference.attachment = 11;
	depthAttachmentResolveReference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

	std::vector<vk::AttachmentReference2> colorAttachments = {
		colorAttachmentReference,
		positionAttachmentReference,
		normalAttachmentReference,
		tangentAttachmentReference,
		bitangentAttachmentReference
	};
	std::vector<vk::AttachmentReference2> resolveAttachments = {
		colorAttachmentResolveReference,
		positionAttachmentResolveReference,
		normalAttachmentResolveReference,
		tangentAttachmentResolveReference,
		bitangentAttachmentResolveReference
	};
	vk::SubpassDescription2 subPassDescription = {};
	subPassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subPassDescription.colorAttachmentCount = colorAttachments.size();
	subPassDescription.pColorAttachments = colorAttachments.data();
	subPassDescription.pDepthStencilAttachment = &depthAttachmentReference;
	if (msaa != vk::SampleCountFlagBits::e1)
		subPassDescription.pResolveAttachments = resolveAttachments.data();
	else
		subPassDescription.pResolveAttachments = nullptr;

	vk::SubpassDescriptionDepthStencilResolve subPassDescriptionDSR;
	subPassDescriptionDSR.depthResolveMode = vk::ResolveModeFlagBits::eSampleZero;
	subPassDescriptionDSR.stencilResolveMode = vk::ResolveModeFlagBits::eSampleZero;
	subPassDescriptionDSR.pDepthStencilResolveAttachment = &depthAttachmentResolveReference;

	if (msaa != vk::SampleCountFlagBits::e1)
	{
		subPassDescription.pNext = &subPassDescriptionDSR;
	}

	vk::SubpassDependency2 dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask =
		vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
	dependency.dstStageMask =
		vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
	dependency.srcAccessMask = vk::AccessFlags();
	dependency.dstAccessMask =
		vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

	std::vector<vk::AttachmentDescription2> attachments;
	if (msaa != vk::SampleCountFlagBits::e1)
	{
		attachments = {
			colorAttachment,
			positionAttachment,
			normalAttachment,
			tangentAttachment,
			bitangentAttachment,
			depthAttachment,
			colorAttachmentResolve,
			colorAttachmentResolve,
			colorAttachmentResolve,
			colorAttachmentResolve,
			colorAttachmentResolve,
			depthAttachmentResolve
		};
	}
	else
	{
		attachments = {
			colorAttachmentResolve,
			colorAttachmentResolve,
			colorAttachmentResolve,
			colorAttachmentResolve,
			colorAttachmentResolve,
			depthAttachment
		};
	}

	vk::RenderPassCreateInfo2 createInfo = {};
	createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	createInfo.pAttachments = attachments.data();
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subPassDescription;
	createInfo.dependencyCount = 1;
	createInfo.pDependencies = &dependency;

	preRenderPass = device.createRenderPass2(createInfo);
}

void AppResources::createDeferredRenderPass()
{
	{
		vk::AttachmentDescription colorAttachment = {};
		colorAttachment.format = vk::Format::eR32G32B32A32Sfloat;
		colorAttachment.samples = vk::SampleCountFlagBits::e1;
		colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
		colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
		colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
		colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

		vk::AttachmentDescription colorAttachment2 = {};
		colorAttachment2.format = vk::Format::eR32G32B32A32Sfloat;
		colorAttachment2.samples = vk::SampleCountFlagBits::e1;
		colorAttachment2.loadOp = vk::AttachmentLoadOp::eClear;
		colorAttachment2.storeOp = vk::AttachmentStoreOp::eStore;
		colorAttachment2.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		colorAttachment2.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		colorAttachment2.initialLayout = vk::ImageLayout::eUndefined;
		colorAttachment2.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

		vk::AttachmentDescription colorAttachment3 = {};
		colorAttachment3.format = vk::Format::eR32G32B32A32Sfloat;
		colorAttachment3.samples = vk::SampleCountFlagBits::e1;
		colorAttachment3.loadOp = vk::AttachmentLoadOp::eClear;
		colorAttachment3.storeOp = vk::AttachmentStoreOp::eStore;
		colorAttachment3.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		colorAttachment3.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		colorAttachment3.initialLayout = vk::ImageLayout::eUndefined;
		colorAttachment3.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

		vk::AttachmentReference colorAttachmentReference = {};
		colorAttachmentReference.attachment = 0;
		colorAttachmentReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

		vk::AttachmentReference colorAttachmentReference2 = {};
		colorAttachmentReference2.attachment = 1;
		colorAttachmentReference2.layout = vk::ImageLayout::eColorAttachmentOptimal;

		vk::AttachmentReference colorAttachmentReference3 = {};
		colorAttachmentReference3.attachment = 2;
		colorAttachmentReference3.layout = vk::ImageLayout::eColorAttachmentOptimal;

		std::vector<vk::AttachmentReference> colorAttachments = {
			colorAttachmentReference,
			colorAttachmentReference2,
			colorAttachmentReference3
		};

		vk::SubpassDescription subPassDescription = {};
		subPassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
		subPassDescription.colorAttachmentCount = colorAttachments.size();
		subPassDescription.pColorAttachments = colorAttachments.data();

		vk::SubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask =
			vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
		dependency.dstStageMask =
			vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
		dependency.srcAccessMask = vk::AccessFlags();
		dependency.dstAccessMask =
			vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

		std::vector<vk::AttachmentDescription> attachments;
		attachments = { colorAttachment, colorAttachment2, colorAttachment3 };

		vk::RenderPassCreateInfo createInfo = {};
		createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		createInfo.pAttachments = attachments.data();
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subPassDescription;
		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &dependency;

		deferredRenderPass = device.createRenderPass(createInfo);
	}

	{
		vk::AttachmentDescription colorAttachment = {};
		colorAttachment.format = vk::Format::eR32G32B32A32Sfloat;
		colorAttachment.samples = vk::SampleCountFlagBits::e1;
		colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
		colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
		colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
		colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

		vk::AttachmentDescription colorAttachment2 = {};
		colorAttachment2.format = vk::Format::eR32G32B32A32Sfloat;
		colorAttachment2.samples = vk::SampleCountFlagBits::e1;
		colorAttachment2.loadOp = vk::AttachmentLoadOp::eClear;
		colorAttachment2.storeOp = vk::AttachmentStoreOp::eStore;
		colorAttachment2.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		colorAttachment2.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		colorAttachment2.initialLayout = vk::ImageLayout::eUndefined;
		colorAttachment2.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

		vk::AttachmentDescription colorAttachment3 = {};
		colorAttachment3.format = vk::Format::eR32G32B32A32Sfloat;
		colorAttachment3.samples = vk::SampleCountFlagBits::e1;
		colorAttachment3.loadOp = vk::AttachmentLoadOp::eLoad;
		colorAttachment3.storeOp = vk::AttachmentStoreOp::eStore;
		colorAttachment3.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		colorAttachment3.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		colorAttachment3.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
		colorAttachment3.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

		vk::AttachmentReference colorAttachmentReference = {};
		colorAttachmentReference.attachment = 0;
		colorAttachmentReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

		vk::AttachmentReference colorAttachmentReference2 = {};
		colorAttachmentReference2.attachment = 1;
		colorAttachmentReference2.layout = vk::ImageLayout::eColorAttachmentOptimal;

		vk::AttachmentReference colorAttachmentReference3 = {};
		colorAttachmentReference3.attachment = 2;
		colorAttachmentReference3.layout = vk::ImageLayout::eColorAttachmentOptimal;

		std::vector<vk::AttachmentReference> colorAttachments = {
			colorAttachmentReference,
			colorAttachmentReference2,
			colorAttachmentReference3
		};

		vk::SubpassDescription subPassDescription = {};
		subPassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
		subPassDescription.colorAttachmentCount = colorAttachments.size();
		subPassDescription.pColorAttachments = colorAttachments.data();

		vk::SubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask =
			vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
		dependency.dstStageMask =
			vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
		dependency.srcAccessMask = vk::AccessFlags();
		dependency.dstAccessMask =
			vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

		std::vector<vk::AttachmentDescription> attachments;
		attachments = { colorAttachment, colorAttachment2, colorAttachment3 };

		vk::RenderPassCreateInfo createInfo = {};
		createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		createInfo.pAttachments = attachments.data();
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subPassDescription;
		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &dependency;

		deferredRenderPassPSF = device.createRenderPass(createInfo);
	}

	{	// imgui
		vk::AttachmentDescription colorAttachment = {};
		colorAttachment.format = vk::Format::eR32G32B32A32Sfloat;
		colorAttachment.samples = vk::SampleCountFlagBits::e1;
		colorAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
		colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
		colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		colorAttachment.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
		colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

		vk::AttachmentReference colorAttachmentReference = {};
		colorAttachmentReference.attachment = 0;
		colorAttachmentReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

		std::vector<vk::AttachmentReference> colorAttachments = {
			colorAttachmentReference
		};

		vk::SubpassDescription subPassDescription = {};
		subPassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
		subPassDescription.colorAttachmentCount = colorAttachments.size();
		subPassDescription.pColorAttachments = colorAttachments.data();

		vk::SubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask =
			vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
		dependency.dstStageMask =
			vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
		dependency.srcAccessMask = vk::AccessFlags();
		dependency.dstAccessMask =
			vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

		std::vector<vk::AttachmentDescription> attachments;
		attachments = { colorAttachment };

		vk::RenderPassCreateInfo createInfo = {};
		createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		createInfo.pAttachments = attachments.data();
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subPassDescription;
		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &dependency;

		imGuiRenderPass = device.createRenderPass(createInfo);
	}
}

void AppResources::createImageViews()
{
	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		createImageView(this, swapChainImages[i].image, vk::ImageViewType::e2D, 1, 0,
						{ vk::ComponentSwizzle::eIdentity,
						 vk::ComponentSwizzle::eIdentity,
						 vk::ComponentSwizzle::eIdentity,
						 vk::ComponentSwizzle::eIdentity },
						swapChainFormat, vk::ImageAspectFlagBits::eColor, swapChainImages[i].imageView);
	}
}

vk::Format findSupportedFormat(vk::PhysicalDevice& pDevice,
							   const std::vector<vk::Format>& candidates,
							   vk::ImageTiling tiling,
							   vk::FormatFeatureFlags features)
{
	for (vk::Format format : candidates)
	{
		vk::FormatProperties props;
		props = pDevice.getFormatProperties(format);

		if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}
	std::cout << "failed to find supported format!\n";
	std::exit(-1);
}

bool checkFormatSupported(vk::PhysicalDevice& pDevice, vk::Format format, vk::FormatFeatureFlags requestedSupport)
{
	vk::FormatProperties prop;
	pDevice.getFormatProperties(format, &prop);
	return (prop.optimalTilingFeatures & requestedSupport) == requestedSupport;
}

vk::Format findDepthFormat(vk::PhysicalDevice& pDevice)
{
	return findSupportedFormat(pDevice,
							   { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
							   vk::ImageTiling::eOptimal,
							   vk::FormatFeatureFlagBits::eDepthStencilAttachment
	);
}

void AppResources::createAttachments()
{

	for (size_t i = 0; i < colorImagesHDR.size(); ++i)
	{
		createAttachment("color-hdr-image" + std::to_string(i),
						 colorImagesHDR[i],
						 vk::Format::eR32G32B32A32Sfloat,
						 vk::SampleCountFlagBits::e1,
						 vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc,
						 vk::ImageAspectFlagBits::eColor);
	}

	createAttachment("color-reference-image",
					 colorReference,
					 vk::Format::eR32G32B32A32Sfloat,
					 vk::SampleCountFlagBits::e1,
					 vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst,
					 vk::ImageAspectFlagBits::eColor);

	std::vector<float>data(width * height * 4, 0.0f);
	vk::Extent3D extent;
	extent.width = width;
	extent.height = height;
	extent.depth = 1;
	fillImageWithStagingBuffer(pDevice, device, graphicsCommandPool, graphicsQueue, colorReference.image, extent, 4U, 1U, data);

	createAttachment("color-image-msaa",
					 colorMsaa,
					 vk::Format::eR32G32B32A32Sfloat,
					 msaa,
					 vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
					 vk::ImageAspectFlagBits::eColor);

	createAttachment("color-image",
					 color,
					 vk::Format::eR32G32B32A32Sfloat,
					 vk::SampleCountFlagBits::e1,
					 vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage,
					 vk::ImageAspectFlagBits::eColor);

	createAttachment("position-image-msaa",
					 positionMsaa,
					 vk::Format::eR32G32B32A32Sfloat,
					 msaa,
					 vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
					 vk::ImageAspectFlagBits::eColor);

	createAttachment("position-image",
					 position,
					 vk::Format::eR32G32B32A32Sfloat,
					 vk::SampleCountFlagBits::e1,
					 vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
					 vk::ImageAspectFlagBits::eColor);

	createAttachment("normal-image-msaa",
					 normalMsaa,
					 vk::Format::eR32G32B32A32Sfloat,
					 msaa,
					 vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
					 vk::ImageAspectFlagBits::eColor);

	createAttachment("normal-image",
					 normal,
					 vk::Format::eR32G32B32A32Sfloat,
					 vk::SampleCountFlagBits::e1,
					 vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage,
					 vk::ImageAspectFlagBits::eColor);

	createAttachment("tangent-image-msaa",
					 tangentMsaa,
					 vk::Format::eR32G32B32A32Sfloat,
					 msaa,
					 vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
					 vk::ImageAspectFlagBits::eColor);

	createAttachment("tangent-image",
					 tangent,
					 vk::Format::eR32G32B32A32Sfloat,
					 vk::SampleCountFlagBits::e1,
					 vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
					 vk::ImageAspectFlagBits::eColor);

	createAttachment("bitangent-image-msaa",
					 bitangentMsaa,
					 vk::Format::eR32G32B32A32Sfloat,
					 msaa,
					 vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
					 vk::ImageAspectFlagBits::eColor);

	createAttachment("bitangent-image",
					 bitangent,
					 vk::Format::eR32G32B32A32Sfloat,
					 vk::SampleCountFlagBits::e1,
					 vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
					 vk::ImageAspectFlagBits::eColor);


	createAttachment("output-mask-image",
					 outputMask,
					 vk::Format::eR32G32B32A32Sfloat,
					 vk::SampleCountFlagBits::e1,
					 vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
					 vk::ImageAspectFlagBits::eColor);

	createAttachment("output-pt-image",
					 outputPT,
					 vk::Format::eR32G32B32A32Sfloat,
					 vk::SampleCountFlagBits::e1,
					 vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
					 vk::ImageAspectFlagBits::eColor);

	createAttachment("output-variance-image",
					 outputVariance,
					 vk::Format::eR32G32B32A32Sfloat,
					 vk::SampleCountFlagBits::e1,
					 vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
					 vk::ImageAspectFlagBits::eColor);

	vk::Format depthFormat = findDepthFormat(pDevice);
	createAttachment("depth-image-msaa",
					 depthMsaa,
					 depthFormat,
					 msaa,
					 vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
					 vk::ImageAspectFlagBits::eDepth);

	createAttachment("depth-image",
					 depth,
					 depthFormat,
					 vk::SampleCountFlagBits::e1,
					 vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
					 vk::ImageAspectFlagBits::eDepth);
}

void AppResources::createAttachment(const std::string& name, SampledTexture& att, vk::Format format, vk::SampleCountFlagBits msaaBits, vk::ImageUsageFlags usageFlags, vk::ImageAspectFlagBits aspectFlags)
{
	att.format = format;
	createImage(this, name,
				swapChainExtent.width,
				swapChainExtent.height,
				1,
				vk::ImageType::e2D,
				att.format,
				vk::ImageLayout::eUndefined,
				vk::ImageTiling::eOptimal,
				vk::ImageCreateFlagBits(),
				usageFlags,
				vk::MemoryPropertyFlagBits::eDeviceLocal,
				att.image,
				att.mem, msaaBits);

	createImageView(this, att.image, vk::ImageViewType::e2D, 1, 0,
					{ vk::ComponentSwizzle::eIdentity,
					 vk::ComponentSwizzle::eIdentity,
					 vk::ComponentSwizzle::eIdentity,
					 vk::ComponentSwizzle::eIdentity },
					att.format, aspectFlags, att.imageView);

	vk::SamplerCreateInfo sampler = {};
	sampler.magFilter = vk::Filter::eNearest;
	sampler.minFilter = vk::Filter::eNearest;
	sampler.mipmapMode = vk::SamplerMipmapMode::eNearest;
	sampler.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	sampler.addressModeV = sampler.addressModeU;
	sampler.addressModeW = sampler.addressModeU;
	sampler.mipLodBias = 0.0f;
	sampler.compareOp = vk::CompareOp::eNever;
	sampler.minLod = 0.0f;
	sampler.maxLod = 1;
	sampler.borderColor = vk::BorderColor::eFloatOpaqueWhite;
	sampler.maxAnisotropy = 1.0f;
	sampler.anisotropyEnable = VK_FALSE;
	device.createSampler(&sampler, nullptr, &att.sampler);
}

void AppResources::createFramebuffersPrePass()
{
	swapChainPreFramebuffers.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		std::vector<vk::ImageView> attachments;
		if (msaa != vk::SampleCountFlagBits::e1)
		{
			attachments = {
					colorMsaa.imageView,
					positionMsaa.imageView,
					normalMsaa.imageView,
					tangentMsaa.imageView,
					bitangentMsaa.imageView,
					depthMsaa.imageView,
					color.imageView,
					position.imageView,
					normal.imageView,
					tangent.imageView,
					bitangent.imageView,
					depth.imageView
			};
		}
		else
		{
			attachments = {
					color.imageView,
					position.imageView,
					normal.imageView,
					tangent.imageView,
					bitangent.imageView,
					depth.imageView
			};
		}

		vk::FramebufferCreateInfo createInfo = {};
		createInfo.renderPass = preRenderPass;
		createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		createInfo.pAttachments = attachments.data();
		createInfo.width = swapChainExtent.width;
		createInfo.height = swapChainExtent.height;
		createInfo.layers = 1;

		swapChainPreFramebuffers[i] = device.createFramebuffer(createInfo);
	}
}

void AppResources::createFramebuffersDeferred()
{
	swapChainDeferredFramebuffers.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		std::vector<vk::ImageView> attachments;
		attachments = {
				colorImagesHDR[i].imageView,
				outputMask.imageView,
				outputPT.imageView
		};

		vk::FramebufferCreateInfo createInfo = {};
		createInfo.renderPass = deferredRenderPass;
		createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		createInfo.pAttachments = attachments.data();
		createInfo.width = swapChainExtent.width;
		createInfo.height = swapChainExtent.height;
		createInfo.layers = 1;

		swapChainDeferredFramebuffers[i] = device.createFramebuffer(createInfo);
	}

	swapChainDeferredFramebuffersPSF.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		std::vector<vk::ImageView> attachments;
		attachments = {
				colorImagesHDR[i].imageView,
				outputVariance.imageView,
				outputPT.imageView
		};

		vk::FramebufferCreateInfo createInfo = {};
		createInfo.renderPass = deferredRenderPassPSF;
		createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		createInfo.pAttachments = attachments.data();
		createInfo.width = swapChainExtent.width;
		createInfo.height = swapChainExtent.height;
		createInfo.layers = 1;

		swapChainDeferredFramebuffersPSF[i] = device.createFramebuffer(createInfo);
	}

	swapChainDeferredFramebuffersImGui.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		std::vector<vk::ImageView> attachments;
		attachments = {
				colorImagesHDR[i].imageView
		};

		vk::FramebufferCreateInfo createInfo = {};
		createInfo.renderPass = imGuiRenderPass;
		createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		createInfo.pAttachments = attachments.data();
		createInfo.width = swapChainExtent.width;
		createInfo.height = swapChainExtent.height;
		createInfo.layers = 1;

		swapChainDeferredFramebuffersImGui[i] = device.createFramebuffer(createInfo);
	}
}

void AppResources::initImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	float fontsize = 13.0f;
	io.Fonts->AddFontDefault();
	// merge in icons from Font Awesome
	static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
	ImFontConfig icons_config;
	icons_config.MergeMode = true;
	icons_config.PixelSnapH = true;
	std::string pathToFont = "res/fonts/fa/";
	pathToFont += FONT_ICON_FILE_NAME_FAS;
	io.Fonts->AddFontFromFileTTF(pathToFont.c_str(), fontsize, &icons_config, icons_ranges);
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForVulkan(windowObj->window, true);

	vk::DescriptorPoolSize pool_sizes[] =
	{
			vk::DescriptorPoolSize({VK_DESCRIPTOR_TYPE_SAMPLER, 1000}),
			vk::DescriptorPoolSize({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000}),
			vk::DescriptorPoolSize({VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000}),
			vk::DescriptorPoolSize({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000}),
			vk::DescriptorPoolSize({VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000}),
			vk::DescriptorPoolSize({VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000}),
			vk::DescriptorPoolSize({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000}),
			vk::DescriptorPoolSize({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000}),
			vk::DescriptorPoolSize({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000}),
			vk::DescriptorPoolSize({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000}),
			vk::DescriptorPoolSize({VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000})
	};

	vk::DescriptorPoolCreateInfo pool_info = {};
	pool_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	imguiPool = device.createDescriptorPool(pool_info);

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = instance;
	init_info.PhysicalDevice = pDevice;
	init_info.Device = device;
	init_info.QueueFamily = gQ;
	init_info.Queue = graphicsQueue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info, imGuiRenderPass);

	vk::CommandBuffer commandBuffer = beginSingleTimeCommands(device, graphicsCommandPool);
	ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
	endSingleTimeCommands(device, graphicsQueue, graphicsCommandPool, commandBuffer);
	ImGui_ImplVulkan_DestroyFontUploadObjects();
	windowObj->gui_control.initTextures(this);
	windowObj->ImGuiLayout();
}

void AppResources::createCommandBuffers()
{
	// Allocate graphics command buffers
	graphicsCommandBuffers.resize(swapChainImages.size());
	graphicsCommandBuffers2.resize(swapChainImages.size());

	vk::CommandBufferAllocateInfo allocInfo = {};
	allocInfo.commandPool = graphicsCommandPool;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandBufferCount = (uint32_t)swapChainImages.size();

	graphicsCommandBuffers = device.allocateCommandBuffers(allocInfo);
	graphicsCommandBuffers2 = device.allocateCommandBuffers(allocInfo);
}

bool AppResources::supportsBlit(vk::Format src, vk::ImageTiling srcTiling, vk::Format dst, vk::ImageTiling dstTiling)
{
	bool supportsBlit = true;

	vk::FormatProperties formatProperties = pDevice.getFormatProperties(src);
	if (srcTiling == vk::ImageTiling::eOptimal && !(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitSrc))
	{
		supportsBlit = false;
	}
	if (srcTiling == vk::ImageTiling::eLinear && !(formatProperties.linearTilingFeatures & vk::FormatFeatureFlagBits::eBlitSrc))
	{
		supportsBlit = false;
	}

	formatProperties = pDevice.getFormatProperties(dst);

	if (dstTiling == vk::ImageTiling::eOptimal && !(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitDst))
	{
		supportsBlit = false;
	}
	if (dstTiling == vk::ImageTiling::eLinear && !(formatProperties.linearTilingFeatures & vk::FormatFeatureFlagBits::eBlitDst))
	{
		supportsBlit = false;
	}
	return supportsBlit;
}

void AppResources::updateUniformData()
{
	scene->updateUniformData(this, &windowObj->gui_control);
	skyBoxScene->updateUniformData(this, &windowObj->gui_control);

	float dt_time = time_delta;
	if (!windowObj->gui_control.referenceMode)
	{
		if (windowObj->gui_control.var.autoCloudTimer && abs(dt_time) < 1.0f)
		{
			windowObj->gui_control.var.cloudTimer += dt_time * windowObj->gui_control.var.cloudTimerDelta;
		}
		if (windowObj->gui_control.var.autoDayCycle && abs(dt_time) < 1.0f)
		{
			windowObj->gui_control.var.time += dt_time * 24.0f / (windowObj->gui_control.var.timeDelta * 60.0f);
			if (windowObj->gui_control.var.time >= 24.0f)
			{
				windowObj->gui_control.var.time = std::fmod(windowObj->gui_control.var.time, 24.0f);
			}
		}
	}
}

void AppResources::updateCommandBuffer(uint32_t i)
{
	auto& cb = graphicsCommandBuffers[i];
	// Prepare data for recording command buffer
	vk::CommandBufferBeginInfo beginInfo = {};
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;

	std::array<vk::ClearValue, 6> prePassClearValues{};
	prePassClearValues[0].color = vk::ClearColorValue(std::array<float, 4>({ 0.1f, 0.1f, 0.1f, 1.0f }));
	prePassClearValues[1].color = vk::ClearColorValue(std::array<float, 4>({ 0.0f, 0.0f, 0.0f, 0.0f }));
	prePassClearValues[2].color = vk::ClearColorValue(std::array<float, 4>({ 0.0f, 0.0f, 0.0f, 0.0f }));
	prePassClearValues[3].color = vk::ClearColorValue(std::array<float, 4>({ 0.0f, 0.0f, 0.0f, 0.0f }));
	prePassClearValues[4].color = vk::ClearColorValue(std::array<float, 4>({ 0.0f, 0.0f, 0.0f, 0.0f }));
	prePassClearValues[5].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

	std::array<vk::ClearValue, 3> deferredClearValues{};
	deferredClearValues[0].color = vk::ClearColorValue(std::array<float, 4>({ 0.1f, 0.1f, 0.1f, 1.0f }));
	deferredClearValues[1].color = vk::ClearColorValue(std::array<float, 4>({ 0.0f, 0.0f, 0.0f, 0.0f }));
	deferredClearValues[2].color = vk::ClearColorValue(std::array<float, 4>({ 0.0f, 0.0f, 0.0f, 0.0f }));

	vk::RenderPassBeginInfo preRenderPassBeginInfo = {};
	preRenderPassBeginInfo.renderPass = preRenderPass;
	preRenderPassBeginInfo.framebuffer = swapChainPreFramebuffers[i];
	preRenderPassBeginInfo.renderArea.offset.x = 0;
	preRenderPassBeginInfo.renderArea.offset.y = 0;
	preRenderPassBeginInfo.renderArea.extent = swapChainExtent;
	preRenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(prePassClearValues.size());
	preRenderPassBeginInfo.pClearValues = prePassClearValues.data();

	vk::RenderPassBeginInfo deferredRenderPassBeginInfo = {};
	deferredRenderPassBeginInfo.renderPass = deferredRenderPass;
	deferredRenderPassBeginInfo.framebuffer = swapChainDeferredFramebuffers[i];
	deferredRenderPassBeginInfo.renderArea.offset.x = 0;
	deferredRenderPassBeginInfo.renderArea.offset.y = 0;
	deferredRenderPassBeginInfo.renderArea.extent = swapChainExtent;
	deferredRenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(deferredClearValues.size());
	deferredRenderPassBeginInfo.pClearValues = deferredClearValues.data();

	vk::RenderPassBeginInfo deferredPsfRenderPassBeginInfo = {};
	deferredPsfRenderPassBeginInfo.renderPass = deferredRenderPassPSF;
	deferredPsfRenderPassBeginInfo.framebuffer = swapChainDeferredFramebuffersPSF[i];
	deferredPsfRenderPassBeginInfo.renderArea.offset.x = 0;
	deferredPsfRenderPassBeginInfo.renderArea.offset.y = 0;
	deferredPsfRenderPassBeginInfo.renderArea.extent = swapChainExtent;
	deferredPsfRenderPassBeginInfo.clearValueCount = static_cast<uint32_t>(deferredClearValues.size());
	deferredPsfRenderPassBeginInfo.pClearValues = deferredClearValues.data();

	cb.begin(&beginInfo);

	vk::ImageSubresourceRange subResourceRange = {};
	subResourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	subResourceRange.baseMipLevel = 0;
	subResourceRange.levelCount = 1;
	subResourceRange.baseArrayLayer = 0;
	subResourceRange.layerCount = 1;

	vk::ImageMemoryBarrier presentToDrawBarrier = {};
	presentToDrawBarrier.srcAccessMask = vk::AccessFlags();
	presentToDrawBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
	presentToDrawBarrier.oldLayout = vk::ImageLayout::eUndefined;
	presentToDrawBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;

	if (gQ != pQ)
	{
		presentToDrawBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		presentToDrawBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	}
	else
	{
		presentToDrawBarrier.srcQueueFamilyIndex = pQ;
		presentToDrawBarrier.dstQueueFamilyIndex = gQ;
	}

	presentToDrawBarrier.image = swapChainImages[i].image;
	presentToDrawBarrier.subresourceRange = subResourceRange;


	cb.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput,
					   vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::DependencyFlags(),
					   0, nullptr, 0, nullptr, 1, &presentToDrawBarrier);

	cb.resetQueryPool(graphicsQueryPool, 0, windowObj->gui_control.graphicsTimings.container.size() + 1);
	cb.writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, graphicsQueryPool, 0);

	skyBoxScene->skyBoxRenderPass(&cb, this);

	cb.writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, graphicsQueryPool, 1);

	scene->rebuildAccelerationStructure(this, &cb);

	cb.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
					   vk::PipelineStageFlagBits::eTopOfPipe, vk::DependencyFlags(),
					   { vk::MemoryBarrier(vk::AccessFlagBits::eMemoryWrite,
										  vk::AccessFlagBits::eMemoryRead) }, {}, {});

	cb.writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, graphicsQueryPool, 2);

	if (windowObj->gui_control.var.currentRenderMode == RM_PATH_SPACE_FILTERING ||
		windowObj->gui_control.var.currentRenderMode == RM_ADAPTIVE_PATH_SPACE_FILTERING_1 ||
		windowObj->gui_control.var.currentRenderMode == RM_ADAPTIVE_PATH_SPACE_FILTERING_2)
	{
		scene->clearHashMap(&cb, this);
	}


	cb.beginRenderPass(preRenderPassBeginInfo, vk::SubpassContents::eInline);
	cb.writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, graphicsQueryPool, 3);

	//----------------------------------------render-generated-skybox------------------------------------//
	skyBoxScene->defaultRenderPass(&cb, this);

	//----------------------------------------render-scene-to-framebuffer-----------------------------------//

	scene->preRenderPass(&cb, this);

	cb.endRenderPass();

	setImageLayout(cb, color.image, color.format,
				   vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eGeneral);
	setImageLayout(cb, position.image, position.format,
				   vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eGeneral);
	setImageLayout(cb, normal.image, normal.format,
				   vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eGeneral);
	setImageLayout(cb, tangent.image, tangent.format,
				   vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eGeneral);
	setImageLayout(cb, bitangent.image, bitangent.format,
				   vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eGeneral);
	setImageLayout(cb, depth.image, depth.format,
				   vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);
	setImageLayout(cb, scene->brightness.image, scene->brightness.format,
				   vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);

	cb.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, graphicsQueryPool, 4);

	//----------------------------------------render-to-swapchain-image-----------------------------------//
	cb.beginRenderPass(deferredRenderPassBeginInfo, vk::SubpassContents::eInline);

	scene->deferredRenderPass(&cb, this);

	cb.endRenderPass();

	setImageLayout(cb, outputMask.image, outputMask.format,
				   vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);

	cb.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, graphicsQueryPool, 5);

	if ((windowObj->gui_control.var.currentRenderMode == RM_PATH_SPACE_FILTERING ||
		windowObj->gui_control.var.currentRenderMode == RM_ADAPTIVE_PATH_SPACE_FILTERING_1 ||
		windowObj->gui_control.var.currentRenderMode == RM_ADAPTIVE_PATH_SPACE_FILTERING_2) &&
		windowObj->gui_control.var.currentPsfMode != PSFM_DEBUG_HASH_CELLS &&
		windowObj->gui_control.var.currentPsfMode != PSFM_DEBUG_HASH_FINGERPRINTS)
	{
		cb.pipelineBarrier(vk::PipelineStageFlagBits::eAllGraphics,
						   vk::PipelineStageFlagBits::eTopOfPipe, vk::DependencyFlags(),
						   { vk::MemoryBarrier(vk::AccessFlagBits::eMemoryWrite,
											  vk::AccessFlagBits::eMemoryRead) }, {}, {});

		cb.beginRenderPass(deferredPsfRenderPassBeginInfo, vk::SubpassContents::eInline);
		scene->psfPass(&cb, this);
		cb.endRenderPass();
	}

	cb.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, graphicsQueryPool, 6);

#if FALSE // We do not do this
	if ((windowObj->gui_control.var.currentRenderMode == RM_ADAPTIVE_PATH_SPACE_FILTERING_1 ||
		windowObj->gui_control.var.currentRenderMode == RM_ADAPTIVE_PATH_SPACE_FILTERING_2) &&
		windowObj->gui_control.var.currentPsfMode != PSFM_DEBUG_HASH_CELLS &&
		windowObj->gui_control.var.currentPsfMode != PSFM_DEBUG_HASH_FINGERPRINTS &&
		windowObj->gui_control.var.currentRenderState == RS_COLOR)
	{
		setImageLayout(cb,
					   colorImagesHDR[i].image,
					   colorImagesHDR[i].format,
					   vk::ImageLayout::eUndefined,
					   vk::ImageLayout::eGeneral);

		scene->filterBrightnessPipeline.setImages(&colorImagesHDR[i], &scene->brightness, swapChainExtent);

		scene->filterBrightnessPipeline.updateDescriptorSets(this);
		scene->filterBrightnessPipeline.dispatch(&cb);

		setImageLayout(cb,
					   colorImagesHDR[i].image,
					   colorImagesHDR[i].format,
					   vk::ImageLayout::eUndefined,
					   vk::ImageLayout::eColorAttachmentOptimal);
	}
#endif 


	if (windowObj->gui_control.referenceMode)
	{
		cb.pipelineBarrier(vk::PipelineStageFlagBits::eAllGraphics,
						   vk::PipelineStageFlagBits::eTopOfPipe, vk::DependencyFlags(),
						   { vk::MemoryBarrier(vk::AccessFlagBits::eMemoryWrite,
											  vk::AccessFlagBits::eMemoryRead) }, {}, {});

		setImageLayout(cb,
					   colorImagesHDR[i].image,
					   colorImagesHDR[i].format,
					   vk::ImageLayout::eUndefined,
					   vk::ImageLayout::eGeneral);

		setImageLayout(cb,
					   colorReference.image,
					   colorReference.format,
					   vk::ImageLayout::eUndefined,
					   vk::ImageLayout::eGeneral);

		applyRefPipe.setImage(&colorReference, &colorImagesHDR[i], &colorImagesHDR[i], swapChainExtent);
		applyRefPipe.updateDescriptorSets(this);
		applyRefPipe.dispatch(&cb);

		setImageLayout(cb,
					   colorImagesHDR[i].image,
					   colorImagesHDR[i].format,
					   vk::ImageLayout::eUndefined,
					   vk::ImageLayout::eColorAttachmentOptimal);
	}

	if (windowObj->gui_control.var.denoise)
	{
		cb.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
						   vk::PipelineStageFlagBits::eTopOfPipe, vk::DependencyFlags(),
						   { vk::MemoryBarrier(vk::AccessFlagBits::eMemoryWrite,
											  vk::AccessFlagBits::eMemoryRead) }, {}, {});

		setImageLayout(cb,
					   colorImagesHDR[i].image,
					   colorImagesHDR[i].format,
					   vk::ImageLayout::eUndefined,
					   vk::ImageLayout::eGeneral);

		std::array<SampledTexture*, 3> images = { &colorImagesHDR[i], &color, &normal };
		denoiser.copyImageToBuffer(this, cb, images, swapChainExtent);

		setImageLayout(cb,
					   colorImagesHDR[i].image,
					   colorImagesHDR[i].format,
					   vk::ImageLayout::eUndefined,
					   vk::ImageLayout::eColorAttachmentOptimal);
	}
	cb.end();
}

void AppResources::updateCommandBuffer2(uint32_t i)
{
	auto& cb = graphicsCommandBuffers2[i];

	// Prepare data for recording command buffer
	vk::CommandBufferBeginInfo beginInfo = {};
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
	cb.begin(&beginInfo);

	std::array<vk::ClearValue, 1> clearValues{};
	clearValues[0].color = vk::ClearColorValue(std::array<float, 4>({ 0.1f, 0.1f, 0.1f, 1.0f }));

	vk::RenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.renderPass = imGuiRenderPass;
	renderPassBeginInfo.framebuffer = swapChainDeferredFramebuffersImGui[i];
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent = swapChainExtent;
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassBeginInfo.pClearValues = clearValues.data();

	if (windowObj->gui_control.var.denoise)
	{

		setImageLayout(cb,
					   colorImagesHDR[i].image,
					   colorImagesHDR[i].format,
					   vk::ImageLayout::eUndefined,
					   vk::ImageLayout::eGeneral);

		denoiser.copyBufferToImage(this, cb, &colorImagesHDR[i], swapChainExtent);


		setImageLayout(cb,
					   colorImagesHDR[i].image,
					   colorImagesHDR[i].format,
					   vk::ImageLayout::eUndefined,
					   vk::ImageLayout::eColorAttachmentOptimal);

		cb.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
						   vk::PipelineStageFlagBits::eTopOfPipe, vk::DependencyFlags(),
						   { vk::MemoryBarrier(vk::AccessFlagBits::eMemoryWrite,
											  vk::AccessFlagBits::eMemoryRead) }, {}, {});
	}

	cb.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, graphicsQueryPool, 7);

	setImageLayout(cb,
				   outputVariance.image,
				   outputVariance.format,
				   vk::ImageLayout::eUndefined,
				   vk::ImageLayout::eGeneral);

	setImageLayout(cb,
				   outputPT.image,
				   outputPT.format,
				   vk::ImageLayout::eUndefined,
				   vk::ImageLayout::eGeneral);

	if (windowObj->gui_control.var.renderLines)
	{
		cb.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
		scene->renderLineVisualization(&cb, this);
		cb.endRenderPass();
	}

	if (!Window::pressedKeys[Window::KEY_G] && !windowObj->hide_imgui_screenshot)
	{
		cb.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cb);
		cb.endRenderPass();
	}

	setImageLayout(cb,
				   colorImagesHDR[i].image,
				   colorImagesHDR[i].format,
				   vk::ImageLayout::eColorAttachmentOptimal,
				   vk::ImageLayout::eTransferSrcOptimal);


	setImageLayout(cb,
				   swapChainImages[i].image,
				   swapChainFormat,
				   vk::ImageLayout::eUndefined,
				   vk::ImageLayout::eTransferDstOptimal);

	cb.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, graphicsQueryPool, 8);

	if (supportsBlit(colorImagesHDR[i].format, vk::ImageTiling::eOptimal, swapChainFormat, vk::ImageTiling::eOptimal))
	{
		vk::Offset3D blitSize;
		blitSize.x = swapChainExtent.width;
		blitSize.y = swapChainExtent.height;
		blitSize.z = 1;
		vk::ImageBlit imageBlitRegion{};
		imageBlitRegion.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		imageBlitRegion.srcSubresource.layerCount = 1;
		imageBlitRegion.srcOffsets[1] = blitSize;
		imageBlitRegion.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		imageBlitRegion.dstSubresource.layerCount = 1;
		imageBlitRegion.dstOffsets[1] = blitSize;

		// Issue the blit command
		cb.blitImage(colorImagesHDR[i].image,
					 vk::ImageLayout::eTransferSrcOptimal,
					 swapChainImages[i].image,
					 vk::ImageLayout::eTransferDstOptimal,
					 1, &imageBlitRegion,
					 vk::Filter::eNearest);
	}
	else
	{
		vk::ImageCopy imageCopyRegion{};
		imageCopyRegion.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		imageCopyRegion.srcSubresource.layerCount = 1;
		imageCopyRegion.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		imageCopyRegion.dstSubresource.layerCount = 1;
		imageCopyRegion.extent.width = swapChainExtent.width;
		imageCopyRegion.extent.height = swapChainExtent.height;
		imageCopyRegion.extent.depth = 1;

		// Issue the copy command
		cb.copyImage(colorImagesHDR[i].image,
					 vk::ImageLayout::eTransferSrcOptimal,
					 swapChainImages[i].image,
					 vk::ImageLayout::eTransferDstOptimal,
					 1, &imageCopyRegion);
	}

	setImageLayout(cb,
				   swapChainImages[i].image,
				   swapChainFormat,
				   vk::ImageLayout::eTransferDstOptimal,
				   vk::ImageLayout::ePresentSrcKHR);


	if (pQ != gQ)
	{
		vk::ImageSubresourceRange subResourceRange = {};
		subResourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		subResourceRange.baseMipLevel = 0;
		subResourceRange.levelCount = 1;
		subResourceRange.baseArrayLayer = 0;
		subResourceRange.layerCount = 1;

		vk::ImageMemoryBarrier drawToPresentBarrier = {};
		drawToPresentBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
		drawToPresentBarrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead;
		drawToPresentBarrier.oldLayout = vk::ImageLayout::ePresentSrcKHR;
		drawToPresentBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
		drawToPresentBarrier.srcQueueFamilyIndex = gQ;
		drawToPresentBarrier.dstQueueFamilyIndex = pQ;
		drawToPresentBarrier.image = swapChainImages[i].image;
		drawToPresentBarrier.subresourceRange = subResourceRange;
		cb.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput,
						   vk::PipelineStageFlagBits::eBottomOfPipe, vk::DependencyFlags(), 0,
						   nullptr, 0, nullptr, 1, &drawToPresentBarrier);
	}
	cb.end();
}

void AppResources::draw(Window* _windowObj)
{
	windowObj = _windowObj;

	updateUniformData();
	// Acquire image
	vk::Result res = device.acquireNextImageKHR(swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE,
												&currentImageIndex);

	// Unless surface is out of date right now, defer swap chain recreation until end of this frame
	if (res == vk::Result::eErrorOutOfDateKHR)
	{
		onWindowSizeChanged(false, windowObj->gui_control.var.vsync);
		return;
	}
	else if (res != vk::Result::eSuccess)
	{
		std::cerr << "failed to acquire image" << std::endl;
		exit(1);
	}
	updateCommandBuffer(currentImageIndex);
	updateCommandBuffer2(currentImageIndex);

	frameCount++;
	fenceValue++;

	vk::CommandBufferSubmitInfoKHR cmdBufInfo;
	cmdBufInfo.commandBuffer = graphicsCommandBuffers[currentImageIndex];

	vk::SemaphoreSubmitInfoKHR waitSemaphore;
	waitSemaphore.semaphore = imageAvailableSemaphore;
	waitSemaphore.stageMask = vk::PipelineStageFlagBits2KHR::eColorAttachmentOutput;

	vk::SemaphoreSubmitInfoKHR signalSemaphore;
	signalSemaphore.semaphore = denoiser.getSemaphore();
	signalSemaphore.stageMask = vk::PipelineStageFlagBits2KHR::eAllCommands;
	signalSemaphore.value = fenceValue;

	vk::SubmitInfo2KHR submits;
	submits.setCommandBufferInfos(cmdBufInfo);
	submits.setWaitSemaphoreInfos(waitSemaphore);
	submits.setSignalSemaphoreInfos(signalSemaphore);

	// Wait for image to be available and perform ray tracing
	graphicsQueue.submit2KHR(submits);

	// submit cuda operations for denoiser
	if (windowObj->gui_control.var.denoise)
	{
		denoiser.denoiseImageBuffer(fenceValue);
	}

	// render imgui etc ontop of the buffer generated by optix
	vk::CommandBufferSubmitInfoKHR cmdBufInfo2;
	cmdBufInfo2.commandBuffer = graphicsCommandBuffers2[currentImageIndex];

	vk::SemaphoreSubmitInfoKHR waitSemaphore2;
	waitSemaphore2.semaphore = denoiser.getSemaphore();
	waitSemaphore2.stageMask = vk::PipelineStageFlagBits2KHR::eAllCommands;
	waitSemaphore2.value = fenceValue;

	vk::SemaphoreSubmitInfoKHR signalSemaphore2;
	signalSemaphore2.semaphore = renderingFinishedSemaphore;
	signalSemaphore2.stageMask = vk::PipelineStageFlagBits2KHR::eAllCommands;

	vk::SubmitInfo2KHR submits2;
	submits2.commandBufferInfoCount = 1;
	submits2.pCommandBufferInfos = &cmdBufInfo2;
	submits2.waitSemaphoreInfoCount = 1;
	submits2.pWaitSemaphoreInfos = &waitSemaphore2;
	submits2.signalSemaphoreInfoCount = 1;
	submits2.pSignalSemaphoreInfos = &signalSemaphore2;

	vk::Fence fence = device.createFence(vk::FenceCreateInfo());
	graphicsQueue.submit2KHR(1, &submits2, fence);

	// Present drawn image
	// Note: semaphore here is not strictly necessary, because commands are processed in submission order within a single queue
	vk::PresentInfoKHR presentInfo = {};
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderingFinishedSemaphore;

	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapChain;
	presentInfo.pImageIndices = &currentImageIndex;

	res = presentQueue.presentKHR(&presentInfo);

	if (res == vk::Result::eSuboptimalKHR ||
		windowResized ||
		res == vk::Result::eErrorOutOfDateKHR)
	{
		onWindowSizeChanged(false, windowObj->gui_control.var.vsync);
	}
	else if (res != vk::Result::eSuccess)
	{
		std::cerr << "failed to submit present command buffer" << std::endl;
		exit(1);
	}
	vk::Result haveIWaited = device.waitForFences({ fence }, true, uint64_t(-1));

	{
		uint64_t timestamps[9];
		vk::Result result = device.getQueryPoolResults(graphicsQueryPool, 0, windowObj->gui_control.graphicsTimings.container.size() + 1, sizeof(timestamps), &timestamps,
													   sizeof(timestamps[0]), vk::QueryResultFlagBits::e64);
		assert(result == vk::Result::eSuccess);
		uint64_t timediff = timestamps[1] - timestamps[0];
		vk::PhysicalDeviceProperties properties = pDevice.getProperties();
		uint64_t nanoseconds = properties.limits.timestampPeriod * timediff;
		windowObj->gui_control.graphicsTimings["Sky Box Generation"].appendData(nanoseconds / 1000000.f);

		timediff = timestamps[2] - timestamps[1];
		nanoseconds = properties.limits.timestampPeriod * timediff;
		windowObj->gui_control.graphicsTimings["Build RT Acceleration Structures"].appendData(nanoseconds / 1000000.f);

		timediff = timestamps[3] - timestamps[2];
		nanoseconds = properties.limits.timestampPeriod * timediff;
		windowObj->gui_control.graphicsTimings["Hash Map Eviction"].appendData(nanoseconds / 1000000.f);

		timediff = timestamps[4] - timestamps[3];
		nanoseconds = properties.limits.timestampPeriod * timediff;
		windowObj->gui_control.graphicsTimings["G-Buffer Prepass"].appendData(nanoseconds / 1000000.f);

		timediff = timestamps[5] - timestamps[4];
		nanoseconds = properties.limits.timestampPeriod * timediff;
		windowObj->gui_control.graphicsTimings["Deferred RT Pass"].appendData(nanoseconds / 1000000.f);

		timediff = timestamps[6] - timestamps[5];
		nanoseconds = properties.limits.timestampPeriod * timediff;
		windowObj->gui_control.graphicsTimings["PSF Pass"].appendData(nanoseconds / 1000000.f);

		timediff = timestamps[7] - timestamps[6];
		nanoseconds = properties.limits.timestampPeriod * timediff;
		windowObj->gui_control.graphicsTimings["Denoiser"].appendData(nanoseconds / 1000000.f);

		timediff = timestamps[8] - timestamps[7];
		nanoseconds = properties.limits.timestampPeriod * timediff;
		windowObj->gui_control.graphicsTimings["ImGui"].appendData(nanoseconds / 1000000.f);

		if (frameTimeProcessing && windowObj->gui_control.graphicsTimings["Sky Box Generation"].getFull())
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
			std::string filePathName = "./csv/" + std::string(date_string) + "_" + windowObj->gui_control.renderModesNames[windowObj->gui_control.var.currentRenderMode] + ".csv";
			windowObj->gui_control.saveGraphToFile(windowObj->gui_control.graphicsTimings, filePathName);
			std::cout << "saved to " << filePathName << "\n";
			frameTimeProcessing = false;
		}
	}

	if (screenshotProcessing && screenShotFrameCount == frameCount)
	{
		if (!useRefBeforeScreenshot)
		{
			screenshotProcessing = false;
			saveScreenShot(this, screenShotFileType);
			windowObj->hide_imgui_screenshot = false;
			screenShotFrameNum = 0;
		}
		else
		{
			useRefNowScreenshot = true;
		}
	}

	if (useRefNowScreenshot)
	{
		if (!windowObj->gui_control.referenceMode)
		{
			windowObj->gui_control.referenceMode = true;
			windowObj->gui_control.currentReferenceIteration = 0;
			device.waitIdle();
			ImGui_ImplGlfw_Shutdown();
			ImGui_ImplVulkan_Shutdown();
			ImPlot::DestroyContext();
			ImGui::DestroyContext();
			onWindowSizeChanged(true, windowObj->gui_control.var.vsync);
		}
		else if (windowObj->gui_control.currentReferenceIteration >= windowObj->gui_control.var.referenceIterations)
		{
			screenshotProcessing = false;
			useRefBeforeScreenshot = false;
			useRefNowScreenshot = false;
			saveScreenShot(this, screenShotFileType);
			windowObj->hide_imgui_screenshot = false;
			screenShotFrameNum = 0;
			windowObj->gui_control.referenceMode = false;
			windowObj->gui_control.currentReferenceIteration = 0;
			device.waitIdle();
			ImGui_ImplGlfw_Shutdown();
			ImGui_ImplVulkan_Shutdown();
			ImPlot::DestroyContext();
			ImGui::DestroyContext();
			onWindowSizeChanged(true, windowObj->gui_control.var.vsync);
		}
	}

	device.destroyFence(fence);

	if (first_draw)
	{
		first_draw = false;
	}
}