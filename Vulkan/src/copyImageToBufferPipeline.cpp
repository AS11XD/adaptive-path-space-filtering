#include "copyImageToBufferPipeline.h"
#include "initialization.h"

CopyImageToBufferPipeline::CopyImageToBufferPipeline()
{
	wx = 16;
	wy = 16;
	wz = 1;
	x = 1;
	y = 1;
	z = 1;
}

CopyImageToBufferPipeline::~CopyImageToBufferPipeline()
{
}

void CopyImageToBufferPipeline::setImages(std::array<SampledTexture*, 3>& _images, vk::Extent2D& extent)
{
	pushConstants.width = extent.width;
	pushConstants.height = extent.height;
	x = extent.width;
	y = extent.height;
	images[0] = _images[0];
	images[1] = _images[1];
	images[2] = _images[2];
}

void CopyImageToBufferPipeline::setBuffers(std::array<Buffer*, 3>& _buffers)
{
	buffers[0] = _buffers[0];
	buffers[1] = _buffers[1];
	buffers[2] = _buffers[2];
}

void CopyImageToBufferPipeline::cleanUpPipeline(AppResources* app, bool full)
{
	app->device.destroyPipelineLayout(pipelineLayout, nullptr);
	app->device.destroyPipeline(pipeline);

	if (full)
	{
		app->device.destroyDescriptorSetLayout(descriptorSetLayout);
		app->device.destroyDescriptorPool(descriptorPool);
	}
}

void CopyImageToBufferPipeline::updateDescriptorSets(AppResources* app)
{
	std::vector<vk::WriteDescriptorSet> wds(6);

	// uniform buffer

	vk::DescriptorImageInfo descriptorImage0Info = {};
	descriptorImage0Info.imageView = images[0]->imageView;
	descriptorImage0Info.imageLayout = vk::ImageLayout::eGeneral;
	wds[0].dstSet = descriptorSet;
	wds[0].descriptorCount = 1;
	wds[0].descriptorType = vk::DescriptorType::eStorageImage;
	wds[0].pImageInfo = &descriptorImage0Info;
	wds[0].dstBinding = 0;

	vk::DescriptorImageInfo descriptorImage1Info = {};
	descriptorImage1Info.imageView = images[1]->imageView;
	descriptorImage1Info.imageLayout = vk::ImageLayout::eGeneral;
	wds[1].dstSet = descriptorSet;
	wds[1].descriptorCount = 1;
	wds[1].descriptorType = vk::DescriptorType::eStorageImage;
	wds[1].pImageInfo = &descriptorImage1Info;
	wds[1].dstBinding = 1;

	vk::DescriptorImageInfo descriptorImage2Info = {};
	descriptorImage2Info.imageView = images[2]->imageView;
	descriptorImage2Info.imageLayout = vk::ImageLayout::eGeneral;
	wds[2].dstSet = descriptorSet;
	wds[2].descriptorCount = 1;
	wds[2].descriptorType = vk::DescriptorType::eStorageImage;
	wds[2].pImageInfo = &descriptorImage1Info;
	wds[2].dstBinding = 2;

	vk::DescriptorBufferInfo descriptorBuffer0Info = {};
	descriptorBuffer0Info.buffer = buffers[0]->buf;
	descriptorBuffer0Info.offset = 0;
	descriptorBuffer0Info.range = VK_WHOLE_SIZE;
	wds[3].dstSet = descriptorSet;
	wds[3].descriptorCount = 1;
	wds[3].descriptorType = vk::DescriptorType::eStorageBuffer;
	wds[3].pBufferInfo = &descriptorBuffer0Info;
	wds[3].dstBinding = 3;

	vk::DescriptorBufferInfo descriptorBuffer1Info = {};
	descriptorBuffer1Info.buffer = buffers[1]->buf;
	descriptorBuffer1Info.offset = 0;
	descriptorBuffer1Info.range = VK_WHOLE_SIZE;
	wds[4].dstSet = descriptorSet;
	wds[4].descriptorCount = 1;
	wds[4].descriptorType = vk::DescriptorType::eStorageBuffer;
	wds[4].pBufferInfo = &descriptorBuffer1Info;
	wds[4].dstBinding = 4;

	vk::DescriptorBufferInfo descriptorBuffer2Info = {};
	descriptorBuffer2Info.buffer = buffers[2]->buf;
	descriptorBuffer2Info.offset = 0;
	descriptorBuffer2Info.range = VK_WHOLE_SIZE;
	wds[5].dstSet = descriptorSet;
	wds[5].descriptorCount = 1;
	wds[5].descriptorType = vk::DescriptorType::eStorageBuffer;
	wds[5].pBufferInfo = &descriptorBuffer2Info;
	wds[5].dstBinding = 5;

	app->device.updateDescriptorSets(wds.size(), wds.data(), 0U, nullptr);
}

void CopyImageToBufferPipeline::bindPushConstants(vk::CommandBuffer* cb)
{
	cb->pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(PushStruct), &pushConstants);
}

void CopyImageToBufferPipeline::createPipeline(AppResources* app, bool full)
{
	if (full)
	{
		std::vector<vk::DescriptorSetLayoutBinding> bindings;
		bindings.emplace_back(0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute);
		bindings.emplace_back(1, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute);
		bindings.emplace_back(2, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute);
		bindings.emplace_back(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute);
		bindings.emplace_back(4, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute);
		bindings.emplace_back(5, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute);

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
	createShaderModule(app->device, "copyimagetobuffer.comp", computeShaderModule);
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

void CopyImageToBufferPipeline::createDescriptorPool(AppResources* app)
{
	vk::DescriptorPoolSize typeCount;
	typeCount.descriptorCount = 1;

	vk::DescriptorPoolCreateInfo createInfo = {};
	createInfo.poolSizeCount = 1;
	createInfo.pPoolSizes = &typeCount;
	createInfo.maxSets = 1;

	descriptorPool = app->device.createDescriptorPool(createInfo);
}

void CopyImageToBufferPipeline::createDescriptorSet(AppResources* app)
{
	// There needs to be one descriptor set per binding point in the shader
	vk::DescriptorSetAllocateInfo allocInfo = {};
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	descriptorSet = app->device.allocateDescriptorSets(allocInfo)[0];
}
