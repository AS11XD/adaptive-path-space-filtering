#pragma once

#include "skyBoxGraphicsPipeline.h"

class SkyBoxSphereGraphicsPipeline : public SkyBoxGraphicsPipeline
{
public:
	SkyBoxSphereGraphicsPipeline();
	~SkyBoxSphereGraphicsPipeline();

	void updateData(AppResources* app, GuiControl* gui_control);
protected:
	void createPipeline(AppResources* app, bool full) override;
};