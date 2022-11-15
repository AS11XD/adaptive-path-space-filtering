#include "skyBoxScene.h"
#include "initialization.h"
#include <iostream>

SkyBoxScene::SkyBoxScene(AppResources* app)
{
	createBuffers(app);
}

SkyBoxScene::~SkyBoxScene()
{
}

void SkyBoxScene::createBuffers(AppResources* app)
{
	skybox = std::move(MeshMerger(*app, generateMeshesFromFile("./res/obj/basic/cube.obj")));
	skysphere = std::move(MeshMerger(*app, generateMeshesFromFile("./res/obj/basic/sphere_high_poly.obj")));

	createBuffer(app->pDevice, app->device, sizeof(SkyBoxGraphicsPipeline::UniformBufferData), vk::BufferUsageFlagBits::eUniformBuffer,
				 vk::MemoryPropertyFlagBits::eHostVisible, "uniform-skybox", uniformBuffer.buffer);

	skyboxTex = loadCubemap(app, "res/textures/skybox/night");
	moonTex = loadTexture2D(app, "res/textures/skybox/moon/moon.png");
	renderedCubemap = createCubemap(app, "rendered-skybox-cubemap", skyBoxGraphicsPipeline.skybox_cubemap_size, skyBoxGraphicsPipeline.skybox_cubemap_size, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment);
	renderedCubemapImageViews.resize(6);
	for (int i = 0; i < 6; ++i)
	{
		createImageView(app, renderedCubemap.image, vk::ImageViewType::e2D, 1, i,
						{ vk::ComponentSwizzle::eR,
						 vk::ComponentSwizzle::eG,
						 vk::ComponentSwizzle::eB,
						 vk::ComponentSwizzle::eA },
						renderedCubemap.format,
						vk::ImageAspectFlagBits::eColor,
						renderedCubemapImageViews[i]);
	}

	skyBoxGraphicsPipeline.setUniformBuffer(&uniformBuffer);
	skyBoxGraphicsPipeline.setSampledTextures(&skyboxTex, &moonTex, &renderedCubemap);
	skyBoxSphereGraphicsPipeline.setUniformBuffer(&uniformBuffer);
	skyBoxSphereGraphicsPipeline.setSampledTextures(&skyboxTex, &moonTex, &renderedCubemap);
	skyBoxCloudGraphicsPipeline.setUniformBuffer(&uniformBuffer);
	skyBoxCloudGraphicsPipeline.setSampledTextures(&skyboxTex, &moonTex, &renderedCubemap);
	skyBoxEndGraphicsPipeline.setUniformBuffer(&uniformBuffer);
	skyBoxEndGraphicsPipeline.setSampledTextures(&skyboxTex, &moonTex, &renderedCubemap);

	createRenderPass(app);
	createFramebuffers(app);

	skyBoxGraphicsPipeline.setRenderPass(&renderPass);
	skyBoxSphereGraphicsPipeline.setRenderPass(&renderPass);
	skyBoxCloudGraphicsPipeline.setRenderPass(&renderPass);
	skyBoxEndGraphicsPipeline.setRenderPass(&app->preRenderPass);

	skyBoxGraphicsPipeline.generatePipeline(app, true);
	skyBoxSphereGraphicsPipeline.generatePipeline(app, true);
	skyBoxCloudGraphicsPipeline.generatePipeline(app, true);
	skyBoxEndGraphicsPipeline.generatePipeline(app, true);
}

void SkyBoxScene::skyBoxRenderPass(vk::CommandBuffer* cb, AppResources* app)
{
	std::array<vk::ClearValue, 2> clearValues{};
	clearValues[0].color = vk::ClearColorValue(std::array<float, 4>({ 0.1f, 0.1f, 0.1f, 1.0f }));
	clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

	vk::RenderPassBeginInfo renderPassBeginInfoSkyBox = {};
	renderPassBeginInfoSkyBox.renderPass = renderPass;
	renderPassBeginInfoSkyBox.renderArea.offset.x = 0;
	renderPassBeginInfoSkyBox.renderArea.offset.y = 0;
	vk::Extent2D skyBoxExtent(skyBoxGraphicsPipeline.skybox_cubemap_size, skyBoxGraphicsPipeline.skybox_cubemap_size);
	renderPassBeginInfoSkyBox.renderArea.extent = skyBoxExtent;
	renderPassBeginInfoSkyBox.clearValueCount = 1;
	renderPassBeginInfoSkyBox.pClearValues = &clearValues[0];

	for (size_t j = 0; j < renderedCubemapFrameBuffers.size(); ++j)
	{
		renderPassBeginInfoSkyBox.framebuffer = renderedCubemapFrameBuffers[j];

		cb->beginRenderPass(renderPassBeginInfoSkyBox, vk::SubpassContents::eInline);

		//--------------------------------------------render-sky--------------------------------------------//
		skyBoxGraphicsPipeline.pushConstants.side = j;
		skyBoxSphereGraphicsPipeline.pushConstants.side = j;
		skyBoxCloudGraphicsPipeline.pushConstants.side = j;
		skyBoxGraphicsPipeline.bind(cb);
		skybox.draw(app, cb);

		skyBoxSphereGraphicsPipeline.bind(cb);
		skysphere.draw(app, cb);

		skyBoxCloudGraphicsPipeline.bind(cb);
		skysphere.draw(app, cb);

		cb->endRenderPass();
	}

	//--------------------------------------------render-scene--------------------------------------------//

	setImageLayout(*cb,
				   renderedCubemap.image,
				   renderedCubemap.format,
				   vk::ImageLayout::eUndefined,
				   vk::ImageLayout::eShaderReadOnlyOptimal, 6);
}

