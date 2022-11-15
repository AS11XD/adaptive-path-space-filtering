#pragma once

#include "graphicsPipeline.h"
#include "defaultPrePassGraphicsPipeline.h"

class LineGraphicsPipeline : public GraphicsPipeline
{
public:
	struct PushStruct
	{
		uint32_t tmp;
	};

	PushStruct pushConstants;
	DefaultPrePassGraphicsPipeline::UniformBuffer* uniformBuffer;

	LineGraphicsPipeline();
	~LineGraphicsPipeline();

	void setUniformBuffer(DefaultPrePassGraphicsPipeline::UniformBuffer* buffer);
	void updateData(AppResources* app, GuiControl* gui_control);
	void cleanUpPipeline(AppResources* app, bool full) override;
protected:
	void bindPushConstants(vk::CommandBuffer* cb);
	void createVertexInformation() override;
	void createPipeline(AppResources* app, bool full) override;
	void createDescriptorPool(AppResources* app) override;
	void createDescriptorSet(AppResources* app) override;
};