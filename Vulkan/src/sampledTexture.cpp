#include "sampledTexture.h"
#include "utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <dds_loader.h>
#include <iostream>
#include <chrono>  // chrono::system_clock
#include <ctime>   // localtime
#include <sstream> // stringstream
#include <iomanip> // put_time
#include <string>  // string
#include "initialization.h"
#include "window.h"

#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT                   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT                  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT                  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT                  0x83F3
#define GL_COMPRESSED_RG_RGTC2_EXT		                  0x8DBD	

#define GL_RGB                            0x1907
#define GL_RGBA                           0x1908

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
				 vk::SampleCountFlagBits samples)
{
	vk::ImageCreateInfo createInfo = {};
	createInfo.imageType = imageType;
	createInfo.extent.width = width;
	createInfo.extent.height = height;
	createInfo.extent.depth = 1;
	createInfo.mipLevels = 1;
	createInfo.arrayLayers = numLayers;
	createInfo.format = format;
	createInfo.flags = createFlags;
	createInfo.tiling = tiling;
	createInfo.initialLayout = initialLayout;
	createInfo.usage = usageFlags;
	createInfo.samples = samples;
	createInfo.sharingMode = vk::SharingMode::eExclusive;

	image = app->device.createImage(createInfo);
	setObjectName(app->device, image, name);

	vk::MemoryRequirements memRequirements;
	memRequirements = app->device.getImageMemoryRequirements(image);

	vk::MemoryAllocateInfo allocInfo{};
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, memoryPropertyFlags, app->pDevice);

	imageMemory = app->device.allocateMemory(allocInfo);

	app->device.bindImageMemory(image, imageMemory, 0);
}

void createImageView(AppResources* app,
					 vk::Image& image,
					 vk::ImageViewType imageViewType,
					 int numLayers,
					 int baseLayer,
					 vk::ComponentMapping components,
					 vk::Format format,
					 vk::ImageAspectFlagBits aspectFlags,
					 vk::ImageView& imageView)
{
	vk::ImageViewCreateInfo createInfo = {};
	createInfo.image = image;
	createInfo.viewType = imageViewType;
	createInfo.format = format;
	createInfo.components = components;
	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = baseLayer;
	createInfo.subresourceRange.layerCount = numLayers;

	imageView = app->device.createImageView(createInfo);
}

std::vector<uint8_t> loadImageData(std::string filename, int* width, int* height, int* nrChannels, vk::Format* format)
{
	if (filename.size() < 4)
		throw std::exception("invalid filename");

	std::string file_extension = "";
	for (int i = filename.size() - 1; i >= 0; i--)
	{
		if (filename.c_str()[i] == '.')
			break;
		else
			file_extension = filename.c_str()[i] + file_extension;
	}

	if (file_extension.compare("png") == 0 ||
		file_extension.compare("jpg") == 0 ||
		file_extension.compare("bmp") == 0)
	{
		uint8_t* data = stbi_load(filename.c_str(), width, height, nrChannels, 0);
		*format = vk::Format::eR8G8B8A8Unorm;
		if (!data)
		{
			throw std::exception(("error loading image: " + filename + " no data could be loaded").c_str());
		}
		if (*nrChannels != 4 && *nrChannels != 3)
		{
			throw std::exception(("error loading image: " + filename + " invalid number of channels").c_str());
		}
		std::vector<uint8_t> data2(*width * *height * 4);
		for (int j = 0; j < *width * *height; ++j)
		{
			data2[4 * j + 0] = data[*nrChannels * j + 0];
			data2[4 * j + 1] = data[*nrChannels * j + 1];
			data2[4 * j + 2] = data[*nrChannels * j + 2];
			data2[4 * j + 3] = (*nrChannels == 4) ? data[*nrChannels * j + 3] : 255;
		}
		stbi_image_free(data);
		return data2;
	}
	if (file_extension.compare("dds") == 0)
	{
		DDS_TEXTURE* texture = new DDS_TEXTURE();
		load_dds_from_file((char*)filename.c_str(), &texture, false);
		if (!texture->pixels)
		{
			throw std::exception(("error loading image: " + filename + " no data could be loaded").c_str());
		}
		*width = texture->width;
		*height = texture->height;
		*nrChannels = texture->channels;
		std::vector<uint8_t> data(texture->sz);
		for (int j = 0; j < data.size(); ++j)
		{
			data[j] = texture->pixels[j];
		}

		switch (texture->format)
		{
			case GL_RGB:
				*format = vk::Format::eR8G8B8Unorm;
				break;
			case GL_RGBA:
				*format = vk::Format::eR8G8B8A8Unorm;
				break;
			case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
				*format = vk::Format::eBc1RgbUnormBlock;
				break;
			case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
				*format = vk::Format::eBc1RgbaUnormBlock;
				break;
			case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
				*format = vk::Format::eBc2UnormBlock;
				break;
			case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
				*format = vk::Format::eBc3UnormBlock;
				break;
			case GL_COMPRESSED_RG_RGTC2_EXT:
				*format = vk::Format::eBc5UnormBlock;
				break;
			default:
				throw std::exception(("error loading image: " + filename + " invalid interior format").c_str());
				break;
		}

		delete texture;
		return data;
	}
	throw std::exception(("error loading image: " + filename + " invalid texture format").c_str());
	return std::vector<uint8_t>();
}