void SkyBoxScene::defaultRenderPass(vk::CommandBuffer* cb, AppResources* app)
{
	skyBoxEndGraphicsPipeline.bind(cb);
	skybox.draw(app, cb);
}

void SkyBoxScene::cleanUp(AppResources* app, bool full)
{
	skyBoxGraphicsPipeline.cleanUpPipeline(app, full);
	skyBoxSphereGraphicsPipeline.cleanUpPipeline(app, full);
	skyBoxCloudGraphicsPipeline.cleanUpPipeline(app, full);
	skyBoxEndGraphicsPipeline.cleanUpPipeline(app, full);

	if (full)
	{
		app->device.destroyRenderPass(renderPass);

		for (int i = 0; i < 6; ++i)
		{
			app->device.destroyFramebuffer(renderedCubemapFrameBuffers[i]);
		}
		for (int i = 0; i < 6; ++i)
		{
			app->device.destroyImageView(renderedCubemapImageViews[i]);
		}

		destroySampledTexture(app->device, skyboxTex);
		destroySampledTexture(app->device, moonTex);
		destroySampledTexture(app->device, renderedCubemap);
		destroyBuffer(app->device, uniformBuffer.buffer);

		skybox.cleanup(app->device);
		skysphere.cleanup(app->device);
	}
}


void SkyBoxScene::createFramebuffers(AppResources* app)
{
	renderedCubemapFrameBuffers.resize(renderedCubemapImageViews.size());
	for (int i = 0; i < renderedCubemapImageViews.size(); ++i)
	{
		vk::FramebufferCreateInfo createInfo = {};
		createInfo.renderPass = renderPass;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &renderedCubemapImageViews[i];
		createInfo.width = skyBoxGraphicsPipeline.skybox_cubemap_size;
		createInfo.height = skyBoxGraphicsPipeline.skybox_cubemap_size;
		createInfo.layers = 1;
		renderedCubemapFrameBuffers[i] = app->device.createFramebuffer(createInfo);
	}
}

void SkyBoxScene::createRenderPass(AppResources* app)
{
	vk::AttachmentDescription colorAttachment = {};
	colorAttachment.format = renderedCubemap.format;
	colorAttachment.samples = vk::SampleCountFlagBits::e1;
	colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
	colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
	colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
	colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::AttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::SubpassDescription subPassDescription = {};
	subPassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subPassDescription.colorAttachmentCount = 1;
	subPassDescription.pColorAttachments = &colorAttachmentReference;

	vk::SubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

	vk::RenderPassCreateInfo createInfo = {};
	createInfo.attachmentCount = 1;
	createInfo.pAttachments = &colorAttachment;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subPassDescription;
	createInfo.dependencyCount = 1;
	createInfo.pDependencies = &dependency;

	renderPass = app->device.createRenderPass(createInfo);
}

void SkyBoxScene::updateUniformData(AppResources* app, GuiControl* guiControl)
{
	skyBoxGraphicsPipeline.updateData(app, guiControl);
	skyBoxSphereGraphicsPipeline.updateData(app, guiControl);
	skyBoxCloudGraphicsPipeline.updateData(app, guiControl);
	skyBoxEndGraphicsPipeline.updateData(app, guiControl);
}

void SkyBoxScene::createPipelines(AppResources* app)
{
	skyBoxGraphicsPipeline.generatePipeline(app, false);
	skyBoxSphereGraphicsPipeline.generatePipeline(app, false);
	skyBoxCloudGraphicsPipeline.generatePipeline(app, false);
	skyBoxEndGraphicsPipeline.generatePipeline(app, false);
}

std::vector<Mesh<Vertex>> SkyBoxScene::generateMeshesFromFile(std::string path)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path.c_str(),
											 aiProcess_Triangulate |
											 aiProcess_JoinIdenticalVertices |
											 aiProcess_SortByPType);

	// If the import failed
	if (!scene)
	{
		throw std::exception(importer.GetErrorString());
	}

	std::vector<Mesh<Vertex>> meshes;
	for (size_t i = 0; i < scene->mNumMeshes; ++i)
	{
		const aiMesh* model = scene->mMeshes[i];

		std::vector<uint32_t> indices;
		indices.reserve(model->mNumFaces * 3);
		for (size_t j = 0; j < model->mNumFaces; ++j)
		{
			const aiFace& face = model->mFaces[j];
			if (face.mNumIndices != 3)
			{
				throw std::exception(("degenerate face " + std::string(scene->mName.data)).c_str());
			}
			else
			{
				indices.push_back(face.mIndices[0]);
				indices.push_back(face.mIndices[1]);
				indices.push_back(face.mIndices[2]);
			}
		}

		std::vector<Vertex> vertices;
		vertices.reserve(model->mNumVertices);
		for (size_t j = 0; j < model->mNumVertices; ++j)
		{
			Vertex v;
			if (model->HasPositions())
			{
				v.position = glm::vec3(model->mVertices[j].x, model->mVertices[j].y, model->mVertices[j].z);
			}
			else
			{
				throw std::exception(("no vertex position! " + std::string(scene->mName.data)).c_str());
			}
			if (model->HasNormals())
			{
				v.normal = glm::vec3(model->mNormals[j].x, model->mNormals[j].y, model->mNormals[j].z);
			}

			if (model->HasTextureCoords(0))
			{
				v.texCoord = glm::vec2(model->mTextureCoords[0][j].x, model->mTextureCoords[0][j].y);
			}
			v.materialID = 0;
			vertices.push_back(v);
		}
		meshes.emplace_back(std::string(model->mName.data), vertices, indices);
	}

	return meshes;
}