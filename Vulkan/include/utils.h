#ifndef UTILS
#define UTILS

#include <vector>
#include <cstring>
#include <vulkan/vulkan.hpp>

#define CAST(a) static_cast<uint32_t>(a.size())
struct Buffer
{
	vk::Buffer buf;
	vk::DeviceMemory mem;
};

typedef uint32_t uint;

template<typename T, typename V>
T ceilDiv(T x, V y)
{
	return x / y + (x % y != 0);
}

std::vector<char> readFile(const std::string& filename);
std::string formatSize(uint64_t size);
uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties, vk::PhysicalDevice& pdevice);
void createBuffer(vk::PhysicalDevice& pDevice, vk::Device& device,
				  const vk::DeviceSize& size, vk::BufferUsageFlags usage,
				  vk::MemoryPropertyFlags properties, std::string name, vk::Buffer& buffer,
				  vk::DeviceMemory& bufferMemory);
void createBuffer(vk::PhysicalDevice& pDevice, vk::Device& device,
				  const vk::DeviceSize& size, vk::BufferUsageFlags usage,
				  vk::MemoryPropertyFlags properties, std::string name, Buffer& buffer);
void createExportBuffer(vk::PhysicalDevice& pDevice, vk::Device& device,
						const vk::DeviceSize& size, vk::BufferUsageFlags usage,
						vk::MemoryPropertyFlags properties, std::string name, vk::Buffer& buffer,
						vk::DeviceMemory& bufferMemory);
void createExportBuffer(vk::PhysicalDevice& pDevice, vk::Device& device,
						const vk::DeviceSize& size, vk::BufferUsageFlags usage,
						vk::MemoryPropertyFlags properties, std::string name, Buffer& buffer);
void destroyBuffer(vk::Device& device, Buffer& buffer);
void copyBuffer(vk::Device& device, vk::Queue& q, vk::CommandPool& commandPool,
				const vk::Buffer& srcBuffer, vk::Buffer& dstBuffer, vk::DeviceSize byteSize, uint64_t dstOffset = 0ULL);
void copyBufferToImage(vk::Device& device, vk::Queue& q, vk::CommandPool& commandPool, vk::Buffer& srcBuffer,
					   vk::Image& dstImage, vk::Extent3D imageExtent, uint32_t layerCount);

vk::CommandBuffer beginSingleTimeCommands(vk::Device& device, vk::CommandPool& commandPool);
void endSingleTimeCommands(vk::Device& device, vk::Queue& q,
						   vk::CommandPool& commandPool, vk::CommandBuffer& commandBuffer);

Buffer addHostCoherentBuffer(vk::PhysicalDevice& pDevice, vk::Device& device, vk::DeviceSize size, std::string name);
Buffer addDeviceOnlyBuffer(vk::PhysicalDevice& pDevice, vk::Device& device, vk::DeviceSize size, std::string name);

template<typename T>
void fillDeviceBuffer(vk::Device& device, vk::DeviceMemory& mem, const std::vector<T>& input)
{
	void* data = device.mapMemory(mem, 0, input.size() * sizeof(T), vk::MemoryMapFlags());
	memcpy(data, input.data(), static_cast<size_t>(input.size() * sizeof(T)));
	device.unmapMemory(mem);
}

template<typename T>
void fillHostBuffer(vk::Device& device, vk::DeviceMemory& mem, std::vector<T>& output)
{
	// copy memory from mem to output
	void* data = device.mapMemory(mem, 0, output.size() * sizeof(T), vk::MemoryMapFlags());
	memcpy(output.data(), data, static_cast<size_t>(output.size() * sizeof(T)));
	device.unmapMemory(mem);
}