SampledTexture loadTexture2D(AppResources* app, std::string filename)
{
	int width, height, nrChannels;
	vk::Format format;
	std::vector<uint8_t> data = loadImageData(filename, &width, &height, &nrChannels, &format);
	if (data.size() <= 0)
	{
		throw std::exception(("error loading image: " + filename).c_str());
	}

	SampledTexture texture;
	texture.name = filename;
	texture.format = format;
	createImage(app,
				filename,
				width, height, 1,
				vk::ImageType::e2D,
				format,
				vk::ImageLayout::eUndefined,
				vk::ImageTiling::eOptimal,
				vk::ImageCreateFlags(),
				vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
				vk::MemoryPropertyFlagBits::eDeviceLocal,
				texture.image, texture.mem, vk::SampleCountFlagBits::e1);

	vk::Extent3D imageExtent = {};
	imageExtent.height = height;
	imageExtent.width = width;
	imageExtent.depth = 1;

	fillImageWithStagingBuffer(app->pDevice, app->device, app->graphicsCommandPool, app->graphicsQueue, texture.image, imageExtent,
							   nrChannels, 1, data);

	vk::SamplerCreateInfo sampler = {};
	sampler.magFilter = vk::Filter::eLinear;
	sampler.minFilter = vk::Filter::eLinear;
	sampler.mipmapMode = vk::SamplerMipmapMode::eLinear;
	sampler.addressModeU = vk::SamplerAddressMode::eRepeat;
	sampler.addressModeV = sampler.addressModeU;
	sampler.addressModeW = sampler.addressModeU;
	sampler.mipLodBias = 0.0f;
	sampler.compareOp = vk::CompareOp::eNever;
	sampler.minLod = 0.0f;
	sampler.maxLod = 1;
	sampler.borderColor = vk::BorderColor::eFloatOpaqueWhite;
	sampler.maxAnisotropy = 1.0f;
	sampler.anisotropyEnable = VK_FALSE;

	app->device.createSampler(&sampler, nullptr, &texture.sampler);

	createImageView(app,
					texture.image, vk::ImageViewType::e2D, 1, 0,
					{ vk::ComponentSwizzle::eR,
					 vk::ComponentSwizzle::eG,
					 vk::ComponentSwizzle::eB,
					 vk::ComponentSwizzle::eA },
					format,
					vk::ImageAspectFlagBits::eColor,
					texture.imageView);

	return texture;
}

