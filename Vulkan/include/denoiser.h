#pragma once

// Code deduced from https://github.com/nvpro-samples/vk_denoise

#include <exception>
#define NOMINMAX
#include <windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#define VK_ENABLE_BETA_EXTENSIONS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <cuda.h>
#include <cuda_runtime.h>
#include <optix_types.h>
#include <driver_types.h>
#include "utils.h"
#include "sampledTexture.h"
#include "copyBufferToImagePipeline.h"
#include "copyImageToBufferPipeline.h"

#define OPTIX_CALL(func)																										\
{																														  		\
	OptixResult result = func;																							  		\
	if (result != OPTIX_SUCCESS) {																						  		\
		std::stringstream ss;																									\
		ss << "Optix call (" << #func << " ) failed with code " << result << " (" << __FILE__ << ":" << __LINE__ << ")\n";		\
		throw std::runtime_error(ss.str().c_str());																		  		\
	}																													  		\
}

#define CUDA_CALL(func)																											\
{																																\
	cudaError_t result = func;																									\
	if (result != cudaSuccess)																									\
	{																															\
		std::stringstream ss;																									\
		ss << "CUDA call (" << #func << " ) failed with code " << result << " (" __FILE__ << ":" << __LINE__ << ")\n";			\
		throw std::runtime_error(ss.str().c_str());																				\
	}																															\
}

class AppResources;

class Denoiser
{
public:
	Denoiser();
	~Denoiser();
	void init(AppResources* app,
			  OptixDenoiserOptions _options,
			  OptixPixelFormat _pixelFormat,
			  bool hdr);

	void cleanup(AppResources* app);

	void createSemaphore();
	void allocateBuffers(vk::Extent2D& _imageSize, bool update);
	void denoiseImageBuffer(uint64_t& fenceValue);
	void reCreatePipelines(AppResources* app);
	vk::Semaphore getSemaphore();

	void copyImageToBuffer(AppResources* app, vk::CommandBuffer& cb, std::array<SampledTexture*, 3>& imgIn, vk::Extent2D& extent);
	void copyBufferToImage(AppResources* app, vk::CommandBuffer& cb, SampledTexture* imgIn, vk::Extent2D& extent);
private:
	struct CudaInteractionBuffer
	{
		Buffer buffer;
		HANDLE handle;
		void* cudaPtr = nullptr;

		void destroy(vk::Device& device)
		{
			device.destroyBuffer(buffer.buf);
			device.freeMemory(buffer.mem);
			CloseHandle(handle);
		}
	};

	struct Semaphore
	{
		vk::Semaphore vkSemaphore;
		cudaExternalSemaphore_t cuSemaphore;
		HANDLE handle = nullptr;
	};

	OptixDeviceContext optixDevice;
	OptixDenoiser denoiser = nullptr;
	OptixDenoiserOptions denoiseOptions;
	OptixDenoiserSizes denoiseSizes;
	CUdeviceptr denoiseStateBuffer;
	CUdeviceptr denoiseScratchBuffer;
	CUdeviceptr denoiseIntensity;
	CUdeviceptr denoiseMinRGB;
	CUcontext cudaContext = nullptr;

	vk::Device device;
	vk::PhysicalDevice pDevice;
	uint32_t queueIdx;

	Semaphore semaphore;

	vk::Extent2D imageSize;
	std::array<CudaInteractionBuffer, 3> pixelBufferIn;  // RGB, Albedo, normal
	CudaInteractionBuffer pixelBufferOut;

	OptixPixelFormat pixelFormat;
	uint32_t sizeofPixel;
	uint32_t denoiseAlpha = 0;
	CUstream cuStream;

	CopyImageToBufferPipeline copyImgToBufPipe;
	CopyBufferToImagePipeline copyBufToImgPipe;

	void destroyBuffers();
	void createBufferCuda(CudaInteractionBuffer& buf);
};