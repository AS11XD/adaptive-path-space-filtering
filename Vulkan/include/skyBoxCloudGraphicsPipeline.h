#pragma once

#include "skyBoxGraphicsPipeline.h"

class SkyBoxCloudGraphicsPipeline : public SkyBoxGraphicsPipeline
{
public:
	SkyBoxCloudGraphicsPipeline();
	~SkyBoxCloudGraphicsPipeline();

	void updateData(AppResources* app, GuiControl* gui_control);
protected:
	void createPipeline(AppResources* app, bool full) override;
};