SampledTexture createCubemap(AppResources* app, std::string name, uint32_t width, uint32_t height, vk::ImageUsageFlags imageUsage, vk::ImageLayout layout, vk::Format format)
{
	SampledTexture cubemap;
	cubemap.format = format;
	createImage(app,
				name,
				width, height, 6,
				vk::ImageType::e2D,
				format,
				layout,
				vk::ImageTiling::eOptimal,
				vk::ImageCreateFlagBits::eCubeCompatible,
				imageUsage,
				vk::MemoryPropertyFlagBits::eDeviceLocal,
				cubemap.image, cubemap.mem, vk::SampleCountFlagBits::e1);

	vk::SamplerCreateInfo sampler = {};
	sampler.magFilter = vk::Filter::eLinear;
	sampler.minFilter = vk::Filter::eLinear;
	sampler.mipmapMode = vk::SamplerMipmapMode::eLinear;
	sampler.addressModeU = vk::SamplerAddressMode::eClampToEdge;
	sampler.addressModeV = sampler.addressModeU;
	sampler.addressModeW = sampler.addressModeU;
	sampler.mipLodBias = 0.0f;
	sampler.compareOp = vk::CompareOp::eNever;
	sampler.minLod = 0.0f;
	sampler.maxLod = 1;
	sampler.borderColor = vk::BorderColor::eFloatOpaqueWhite;
	sampler.maxAnisotropy = 1.0f;
	sampler.anisotropyEnable = VK_FALSE;

	app->device.createSampler(&sampler, nullptr, &cubemap.sampler);

	createImageView(app,
					cubemap.image, vk::ImageViewType::eCube, 6, 0,
					{ vk::ComponentSwizzle::eR,
					 vk::ComponentSwizzle::eG,
					 vk::ComponentSwizzle::eB,
					 vk::ComponentSwizzle::eA },
					format,
					vk::ImageAspectFlagBits::eColor,
					cubemap.imageView);

	return cubemap;
}


SampledTexture loadCubemap(AppResources* app, std::string foldername, std::string file_extension)
{
	int width, height, nrChannels;
	std::vector<std::vector<uint8_t>> data(6);
	std::vector<uint8_t> data2;
	std::string side[6] = {
			"right",
			"left",
			"top",
			"bottom",
			"front",
			"back"
	};
	vk::Format format = vk::Format::eR8G8B8A8Unorm;
	for (int i = 0; i < 6; ++i)
	{
		data[i] = loadImageData((foldername + "/" + side[i] + "." + file_extension).c_str(), &width, &height, &nrChannels, &format);
		if (data.size() <= 0)
		{
			throw std::exception(("error loading image: " + foldername + "/" + side[i] + "." + file_extension).c_str());
		}
		if (i == 0)
		{
			data2 = std::vector<uint8_t>(width * height * 4 * 6);
		}

		for (int j = 0; j < data[i].size(); ++j)
		{
			data2[data[i].size() * i + j] = data[i][j];
		}
	}

	SampledTexture cubemap = createCubemap(app, foldername + "cubemap", width, height, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::ImageLayout::eUndefined, format);
	cubemap.name = foldername + "cubemap";

	vk::Extent3D imageExtent = {};
	imageExtent.height = height;
	imageExtent.width = width;
	imageExtent.depth = 1;

	fillImageWithStagingBuffer(app->pDevice, app->device, app->graphicsCommandPool, app->graphicsQueue, cubemap.image, imageExtent,
							   nrChannels, 6, data2);
	return cubemap;
}

void destroySampledTexture(vk::Device& device, SampledTexture& sampledTexture)
{
	device.destroyImageView(sampledTexture.imageView);
	device.destroyImage(sampledTexture.image);
	device.destroySampler(sampledTexture.sampler);
	device.freeMemory(sampledTexture.mem);
}

int32_t saveImage(std::string path, std::vector<char>& data, int width, int height, int components)
{
	return stbi_write_png(path.c_str(), width, height, components, data.data(), width * components);
}

int32_t saveImageHdr(std::string path, std::vector<float>& data, int width, int height, int components)
{
	return stbi_write_hdr(path.c_str(), width, height, components, data.data());
}

