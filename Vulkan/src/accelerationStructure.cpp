#include "accelerationStructure.h"
#include "initialization.h"
#include "utils.h"

AccelerationStructure::AccelerationStructure()
{
}

AccelerationStructure::AccelerationStructure(AppResources* _app, MeshMerger<Vertex>* meshMerger, std::vector<glm::mat4>& transforms) : app(_app)
{
	generateBottomLevelAS(meshMerger, nullptr);
	generateTopLevelAS(meshMerger, transforms, nullptr);
}

AccelerationStructure::~AccelerationStructure()
{
}

void AccelerationStructure::cleanup()
{
	if (app)
	{
		for (auto& bottomLevelAS : bottomLevelASs)
		{
			app->device.destroyAccelerationStructureKHR(bottomLevelAS);
		}
		app->device.destroyAccelerationStructureKHR(topLevelAS);

		for (auto& bottomLevelASBuffer : bottomLevelASBuffers)
		{
			app->device.destroyBuffer(bottomLevelASBuffer.buf);
			app->device.freeMemory(bottomLevelASBuffer.mem);
		}
		app->device.destroyBuffer(topLevelASBuffer.buf);
		app->device.freeMemory(topLevelASBuffer.mem);
		for (auto& scratchBuffer1 : scratchBuffers1)
		{
			app->device.destroyBuffer(scratchBuffer1.buf);
			app->device.freeMemory(scratchBuffer1.mem);
		}
		app->device.destroyBuffer(scratchBuffer2.buf);
		app->device.freeMemory(scratchBuffer2.mem);
		app->device.destroyBuffer(instanceBuffer.buf);
		app->device.freeMemory(instanceBuffer.mem);
	}
}

void AccelerationStructure::rebuildAccelerationStructure(MeshMerger<Vertex>* meshMerger, std::vector<glm::mat4>& transforms, vk::CommandBuffer* cb)
{
	//generateBottomLevelAS(meshMerger, cb);
	//
	//cb->pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
	//										  vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR, vk::DependencyFlags(),
	//										  { vk::MemoryBarrier(vk::AccessFlagBits::eAccelerationStructureWriteKHR,
	//															 vk::AccessFlagBits::eAccelerationStructureReadKHR) }, {}, {});
	generateTopLevelAS(meshMerger, transforms, cb);
}

