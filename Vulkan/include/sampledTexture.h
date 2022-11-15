#pragma once

#include <string>
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

class AppResources;

struct SampledTexture
{
	std::string name;
	vk::ImageView imageView;
	vk::Image image;
	vk::Sampler sampler;
	vk::DeviceMemory mem;
	vk::Format format;

	bool operator==(const SampledTexture& other) const
	{
		return name.compare(other.name) == 0;
	}

	SampledTexture& operator=(const SampledTexture& other)
	{
		if (this == &other)
			return *this;

		name = other.name;
		imageView = other.imageView;
		image = other.image;
		sampler = other.sampler;
		mem = other.mem;
		format = other.format;

		return *this;
	}
};

void createImage(AppResources* app,
				 std::string name,
				 int width,
				 int height,
				 int numLayers,
				 vk::ImageType imageType,
				 vk::Format format,
				 vk::ImageLayout initialLayout,
				 vk::ImageTiling tiling,
				 vk::ImageCreateFlags createFlags,
				 vk::ImageUsageFlags usageFlags,
				 vk::MemoryPropertyFlags memoryPropertyFlags,
				 vk::Image& image,
				 vk::DeviceMemory& imageMemory,
				 vk::SampleCountFlagBits samples);

void createImageView(AppResources* app,
					 vk::Image& image,
					 vk::ImageViewType imageViewType,
					 int numLayers,
					 int baseLayer,
					 vk::ComponentMapping components,
					 vk::Format format,
					 vk::ImageAspectFlagBits aspectFlags,
					 vk::ImageView& imageView);

std::vector<uint8_t> loadImageData(std::string filename, int* width, int* height, int* nrChannels, vk::Format* format);

SampledTexture loadTexture2D(AppResources* app, std::string filename);

SampledTexture createCubemap(AppResources* app, std::string name, uint32_t width, uint32_t height, vk::ImageUsageFlags imageUsage, vk::ImageLayout layout = vk::ImageLayout::eUndefined, vk::Format format = vk::Format::eR8G8B8A8Unorm);
SampledTexture loadCubemap(AppResources* app, std::string foldername, std::string file_extension = "png");

void destroySampledTexture(vk::Device& device, SampledTexture& sampledTexture);

int32_t saveImage(std::string path, std::vector<char>& data, int width, int height, int components);
int32_t saveImageHdr(std::string path, std::vector<float>& data, int width, int height, int components);

void saveScreenShot(AppResources* app, int32_t fileType);