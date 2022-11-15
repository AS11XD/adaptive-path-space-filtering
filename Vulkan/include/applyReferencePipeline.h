#pragma once

#include "computePipeline.h"
#include "utils.h"
#include "sampledTexture.h"

class ApplyReferencePipeline : public ComputePipeline
{
public:
	struct PushStruct
	{
		uint32_t width;
		uint32_t height;
		uint32_t currentIteration;
		uint32_t maxReferenceIteration;
		float alpha;
		uint32_t useExponentialMovingAverage;
		uint32_t referenceMode;
		float pad[1];
	};

	ApplyReferencePipeline();
	~ApplyReferencePipeline();

	void setImage(SampledTexture* _reference, SampledTexture* _imageIn, SampledTexture* _imageOut, vk::Extent2D& extent);
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
	SampledTexture* reference;
};
