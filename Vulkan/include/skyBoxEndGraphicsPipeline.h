#pragma once

#include "skyBoxGraphicsPipeline.h"

class SkyBoxEndGraphicsPipeline : public SkyBoxGraphicsPipeline
{
public:
	struct PushStruct
	{
		glm::mat4 M;
		uint32_t tonemapping;
		float pad[3];
	};
	PushStruct pushConstants;

	SkyBoxEndGraphicsPipeline();
	~SkyBoxEndGraphicsPipeline();

	void updateData(AppResources* app, GuiControl* gui_control);
protected:
	void createPipeline(AppResources* app, bool full) override;
	void bindPushConstants(vk::CommandBuffer* cb);
};