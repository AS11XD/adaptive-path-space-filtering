#include "lineGraphicsPipeline.h"
#include "initialization.h"
#include "guiControl.h"

LineGraphicsPipeline::LineGraphicsPipeline()
{
}

LineGraphicsPipeline::~LineGraphicsPipeline()
{
}

void LineGraphicsPipeline::setUniformBuffer(DefaultPrePassGraphicsPipeline::UniformBuffer* buffer)
{
	uniformBuffer = buffer;
}

void LineGraphicsPipeline::updateData(AppResources* app, GuiControl* gui_control)
{
	pushConstants.tmp = 0;
}

void LineGraphicsPipeline::cleanUpPipeline(AppResources* app, bool full)
{
	app->device.destroyPipelineLayout(pipelineLayout, nullptr);
	app->device.destroyPipeline(pipeline);

	if (full)
	{
		app->device.destroyDescriptorSetLayout(descriptorSetLayout);
		app->device.destroyDescriptorPool(descriptorPool);
	}
}

void LineGraphicsPipeline::bindPushConstants(vk::CommandBuffer* cb)
{
	cb->pushConstants(pipelineLayout,
					  vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
					  0, sizeof(PushStruct),
					  &pushConstants);
}

void LineGraphicsPipeline::createVertexInformation()
{
	// Binding and attribute descriptions
	vertexBindingDescription.binding = 0;
	vertexBindingDescription.stride = sizeof(float) * 6;
	vertexBindingDescription.inputRate = vk::VertexInputRate::eVertex;

	vertexAttributeDescriptions.resize(2);
	vertexAttributeDescriptions[0].binding = 0;
	vertexAttributeDescriptions[0].location = 0;
	vertexAttributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;

	vertexAttributeDescriptions[1].binding = 0;
	vertexAttributeDescriptions[1].location = 1;
	vertexAttributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
	vertexAttributeDescriptions[1].offset = sizeof(float) * 3;
}

