#pragma once

#include "graphicsPipeline.h"
#include <glm/glm.hpp>
#include "utils.h"
#include "material.h"
#include "sampledTexture.h"
#include "defaultPrePassGraphicsPipeline.h"
#include "accelerationStructure.h"

class GuiControl;

class DefaultDeferredGraphicsPipeline : public GraphicsPipeline
{
public:
	struct UniformBufferData
	{
		glm::mat4 VP;
		glm::vec4 cameraPos;
	};

	struct UniformBuffer
	{
		Buffer buffer;
		UniformBufferData bufferData;
	};

	struct PushStruct
	{
		float gamma;
		float exposure;
		uint32_t renderState;
		uint32_t renderMode;
		uint32_t sampleStrategyBRDF;
		uint32_t width;
		uint32_t height;
		uint32_t frameCount;
		uint32_t iterations;
		uint32_t disableFirstBounce;
		uint32_t sampleEnvironmentMap;
		uint32_t lightCount;
		uint32_t keepCurrentLines;
		uint32_t drawNormalMaps;
		uint32_t denoise;
		uint32_t toneMapping;
		uint32_t hashMapSize;
		float gridScale;
		uint32_t gridJitter;
		uint32_t psfMode;
		uint32_t varianceInterpolationSize;
		uint32_t showPTComparison;
		uint32_t varianceFuncitonUVfHM;
		uint32_t varianceFunctionPSR;
		float varianceThresholdsUVfHM[2];
		float varianceThresholdsPSR[2];
		float varianceConst;
		uint32_t usePrimaryMethods;
		uint32_t maxSamples;
		uint32_t referenceMode;
		//float pad[1];
	};

	PushStruct pushConstants;

	DefaultDeferredGraphicsPipeline();
	~DefaultDeferredGraphicsPipeline();

	void setUniformBuffer(UniformBuffer* buffer);
	void setAveragePathDepthBuffer(Buffer* _averagePathDepthBuffer);
	void setLightBuffer(Buffer* _lightBuffer, uint32_t lightCount);
	void updateLightCount(uint32_t lightCount);
	void setVertexIndexBuffer(Buffer* _vertexBuffer, Buffer* _indexBuffer);
	void setMeshOffsetBuffer(Buffer* _meshOffsetBuffer);
	void setMeshTransformBuffer(Buffer* _transformBuffer);
	void setMeshNormalTransformBuffer(Buffer* _normalTransformBuffer);
	void setEnvironmentMap(SampledTexture* _renderedCubemap);
	void setAccelerationStructure(AccelerationStructure* _accelerationStructure);
	void setLineVisualizationBuffers(Buffer* _drawIndirectLinesBuffer, Buffer* _linesBuffer);
	void setHashMapBuffers(Buffer* _hashMapBuffer, Buffer* _hashMapBuffer2, uint32_t _hashMapSize);
	void setVarianceInterpolationBuffer(Buffer* _varianceInterpolationBuffer);
	void setBrightnessTexture(SampledTexture* _brightness);
	void setTexturesAndMaterials(std::vector<SampledTexture>* _textures, Buffer* _materialBuffer);
	void cleanUpPipeline(AppResources* app, bool full) override;
	void updateData(AppResources* app, GuiControl* gui_control);
	void bindPSF(vk::CommandBuffer* cb);
protected:
	void bindPushConstants(vk::CommandBuffer* cb);
	void createVertexInformation() override;
	void createPipeline(AppResources* app, bool full) override;
	void createDescriptorPool(AppResources* app) override;
	void createDescriptorSet(AppResources* app) override;

private:
	UniformBuffer* uniformBuffer;
	std::vector<SampledTexture>* textures;
	SampledTexture* renderedCubemap;
	Buffer* materialBuffer;
	Buffer* lightBuffer;
	Buffer* vertexBuffer;
	Buffer* indexBuffer;
	Buffer* meshOffsetBuffer;
	Buffer* transformBuffer;
	Buffer* normalTransformBuffer;
	AccelerationStructure* accelerationStructure;
	Buffer* drawIndirectLinesBuffer;
	Buffer* linesBuffer;
	Buffer* varianceInterpolationBuffer;
	Buffer* hashMapBuffer;
	Buffer* hashMapBuffer2;
	Buffer* averagePathDepthBuffer;
	SampledTexture* brightness;
	uint32_t hashMapSize;
	vk::Pipeline pipelinePSF;

	std::vector<float> getUniformBufferData(UniformBuffer& ub);
};