#include "defaultDeferredGraphicsPipeline.h"
#include "initialization.h"
#include "guiControl.h"
#include "window.h"
#include <glm/gtc/type_ptr.hpp>

DefaultDeferredGraphicsPipeline::DefaultDeferredGraphicsPipeline()
{
}

DefaultDeferredGraphicsPipeline::~DefaultDeferredGraphicsPipeline()
{
}

void DefaultDeferredGraphicsPipeline::setUniformBuffer(UniformBuffer* buffer)
{
	uniformBuffer = buffer;
}

void DefaultDeferredGraphicsPipeline::setAveragePathDepthBuffer(Buffer* _averagePathDepthBuffer)
{
	averagePathDepthBuffer = _averagePathDepthBuffer;
}

void DefaultDeferredGraphicsPipeline::setLightBuffer(Buffer* _lightBuffer, uint32_t _lightCount)
{
	lightBuffer = _lightBuffer;
	pushConstants.lightCount = _lightCount;
}

void DefaultDeferredGraphicsPipeline::updateLightCount(uint32_t _lightCount)
{
	pushConstants.lightCount = _lightCount;
}

void DefaultDeferredGraphicsPipeline::setVertexIndexBuffer(Buffer* _vertexBuffer, Buffer* _indexBuffer)
{
	vertexBuffer = _vertexBuffer;
	indexBuffer = _indexBuffer;
}

void DefaultDeferredGraphicsPipeline::setMeshOffsetBuffer(Buffer* _meshOffsetBuffer)
{
	meshOffsetBuffer = _meshOffsetBuffer;
}

void DefaultDeferredGraphicsPipeline::setMeshTransformBuffer(Buffer* _transformBuffer)
{
	transformBuffer = _transformBuffer;
}

void DefaultDeferredGraphicsPipeline::setMeshNormalTransformBuffer(Buffer* _normalTransformBuffer)
{
	normalTransformBuffer = _normalTransformBuffer;
}

void DefaultDeferredGraphicsPipeline::setEnvironmentMap(SampledTexture* _renderedCubemap)
{
	renderedCubemap = _renderedCubemap;
}

void DefaultDeferredGraphicsPipeline::setAccelerationStructure(AccelerationStructure* _accelerationStructure)
{
	accelerationStructure = _accelerationStructure;
}

void DefaultDeferredGraphicsPipeline::setLineVisualizationBuffers(Buffer* _drawIndirectLinesBuffer, Buffer* _linesBuffer)
{
	drawIndirectLinesBuffer = _drawIndirectLinesBuffer;
	linesBuffer = _linesBuffer;
}

void DefaultDeferredGraphicsPipeline::setHashMapBuffers(Buffer* _hashMapBuffer, Buffer* _hashMapBuffer2, uint32_t _hashMapSize)
{
	hashMapBuffer = _hashMapBuffer;
	hashMapBuffer2 = _hashMapBuffer2;
	hashMapSize = _hashMapSize;
}

void DefaultDeferredGraphicsPipeline::setVarianceInterpolationBuffer(Buffer* _varianceInterpolationBuffer)
{
	varianceInterpolationBuffer = _varianceInterpolationBuffer;
}

void DefaultDeferredGraphicsPipeline::setBrightnessTexture(SampledTexture* _brightness)
{
	brightness = _brightness;
}

void DefaultDeferredGraphicsPipeline::setTexturesAndMaterials(std::vector<SampledTexture>* _textures, Buffer* _materialBuffer)
{
	textures = _textures;
	materialBuffer = _materialBuffer;
}

void DefaultDeferredGraphicsPipeline::cleanUpPipeline(AppResources* app, bool full)
{
	app->device.destroyPipelineLayout(pipelineLayout, nullptr);
	app->device.destroyPipeline(pipeline);
	app->device.destroyPipeline(pipelinePSF);

	if (full)
	{
		app->device.destroyDescriptorSetLayout(descriptorSetLayout);
		app->device.destroyDescriptorPool(descriptorPool);
	}
}

