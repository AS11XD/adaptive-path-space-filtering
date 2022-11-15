#include "denoiser.h"
#include <iostream>
#include <optix.h>
#include <optix_function_table_definition.h>
#include <optix_stubs.h>
#include "initialization.h"

Denoiser::Denoiser()
{
}

Denoiser::~Denoiser()
{
}

void Denoiser::init(AppResources* app, OptixDenoiserOptions _options, OptixPixelFormat _pixelFormat, bool hdr)
{
	queueIdx = app->gQ;
	device = app->device;
	pDevice = app->pDevice;

	CUresult cuRes = cuInit(0);  // Initialize CUDA driver API.
	if (cuRes != CUDA_SUCCESS)
	{
		throw std::exception("ERROR: initOptiX() cuInit() failed: " + cuRes);
	}

	CUdevice device = 0;
	cuRes = cuCtxCreate(&cudaContext, CU_CTX_SCHED_SPIN, device);
	if (cuRes != CUDA_SUCCESS)
	{
		throw std::exception("ERROR: initOptiX() cuCtxCreate() failed: " + cuRes);
	}

	// PERF Use CU_STREAM_NON_BLOCKING if there is any work running in parallel on multiple streams.
	cuRes = cuStreamCreate(&cuStream, CU_STREAM_DEFAULT);
	if (cuRes != CUDA_SUCCESS)
	{
		throw std::exception("ERROR: initOptiX() cuStreamCreate() failed: " + cuRes);
	}

	OPTIX_CALL(optixInit());
	OPTIX_CALL(optixDeviceContextCreate(cudaContext, nullptr, &optixDevice));

	pixelFormat = _pixelFormat;
	switch (pixelFormat)
	{

		case OPTIX_PIXEL_FORMAT_FLOAT3:
			sizeofPixel = static_cast<uint32_t>(3 * sizeof(float));
			denoiseAlpha = 0;
			break;
		case OPTIX_PIXEL_FORMAT_FLOAT4:
			sizeofPixel = static_cast<uint32_t>(4 * sizeof(float));
			denoiseAlpha = 1;
			break;
		case OPTIX_PIXEL_FORMAT_UCHAR3:
			sizeofPixel = static_cast<uint32_t>(3 * sizeof(uint8_t));
			denoiseAlpha = 0;
			break;
		case OPTIX_PIXEL_FORMAT_UCHAR4:
			sizeofPixel = static_cast<uint32_t>(4 * sizeof(uint8_t));
			denoiseAlpha = 1;
			break;
		case OPTIX_PIXEL_FORMAT_HALF3:
			sizeofPixel = static_cast<uint32_t>(3 * sizeof(uint16_t));
			denoiseAlpha = 0;
			break;
		case OPTIX_PIXEL_FORMAT_HALF4:
			sizeofPixel = static_cast<uint32_t>(4 * sizeof(uint16_t));
			denoiseAlpha = 1;
			break;
		default:
			assert(!"unsupported");
			break;
	}


	// This is to use RGB + Albedo + Normal
	denoiseOptions = _options;
	OptixDenoiserModelKind modelKind = hdr ? OPTIX_DENOISER_MODEL_KIND_HDR : OPTIX_DENOISER_MODEL_KIND_LDR;
	OPTIX_CALL(optixDenoiserCreate(optixDevice, modelKind, &denoiseOptions, &denoiser));

	copyImgToBufPipe.generatePipeline(app, true);
	copyBufToImgPipe.generatePipeline(app, true);
}

void Denoiser::createSemaphore()
{
	auto handleType = vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32;

	vk::SemaphoreTypeCreateInfo timelineCreateInfo;
	timelineCreateInfo.semaphoreType = vk::SemaphoreType::eTimeline;
	timelineCreateInfo.initialValue = 0;

	vk::SemaphoreCreateInfo sci;
	sci.pNext = &timelineCreateInfo;
	vk::ExportSemaphoreCreateInfo esci;
	esci.pNext = &timelineCreateInfo;
	sci.pNext = &esci;
	esci.handleTypes = handleType;
	semaphore.vkSemaphore = device.createSemaphore(sci);

	semaphore.handle = device.getSemaphoreWin32HandleKHR({ semaphore.vkSemaphore, handleType });

	cudaExternalSemaphoreHandleDesc externalSemaphoreHandleDesc;
	std::memset(&externalSemaphoreHandleDesc, 0, sizeof(externalSemaphoreHandleDesc));
	externalSemaphoreHandleDesc.flags = 0;
	externalSemaphoreHandleDesc.type = cudaExternalSemaphoreHandleTypeTimelineSemaphoreWin32;
	externalSemaphoreHandleDesc.handle.win32.handle = (void*)semaphore.handle;

	CUDA_CALL(cudaImportExternalSemaphore(&semaphore.cuSemaphore, &externalSemaphoreHandleDesc));
}