void AccelerationStructure::generateBottomLevelAS(MeshMerger<Vertex>* meshMerger, vk::CommandBuffer* cb)
{
	if (!cb)
	{
		bottomLevelASs.resize(meshMerger->mergedMeshes.size());
		bottomLevelASHandles.resize(meshMerger->mergedMeshes.size());
		bottomLevelASBuffers.resize(meshMerger->mergedMeshes.size());
		scratchBuffers1.resize(meshMerger->mergedMeshes.size());
	}

	vk::BufferDeviceAddressInfoKHR vertexBufferDeviceInfo;
	vertexBufferDeviceInfo.buffer = meshMerger->vertexBuffer.buf;
	vk::DeviceAddress vertexBufferAdress = app->device.getBufferAddressKHR(vertexBufferDeviceInfo);

	vk::BufferDeviceAddressInfoKHR indexBufferDeviceInfo;
	indexBufferDeviceInfo.buffer = meshMerger->indexBuffer.buf;
	vk::DeviceAddress indexBufferAdress = app->device.getBufferAddressKHR(indexBufferDeviceInfo);

	vk::AccelerationStructureGeometryTrianglesDataKHR asGtris;
	asGtris.vertexData = vertexBufferAdress;
	asGtris.indexData = indexBufferAdress;
	asGtris.vertexFormat = vk::Format::eR32G32B32Sfloat;
	asGtris.maxVertex = meshMerger->vertexCount;
	asGtris.vertexStride = sizeof(Vertex);
	asGtris.indexType = vk::IndexType::eUint32;

	vk::AccelerationStructureGeometryKHR asGeometry;
	asGeometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
	asGeometry.geometryType = vk::GeometryTypeKHR::eTriangles;
	asGeometry.geometry.triangles = asGtris;

	size_t i = 0;
	for (auto& m : meshMerger->mergedMeshes)
	{
		uint32_t primitiveCount = (m.endIdx - m.startIdx) / 3;
		if (!cb)
		{

			vk::AccelerationStructureBuildGeometryInfoKHR asBuildSizeGeometry;
			asBuildSizeGeometry.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
			asBuildSizeGeometry.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;
			asBuildSizeGeometry.geometryCount = 1;
			asBuildSizeGeometry.pGeometries = &asGeometry;

			vk::AccelerationStructureBuildSizesInfoKHR asBuildSizes = app->device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, asBuildSizeGeometry, primitiveCount);

			createBuffer(app->pDevice, app->device,
						 asBuildSizes.accelerationStructureSize,
						 vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
						 vk::MemoryPropertyFlagBits::eDeviceLocal,
						 "bottom-level-as-" + std::to_string(i), bottomLevelASBuffers[i].buf, bottomLevelASBuffers[i].mem);

			vk::AccelerationStructureCreateInfoKHR asCreate;
			asCreate.buffer = bottomLevelASBuffers[i].buf;
			asCreate.size = asBuildSizes.accelerationStructureSize;
			asCreate.type = vk::AccelerationStructureTypeKHR::eBottomLevel;

			bottomLevelASs[i] = app->device.createAccelerationStructureKHR(asCreate);

			createBuffer(app->pDevice, app->device,
						 asBuildSizes.buildScratchSize,
						 vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
						 vk::MemoryPropertyFlagBits::eDeviceLocal,
						 "as-scratch1-" + std::to_string(i), scratchBuffers1[i].buf, scratchBuffers1[i].mem);
		}

		vk::BufferDeviceAddressInfoKHR scratchBufferDeviceInfo;
		scratchBufferDeviceInfo.buffer = scratchBuffers1[i].buf;
		vk::DeviceAddress scratchBufferAdress = app->device.getBufferAddressKHR(scratchBufferDeviceInfo);

		vk::AccelerationStructureBuildGeometryInfoKHR asBuildGeometry;
		asBuildGeometry.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
		asBuildGeometry.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;
		if (!cb)
		{
			asBuildGeometry.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
		}
		else
		{
			asBuildGeometry.mode = vk::BuildAccelerationStructureModeKHR::eUpdate;
			asBuildGeometry.srcAccelerationStructure = bottomLevelASs[i];
		}
		asBuildGeometry.dstAccelerationStructure = bottomLevelASs[i];
		asBuildGeometry.geometryCount = 1;
		asBuildGeometry.pGeometries = &asGeometry;
		asBuildGeometry.scratchData = scratchBufferAdress;

		vk::AccelerationStructureBuildRangeInfoKHR asBuildRange;
		asBuildRange.primitiveCount = primitiveCount;
		asBuildRange.primitiveOffset = m.startIdx * sizeof(uint32_t);
		asBuildRange.firstVertex = 0;
		asBuildRange.transformOffset = 0;
		std::vector<vk::AccelerationStructureBuildRangeInfoKHR*> asBuildRanges = { &asBuildRange };
		if (!cb)
		{
			vk::CommandBuffer commandBuffer = beginSingleTimeCommands(app->device, app->graphicsCommandPool);

			commandBuffer.buildAccelerationStructuresKHR(1, &asBuildGeometry, asBuildRanges.data());

			endSingleTimeCommands(app->device, app->graphicsQueue, app->graphicsCommandPool, commandBuffer);

			vk::AccelerationStructureDeviceAddressInfoKHR asDeviceAddressInfo;
			asDeviceAddressInfo.accelerationStructure = bottomLevelASs[i];
			bottomLevelASHandles[i] = app->device.getAccelerationStructureAddressKHR(&asDeviceAddressInfo);
		}
		else
		{
			cb->buildAccelerationStructuresKHR(1, &asBuildGeometry, asBuildRanges.data());
		}
		i++;
	}
}