void saveScreenShot(AppResources* app, int32_t fileType)
{
	vk::Image srcImage;
	vk::Format srcFormat, dstFormat;
	vk::ImageLayout srcLayout;
	switch (fileType)
	{
		case SCREENSHOT_FILE_TYPE::SFT_PNG:
			srcImage = app->swapChainImages[app->currentImageIndex].image;
			srcFormat = app->swapChainFormat;
			srcLayout = vk::ImageLayout::ePresentSrcKHR;
			dstFormat = vk::Format::eR8G8B8A8Unorm;
			break;
		case SCREENSHOT_FILE_TYPE::SFT_HDR:
			srcImage = app->colorImagesHDR[app->currentImageIndex].image;
			srcFormat = app->colorImagesHDR[app->currentImageIndex].format;
			srcLayout = vk::ImageLayout::eColorAttachmentOptimal;
			dstFormat = vk::Format::eR32G32B32A32Sfloat;
			break;
		default:
			break;
	}

	vk::Image dstImage;
	vk::DeviceMemory dstImageMemory;
	createImage(app, "screenshot",
				app->swapChainExtent.width,
				app->swapChainExtent.height,
				1,
				vk::ImageType::e2D,
				dstFormat,
				vk::ImageLayout::eUndefined,
				vk::ImageTiling::eLinear,
				vk::ImageCreateFlagBits(),
				vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc,
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
				dstImage,
				dstImageMemory, vk::SampleCountFlagBits::e1);

	vk::CommandBuffer commandBuffer = beginSingleTimeCommands(app->device, app->graphicsCommandPool);

	setImageLayout(commandBuffer, dstImage, dstFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
	setImageLayout(commandBuffer, srcImage, srcFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferSrcOptimal);

	bool supportsBlit = app->supportsBlit(srcFormat, vk::ImageTiling::eOptimal, dstFormat, vk::ImageTiling::eLinear);
	if (supportsBlit)
	{
		// Define the region to blit (we will blit the whole swapchain image)
		vk::Offset3D blitSize;
		blitSize.x = app->swapChainExtent.width;
		blitSize.y = app->swapChainExtent.height;
		blitSize.z = 1;
		vk::ImageBlit imageBlitRegion{};
		imageBlitRegion.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		imageBlitRegion.srcSubresource.layerCount = 1;
		imageBlitRegion.srcOffsets[1] = blitSize;
		imageBlitRegion.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		imageBlitRegion.dstSubresource.layerCount = 1;
		imageBlitRegion.dstOffsets[1] = blitSize;

		// Issue the blit command
		commandBuffer.blitImage(
			srcImage, vk::ImageLayout::eTransferSrcOptimal,
			dstImage, vk::ImageLayout::eTransferDstOptimal,
			1,
			&imageBlitRegion,
			vk::Filter::eNearest);
	}
	else
	{
		// Otherwise use image copy (requires us to manually flip components)
		vk::ImageCopy imageCopyRegion{};
		imageCopyRegion.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		imageCopyRegion.srcSubresource.layerCount = 1;
		imageCopyRegion.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		imageCopyRegion.dstSubresource.layerCount = 1;
		imageCopyRegion.extent.width = app->swapChainExtent.width;
		imageCopyRegion.extent.height = app->swapChainExtent.height;
		imageCopyRegion.extent.depth = 1;

		// Issue the copy command
		commandBuffer.copyImage(
			srcImage, vk::ImageLayout::eTransferSrcOptimal,
			dstImage, vk::ImageLayout::eTransferDstOptimal,
			1,
			&imageCopyRegion);
	}

	setImageLayout(commandBuffer, dstImage, dstFormat, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral);
	setImageLayout(commandBuffer, srcImage, srcFormat, vk::ImageLayout::eTransferSrcOptimal, srcLayout);

	endSingleTimeCommands(app->device, app->graphicsQueue, app->graphicsCommandPool, commandBuffer);

	std::stringstream ss;
	if (fileType == SCREENSHOT_FILE_TYPE::SFT_PNG)
	{
		std::vector<char> data(4 * app->swapChainExtent.width * app->swapChainExtent.height);
		fillHostBuffer(app->device, dstImageMemory, data);

		bool colorSwizzle = false;
		// Check if source is BGR
		// Note: Not complete, only contains most common and basic BGR surface formats for demonstration purposes
		if (!supportsBlit)
		{
			std::vector<vk::Format> formatsBGR = { vk::Format::eB8G8R8A8Srgb, vk::Format::eB8G8R8A8Unorm, vk::Format::eB8G8R8A8Snorm };
			colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), srcFormat) != formatsBGR.end());
		}

		if (colorSwizzle)
		{
			for (size_t x = 0; x < app->swapChainExtent.width; ++x)
			{
				for (size_t y = 0; y < app->swapChainExtent.height; ++y)
				{
					float r, b;
					r = data[x * 4 + y * app->swapChainExtent.width * 4 + 2];
					b = data[x * 4 + y * app->swapChainExtent.width * 4 + 0];
					data[x * 4 + y * app->swapChainExtent.width * 4 + 0] = r;
					data[x * 4 + y * app->swapChainExtent.width * 4 + 2] = b;
				}
			}
		}

		auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);
		ss << "screen/png/";
		ss << std::put_time(std::localtime(&in_time_t), "%B-%d-%Y_%H-%M-%S-");
		ss << (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()) % 1000;
		ss << "_" << app->windowObj->gui_control.renderModesNames[app->windowObj->gui_control.var.currentRenderMode];
		if (app->windowObj->gui_control.referenceMode)
			ss << "-ref";
		ss << ".png";

		int32_t res = saveImage(ss.str().c_str(), data, app->swapChainExtent.width, app->swapChainExtent.height, 4);
		if (res != 1)
			std::cout << "error saving screenshot\n";
	}
	else if (fileType == SCREENSHOT_FILE_TYPE::SFT_HDR)
	{
		std::vector<float> data(4 * app->swapChainExtent.width * app->swapChainExtent.height);
		fillHostBuffer(app->device, dstImageMemory, data);

		bool colorSwizzle = false;
		// Check if source is BGR
		// Note: Not complete, only contains most common and basic BGR surface formats for demonstration purposes
		if (!supportsBlit)
		{
			std::vector<vk::Format> formatsBGR = { vk::Format::eB8G8R8A8Srgb, vk::Format::eB8G8R8A8Unorm, vk::Format::eB8G8R8A8Snorm };
			colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), srcFormat) != formatsBGR.end());
		}

		if (colorSwizzle)
		{
			for (size_t x = 0; x < app->swapChainExtent.width; ++x)
			{
				for (size_t y = 0; y < app->swapChainExtent.height; ++y)
				{
					float r, b;
					r = data[x * 4 + y * app->swapChainExtent.width * 4 + 2];
					b = data[x * 4 + y * app->swapChainExtent.width * 4 + 0];
					data[x * 4 + y * app->swapChainExtent.width * 4 + 0] = r;
					data[x * 4 + y * app->swapChainExtent.width * 4 + 2] = b;
				}
			}
		}

		auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);
		ss << "screen/hdr/";
		ss << std::put_time(std::localtime(&in_time_t), "%B-%d-%Y_%H-%M-%S-");
		ss << (std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()) % 1000;
		ss << "_" << app->windowObj->gui_control.renderModesNames[app->windowObj->gui_control.var.currentRenderMode];
		if (app->windowObj->gui_control.referenceMode)
			ss << "-ref";
		ss << ".hdr";
		int32_t res = saveImageHdr(ss.str().c_str(), data, app->swapChainExtent.width, app->swapChainExtent.height, 4);
		if (res != 1)
			std::cout << "error saving screenshot\n";
	}
	else
	{
		std::cout << "Illegal Screenshot File Type\n";
	}

	app->device.freeMemory(dstImageMemory);
	app->device.destroyImage(dstImage);

	if (app->windowObj->gui_control.applyPythonPipeline)
	{
		app->windowObj->gui_control.pythonPipelineImageNames.push_back(ss.str());
		if (app->windowObj->gui_control.aspQueueEmpty() || !app->windowObj->gui_control.advancedScreenshot)
		{
			std::stringstream ss;
			ss << "cd \"../Python/ImageHighlighter\" && ";
			ss << "python ./ImageHighlighterCmd.py ";
			ss << "\"" << app->windowObj->gui_control.pythonRectPathCSV << "\" ";
			size_t i = 0;
			for (auto& s : app->windowObj->gui_control.pythonPipelineImageNames)
			{
				ss << "\"" << "../../Vulkan/" << s << "\"";
				if (i < app->windowObj->gui_control.pythonPipelineImageNames.size() - 1)
					ss << " ";
				i++;
			}
			system(ss.str().c_str());
			app->windowObj->gui_control.pythonPipelineImageNames.clear();
		}
	}
}