void DefaultDeferredGraphicsPipeline::updateData(AppResources* app, GuiControl* gui_control)
{
	pushConstants.gamma = gui_control->var.gamma;
	pushConstants.exposure = gui_control->var.exposure;
	pushConstants.renderState = gui_control->var.currentRenderState;
	pushConstants.renderMode = gui_control->var.currentRenderMode;
	pushConstants.sampleStrategyBRDF = gui_control->var.sampleStrategyBRDF;
	pushConstants.width = app->swapChainExtent.width;
	pushConstants.height = app->swapChainExtent.height;
	pushConstants.frameCount = app->frameCount;
	pushConstants.iterations = gui_control->var.pathIterations;
	pushConstants.disableFirstBounce = gui_control->var.disableFirstBounce;
	pushConstants.sampleEnvironmentMap = gui_control->var.sampleEnvironmentMap;
	pushConstants.keepCurrentLines = gui_control->var.keepCurrentLines;
	pushConstants.drawNormalMaps = gui_control->var.drawNormalMaps;
	pushConstants.denoise = gui_control->var.denoise;
	pushConstants.toneMapping = gui_control->var.toneMapping;
	// update according to grid scale
	pushConstants.hashMapSize = gui_control->calculateHashMapSize(app, hashMapSize);
	pushConstants.gridScale = gui_control->var.gridScale;
	pushConstants.gridJitter = gui_control->var.gridJitter;
	pushConstants.psfMode = gui_control->var.currentPsfMode;
	pushConstants.varianceInterpolationSize = gui_control->varianceColors.size();
	pushConstants.showPTComparison = gui_control->var.showPTComparison;
	pushConstants.varianceFuncitonUVfHM = gui_control->var.varianceFuncitonUVfHM;
	pushConstants.varianceFunctionPSR = gui_control->var.varianceFunctionPSR;
	pushConstants.varianceThresholdsUVfHM[0] = gui_control->var.varianceThresholdsUVfHM[0];
	pushConstants.varianceThresholdsUVfHM[1] = gui_control->var.varianceThresholdsUVfHM[1];
	pushConstants.varianceThresholdsPSR[0] = gui_control->var.varianceThresholdsPSR[0];
	pushConstants.varianceThresholdsPSR[1] = gui_control->var.varianceThresholdsPSR[1];
	pushConstants.varianceConst = gui_control->var.varianceConst;
	pushConstants.usePrimaryMethods = gui_control->var.usePrimaryMethods;
	pushConstants.maxSamples = gui_control->var.maxSamples;
	pushConstants.referenceMode = gui_control->referenceMode;

	uniformBuffer->bufferData.VP = glm::inverse(gui_control->camera.getVP(gui_control->var.jitter, app->frameCount));
	uniformBuffer->bufferData.cameraPos = glm::vec4(gui_control->camera.getPosition(), 1.0f);
	fillDeviceBuffer(app->device, uniformBuffer->buffer.mem, getUniformBufferData(*uniformBuffer));

}

void DefaultDeferredGraphicsPipeline::bindPSF(vk::CommandBuffer* cb)
{
	cb->bindPipeline(vk::PipelineBindPoint::eGraphics, pipelinePSF);
	cb->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	bindPushConstants(cb);
}

void DefaultDeferredGraphicsPipeline::bindPushConstants(vk::CommandBuffer* cb)
{
	cb->pushConstants(pipelineLayout,
					  vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
					  0, sizeof(PushStruct),
					  &pushConstants);
}

void DefaultDeferredGraphicsPipeline::createVertexInformation()
{
}

