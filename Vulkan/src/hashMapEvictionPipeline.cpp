#include "hashMapEvictionPipeline.h"
#include "initialization.h"
#include "window.h"

HashMapEvictionPipeline::HashMapEvictionPipeline()
{
	wx = 128;
	wy = 1;
	wz = 1;
	x = 1;
	y = 1;
	z = 1;
}

HashMapEvictionPipeline::~HashMapEvictionPipeline()
{
}

void HashMapEvictionPipeline::setHashMaps(Buffer* _hashMap, Buffer* _hashMap2, uint32_t _maxHashMapSize)
{
	hashMap = _hashMap;
	hashMap2 = _hashMap2;
	maxHashMapSize = _maxHashMapSize;
}

void HashMapEvictionPipeline::setHashMapOccupation(Buffer* _hashMapOccupation)
{
	hashMapOccupation = _hashMapOccupation;
}

void HashMapEvictionPipeline::cleanUpPipeline(AppResources* app, bool full)
{
	app->device.destroyPipelineLayout(pipelineLayout, nullptr);
	app->device.destroyPipeline(pipeline);

	if (full)
	{
		app->device.destroyDescriptorSetLayout(descriptorSetLayout);
		app->device.destroyDescriptorPool(descriptorPool);
	}
}

void HashMapEvictionPipeline::updateDescriptorSets(AppResources* app)
{
	pushConstants.hashMapSize = app->windowObj->gui_control.calculateHashMapSize(app, maxHashMapSize);
	pushConstants.frameCount = app->frameCount;
	pushConstants.evictionTime = app->windowObj->gui_control.var.evictionTime;
	pushConstants.maxSamples = app->windowObj->gui_control.var.maxSamples;
	pushConstants.calcHashMapOccupation = (app->windowObj->gui_control.var.currentPsfMode == PSFM_HASH_MAP_OCCUPATION);
	x = pushConstants.hashMapSize;

	std::vector<vk::WriteDescriptorSet> wds(3);

	vk::DescriptorBufferInfo descriptorBuffer0Info = {};
	descriptorBuffer0Info.buffer = hashMap->buf;
	descriptorBuffer0Info.offset = 0;
	descriptorBuffer0Info.range = VK_WHOLE_SIZE;
	wds[0].dstSet = descriptorSet;
	wds[0].descriptorCount = 1;
	wds[0].descriptorType = vk::DescriptorType::eStorageBuffer;
	wds[0].pBufferInfo = &descriptorBuffer0Info;
	wds[0].dstBinding = 0;

	vk::DescriptorBufferInfo descriptorBuffer1Info = {};
	descriptorBuffer1Info.buffer = hashMap2->buf;
	descriptorBuffer1Info.offset = 0;
	descriptorBuffer1Info.range = VK_WHOLE_SIZE;
	wds[1].dstSet = descriptorSet;
	wds[1].descriptorCount = 1;
	wds[1].descriptorType = vk::DescriptorType::eStorageBuffer;
	wds[1].pBufferInfo = &descriptorBuffer1Info;
	wds[1].dstBinding = 1;

	vk::DescriptorBufferInfo descriptorBuffer2Info = {};
	descriptorBuffer2Info.buffer = hashMapOccupation->buf;
	descriptorBuffer2Info.offset = 0;
	descriptorBuffer2Info.range = VK_WHOLE_SIZE;
	wds[2].dstSet = descriptorSet;
	wds[2].descriptorCount = 1;
	wds[2].descriptorType = vk::DescriptorType::eStorageBuffer;
	wds[2].pBufferInfo = &descriptorBuffer2Info;
	wds[2].dstBinding = 2;

	app->device.updateDescriptorSets(wds.size(), wds.data(), 0U, nullptr);
}

void HashMapEvictionPipeline::bindPushConstants(vk::CommandBuffer* cb)
{
	cb->pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(PushStruct), &pushConstants);
}

void HashMapEvictionPipeline::createPipeline(AppResources* app, bool full)
{
	if (full)
	{
		std::vector<vk::DescriptorSetLayoutBinding> bindings;
		bindings.emplace_back(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute);
		bindings.emplace_back(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute);
		bindings.emplace_back(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute);

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
	createShaderModule(app->device, "hashmapeviction.comp", computeShaderModule);
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

void HashMapEvictionPipeline::createDescriptorPool(AppResources* app)
{
	vk::DescriptorPoolSize typeCount;
	typeCount.descriptorCount = 1;

	vk::DescriptorPoolCreateInfo createInfo = {};
	createInfo.poolSizeCount = 1;
	createInfo.pPoolSizes = &typeCount;
	createInfo.maxSets = 1;

	descriptorPool = app->device.createDescriptorPool(createInfo);
}

void HashMapEvictionPipeline::createDescriptorSet(AppResources* app)
{
	// There needs to be one descriptor set per binding point in the shader
	vk::DescriptorSetAllocateInfo allocInfo = {};
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptorSetLayout;

	descriptorSet = app->device.allocateDescriptorSets(allocInfo)[0];
}
