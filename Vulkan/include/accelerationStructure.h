#pragma once

#define VK_ENABLE_BETA_EXTENSIONS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include "meshMerger.h"

class AppResources;

class AccelerationStructure
{
public:
	AccelerationStructure();
	AccelerationStructure(AppResources* _app, MeshMerger<Vertex>* meshMerger, std::vector<glm::mat4>& transforms);
	~AccelerationStructure();
	void cleanup();
	void rebuildAccelerationStructure(MeshMerger<Vertex>* meshMerger, std::vector<glm::mat4>& transforms, vk::CommandBuffer* cb);

	std::vector<vk::AccelerationStructureKHR> bottomLevelASs;
	vk::AccelerationStructureKHR topLevelAS;
private:
	void generateBottomLevelAS(MeshMerger<Vertex>* meshMerger, vk::CommandBuffer* cb);
	void generateTopLevelAS(MeshMerger<Vertex>* meshMerger, std::vector<glm::mat4>& transforms, vk::CommandBuffer* cb);
	AppResources* app = nullptr;
	std::vector<vk::DeviceAddress> bottomLevelASHandles;
	vk::DeviceAddress topLevelASHandle;
	std::vector<Buffer> bottomLevelASBuffers;
	Buffer topLevelASBuffer;
	std::vector<Buffer> scratchBuffers1;
	Buffer scratchBuffer2;
	Buffer instanceBuffer;
};
