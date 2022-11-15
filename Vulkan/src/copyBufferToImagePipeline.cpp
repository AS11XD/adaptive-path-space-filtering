#include "CopyBufferToImagePipeline.h"
#include "initialization.h"
#include "window.h"

CopyBufferToImagePipeline::CopyBufferToImagePipeline()
{
	wx = 16;
	wy = 16;
	wz = 1;
	x = 1;
	y = 1;
	z = 1;
}

CopyBufferToImagePipeline::~CopyBufferToImagePipeline()
{
}

void CopyBufferToImagePipeline::setImage(SampledTexture* _image, vk::Extent2D& extent)
{
	pushConstants.width = extent.width;
	pushConstants.height = extent.height;
	x = extent.width;
	y = extent.height;
	image = _image;
}

void CopyBufferToImagePipeline::setBuffer(Buffer* _buffer)
{
	buffer = _buffer;
}

void CopyBufferToImagePipeline::cleanUpPipeline(AppResources* app, bool full)
{
	app->device.destroyPipelineLayout(pipelineLayout, nullptr);
	app->device.destroyPipeline(pipeline);

	if (full)
	{
		app->device.destroyDescriptorSetLayout(descriptorSetLayout);
		app->device.destroyDescriptorPool(descriptorPool);
	}
}

void CopyBufferToImagePipeline::updateDescriptorSets(AppResources* app)
{
	pushConstants.exposure = app->windowObj->gui_control.var.exposure;
	pushConstants.toneMapping = app->windowObj->gui_control.var.toneMapping;

	std::vector<vk::WriteDescriptorSet> wds(2);

	// uniform buffer
	vk::DescriptorBufferInfo descriptorBuffer0Info = {};
	descriptorBuffer0Info.buffer = buffer->buf;
	descriptorBuffer0Info.offset = 0;
	descriptorBuffer0Info.range = VK_WHOLE_SIZE;
	wds[0].dstSet = descriptorSet;
	wds[0].descriptorCount = 1;
	wds[0].descriptorType = vk::DescriptorType::eStorageBuffer;
	wds[0].pBufferInfo = &descriptorBuffer0Info;
	wds[0].dstBinding = 0;

	vk::DescriptorImageInfo descriptorImage0Info = {};
	descriptorImage0Info.imageView = image->imageView;
	descriptorImage0Info.imageLayout = vk::ImageLayout::eGeneral;
	wds[1].dstSet = descriptorSet;
	wds[1].descriptorCount = 1;
	wds[1].descriptorType = vk::DescriptorType::eStorageImage;
	wds[1].pImageInfo = &descriptorImage0Info;
	wds[1].dstBinding = 1;

	app->device.updateDescriptorSets(wds.size(), wds.data(), 0U, nullptr);
}

void CopyBufferToImagePipeline::bindPushConstants(vk::CommandBuffer* cb)
{
	cb->pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(PushStruct), &pushConstants);
}

void CopyBufferToImagePipeline::createPipeline(AppResources* app, bool full)
{
	if (full)
	{
		std::vector<vk::DescriptorSetLayoutBinding> bindings;
		bindings.emplace_back(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute);
		bindings.emplace_back(1, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute);

		vk::DescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = {};
		descriptorLayoutCreateInfo.bindingCount = bindings.size();
		descriptorLayoutCreateInfo.pBindings = bindings.data();

		descriptorSetLayout = app->device.createDescriptorSetLayout(descriptorLayoutCreateInfo);
	}

	vk::PushConstantRange pcr(vk::ShaderStageFlagBits::eCompute, 0, sizeof(PushStruct));
	vk::PipelineLayoutCreateInfo layoutCreateInfo(vk::PipelineLayoutCreateFlags(), 1U, &descriptorSetLayout, 1U,
												  &pcr);

	pipelineLayout = app->device.createPipelineLayout(layoutCreateInfo);
	vk::ShaderModule computeShaderModule;
	createShaderModule(app->device, "copybuffertoimage.comp", computeShaderModule);
	// set workgroupsize
	auto specEntries = std::array<vk::SpecializationMapEntry, 3>{
		vk::SpecializationMapEntry{ 0U, 0U, sizeof(uint32_t) },
			vk::SpecializationMapEntry{ 1U, sizeof(uint32_t), sizeof(uint32_t) },
			vk::SpecializationMapEntry{ 2U, 2 * sizeof(uint32_t), sizeof(uint32_t) }
	};
	std::array<int, 3> specValues = { wx, wy, wz };
	vk::SpecializationInfo specInfo = vk::SpecializationInfo(CAST(specEntries), specEntries.data(),
															 CAST(specValues) * sizeof(int), specValues.data());
	vk::PipelineShaderStageCreateInfo computeStageInfo(vk::PipelineShaderStageCreateFlags(),
													   vk::ShaderStageFlagBits::eCompute,
													   computeShaderModule, "main", &specInfo);
	vk::ComputePipelineCreateInfo computeInfo(vk::PipelineCreateFlags(), computeStageInfo, pipelineLayout);
	pipeline = app->device.createComputePipeline(nullptr, computeInfo, nullptr).value;
	app->device.destroyShaderModule(computeShaderModule);
}

void CopyBufferToImagePipeline::createDescriptorPool(AppResources* app)
{
	vk::DescriptorPoolSize typeCount;
	typeCount.descriptorCount = 1;

	vk::DescriptorPoolCreateInfo createInfo = {};
	createInfo.poolSizeCount = 1;
	createInfo.pPoolSizes = &typeCount;
	createInfo.maxSets = 1;

	descriptorPool = app->device.createDescriptorPool(createInfo);
}

void CopyBufferToImagePipeline::createDescriptorSet(AppResources* app)
{
	// There needs to be one descriptor set per binding point in the shader
	vk::DescriptorSetAllocateInfo allocInfo = {};
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	descriptorSet = app->device.allocateDescriptorSets(allocInfo)[0];
}