void Denoiser::allocateBuffers(vk::Extent2D& _imageSize, bool update)
{
	imageSize = _imageSize;

	if (update)
	{
		destroyBuffers();
	}

	vk::DeviceSize bufferSize = static_cast<unsigned long long>(imageSize.width) * imageSize.height * 4 * sizeof(float);

	// Using direct method
	vk::BufferUsageFlags usage{ vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst
							   | vk::BufferUsageFlagBits::eTransferSrc };

	createExportBuffer(pDevice, device, bufferSize, usage, vk::MemoryPropertyFlagBits::eDeviceLocal, "optix-rgb", pixelBufferIn[0].buffer);
	createBufferCuda(pixelBufferIn[0]);  // Exporting the buffer to Cuda handle and pointers

	if (denoiseOptions.guideAlbedo)
	{
		createExportBuffer(pDevice, device, bufferSize, usage, vk::MemoryPropertyFlagBits::eDeviceLocal, "optix-albedo", pixelBufferIn[1].buffer);
		createBufferCuda(pixelBufferIn[1]);
	}
	if (denoiseOptions.guideNormal)
	{
		createExportBuffer(pDevice, device, bufferSize, usage, vk::MemoryPropertyFlagBits::eDeviceLocal, "optix-normal", pixelBufferIn[2].buffer);
		createBufferCuda(pixelBufferIn[2]);
	}

	// Output image/buffer
	createExportBuffer(pDevice, device, bufferSize, usage | vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eDeviceLocal, "optix-albedo", pixelBufferOut.buffer);
	createBufferCuda(pixelBufferOut);

	// Computing the amount of memory needed to do the denoiser
	OPTIX_CALL(optixDenoiserComputeMemoryResources(denoiser, imageSize.width, imageSize.height, &denoiseSizes));

	CUDA_CALL(cudaMalloc((void**)&denoiseStateBuffer, denoiseSizes.stateSizeInBytes));
	CUDA_CALL(cudaMalloc((void**)&denoiseScratchBuffer, denoiseSizes.withoutOverlapScratchSizeInBytes));
	CUDA_CALL(cudaMalloc((void**)&denoiseMinRGB, 4 * sizeof(float)));
	if (pixelFormat == OPTIX_PIXEL_FORMAT_FLOAT3 || pixelFormat == OPTIX_PIXEL_FORMAT_FLOAT4)
		CUDA_CALL(cudaMalloc((void**)&denoiseIntensity, sizeof(float)));

	OPTIX_CALL(optixDenoiserSetup(denoiser, cuStream, imageSize.width, imageSize.height, denoiseStateBuffer,
			   denoiseSizes.stateSizeInBytes, denoiseScratchBuffer, denoiseSizes.withoutOverlapScratchSizeInBytes));
}

void Denoiser::cleanup(AppResources* app)
{
	device.destroy(semaphore.vkSemaphore);
	copyBufToImgPipe.cleanUpPipeline(app, true);
	copyImgToBufPipe.cleanUpPipeline(app, true);

	destroyBuffers();
}

void Denoiser::denoiseImageBuffer(uint64_t& fenceValue)
{
	uint32_t         rowStrideInBytes = sizeofPixel * imageSize.width;

	//std::vector<OptixImage2D> inputLayer;  // Order: RGB, Albedo, Normal

	// Create and set our OptiX layers
	OptixDenoiserLayer layer = {};
	// Input
	layer.input.data = (CUdeviceptr)pixelBufferIn[0].cudaPtr;
	layer.input.width = imageSize.width;
	layer.input.height = imageSize.height;
	layer.input.rowStrideInBytes = rowStrideInBytes;
	layer.input.pixelStrideInBytes = sizeofPixel;
	layer.input.format = pixelFormat;

	// Output
	layer.output.data = (CUdeviceptr)pixelBufferOut.cudaPtr;
	layer.output.width = imageSize.width;
	layer.output.height = imageSize.height;
	layer.output.rowStrideInBytes = rowStrideInBytes;
	layer.output.pixelStrideInBytes = sizeof(float) * 4;
	layer.output.format = pixelFormat;


	OptixDenoiserGuideLayer guide_layer = {};
	// albedo
	if (denoiseOptions.guideAlbedo)
	{
		guide_layer.albedo.data = (CUdeviceptr)pixelBufferIn[1].cudaPtr;
		guide_layer.albedo.width = imageSize.width;
		guide_layer.albedo.height = imageSize.height;
		guide_layer.albedo.rowStrideInBytes = rowStrideInBytes;
		guide_layer.albedo.pixelStrideInBytes = sizeofPixel;
		guide_layer.albedo.format = pixelFormat;
	}

	// normal
	if (denoiseOptions.guideNormal)
	{
		guide_layer.normal.data = (CUdeviceptr)pixelBufferIn[2].cudaPtr;
		guide_layer.normal.width = imageSize.width;
		guide_layer.normal.height = imageSize.height;
		guide_layer.normal.rowStrideInBytes = rowStrideInBytes;
		guide_layer.normal.pixelStrideInBytes = sizeofPixel;
		guide_layer.normal.format = pixelFormat;
	}

	// Wait from Vulkan (Copy to Buffer)
	cudaExternalSemaphoreWaitParams waitParams{};
	waitParams.flags = 0;
	waitParams.params.fence.value = fenceValue;
	cudaWaitExternalSemaphoresAsync(&semaphore.cuSemaphore, &waitParams, 1, nullptr);

	if (denoiseIntensity != 0)
	{
		OPTIX_CALL(optixDenoiserComputeIntensity(denoiser, cuStream, &layer.input, denoiseIntensity, denoiseScratchBuffer,
				   denoiseSizes.withoutOverlapScratchSizeInBytes));
	}

	OptixDenoiserParams denoiserParams{};
	denoiserParams.denoiseAlpha = denoiseAlpha;
	denoiserParams.hdrIntensity = denoiseIntensity;
	denoiserParams.blendFactor = 0.0f;  // Fully denoised


	// Execute the denoiser
	OPTIX_CALL(optixDenoiserInvoke(denoiser, cuStream, &denoiserParams, denoiseStateBuffer, denoiseSizes.stateSizeInBytes, &guide_layer,
			   &layer, 1, 0, 0, denoiseScratchBuffer, denoiseSizes.withoutOverlapScratchSizeInBytes));


	CUDA_CALL(cudaStreamSynchronize(cuStream));  // Making sure the denoiser is done

	cudaExternalSemaphoreSignalParams sigParams{};
	sigParams.flags = 0;
	sigParams.params.fence.value = ++fenceValue;
	cudaSignalExternalSemaphoresAsync(&semaphore.cuSemaphore, &sigParams, 1, cuStream);
}

