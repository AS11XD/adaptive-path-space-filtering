#pragma once
#include <vulkan/vulkan.hpp>
#include <utils.h>
#include <vector>
#include <glm/glm.hpp>
#include "mesh.h"
#include "initialization.h"
#include <iostream>

struct MergedMesh
{
	uint32_t startIdx = 0;
	uint32_t endIdx = 0;
};

template<typename T>
class MeshMerger
{
public:
	MeshMerger();
	MeshMerger(MeshMerger<T>&& other) noexcept;
	MeshMerger(AppResources& app, std::vector<Mesh<T>>& meshes);
	~MeshMerger();

	MeshMerger<T>& operator=(MeshMerger<T>&& other) noexcept;

	void cleanup(vk::Device& device);

	void draw(AppResources* app,
			  vk::CommandBuffer* cb);


	Buffer vertexBuffer;
	uint32_t vertexCount;
	Buffer indexBuffer;
	uint32_t indexCount;
	Buffer offsetBuffer;
	std::vector<MergedMesh> mergedMeshes;
};


template<typename T>
MeshMerger<T>::MeshMerger()
{
}

template<typename T>
inline MeshMerger<T>::MeshMerger(MeshMerger<T>&& other) noexcept
{
	vertexBuffer = other.vertexBuffer;
	vertexCount = other.vertexCount;
	indexBuffer = other.indexBuffer;
	indexCount = other.indexCount;
	offsetBuffer = other.offsetBuffer;
	mergedMeshes = std::move(other.mergedMeshes);
}

template<typename T>
MeshMerger<T>::MeshMerger(AppResources& app, std::vector<Mesh<T>>& meshes)
{
	std::vector<T> vertices;
	std::vector<uint32_t> indices;
	std::string name;
	mergedMeshes.resize(meshes.size());
	std::vector<uint32_t> meshOffset(meshes.size());
	uint32_t offset = 0;
	uint32_t meshIdx = 0;
	for (Mesh<T>& m : meshes)
	{
		name += m.name;

		mergedMeshes[meshIdx].startIdx = indices.size();
		meshOffset[meshIdx] = indices.size();
		std::vector<uint32_t> modelIndices(m.indices);
		for (uint32_t& idx : modelIndices)
		{
			idx += offset;
		}
		offset += m.vertices.size();
		vertices.insert(vertices.end(), m.vertices.begin(), m.vertices.end());
		indices.insert(indices.end(), modelIndices.begin(), modelIndices.end());
		mergedMeshes[meshIdx].endIdx = indices.size();
		meshIdx++;
	}

	createBuffer(app.pDevice, app.device, meshOffset.size() * sizeof(uint32_t), vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eDeviceLocal, "offset-buffer-" + name, offsetBuffer.buf, offsetBuffer.mem);
	fillDeviceWithStagingBuffer(app.pDevice, app.device, app.transferCommandPool, app.transferQueue, offsetBuffer, meshOffset);

	vertexCount = vertices.size();
	uint32_t verticesSize = (uint32_t)(vertices.size() * sizeof(vertices[0]));

	// Then allocate a gpu only buffer for vertices
	createBuffer(app.pDevice, app.device, verticesSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress, vk::MemoryPropertyFlagBits::eDeviceLocal, "vertex-buffer-" + name, vertexBuffer.buf, vertexBuffer.mem);
	fillDeviceWithStagingBuffer(app.pDevice, app.device, app.transferCommandPool, app.transferQueue, vertexBuffer, vertices);

	indexCount = indices.size();
	uint32_t indicesSize = (uint32_t)(indices.size() * sizeof(indices[0]));

	createBuffer(app.pDevice, app.device, indicesSize, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress, vk::MemoryPropertyFlagBits::eDeviceLocal, "index-buffer-" + name, indexBuffer.buf, indexBuffer.mem);
	fillDeviceWithStagingBuffer(app.pDevice, app.device, app.transferCommandPool, app.transferQueue, indexBuffer, indices);
}

template<typename T>
MeshMerger<T>::~MeshMerger()
{
}

template<typename T>
inline MeshMerger<T>& MeshMerger<T>::operator=(MeshMerger<T>&& other) noexcept
{
	if (this == &other)
		return *this;

	vertexBuffer = other.vertexBuffer;
	vertexCount = other.vertexCount;
	indexBuffer = other.indexBuffer;
	indexCount = other.indexCount;
	offsetBuffer = other.offsetBuffer;
	mergedMeshes = std::move(other.mergedMeshes);
	return *this;
}

template<typename T>
void MeshMerger<T>::cleanup(vk::Device& device)
{
	device.destroyBuffer(offsetBuffer.buf);
	device.freeMemory(offsetBuffer.mem);

	device.destroyBuffer(vertexBuffer.buf);
	device.freeMemory(vertexBuffer.mem);

	device.destroyBuffer(indexBuffer.buf);
	device.freeMemory(indexBuffer.mem);
}

template<typename T>
void MeshMerger<T>::draw(AppResources* app, vk::CommandBuffer* cb)
{
	vk::DeviceSize offset = 0;
	cb->bindVertexBuffers(0, 1, &vertexBuffer.buf, &offset);

	cb->bindIndexBuffer(indexBuffer.buf, 0, vk::IndexType::eUint32);
	cb->drawIndexed(indexCount, 1, 0, 0, 0);
}