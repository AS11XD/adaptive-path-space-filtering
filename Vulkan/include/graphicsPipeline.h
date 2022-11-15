#pragma once

#include <vulkan/vulkan.hpp>

class AppResources;

class GraphicsPipeline
{
public:
	GraphicsPipeline();
	~GraphicsPipeline();

	void generatePipeline(AppResources* app, bool full);
	virtual void cleanUpPipeline(AppResources* app, bool full);
	void bind(vk::CommandBuffer* cb);

protected:
	virtual void bindPushConstants(vk::CommandBuffer* cb);
	virtual void createVertexInformation();
	virtual void createPipeline(AppResources* app, bool full);
	virtual void createDescriptorPool(AppResources* app);
	virtual void createDescriptorSet(AppResources* app);

	vk::Pipeline pipeline;
	vk::PipelineLayout pipelineLayout;
	vk::DescriptorPool descriptorPool;
	vk::DescriptorSetLayout descriptorSetLayout;
	vk::DescriptorSet descriptorSet;

	vk::VertexInputBindingDescription vertexBindingDescription;
	std::vector<vk::VertexInputAttributeDescription> vertexAttributeDescriptions;
};