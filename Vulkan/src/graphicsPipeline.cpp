#include "graphicsPipeline.h"
#include "initialization.h"

GraphicsPipeline::GraphicsPipeline()
{
}

GraphicsPipeline::~GraphicsPipeline()
{
}

void GraphicsPipeline::generatePipeline(AppResources* app, bool full)
{
	if (full)
	{
		createVertexInformation();
	}
	createPipeline(app, full);

	if (full)
	{
		createDescriptorPool(app);
		createDescriptorSet(app);
	}
}

void GraphicsPipeline::cleanUpPipeline(AppResources* app, bool full)
{
}

void GraphicsPipeline::bind(vk::CommandBuffer* cb)
{
	cb->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	cb->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	bindPushConstants(cb);
}

void GraphicsPipeline::bindPushConstants(vk::CommandBuffer* cb)
{
}

void GraphicsPipeline::createVertexInformation()
{
}

void GraphicsPipeline::createPipeline(AppResources* app, bool full)
{
}

void GraphicsPipeline::createDescriptorPool(AppResources* app)
{
}

void GraphicsPipeline::createDescriptorSet(AppResources* app)
{
}