template<typename T>
void fillDeviceWithStagingBuffer(vk::PhysicalDevice& pDevice, vk::Device& device,
								 vk::CommandPool& commandPool, vk::Queue& q,
								 Buffer& b, const std::vector<T>& data, uint64_t dstOffset = 0ULL)
{
	// Buffer b requires the eTransferSrc bit
	// data (host) -> staging (device) -> Buffer b (device)
	vk::Buffer staging;
	vk::DeviceMemory mem;
	vk::DeviceSize byteSize = data.size() * sizeof(T);

	createBuffer(pDevice, device, byteSize, vk::BufferUsageFlagBits::eTransferSrc,
				 vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible, "staging",
				 staging, mem);
	// V host -> staging V
	fillDeviceBuffer<T>(device, mem, data);
	// V staging -> buffer V
	copyBuffer(device, q, commandPool, staging, b.buf, byteSize, dstOffset);
	device.destroyBuffer(staging);
	device.freeMemory(mem);
}

template<typename T>
void fillImageWithStagingBuffer(vk::PhysicalDevice& pDevice, vk::Device& device,
								vk::CommandPool& commandPool, vk::Queue& q, vk::Image& dstImage,
								vk::Extent3D imageExtent, uint32_t nrChannels, uint32_t layerCount,
								const std::vector<T>& data)
{
	vk::Buffer staging;
	vk::DeviceMemory mem;
	vk::DeviceSize byteSize = sizeof(T) * data.size();

	createBuffer(pDevice, device, byteSize, vk::BufferUsageFlagBits::eTransferSrc,
				 vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible,
				 "staging-Image-copy",
				 staging, mem);

	fillDeviceBuffer<T>(device, mem, data);

	copyBufferToImage(device, q, commandPool, staging, dstImage, imageExtent, layerCount);
	device.destroyBuffer(staging);
	device.freeMemory(mem);
}

template<typename T>
void fillHostWithStagingBuffer(vk::PhysicalDevice& pDevice, vk::Device& device,
							   vk::CommandPool& commandPool, vk::Queue& q,
							   const Buffer& b, std::vector<T>& data)
{
	// Buffer b requires the eTransferDst bit
	// Buffer b (device) -> staging (device) -> data (host)
	vk::Buffer staging;
	vk::DeviceMemory mem;
	vk::DeviceSize byteSize = data.size() * sizeof(T);

	createBuffer(pDevice, device, byteSize, vk::BufferUsageFlagBits::eTransferDst,
				 vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible, "staging",
				 staging, mem);
	// V buffer -> staging V
	copyBuffer(device, q, commandPool, b.buf, staging, byteSize);
	// V staging -> host V
	fillHostBuffer<T>(device, mem, data);

	device.destroyBuffer(staging);
	device.freeMemory(mem);
}

template<typename T>
void setObjectName(vk::Device& device, T handle, std::string name)
{
#ifndef NDEBUG
	vk::DebugUtilsObjectNameInfoEXT infoEXT(handle.objectType, uint64_t(static_cast<typename T::CType>(handle)),
											name.c_str());
	device.setDebugUtilsObjectNameEXT(infoEXT);
#endif
}

static void createShaderModule(vk::Device device, const std::string& filename, vk::ShaderModule& shaderModule)
{
	device.destroyShaderModule(shaderModule);
	std::vector<char> shader = readFile("./build/shaders/" + filename + ".spv");
	vk::ShaderModuleCreateInfo smi({}, static_cast<uint32_t> (shader.size()),
								   reinterpret_cast<const uint32_t*>(shader.data()));
	shaderModule = device.createShaderModule(smi);
}

