#pragma once

#include "graphicsPipeline.h"
#include <glm/glm.hpp>
#include "utils.h"
#include "sampledTexture.h"

class GuiControl;

class SkyBoxGraphicsPipeline : public GraphicsPipeline
{
public:
	struct PushStruct
	{
		float time;
		float sunMoonSize;
		float sunMoonImageSize;
		float moonLuminance;
		glm::mat4 cloudVP;
		glm::mat4 M;
		glm::vec3 moonPos;
		float padding1[1];
		glm::vec3 sunPos;
		float padding2[1];
		glm::vec3 cameraPos;
		uint32_t side;
		glm::vec3 moonCol;
		float padding3[1];
		glm::vec3 sunCol;
		float padding4[1];
	};

	struct UniformBufferData
	{
		glm::mat4 matrices[6];
	};

	struct UniformBuffer
	{
		Buffer buffer;
		UniformBufferData bufferData;
	};

	SampledTexture* skyboxTex;
	SampledTexture* moonTex;
	SampledTexture* renderedCubemap;
	UniformBuffer* uniformBuffer;
	vk::RenderPass* renderPass;

	uint32_t skybox_cubemap_size = 512;
	PushStruct pushConstants;

	SkyBoxGraphicsPipeline();
	~SkyBoxGraphicsPipeline();

	void setUniformBuffer(UniformBuffer* buffer);
	void setRenderPass(vk::RenderPass* _renderPass);
	void setSampledTextures(SampledTexture* _skyBoxTex, SampledTexture* _moonTex, SampledTexture* _renderedCubemap);
	void updateData(AppResources* app, GuiControl* gui_control);
	void cleanUpPipeline(AppResources* app, bool full) override;
protected:
	void createPipeline(AppResources* app, bool full) override;
	void bindPushConstants(vk::CommandBuffer* cb);
	void createVertexInformation() override;
	void createDescriptorPool(AppResources* app) override;
	void createDescriptorSet(AppResources* app) override;

	float sun_moon_size = 50.0f;
	float sun_moon_image_size = 30.0f;
	float moonLuminance = 0.02f;
	float cloudsSize = 300.0f;
	float cloudsHeight = 200.0f;

	std::vector<float> getUniformBufferData(UniformBuffer& ub);
};