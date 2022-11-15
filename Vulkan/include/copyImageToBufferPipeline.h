#pragma once

#include "computePipeline.h"
#include "utils.h"
#include "sampledTexture.h"

class CopyImageToBufferPipeline : public ComputePipeline
{
public:
	struct PushStruct
	{
		uint32_t width;
		uint32_t height;
	};

	CopyImageToBufferPipeline();
	~CopyImageToBufferPipeline();

	void setImages(std::array<SampledTexture*, 3>& _images, vk::Extent2D& extent);
	void setBuffers(std::array<Buffer*, 3>& _buffers);
	void cleanUpPipeline(AppResources* app, bool full) override;
	void updateDescriptorSets(AppResources* app);

protected:
	void bindPushConstants(vk::CommandBuffer* cb) override;
	void createPipeline(AppResources* app, bool full) override;
	void createDescriptorPool(AppResources* app) override;
	void createDescriptorSet(AppResources* app) override;

	PushStruct pushConstants;

	std::array<SampledTexture*, 3> images;
	std::array<Buffer*, 3> buffers;
};
