#pragma once

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include "skyBoxGraphicsPipeline.h"
#include "skyBoxSphereGraphicsPipeline.h"
#include "skyBoxCloudGraphicsPipeline.h"
#include "skyBoxEndGraphicsPipeline.h"
#include "mesh.h"
#include "guiControl.h"
#include "meshMerger.h"

class AppResources;

class SkyBoxScene
{
public:
	SkyBoxGraphicsPipeline skyBoxGraphicsPipeline;
	SkyBoxGraphicsPipeline::UniformBuffer uniformBuffer;
	SampledTexture skyboxTex;
	SampledTexture moonTex;
	SampledTexture renderedCubemap;

	SkyBoxScene(AppResources* app);
	~SkyBoxScene();

	void skyBoxRenderPass(vk::CommandBuffer* cb, AppResources* app);
	void defaultRenderPass(vk::CommandBuffer* cb, AppResources* app);
	void cleanUp(AppResources* app, bool full);
	void createFramebuffers(AppResources* app);
	void createRenderPass(AppResources* app);
	void updateUniformData(AppResources* app, GuiControl* guiControl);
	void createPipelines(AppResources* app);

private:
	MeshMerger<Vertex> skysphere;
	MeshMerger<Vertex> skybox;
	vk::RenderPass renderPass;

	void createBuffers(AppResources* app);
	std::vector<Mesh<Vertex>> generateMeshesFromFile(std::string path);

	std::vector<vk::ImageView> renderedCubemapImageViews;
	std::vector<vk::Framebuffer> renderedCubemapFrameBuffers;

	SkyBoxSphereGraphicsPipeline skyBoxSphereGraphicsPipeline;
	SkyBoxCloudGraphicsPipeline skyBoxCloudGraphicsPipeline;
	SkyBoxEndGraphicsPipeline skyBoxEndGraphicsPipeline;
};
