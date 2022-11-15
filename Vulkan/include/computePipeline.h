#pragma once

#include <vulkan/vulkan.hpp>

class AppResources;

class ComputePipeline
{
public:
	ComputePipeline();
	~ComputePipeline();

	void generatePipeline(AppResources* app, bool full);
	virtual void cleanUpPipeline(AppResources* app, bool full);
	void dispatch(vk::CommandBuffer* cb);

protected:
	uint32_t wx, wy, wz;
	uint32_t x, y, z;

	virtual void bindPushConstants(vk::CommandBuffer* cb);
	virtual void createPipeline(AppResources* app, bool full);
	virtual void createDescriptorPool(AppResources* app);
	virtual void createDescriptorSet(AppResources* app);

	vk::Pipeline pipeline;
	vk::PipelineLayout pipelineLayout;
	vk::DescriptorPool descriptorPool;
	vk::DescriptorSetLayout descriptorSetLayout;
	vk::DescriptorSet descriptorSet;
};