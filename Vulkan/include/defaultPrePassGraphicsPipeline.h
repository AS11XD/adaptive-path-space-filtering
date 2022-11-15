#pragma once

#include "graphicsPipeline.h"
#include <glm/glm.hpp>
#include "utils.h"
#include "material.h"
#include "sampledTexture.h"

class GuiControl;

class DefaultPrePassGraphicsPipeline : public GraphicsPipeline
{
public:
	struct PushStruct
	{
		uint32_t drawNormalMaps;
		uint32_t width;
		uint32_t height;
		uint32_t keepCurrentLines;
	};

	struct UniformBufferData
	{
		glm::mat4 VP;
	};

	struct UniformBuffer
	{
		Buffer buffer;
		UniformBufferData bufferData;
	};

	UniformBuffer* uniformBuffer;
	PushStruct pushConstants;

	DefaultPrePassGraphicsPipeline();
	~DefaultPrePassGraphicsPipeline();

	void setUniformBuffer(UniformBuffer* buffer);
	void setMeshTransformBuffer(Buffer* _meshTransformBuffer);
	void setMeshNormalTransformBuffer(Buffer* _meshNormalTransformBuffer);
	void setTexturesAndMaterials(std::vector<SampledTexture>* _textures, Buffer* _materialBuffer);
	void setLineVisualizationBuffers(Buffer* _drawIndirectLinesBuffer, Buffer* _linesBuffer);
	void updateData(AppResources* app, GuiControl* gui_control);
	void cleanUpPipeline(AppResources* app, bool full) override;
protected:
	void bindPushConstants(vk::CommandBuffer* cb);
	void createVertexInformation() override;
	void createPipeline(AppResources* app, bool full) override;
	void createDescriptorPool(AppResources* app) override;
	void createDescriptorSet(AppResources* app) override;

private:
	std::vector<SampledTexture>* textures;
	Buffer* materialBuffer;
	Buffer* meshTransformBuffer;
	Buffer* meshNormalTransformBuffer;
	Buffer* drawIndirectLinesBuffer;
	Buffer* linesBuffer;

	std::vector<float> getUniformBufferData(UniformBuffer& ub);
};