static void setImageLayout(vk::CommandBuffer const& commandBuffer,
						   vk::Image image,
						   vk::Format format,
						   vk::ImageLayout oldImageLayout,
						   vk::ImageLayout newImageLayout,
						   uint32_t layerCount = 1)
{
	vk::AccessFlags sourceAccessMask;
	switch (oldImageLayout)
	{
		case vk::ImageLayout::ePresentSrcKHR:
			sourceAccessMask = vk::AccessFlagBits::eMemoryRead;
			break;
		case vk::ImageLayout::eTransferDstOptimal:
			sourceAccessMask = vk::AccessFlagBits::eTransferWrite;
			break;
		case vk::ImageLayout::eTransferSrcOptimal:
			sourceAccessMask = vk::AccessFlagBits::eTransferRead;
			break;
		case vk::ImageLayout::ePreinitialized:
			sourceAccessMask = vk::AccessFlagBits::eHostWrite;
			break;
		case vk::ImageLayout::eColorAttachmentOptimal:
			sourceAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
			break;
		case vk::ImageLayout::eGeneral:  // sourceAccessMask is empty
		case vk::ImageLayout::eUndefined:
			break;
		default:
			assert(false);
			break;
	}

	vk::PipelineStageFlags sourceStage;
	switch (oldImageLayout)
	{
		case vk::ImageLayout::eGeneral:
		case vk::ImageLayout::ePreinitialized:
			sourceStage = vk::PipelineStageFlagBits::eHost;
			break;
		case vk::ImageLayout::eTransferDstOptimal:
			sourceStage = vk::PipelineStageFlagBits::eTransfer;
			break;
		case vk::ImageLayout::eTransferSrcOptimal:
			sourceStage = vk::PipelineStageFlagBits::eTransfer;
			break;
		case vk::ImageLayout::ePresentSrcKHR:
			sourceStage = vk::PipelineStageFlagBits::eTransfer;
			break;
		case vk::ImageLayout::eColorAttachmentOptimal:
			sourceStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			break;
		case vk::ImageLayout::eUndefined:
			sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
			break;
		default:
			assert(false);
			break;
	}

	vk::AccessFlags destinationAccessMask;
	switch (newImageLayout)
	{
		case vk::ImageLayout::eColorAttachmentOptimal:
			destinationAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
			break;
		case vk::ImageLayout::eDepthStencilAttachmentOptimal:
			destinationAccessMask =
				vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
			break;
		case vk::ImageLayout::eGeneral:  // empty destinationAccessMask
		case vk::ImageLayout::ePresentSrcKHR:
			break;
		case vk::ImageLayout::eShaderReadOnlyOptimal:
			destinationAccessMask = vk::AccessFlagBits::eShaderRead;
			break;
		case vk::ImageLayout::eTransferSrcOptimal:
			destinationAccessMask = vk::AccessFlagBits::eTransferRead;
			break;
		case vk::ImageLayout::eTransferDstOptimal:
			destinationAccessMask = vk::AccessFlagBits::eTransferWrite;
			break;
		default:
			assert(false);
			break;
	}

	vk::PipelineStageFlags destinationStage;
	switch (newImageLayout)
	{
		case vk::ImageLayout::eColorAttachmentOptimal:
			destinationStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			break;
		case vk::ImageLayout::eDepthStencilAttachmentOptimal:
			destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
			break;
		case vk::ImageLayout::eGeneral:
			destinationStage = vk::PipelineStageFlagBits::eHost;
			break;
		case vk::ImageLayout::ePresentSrcKHR:
			destinationStage = vk::PipelineStageFlagBits::eBottomOfPipe;
			break;
		case vk::ImageLayout::eShaderReadOnlyOptimal:
			destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
			break;
		case vk::ImageLayout::eTransferDstOptimal:
		case vk::ImageLayout::eTransferSrcOptimal:
			destinationStage = vk::PipelineStageFlagBits::eTransfer;
			break;
		default:
			assert(false);
			break;
	}

	vk::ImageAspectFlags aspectMask;
	if (newImageLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
	{
		aspectMask = vk::ImageAspectFlagBits::eDepth;
		if (format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint)
		{
			aspectMask |= vk::ImageAspectFlagBits::eStencil;
		}
	}
	else
	{
		if (format == vk::Format::eD32Sfloat || format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint)
		{
			aspectMask = vk::ImageAspectFlagBits::eDepth;
		}
		else
		{
			aspectMask = vk::ImageAspectFlagBits::eColor;
		}
	}

	vk::ImageSubresourceRange imageSubresourceRange(aspectMask, 0, 1, 0, layerCount);
	vk::ImageMemoryBarrier imageMemoryBarrier(sourceAccessMask,
											  destinationAccessMask,
											  oldImageLayout,
											  newImageLayout,
											  VK_QUEUE_FAMILY_IGNORED,
											  VK_QUEUE_FAMILY_IGNORED,
											  image,
											  imageSubresourceRange);
	return commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, nullptr, nullptr, imageMemoryBarrier);
}

#endif