void Denoiser::reCreatePipelines(AppResources* app)
{
	copyImgToBufPipe.cleanUpPipeline(app, false);
	copyBufToImgPipe.cleanUpPipeline(app, false);
	copyImgToBufPipe.generatePipeline(app, false);
	copyBufToImgPipe.generatePipeline(app, false);
}

vk::Semaphore Denoiser::getSemaphore()
{
	return semaphore.vkSemaphore;
}

void Denoiser::copyImageToBuffer(AppResources* app, vk::CommandBuffer& cb, std::array<SampledTexture*, 3>& imgIn, vk::Extent2D& extent)
{
	copyImgToBufPipe.setImages(imgIn, extent);
	std::array<Buffer*, 3> buffers = { &pixelBufferIn[0].buffer, &pixelBufferIn[1].buffer, &pixelBufferIn[2].buffer };
	copyImgToBufPipe.setBuffers(buffers);
	copyImgToBufPipe.updateDescriptorSets(app);
	copyImgToBufPipe.dispatch(&cb);
}

void Denoiser::copyBufferToImage(AppResources* app, vk::CommandBuffer& cb, SampledTexture* imgIn, vk::Extent2D& extent)
{
	copyBufToImgPipe.setBuffer(&pixelBufferOut.buffer);
	copyBufToImgPipe.setImage(imgIn, extent);
	copyBufToImgPipe.updateDescriptorSets(app);
	copyBufToImgPipe.dispatch(&cb);
}

void Denoiser::destroyBuffers()
{
	for (auto& p : pixelBufferIn)
		p.destroy(device);               // Closing Handle

	pixelBufferOut.destroy(device);  // Closing Handle

	if (denoiseStateBuffer != 0)
	{
		CUDA_CALL(cudaFree((void*)denoiseStateBuffer));
	}
	if (denoiseScratchBuffer != 0)
	{
		CUDA_CALL(cudaFree((void*)denoiseScratchBuffer));
	}
	if (denoiseIntensity != 0)
	{
		CUDA_CALL(cudaFree((void*)denoiseIntensity));
	}
	if (denoiseMinRGB != 0)
	{
		CUDA_CALL(cudaFree((void*)denoiseMinRGB));
	}
}

void Denoiser::createBufferCuda(CudaInteractionBuffer& buf)
{
	buf.handle = device.getMemoryWin32HandleKHR({ buf.buffer.mem, vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32 });
	auto req = device.getBufferMemoryRequirements(buf.buffer.buf);

	cudaExternalMemoryHandleDesc cudaExtMemHandleDesc{};
	cudaExtMemHandleDesc.size = req.size;
	cudaExtMemHandleDesc.type = cudaExternalMemoryHandleTypeOpaqueWin32;
	cudaExtMemHandleDesc.handle.win32.handle = buf.handle;

	cudaExternalMemory_t cudaExtMemVertexBuffer{};
	CUDA_CALL(cudaImportExternalMemory(&cudaExtMemVertexBuffer, &cudaExtMemHandleDesc));

	cudaExternalMemoryBufferDesc cudaExtBufferDesc{};
	cudaExtBufferDesc.offset = 0;
	cudaExtBufferDesc.size = req.size;
	cudaExtBufferDesc.flags = 0;
	CUDA_CALL(cudaExternalMemoryGetMappedBuffer(&buf.cudaPtr, cudaExtMemVertexBuffer, &cudaExtBufferDesc));
}
