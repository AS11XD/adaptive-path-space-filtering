#include "computePipeline.h"
#include "initialization.h"

ComputePipeline::ComputePipeline()
{
}

ComputePipeline::~ComputePipeline()
{
}

void ComputePipeline::generatePipeline(AppResources* app, bool full)
{
	createPipeline(app, full);

	if (full)
	{
		createDescriptorPool(app);
		createDescriptorSet(app);
	}
}

void ComputePipeline::cleanUpPipeline(AppResources* app, bool full)
{
}

void ComputePipeline::dispatch(vk::CommandBuffer* cb)
{
	cb->bindPipeline(vk::PipelineBindPoint::eCompute, pipeline);
	cb->bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	bindPushConstants(cb);
	cb->dispatch((x + wx - 1) / wx, (y + wy - 1) / wy, (z + wz - 1) / wz);
}

void ComputePipeline::bindPushConstants(vk::CommandBuffer* cb)
{
}

void ComputePipeline::createPipeline(AppResources* app, bool full)
{
}

void ComputePipeline::createDescriptorPool(AppResources* app)
{
}

void ComputePipeline::createDescriptorSet(AppResources* app)
{
}