void LineGraphicsPipeline::createPipeline(AppResources* app, bool full)
{
	vk::ShaderModule vertexShaderModule, fragmentShaderModule;
	createShaderModule(app->device, "line.vert", vertexShaderModule);
	createShaderModule(app->device, "line.frag", fragmentShaderModule);

	// Set up shader stage info
	vk::PipelineShaderStageCreateInfo vertexShaderCreateInfo{};
	vertexShaderCreateInfo.stage = vk::ShaderStageFlagBits::eVertex;
	vertexShaderCreateInfo.module = vertexShaderModule;
	vertexShaderCreateInfo.pName = "main";

	vk::PipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
	fragmentShaderCreateInfo.stage = vk::ShaderStageFlagBits::eFragment;
	fragmentShaderCreateInfo.module = fragmentShaderModule;
	fragmentShaderCreateInfo.pName = "main";

	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

	// Describe vertex input
	vk::PipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = vertexAttributeDescriptions.size();
	vertexInputCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

	// Describe input assembly
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
	inputAssemblyCreateInfo.topology = vk::PrimitiveTopology::eLineList;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

	// Describe viewport and scissor
	vk::Viewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)app->swapChainExtent.width;
	viewport.height = (float)app->swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vk::Rect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = app->swapChainExtent.width;
	scissor.extent.height = app->swapChainExtent.height;

	// Note: scissor test is always enabled (although dynamic scissor is possible)
	// Number of viewports must match number of scissors
	vk::PipelineViewportStateCreateInfo viewportCreateInfo = {};
	viewportCreateInfo.viewportCount = 1;
	viewportCreateInfo.pViewports = &viewport;
	viewportCreateInfo.scissorCount = 1;
	viewportCreateInfo.pScissors = &scissor;

	// Describe rasterization
	// Note: depth bias and using polygon modes other than fill require changes to logical device creation (device features)
	vk::PipelineRasterizationStateCreateInfo rasterizationCreateInfo = {};
	rasterizationCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationCreateInfo.polygonMode = vk::PolygonMode::eLine;
	rasterizationCreateInfo.cullMode = vk::CullModeFlagBits::eNone;
	rasterizationCreateInfo.frontFace = vk::FrontFace::eClockwise;
	rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationCreateInfo.depthBiasClamp = 0.0f;
	rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;
	rasterizationCreateInfo.lineWidth = 2.0f;

	// Describe multisampling
	// Note: using multisampling also requires turning on device features
	vk::PipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
	multisampleCreateInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;
	multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
	multisampleCreateInfo.minSampleShading = 1.0f;
	multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

	// Describing color blending
	// Note: all paramaters except blendEnable and colorWriteMask are irrelevant here
	vk::PipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.blendEnable = VK_FALSE;
	colorBlendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	colorBlendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	colorBlendAttachmentState.colorBlendOp = vk::BlendOp::eAdd;
	colorBlendAttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
	colorBlendAttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	colorBlendAttachmentState.alphaBlendOp = vk::BlendOp::eAdd;
	colorBlendAttachmentState.colorWriteMask =
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
		vk::ColorComponentFlagBits::eA;

	// Note: all attachments must have the same values unless a device feature is enabled
	vk::PipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
	colorBlendCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendCreateInfo.logicOp = vk::LogicOp::eCopy;
	colorBlendCreateInfo.attachmentCount = 1;
	colorBlendCreateInfo.pAttachments = &colorBlendAttachmentState;
	colorBlendCreateInfo.blendConstants[0] = 0.0f;
	colorBlendCreateInfo.blendConstants[1] = 0.0f;
	colorBlendCreateInfo.blendConstants[2] = 0.0f;
	colorBlendCreateInfo.blendConstants[3] = 0.0f;

	if (full)
	{
		std::vector<vk::DescriptorSetLayoutBinding> bindings;
		bindings.emplace_back(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);
		// TODO bind other useful buffers

		vk::DescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = {};
		descriptorLayoutCreateInfo.bindingCount = bindings.size();
		descriptorLayoutCreateInfo.pBindings = bindings.data();

		descriptorSetLayout = app->device.createDescriptorSetLayout(descriptorLayoutCreateInfo);
	}

	vk::PushConstantRange pcr(
		vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
		0, sizeof(PushStruct));
	vk::PipelineLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.setLayoutCount = 1;
	layoutCreateInfo.pSetLayouts = &descriptorSetLayout;
	layoutCreateInfo.pushConstantRangeCount = 1;
	layoutCreateInfo.pPushConstantRanges = &pcr;

	pipelineLayout = app->device.createPipelineLayout(layoutCreateInfo);

	// Create the graphics pipeline
	vk::GraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStages.data();
	pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	pipelineCreateInfo.pViewportState = &viewportCreateInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
	pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
	pipelineCreateInfo.layout = pipelineLayout;
	pipelineCreateInfo.renderPass = app->imGuiRenderPass;
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex = -1;

	pipeline = app->device.createGraphicsPipeline(nullptr, pipelineCreateInfo).value;

	app->device.destroyShaderModule(vertexShaderModule);
	app->device.destroyShaderModule(fragmentShaderModule);
}

void LineGraphicsPipeline::createDescriptorPool(AppResources* app)
{
	// This describes how many descriptor sets we'll create from this pool for each type
	vk::DescriptorPoolSize typeCount;
	typeCount.descriptorCount = 1;
	vk::DescriptorPoolCreateInfo createInfo = {};
	createInfo.poolSizeCount = 1;
	createInfo.pPoolSizes = &typeCount;
	createInfo.maxSets = 1;

	descriptorPool = app->device.createDescriptorPool(createInfo);
}

void LineGraphicsPipeline::createDescriptorSet(AppResources* app)
{
	// There needs to be one descriptor set per binding point in the shader
	vk::DescriptorSetAllocateInfo allocInfo = {};
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	descriptorSet = app->device.allocateDescriptorSets(allocInfo)[0];

	std::vector<vk::WriteDescriptorSet> wds(1);

	// uniform buffer

	vk::DescriptorBufferInfo descriptorBufferInfo = {};
	descriptorBufferInfo.buffer = uniformBuffer->buffer.buf;
	descriptorBufferInfo.offset = 0;
	descriptorBufferInfo.range = VK_WHOLE_SIZE;
	wds[0].dstSet = descriptorSet;
	wds[0].descriptorCount = 1;
	wds[0].descriptorType = vk::DescriptorType::eUniformBuffer;
	wds[0].pBufferInfo = &descriptorBufferInfo;
	wds[0].dstBinding = 0;

	app->device.updateDescriptorSets(wds.size(), wds.data(), 0U, nullptr);
}
