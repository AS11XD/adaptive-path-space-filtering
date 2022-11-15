#pragma once

#include "computePipeline.h"
#include "utils.h"
#include "sampledTexture.h"

class FilterBrightnessPipeline : public ComputePipeline
{
public:
	struct PushStruct
	{
		uint32_t width;
		uint32_t height;
		float pad[2];
	};

	FilterBrightnessPipeline();
	~FilterBrightnessPipeline();

	void setImages(SampledTexture* _imageIn, SampledTexture* _imageOut, vk::Extent2D& extent);
	void cleanUpPipeline(AppResources* app, bool full) override;
	void updateDescriptorSets(AppResources* app);

protected:
	void bindPushConstants(vk::CommandBuffer* cb) override;
	void createPipeline(AppResources* app, bool full) override;
	void createDescriptorPool(AppResources* app) override;
	void createDescriptorSet(AppResources* app) override;

	PushStruct pushConstants;

	SampledTexture* imageIn;
	SampledTexture* imageOut;
};