void AccelerationStructure::generateTopLevelAS(MeshMerger<Vertex>* meshMerger, std::vector<glm::mat4>& transforms, vk::CommandBuffer* cb)
{
	std::vector<vk::AccelerationStructureInstanceKHR> instances(bottomLevelASHandles.size());
	for (size_t i = 0; i < instances.size(); ++i)
	{
		glm::mat4 T = transforms[i + 1];

		vk::TransformMatrixKHR instanceTransform = std::array<std::array<float, 4>, 3>{
			T[0][0], T[1][0], T[2][0], T[3][0],
				T[0][1], T[1][1], T[2][1], T[3][1],
				T[0][2], T[1][2], T[2][2], T[3][2]
		};
		instances[i].transform = instanceTransform;
		instances[i].instanceCustomIndex = 0;
		instances[i].mask = 0xFF;
		instances[i].instanceShaderBindingTableRecordOffset = 0;
		instances[i].flags = (VkGeometryInstanceFlagsKHR)vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable;
		instances[i].accelerationStructureReference = bottomLevelASHandles[i];
	}

	if (!cb)
	{
		createBuffer(app->pDevice, app->device, sizeof(vk::AccelerationStructureInstanceKHR) * instances.size(),
					 vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst,
					 vk::MemoryPropertyFlagBits::eDeviceLocal,
					 "as-instances", instanceBuffer.buf, instanceBuffer.mem);

	}
	fillDeviceWithStagingBuffer(app->pDevice, app->device, app->graphicsCommandPool, app->graphicsQueue, instanceBuffer, instances);

	vk::BufferDeviceAddressInfoKHR instanceBufferDeviceInfo;
	instanceBufferDeviceInfo.buffer = instanceBuffer.buf;
	vk::DeviceAddress instanceBufferAdress = app->device.getBufferAddressKHR(instanceBufferDeviceInfo);

	vk::DeviceOrHostAddressConstKHR instanceDataDeviceAddress = {};
	instanceDataDeviceAddress.deviceAddress = instanceBufferAdress;

	vk::AccelerationStructureGeometryInstancesDataKHR asInstances;
	asInstances.arrayOfPointers = VK_FALSE;
	asInstances.data = instanceDataDeviceAddress;

	vk::AccelerationStructureGeometryKHR asGeometry = {};
	asGeometry.flags = vk::GeometryFlagBitsKHR::eOpaque;
	asGeometry.geometryType = vk::GeometryTypeKHR::eInstances;
	asGeometry.geometry.instances = asInstances;

	const uint32_t primitiveCount = instances.size();
	if (!cb)
	{
		vk::AccelerationStructureBuildGeometryInfoKHR asBuildSizeGeometry;
		asBuildSizeGeometry.type = vk::AccelerationStructureTypeKHR::eTopLevel;
		asBuildSizeGeometry.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;
		asBuildSizeGeometry.geometryCount = 1;
		asBuildSizeGeometry.pGeometries = &asGeometry;

		vk::AccelerationStructureBuildSizesInfoKHR asBuildSizes = app->device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, asBuildSizeGeometry, primitiveCount);

		createBuffer(app->pDevice, app->device,
					 asBuildSizes.accelerationStructureSize,
					 vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
					 vk::MemoryPropertyFlagBits::eDeviceLocal,
					 "top-level-as", topLevelASBuffer.buf, topLevelASBuffer.mem);
		vk::AccelerationStructureCreateInfoKHR asCreate;
		asCreate.buffer = topLevelASBuffer.buf;
		asCreate.size = asBuildSizes.accelerationStructureSize;
		asCreate.type = vk::AccelerationStructureTypeKHR::eTopLevel;

		topLevelAS = app->device.createAccelerationStructureKHR(asCreate);

		createBuffer(app->pDevice, app->device,
					 asBuildSizes.buildScratchSize,
					 vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
					 vk::MemoryPropertyFlagBits::eDeviceLocal,
					 "as-scratch2", scratchBuffer2.buf, scratchBuffer2.mem);
	}

	vk::BufferDeviceAddressInfoKHR scratchBufferDeviceInfo;
	scratchBufferDeviceInfo.buffer = scratchBuffer2.buf;
	vk::DeviceAddress scratchBufferAdress = app->device.getBufferAddressKHR(scratchBufferDeviceInfo);

	vk::AccelerationStructureBuildGeometryInfoKHR asBuildGeometry;
	asBuildGeometry.type = vk::AccelerationStructureTypeKHR::eTopLevel;
	asBuildGeometry.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;
	if (!cb)
	{
		asBuildGeometry.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
	}
	else
	{
		asBuildGeometry.mode = vk::BuildAccelerationStructureModeKHR::eUpdate;
		asBuildGeometry.srcAccelerationStructure = topLevelAS;
	}
	asBuildGeometry.dstAccelerationStructure = topLevelAS;
	asBuildGeometry.geometryCount = 1;
	asBuildGeometry.pGeometries = &asGeometry;
	asBuildGeometry.scratchData = scratchBufferAdress;

	vk::AccelerationStructureBuildRangeInfoKHR asBuildRange;
	asBuildRange.primitiveCount = primitiveCount;
	asBuildRange.primitiveOffset = 0;
	asBuildRange.firstVertex = 0;
	asBuildRange.transformOffset = 0;
	std::vector<vk::AccelerationStructureBuildRangeInfoKHR*> asBuildRanges = { &asBuildRange };

	if (!cb)
	{
		vk::CommandBuffer commandBuffer = beginSingleTimeCommands(app->device, app->graphicsCommandPool);

		commandBuffer.buildAccelerationStructuresKHR(1, &asBuildGeometry, asBuildRanges.data());

		endSingleTimeCommands(app->device, app->graphicsQueue, app->graphicsCommandPool, commandBuffer);
		vk::AccelerationStructureDeviceAddressInfoKHR asDeviceAddressInfo;
		asDeviceAddressInfo.accelerationStructure = topLevelAS;
		topLevelASHandle = app->device.getAccelerationStructureAddressKHR(&asDeviceAddressInfo);
	}
	else
	{
		cb->buildAccelerationStructuresKHR(1, &asBuildGeometry, asBuildRanges.data());
	}
}