void DefaultDeferredGraphicsPipeline::createPipeline(AppResources* app, bool full)
{
	vk::ShaderModule vertexShaderModule, fragmentShaderModule, fragmentShaderModulePSF;
	createShaderModule(app->device, "default.vert", vertexShaderModule);
	createShaderModule(app->device, "default.frag", fragmentShaderModule);
	createShaderModule(app->device, "psf.frag", fragmentShaderModulePSF);

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
	vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;

	// Describe input assembly
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
	inputAssemblyCreateInfo.topology = vk::PrimitiveTopology::eTriangleList;
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
	rasterizationCreateInfo.polygonMode = vk::PolygonMode::eFill;
	rasterizationCreateInfo.cullMode = vk::CullModeFlagBits::eBack;
	rasterizationCreateInfo.frontFace = vk::FrontFace::eClockwise;
	rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationCreateInfo.depthBiasClamp = 0.0f;
	rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;
	rasterizationCreateInfo.lineWidth = 1.0f;

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

	vk::PipelineColorBlendAttachmentState colorBlend2AttachmentState = {};
	colorBlend2AttachmentState.blendEnable = VK_FALSE;
	colorBlend2AttachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	colorBlend2AttachmentState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	colorBlend2AttachmentState.colorBlendOp = vk::BlendOp::eAdd;
	colorBlend2AttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
	colorBlend2AttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	colorBlend2AttachmentState.alphaBlendOp = vk::BlendOp::eAdd;
	colorBlend2AttachmentState.colorWriteMask =
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
		vk::ColorComponentFlagBits::eA;

	vk::PipelineColorBlendAttachmentState colorBlend3AttachmentState = {};
	colorBlend3AttachmentState.blendEnable = VK_FALSE;
	colorBlend3AttachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	colorBlend3AttachmentState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	colorBlend3AttachmentState.colorBlendOp = vk::BlendOp::eAdd;
	colorBlend3AttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
	colorBlend3AttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	colorBlend3AttachmentState.alphaBlendOp = vk::BlendOp::eAdd;
	colorBlend3AttachmentState.colorWriteMask =
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
		vk::ColorComponentFlagBits::eA;

	std::vector<vk::PipelineColorBlendAttachmentState> attachmentstates = {
		colorBlendAttachmentState,
		colorBlend2AttachmentState,
		colorBlend3AttachmentState
	};

	// Note: all attachments must have the same values unless a device feature is enabled
	vk::PipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
	colorBlendCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendCreateInfo.logicOp = vk::LogicOp::eCopy;
	colorBlendCreateInfo.attachmentCount = attachmentstates.size();
	colorBlendCreateInfo.pAttachments = attachmentstates.data();
	colorBlendCreateInfo.blendConstants[0] = 0.0f;
	colorBlendCreateInfo.blendConstants[1] = 0.0f;
	colorBlendCreateInfo.blendConstants[2] = 0.0f;
	colorBlendCreateInfo.blendConstants[3] = 0.0f;

	if (full)
	{
		std::vector<vk::DescriptorSetLayoutBinding> bindings;
		bindings.emplace_back(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(2, vk::DescriptorType::eCombinedImageSampler, textures->size(), vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(3, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(4, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(5, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(6, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(7, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(8, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(9, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(10, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(11, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(12, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(13, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(14, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(15, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(16, vk::DescriptorType::eAccelerationStructureKHR, 1, vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(17, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(18, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(19, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(20, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(21, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(22, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(23, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
		bindings.emplace_back(24, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment);
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
	pipelineCreateInfo.renderPass = app->deferredRenderPass;
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex = -1;

	pipeline = app->device.createGraphicsPipeline(nullptr, pipelineCreateInfo).value;
	shaderStages[1].module = fragmentShaderModulePSF;
	pipelinePSF = app->device.createGraphicsPipeline(nullptr, pipelineCreateInfo).value;

	app->device.destroyShaderModule(vertexShaderModule);
	app->device.destroyShaderModule(fragmentShaderModule);
	app->device.destroyShaderModule(fragmentShaderModulePSF);
}

void DefaultDeferredGraphicsPipeline::createDescriptorPool(AppResources* app)
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

void DefaultDeferredGraphicsPipeline::createDescriptorSet(AppResources* app)
{
	// There needs to be one descriptor set per binding point in the shader
	vk::DescriptorSetAllocateInfo allocInfo = {};
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	descriptorSet = app->device.allocateDescriptorSets(allocInfo)[0];

	std::vector<vk::WriteDescriptorSet> wds(25);

	// uniform buffer
	size_t wdsIdx = 0;
	vk::DescriptorBufferInfo descriptorBufferInfo = {};
	descriptorBufferInfo.buffer = uniformBuffer->buffer.buf;
	descriptorBufferInfo.offset = 0;
	descriptorBufferInfo.range = VK_WHOLE_SIZE;
	wds[wdsIdx].dstSet = descriptorSet;
	wds[wdsIdx].descriptorCount = 1;
	wds[wdsIdx].descriptorType = vk::DescriptorType::eUniformBuffer;
	wds[wdsIdx].pBufferInfo = &descriptorBufferInfo;
	wds[wdsIdx].dstBinding = wdsIdx;
	wdsIdx++;

	vk::DescriptorBufferInfo materialDescriptorBufferInfo = {};
	materialDescriptorBufferInfo.buffer = materialBuffer->buf;
	materialDescriptorBufferInfo.offset = 0;
	materialDescriptorBufferInfo.range = VK_WHOLE_SIZE;
	wds[wdsIdx].dstSet = descriptorSet;
	wds[wdsIdx].descriptorCount = 1;
	wds[wdsIdx].descriptorType = vk::DescriptorType::eStorageBuffer;
	wds[wdsIdx].pBufferInfo = &materialDescriptorBufferInfo;
	wds[wdsIdx].dstBinding = wdsIdx;
	wdsIdx++;

	std::vector<vk::DescriptorImageInfo> descriptorImageInfos(textures->size());
	for (size_t i = 0; i < textures->size(); ++i)
	{
		descriptorImageInfos[i].imageView = textures->at(i).imageView;
		descriptorImageInfos[i].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		descriptorImageInfos[i].sampler = textures->at(i).sampler;
	}

	wds[wdsIdx].dstSet = descriptorSet;
	wds[wdsIdx].descriptorCount = descriptorImageInfos.size();;
	wds[wdsIdx].descriptorType = vk::DescriptorType::eCombinedImageSampler;
	wds[wdsIdx].pImageInfo = descriptorImageInfos.data();
	wds[wdsIdx].dstBinding = wdsIdx;
	wdsIdx++;

	vk::DescriptorImageInfo colorDescriptorImageInfo;
	colorDescriptorImageInfo.imageView = app->color.imageView;
	colorDescriptorImageInfo.imageLayout = vk::ImageLayout::eGeneral;
	colorDescriptorImageInfo.sampler = app->color.sampler;
	wds[wdsIdx].dstSet = descriptorSet;
	wds[wdsIdx].descriptorCount = 1;
	wds[wdsIdx].descriptorType = vk::DescriptorType::eCombinedImageSampler;
	wds[wdsIdx].pImageInfo = &colorDescriptorImageInfo;
	wds[wdsIdx].dstBinding = wdsIdx;
	wdsIdx++;

	vk::DescriptorImageInfo positionDescriptorImageInfo;
	positionDescriptorImageInfo.imageView = app->position.imageView;
	positionDescriptorImageInfo.imageLayout = vk::ImageLayout::eGeneral;
	positionDescriptorImageInfo.sampler = app->position.sampler;
	wds[wdsIdx].dstSet = descriptorSet;
	wds[wdsIdx].descriptorCount = 1;
	wds[wdsIdx].descriptorType = vk::DescriptorType::eCombinedImageSampler;
	wds[wdsIdx].pImageInfo = &positionDescriptorImageInfo;
	wds[wdsIdx].dstBinding = wdsIdx;
	wdsIdx++;

	vk::DescriptorImageInfo normalDescriptorImageInfo;
	normalDescriptorImageInfo.imageView = app->normal.imageView;
	normalDescriptorImageInfo.imageLayout = vk::ImageLayout::eGeneral;
	normalDescriptorImageInfo.sampler = app->normal.sampler;
	wds[wdsIdx].dstSet = descriptorSet;
	wds[wdsIdx].descriptorCount = 1;
	wds[wdsIdx].descriptorType = vk::DescriptorType::eCombinedImageSampler;
	wds[wdsIdx].pImageInfo = &normalDescriptorImageInfo;
	wds[wdsIdx].dstBinding = wdsIdx;
	wdsIdx++;

	vk::DescriptorImageInfo tangentDescriptorImageInfo;
	tangentDescriptorImageInfo.imageView = app->tangent.imageView;
	tangentDescriptorImageInfo.imageLayout = vk::ImageLayout::eGeneral;
	tangentDescriptorImageInfo.sampler = app->tangent.sampler;
	wds[wdsIdx].dstSet = descriptorSet;
	wds[wdsIdx].descriptorCount = 1;
	wds[wdsIdx].descriptorType = vk::DescriptorType::eCombinedImageSampler;
	wds[wdsIdx].pImageInfo = &tangentDescriptorImageInfo;
	wds[wdsIdx].dstBinding = wdsIdx;
	wdsIdx++;

	vk::DescriptorImageInfo bitangentDescriptorImageInfo;
	bitangentDescriptorImageInfo.imageView = app->bitangent.imageView;
	bitangentDescriptorImageInfo.imageLayout = vk::ImageLayout::eGeneral;
	bitangentDescriptorImageInfo.sampler = app->bitangent.sampler;
	wds[wdsIdx].dstSet = descriptorSet;
	wds[wdsIdx].descriptorCount = 1;
	wds[wdsIdx].descriptorType = vk::DescriptorType::eCombinedImageSampler;
	wds[wdsIdx].pImageInfo = &bitangentDescriptorImageInfo;
	wds[wdsIdx].dstBinding = wdsIdx;
	wdsIdx++;

	vk::DescriptorImageInfo depthDescriptorImageInfo;
	depthDescriptorImageInfo.imageView = app->depth.imageView;
	depthDescriptorImageInfo.imageLayout = vk::ImageLayout::eGeneral;
	depthDescriptorImageInfo.sampler = app->depth.sampler;
	wds[wdsIdx].dstSet = descriptorSet;
	wds[wdsIdx].descriptorCount = 1;
	wds[wdsIdx].descriptorType = vk::DescriptorType::eCombinedImageSampler;
	wds[wdsIdx].pImageInfo = &depthDescriptorImageInfo;
	wds[wdsIdx].dstBinding = wdsIdx;
	wdsIdx++;

	vk::DescriptorImageInfo skyboxImageInfo = {};
	skyboxImageInfo.imageView = renderedCubemap->imageView;
	skyboxImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	skyboxImageInfo.sampler = renderedCubemap->sampler;
	wds[wdsIdx].dstSet = descriptorSet;
	wds[wdsIdx].descriptorCount = 1;
	wds[wdsIdx].descriptorType = vk::DescriptorType::eCombinedImageSampler;
	wds[wdsIdx].pImageInfo = &skyboxImageInfo;
	wds[wdsIdx].dstBinding = wdsIdx;
	wdsIdx++;

	vk::DescriptorBufferInfo lightDescriptorBufferInfo = {};
	lightDescriptorBufferInfo.buffer = lightBuffer->buf;
	lightDescriptorBufferInfo.offset = 0;
	lightDescriptorBufferInfo.range = VK_WHOLE_SIZE;
	wds[wdsIdx].dstSet = descriptorSet;
	wds[wdsIdx].descriptorCount = 1;
	wds[wdsIdx].descriptorType = vk::DescriptorType::eStorageBuffer;
	wds[wdsIdx].pBufferInfo = &lightDescriptorBufferInfo;
	wds[wdsIdx].dstBinding = wdsIdx;
	wdsIdx++;

	vk::DescriptorBufferInfo vertexDescriptorBufferInfo = {};
	vertexDescriptorBufferInfo.buffer = vertexBuffer->buf;
	vertexDescriptorBufferInfo.offset = 0;
	vertexDescriptorBufferInfo.range = VK_WHOLE_SIZE;
	wds[wdsIdx].dstSet = descriptorSet;
	wds[wdsIdx].descriptorCount = 1;
	wds[wdsIdx].descriptorType = vk::DescriptorType::eStorageBuffer;
	wds[wdsIdx].pBufferInfo = &vertexDescriptorBufferInfo;
	wds[wdsIdx].dstBinding = wdsIdx;
	wdsIdx++;

	vk::DescriptorBufferInfo indexDescriptorBufferInfo = {};
	indexDescriptorBufferInfo.buffer = indexBuffer->buf;
	indexDescriptorBufferInfo.offset = 0;
	indexDescriptorBufferInfo.range = VK_WHOLE_SIZE;
	wds[wdsIdx].dstSet = descriptorSet;
	wds[wdsIdx].descriptorCount = 1;
	wds[wdsIdx].descriptorType = vk::DescriptorType::eStorageBuffer;
	wds[wdsIdx].pBufferInfo = &indexDescriptorBufferInfo;
	wds[wdsIdx].dstBinding = wdsIdx;
	wdsIdx++;

	vk::DescriptorBufferInfo offsetDescriptorBufferInfo = {};
	offsetDescriptorBufferInfo.buffer = meshOffsetBuffer->buf;
	offsetDescriptorBufferInfo.offset = 0;
	offsetDescriptorBufferInfo.range = VK_WHOLE_SIZE;
	wds[wdsIdx].dstSet = descriptorSet;
	wds[wdsIdx].descriptorCount = 1;
	wds[wdsIdx].descriptorType = vk::DescriptorType::eStorageBuffer;
	wds[wdsIdx].pBufferInfo = &offsetDescriptorBufferInfo;
	wds[wdsIdx].dstBinding = wdsIdx;
	wdsIdx++;

	vk::DescriptorBufferInfo transformDescriptorBufferInfo = {};
	transformDescriptorBufferInfo.buffer = transformBuffer->buf;
	transformDescriptorBufferInfo.offset = 0;
	transformDescriptorBufferInfo.range = VK_WHOLE_SIZE;
	wds[wdsIdx].dstSet = descriptorSet;
	wds[wdsIdx].descriptorCount = 1;
	wds[wdsIdx].descriptorType = vk::DescriptorType::eStorageBuffer;
	wds[wdsIdx].pBufferInfo = &transformDescriptorBufferInfo;
	wds[wdsIdx].dstBinding = wdsIdx;
	wdsIdx++;

	vk::DescriptorBufferInfo normalTransformDescriptorBufferInfo = {};
	normalTransformDescriptorBufferInfo.buffer = normalTransformBuffer->buf;
	normalTransformDescriptorBufferInfo.offset = 0;
	normalTransformDescriptorBufferInfo.range = VK_WHOLE_SIZE;
	wds[wdsIdx].dstSet = descriptorSet;
	wds[wdsIdx].descriptorCount = 1;
	wds[wdsIdx].descriptorType = vk::DescriptorType::eStorageBuffer;
	wds[wdsIdx].pBufferInfo = &normalTransformDescriptorBufferInfo;
	wds[wdsIdx].dstBinding = wdsIdx;
	wdsIdx++;

	vk::WriteDescriptorSetAccelerationStructureKHR asWriteDescriptor = {};
	asWriteDescriptor.accelerationStructureCount = 1;
	asWriteDescriptor.pAccelerationStructures = &accelerationStructure->topLevelAS;
	wds[wdsIdx].pNext = &asWriteDescriptor;
	wds[wdsIdx].dstSet = descriptorSet;
	wds[wdsIdx].descriptorCount = 1;
	wds[wdsIdx].descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
	wds[wdsIdx].dstBinding = wdsIdx;
	wdsIdx++;

	vk::DescriptorBufferInfo drawIndirectLinesDescriptorBufferInfo = {};
	drawIndirectLinesDescriptorBufferInfo.buffer = drawIndirectLinesBuffer->buf;
	drawIndirectLinesDescriptorBufferInfo.offset = 0;
	drawIndirectLinesDescriptorBufferInfo.range = VK_WHOLE_SIZE;
	wds[wdsIdx].dstSet = descriptorSet;
	wds[wdsIdx].descriptorCount = 1;
	wds[wdsIdx].descriptorType = vk::DescriptorType::eStorageBuffer;
	wds[wdsIdx].pBufferInfo = &drawIndirectLinesDescriptorBufferInfo;
	wds[wdsIdx].dstBinding = wdsIdx;
	wdsIdx++;

	vk::DescriptorBufferInfo linesDescriptorBufferInfo = {};
	linesDescriptorBufferInfo.buffer = linesBuffer->buf;
	linesDescriptorBufferInfo.offset = 0;
	linesDescriptorBufferInfo.range = VK_WHOLE_SIZE;
	wds[wdsIdx].dstSet = descriptorSet;
	wds[wdsIdx].descriptorCount = 1;
	wds[wdsIdx].descriptorType = vk::DescriptorType::eStorageBuffer;
	wds[wdsIdx].pBufferInfo = &linesDescriptorBufferInfo;
	wds[wdsIdx].dstBinding = wdsIdx;
	wdsIdx++;

	vk::DescriptorBufferInfo hashMapDescriptorBufferInfo = {};
	hashMapDescriptorBufferInfo.buffer = hashMapBuffer->buf;
	hashMapDescriptorBufferInfo.offset = 0;
	hashMapDescriptorBufferInfo.range = VK_WHOLE_SIZE;
	wds[wdsIdx].dstSet = descriptorSet;
	wds[wdsIdx].descriptorCount = 1;
	wds[wdsIdx].descriptorType = vk::DescriptorType::eStorageBuffer;
	wds[wdsIdx].pBufferInfo = &hashMapDescriptorBufferInfo;
	wds[wdsIdx].dstBinding = wdsIdx;
	wdsIdx++;

	vk::DescriptorBufferInfo hashMap2DescriptorBufferInfo = {};
	hashMap2DescriptorBufferInfo.buffer = hashMapBuffer2->buf;
	hashMap2DescriptorBufferInfo.offset = 0;
	hashMap2DescriptorBufferInfo.range = VK_WHOLE_SIZE;
	wds[wdsIdx].dstSet = descriptorSet;
	wds[wdsIdx].descriptorCount = 1;
	wds[wdsIdx].descriptorType = vk::DescriptorType::eStorageBuffer;
	wds[wdsIdx].pBufferInfo = &hashMap2DescriptorBufferInfo;
	wds[wdsIdx].dstBinding = wdsIdx;
	wdsIdx++;

	vk::DescriptorImageInfo brightnessDescriptorImageInfo;
	brightnessDescriptorImageInfo.imageView = brightness->imageView;
	brightnessDescriptorImageInfo.imageLayout = vk::ImageLayout::eGeneral;
	brightnessDescriptorImageInfo.sampler = brightness->sampler;
	wds[wdsIdx].dstSet = descriptorSet;
	wds[wdsIdx].descriptorCount = 1;
	wds[wdsIdx].descriptorType = vk::DescriptorType::eCombinedImageSampler;
	wds[wdsIdx].pImageInfo = &brightnessDescriptorImageInfo;
	wds[wdsIdx].dstBinding = wdsIdx;
	wdsIdx++;

	vk::DescriptorBufferInfo varianceInterpolationDescriptorBufferInfo = {};
	varianceInterpolationDescriptorBufferInfo.buffer = varianceInterpolationBuffer->buf;
	varianceInterpolationDescriptorBufferInfo.offset = 0;
	varianceInterpolationDescriptorBufferInfo.range = VK_WHOLE_SIZE;
	wds[wdsIdx].dstSet = descriptorSet;
	wds[wdsIdx].descriptorCount = 1;
	wds[wdsIdx].descriptorType = vk::DescriptorType::eStorageBuffer;
	wds[wdsIdx].pBufferInfo = &varianceInterpolationDescriptorBufferInfo;
	wds[wdsIdx].dstBinding = wdsIdx;
	wdsIdx++;

	vk::DescriptorImageInfo outputMaskDescriptorImageInfo;
	outputMaskDescriptorImageInfo.imageView = app->outputMask.imageView;
	outputMaskDescriptorImageInfo.imageLayout = vk::ImageLayout::eGeneral;
	outputMaskDescriptorImageInfo.sampler = app->outputMask.sampler;
	wds[wdsIdx].dstSet = descriptorSet;
	wds[wdsIdx].descriptorCount = 1;
	wds[wdsIdx].descriptorType = vk::DescriptorType::eCombinedImageSampler;
	wds[wdsIdx].pImageInfo = &outputMaskDescriptorImageInfo;
	wds[wdsIdx].dstBinding = wdsIdx;
	wdsIdx++;

	vk::DescriptorBufferInfo averagePathDepthDescriptorBufferInfo = {};
	averagePathDepthDescriptorBufferInfo.buffer = averagePathDepthBuffer->buf;
	averagePathDepthDescriptorBufferInfo.offset = 0;
	averagePathDepthDescriptorBufferInfo.range = VK_WHOLE_SIZE;
	wds[wdsIdx].dstSet = descriptorSet;
	wds[wdsIdx].descriptorCount = 1;
	wds[wdsIdx].descriptorType = vk::DescriptorType::eStorageBuffer;
	wds[wdsIdx].pBufferInfo = &averagePathDepthDescriptorBufferInfo;
	wds[wdsIdx].dstBinding = wdsIdx;
	wdsIdx++;

	app->device.updateDescriptorSets(wds.size(), wds.data(), 0U, nullptr);
}

std::vector<float> DefaultDeferredGraphicsPipeline::getUniformBufferData(UniformBuffer& ub)
{
	std::vector<float> v;
	v.insert(v.end(), glm::value_ptr(ub.bufferData.VP), glm::value_ptr(ub.bufferData.VP) + 20);
	return v;
}
