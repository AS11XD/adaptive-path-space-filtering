#pragma once

#include "computePipeline.h"
#include "utils.h"
#include "sampledTexture.h"

class CopyBufferToImagePipeline : public ComputePipeline
{
public:
	struct PushStruct
	{
		uint32_t width;
		uint32_t height;
		float exposure;
		uint32_t toneMapping;
	};

	CopyBufferToImagePipeline();
	~CopyBufferToImagePipeline();

	void setImage(SampledTexture* _images, vk::Extent2D& extent);
	void setBuffer(Buffer* _buffers);
	void cleanUpPipeline(AppResources* app, bool full) override;
	void updateDescriptorSets(AppResources* app);

protected:
	void bindPushConstants(vk::CommandBuffer* cb) override;
	void createPipeline(AppResources* app, bool full) override;
	void createDescriptorPool(AppResources* app) override;
	void createDescriptorSet(AppResources* app) override;

	PushStruct pushConstants;

	SampledTexture* image;
	Buffer* buffer;
};
