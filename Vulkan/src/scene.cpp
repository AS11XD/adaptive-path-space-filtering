#include "scene.h"
#include <iostream>
#include <exception>
#include <guiControl.h>
#include <glm/gtx/transform.hpp>
#include "skyBoxScene.h"
#include "window.h"

Scene::Scene(std::string path, AppResources* app, GuiControl* guiControl, SampledTexture* renderedCubemap, bool loadNormalMaps, glm::mat4 _M, glm::vec3 sceneUp, bool mirror) : M(_M)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path.c_str(), sceneLoaderFlags);
	// If the import failed
	if (!scene)
	{
		throw std::exception(importer.GetErrorString());
	}
	directory = findBaseDir(path);

	std::string name = path.substr(directory.size(), path.find(".") - directory.size());
	loadLights(name, scene, app, sceneUp, mirror);
	loadMeshes(name, scene, app, loadNormalMaps, sceneUp, mirror);
	createScene(app, guiControl, renderedCubemap, path);
}

Scene::Scene(std::vector<std::string>& paths, AppResources* app, GuiControl* guiControl, SampledTexture* renderedCubemap, bool loadNormalMaps, glm::mat4 _M, glm::vec3 sceneUp, bool mirror) : M(_M)
{
	std::string combinedName = "";
	for (size_t i = 0; i < paths.size(); ++i)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(paths[i].c_str(), sceneLoaderFlags);

		// If the import failed
		if (!scene)
		{
			throw std::exception(importer.GetErrorString());
		}
		directory = findBaseDir(paths[i]);
		std::string name = paths[i].substr(directory.size(), paths[i].find(".") - directory.size());
		loadLights(name, scene, app, sceneUp, mirror);
		loadMeshes(name, scene, app, loadNormalMaps, sceneUp, mirror);
	}

	createScene(app, guiControl, renderedCubemap, combinedName);
}

Scene::~Scene()
{
}

void Scene::cleanUp(AppResources* app, bool full)
{
	hashMapEvictionPipeline.cleanUpPipeline(app, full);
	defaultPrePassGraphicsPipeline.cleanUpPipeline(app, full);
	defaultDeferredGraphicsPipeline.cleanUpPipeline(app, true);
	filterBrightnessPipeline.cleanUpPipeline(app, true);
	lineGraphicsPipeline.cleanUpPipeline(app, full);
	destroySampledTexture(app->device, brightness);

	if (full)
	{
		app->device.destroyBuffer(uniformBufferPrepass.buffer.buf);
		app->device.freeMemory(uniformBufferPrepass.buffer.mem);
		app->device.destroyBuffer(uniformBufferDeferred.buffer.buf);
		app->device.freeMemory(uniformBufferDeferred.buffer.mem);
		app->device.destroyBuffer(materialBuffer.buf);
		app->device.freeMemory(materialBuffer.mem);
		app->device.destroyBuffer(transformBuffer.buf);
		app->device.freeMemory(transformBuffer.mem);
		app->device.destroyBuffer(normalTransformBuffer.buf);
		app->device.freeMemory(normalTransformBuffer.mem);
		app->device.destroyBuffer(lightBuffer.buf);
		app->device.freeMemory(lightBuffer.mem);
		app->device.destroyBuffer(drawIndirectLinesBuffer.buf);
		app->device.freeMemory(drawIndirectLinesBuffer.mem);
		app->device.destroyBuffer(linesBuffer.buf);
		app->device.freeMemory(linesBuffer.mem);
		app->device.destroyBuffer(varianceInterpolationBuffer.buf);
		app->device.freeMemory(varianceInterpolationBuffer.mem);
		app->device.destroyBuffer(hashMapBuffer.buf);
		app->device.freeMemory(hashMapBuffer.mem);
		app->device.destroyBuffer(hashMapBuffer2.buf);
		app->device.freeMemory(hashMapBuffer2.mem);
		app->device.destroyBuffer(hashMapOccupationBuffer.buf);
		app->device.freeMemory(hashMapOccupationBuffer.mem);
		app->device.destroyBuffer(averagePathDepthBuffer.buf);
		app->device.freeMemory(averagePathDepthBuffer.mem);

		destroyTextures(app);

		meshMerger.cleanup(app->device);

		accelerationStructure.cleanup();
	}
}

void Scene::createPipelines(AppResources* app)
{
	hashMapEvictionPipeline.generatePipeline(app, false);
	defaultPrePassGraphicsPipeline.generatePipeline(app, false);
	filterBrightnessPipeline.generatePipeline(app, true);
	lineGraphicsPipeline.generatePipeline(app, false);
	createBrightnessTexture(app);
	defaultDeferredGraphicsPipeline.setBrightnessTexture(&brightness);
	defaultDeferredGraphicsPipeline.generatePipeline(app, true);
}

void Scene::createScene(AppResources* app, GuiControl* guiControl, SampledTexture* renderedCubemap, const std::string& name)
{
	setCenterOfMassTransforms();
	setTransformations(app, name, true);
	meshMerger = std::move(MeshMerger(*app, meshes));

	createBuffer(app->pDevice, app->device, sizeof(DefaultPrePassGraphicsPipeline::UniformBufferData), vk::BufferUsageFlagBits::eUniformBuffer,
				 vk::MemoryPropertyFlagBits::eHostVisible, "uniform-prepass-" + name, uniformBufferPrepass.buffer);

	createBuffer(app->pDevice, app->device, sizeof(DefaultDeferredGraphicsPipeline::UniformBufferData), vk::BufferUsageFlagBits::eUniformBuffer,
				 vk::MemoryPropertyFlagBits::eHostVisible, "uniform-deferred-" + name, uniformBufferDeferred.buffer);

	createBuffer(app->pDevice, app->device,
				 (uint32_t)(materials.size() * sizeof(materials[0])),
				 vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
				 vk::MemoryPropertyFlagBits::eDeviceLocal, "material-buffer-" + name,
				 materialBuffer.buf, materialBuffer.mem);

	createBuffer(app->pDevice, app->device,
				 (uint32_t)(sizeof(Light) * lights.size()),
				 vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
				 vk::MemoryPropertyFlagBits::eDeviceLocal, "light-buffer-" + name,
				 lightBuffer.buf, lightBuffer.mem);

	createBuffer(app->pDevice, app->device,
				 (uint32_t)(sizeof(vk::DrawIndirectCommand)),
				 vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer | vk::BufferUsageFlagBits::eTransferDst,
				 vk::MemoryPropertyFlagBits::eDeviceLocal, "draw-indirect-lines-buffer-" + name,
				 drawIndirectLinesBuffer.buf, drawIndirectLinesBuffer.mem);

	createBuffer(app->pDevice, app->device,
				 (uint32_t)(sizeof(LineVertex) * 1000),
				 vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
				 vk::MemoryPropertyFlagBits::eDeviceLocal, "lines-buffer-" + name,
				 linesBuffer.buf, linesBuffer.mem);

	createBuffer(app->pDevice, app->device,
				 (uint32_t)(sizeof(VarianceInterpolationColors) * guiControl->varianceColors.size()),
				 vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
				 vk::MemoryPropertyFlagBits::eDeviceLocal, "variance-interpolation-buffer-" + name,
				 varianceInterpolationBuffer.buf, varianceInterpolationBuffer.mem);

	std::vector<VarianceInterpolationColors> varianceInterpolationColors(guiControl->varianceColors.size());
	for (size_t i = 0; i < guiControl->varianceColors.size(); ++i)
	{
		varianceInterpolationColors[i].value = guiControl->varianceValues[i];
		uint32_t cr = (guiControl->varianceColors[i] >> 0) & 0x000000ff;
		uint32_t cg = (guiControl->varianceColors[i] >> 8) & 0x000000ff;
		uint32_t cb = (guiControl->varianceColors[i] >> 16) & 0x000000ff;
		uint32_t ca = (guiControl->varianceColors[i] >> 24) & 0x000000ff;
		varianceInterpolationColors[i].color = glm::vec4(cr, cg, cb, ca) / 255.0f;
	}
	fillDeviceWithStagingBuffer(app->pDevice, app->device, app->transferCommandPool, app->transferQueue,
								varianceInterpolationBuffer, varianceInterpolationColors);

	createBuffer(app->pDevice, app->device,
				 (uint32_t)(sizeof(HashMapCell) * hashMapSize),
				 vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
				 vk::MemoryPropertyFlagBits::eDeviceLocal, "hash-map-buffer-" + name,
				 hashMapBuffer.buf, hashMapBuffer.mem);

	createBuffer(app->pDevice, app->device,
				 (uint32_t)(sizeof(HashMapCell) * hashMapSize),
				 vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
				 vk::MemoryPropertyFlagBits::eDeviceLocal, "hash-map-buffer-2-" + name,
				 hashMapBuffer2.buf, hashMapBuffer2.mem);

	createBuffer(app->pDevice, app->device,
				 (uint32_t)(sizeof(uint32_t)),
				 vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc,
				 vk::MemoryPropertyFlagBits::eDeviceLocal, "hash-map-occupation-buffer-" + name,
				 hashMapOccupationBuffer.buf, hashMapOccupationBuffer.mem);


	createBuffer(app->pDevice, app->device,
				 (uint32_t)(sizeof(uint32_t)),
				 vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc,
				 vk::MemoryPropertyFlagBits::eDeviceLocal, "average-path-length-buffer-" + name,
				 averagePathDepthBuffer.buf, averagePathDepthBuffer.mem);

	clearHashMap(app);

	createBrightnessTexture(app);

	std::vector<vk::DrawIndirectCommand> drawIndirectCommand(1);
	drawIndirectCommand[0].vertexCount = 0;
	drawIndirectCommand[0].instanceCount = 1;
	drawIndirectCommand[0].firstVertex = 0;
	drawIndirectCommand[0].firstInstance = 0;
	fillDeviceWithStagingBuffer(app->pDevice, app->device, app->transferCommandPool, app->transferQueue,
								drawIndirectLinesBuffer, drawIndirectCommand);

	fillDeviceWithStagingBuffer(app->pDevice, app->device, app->transferCommandPool, app->transferQueue,
								materialBuffer, materials);

	std::vector<Light> lightsTmp;
	for (auto& l : lights)
	{
		lightsTmp.push_back(l.light);
	}
	fillDeviceWithStagingBuffer(app->pDevice, app->device, app->transferCommandPool, app->transferQueue,
								lightBuffer, lightsTmp);

	generateAccelerationStructure(app, transforms);
	hashMapEvictionPipeline.setHashMaps(&hashMapBuffer, &hashMapBuffer2, hashMapSize);
	hashMapEvictionPipeline.setHashMapOccupation(&hashMapOccupationBuffer);
	hashMapEvictionPipeline.generatePipeline(app, true);
	defaultPrePassGraphicsPipeline.setUniformBuffer(&uniformBufferPrepass);
	defaultPrePassGraphicsPipeline.setTexturesAndMaterials(&textures, &materialBuffer);
	defaultPrePassGraphicsPipeline.setMeshTransformBuffer(&transformBuffer);
	defaultPrePassGraphicsPipeline.setMeshNormalTransformBuffer(&normalTransformBuffer);
	defaultPrePassGraphicsPipeline.setLineVisualizationBuffers(&drawIndirectLinesBuffer, &linesBuffer);
	defaultPrePassGraphicsPipeline.generatePipeline(app, true);
	defaultDeferredGraphicsPipeline.setUniformBuffer(&uniformBufferDeferred);
	defaultDeferredGraphicsPipeline.setAveragePathDepthBuffer(&averagePathDepthBuffer);
	defaultDeferredGraphicsPipeline.setLightBuffer(&lightBuffer, lights.size());
	defaultDeferredGraphicsPipeline.setAccelerationStructure(&accelerationStructure);
	defaultDeferredGraphicsPipeline.setVertexIndexBuffer(&meshMerger.vertexBuffer, &meshMerger.indexBuffer);
	defaultDeferredGraphicsPipeline.setMeshOffsetBuffer(&meshMerger.offsetBuffer);
	defaultDeferredGraphicsPipeline.setMeshTransformBuffer(&transformBuffer);
	defaultDeferredGraphicsPipeline.setMeshNormalTransformBuffer(&normalTransformBuffer);
	defaultDeferredGraphicsPipeline.setEnvironmentMap(renderedCubemap);
	defaultDeferredGraphicsPipeline.setTexturesAndMaterials(&textures, &materialBuffer);
	defaultDeferredGraphicsPipeline.setLineVisualizationBuffers(&drawIndirectLinesBuffer, &linesBuffer);
	defaultDeferredGraphicsPipeline.setHashMapBuffers(&hashMapBuffer, &hashMapBuffer2, hashMapSize);
	defaultDeferredGraphicsPipeline.setVarianceInterpolationBuffer(&varianceInterpolationBuffer);
	defaultDeferredGraphicsPipeline.setBrightnessTexture(&brightness);
	defaultDeferredGraphicsPipeline.generatePipeline(app, true);
	filterBrightnessPipeline.generatePipeline(app, true);
	lineGraphicsPipeline.setUniformBuffer(&uniformBufferPrepass);
	lineGraphicsPipeline.generatePipeline(app, true);
}

void Scene::updateUniformData(AppResources* app, GuiControl* guiControl)
{
	std::vector<vk::DrawIndirectCommand> drawIndirectCommand(1);
	drawIndirectCommand[0].vertexCount = 0;
	drawIndirectCommand[0].instanceCount = 1;
	drawIndirectCommand[0].firstVertex = 0;
	drawIndirectCommand[0].firstInstance = 0;
	if (!guiControl->var.keepCurrentLines)
	{
		fillDeviceWithStagingBuffer(app->pDevice, app->device, app->transferCommandPool, app->transferQueue,
									drawIndirectLinesBuffer, drawIndirectCommand);
	}

	hashMapEvictionPipeline.updateDescriptorSets(app);
	defaultPrePassGraphicsPipeline.updateData(app, guiControl);
	defaultDeferredGraphicsPipeline.updateData(app, guiControl);
	lineGraphicsPipeline.updateData(app, guiControl);

	guiControl->lights = &lights;
	lights[0].light.position = Sun::position;
	lights[0].light.direction = Sun::direction;
	lights[0].light.intensity = std::max(Sun::intensity * Sun::direction.y, 0.0f);

	lights[1].light.position = Moon::position;
	lights[1].light.direction = Moon::direction;
	lights[1].light.intensity = std::max(Moon::intensity * Moon::direction.y, 0.0f);

	lights[2].light.position = app->windowObj->gui_control.camera.getPosition() + app->windowObj->gui_control.camera.getLookAt() * 0.2f + app->windowObj->gui_control.camera.getSide() * 0.2f;
	lights[2].light.intensity = Torch::intensity;

	std::vector<Light> lightsTmp;
	for (auto& l : lights)
	{
		if (l.light.on && l.light.intensity > 0.01f)
		{
			lightsTmp.push_back(l.light);
		}
	}
	if (lightsTmp.size() > 0)
	{
		fillDeviceWithStagingBuffer(app->pDevice, app->device, app->transferCommandPool, app->transferQueue,
									lightBuffer, lightsTmp);
	}
	defaultDeferredGraphicsPipeline.updateLightCount(lightsTmp.size());

	glm::mat4 t = glm::rotate(glm::radians(std::fmod(guiControl->var.cloudTimer * 20.0f, 360.0f)), glm::vec3(0.0f, 1.0f, 0.0f));
	transformDictionary["Groudon_polygon0_1"] = t;
	transformDictionary["Groudon_polygon1_2"] = t;
	transformDictionary["BistroExterior_Bistro_Research_Exterior_Linde_Tree_Large_linde_tree_large_4051_120"] = t;
	transformDictionary["BistroExterior_Bistro_Research_Exterior_Linde_Tree_Large_linde_tree_large_4051_121"] = t;
	transformDictionary["BistroExterior_Bistro_Research_Exterior_Linde_Tree_Large_linde_tree_large_4051_122"] = t;
	setTransformations(app, "", false);
}

void Scene::generateAccelerationStructure(AppResources* app, std::vector<glm::mat4>& _transforms)
{
	accelerationStructure = AccelerationStructure(app, &meshMerger, _transforms);
}

void Scene::rebuildAccelerationStructure(AppResources* app, vk::CommandBuffer* cb)
{
	accelerationStructure.rebuildAccelerationStructure(&meshMerger, transforms, cb);
}

void Scene::clearHashMap(vk::CommandBuffer* cb, AppResources* app)
{
	std::vector<uint32_t> hashMapOccClear(1);
	hashMapOccClear[0] = 0;
	fillDeviceWithStagingBuffer(app->pDevice, app->device, app->transferCommandPool, app->transferQueue,
								hashMapOccupationBuffer, hashMapOccClear);

	fillDeviceWithStagingBuffer(app->pDevice, app->device, app->transferCommandPool, app->transferQueue,
								averagePathDepthBuffer, hashMapOccClear);

	hashMapEvictionPipeline.dispatch(cb);
}

void Scene::preRenderPass(vk::CommandBuffer* cb, AppResources* app)
{
	defaultPrePassGraphicsPipeline.bind(cb);

	meshMerger.draw(app, cb);
}

void Scene::deferredRenderPass(vk::CommandBuffer* cb, AppResources* app)
{
	defaultDeferredGraphicsPipeline.bind(cb);
	cb->draw(3, 1, 0, 0);
}

void Scene::psfPass(vk::CommandBuffer* cb, AppResources* app)
{
	defaultDeferredGraphicsPipeline.bindPSF(cb);
	cb->draw(3, 1, 0, 0);
}

void Scene::renderLineVisualization(vk::CommandBuffer* cb, AppResources* app)
{
	vk::DeviceSize offset = 0;
	cb->bindVertexBuffers(0, 1, &linesBuffer.buf, &offset);
	lineGraphicsPipeline.bind(cb);
	cb->drawIndirect(drawIndirectLinesBuffer.buf, 0, 1, 0);
}

void Scene::clearHashMap(AppResources* app)
{
	std::vector<HashMapCell> hashMapClear(hashMapSize);
	fillDeviceWithStagingBuffer(app->pDevice, app->device, app->transferCommandPool, app->transferQueue,
								hashMapBuffer, hashMapClear);

	fillDeviceWithStagingBuffer(app->pDevice, app->device, app->transferCommandPool, app->transferQueue,
								hashMapBuffer2, hashMapClear);

	std::vector<uint32_t> hashMapOccClear(1);
	hashMapOccClear[0] = 0;
	fillDeviceWithStagingBuffer(app->pDevice, app->device, app->transferCommandPool, app->transferQueue,
								hashMapOccupationBuffer, hashMapOccClear);

	fillDeviceWithStagingBuffer(app->pDevice, app->device, app->transferCommandPool, app->transferQueue,
								averagePathDepthBuffer, hashMapOccClear);
}

uint32_t Scene::getHashMapOccupation(AppResources* app)
{
	std::vector<uint32_t> result(1);
	fillHostWithStagingBuffer(app->pDevice, app->device, app->transferCommandPool, app->transferQueue, hashMapOccupationBuffer, result);
	return result[0];
}

uint32_t Scene::getAveragePathDepth(AppResources* app)
{
	std::vector<uint32_t> result(1);
	fillHostWithStagingBuffer(app->pDevice, app->device, app->transferCommandPool, app->transferQueue, averagePathDepthBuffer, result);
	return result[0];
}

uint32_t Scene::getHashMapSize()
{
	return hashMapSize;
}

void Scene::destroyTextures(AppResources* app)
{
	for (auto& texture : textures)
	{
		destroySampledTexture(app->device, texture);
	}
}

std::string Scene::findBaseDir(std::string filename)
{
	size_t i;
	for (i = filename.size() - 1; i >= 0; --i)
	{
		if (filename[i] == '/' || filename[i] == '\\')
			break;
	}
	filename.resize(i + 1);
	return filename;
}

glm::mat4 Scene::getTransformationFromNode(aiNode& root, const aiString& name)
{
	aiMatrix4x4 transform;
	aiNode* node = root.FindNode(name);
	aiNode* parent = node;
	while (parent != &root)
	{
		transform = transform * parent->mTransformation;
		parent = parent->mParent;
	}
	transform = transform * root.mTransformation;

	glm::mat4 transformation;
	for (size_t j = 0; j < 4; ++j)
	{
		for (size_t i = 0; i < 4; ++i)
		{
			transformation[i][j] = transform[j][i];
		}
	}
	return transformation;
}

void Scene::setTransformations(AppResources* app, const std::string& name, bool init)
{
	transforms = std::vector<glm::mat4>(meshes.size() + 1);
	normalTransforms = std::vector<glm::mat4>(meshes.size() + 1);
	transforms[0] = glm::mat4(1.0f);
	normalTransforms[0] = glm::mat4(1.0f);
	size_t i = 1;
	for (Mesh<Vertex>& m : meshes)
	{
		glm::mat4& trans = transformDictionary[m.name];
		glm::mat4& comtr = glm::translate(comDictionary[m.name]);
		glm::mat4& comti = glm::inverse(comtr);
		transforms[i] = M * comtr * trans * comti;
		normalTransforms[i] = glm::transpose(glm::inverse(M * transforms[i]));
		++i;
	}

	if (init)
	{
		createBuffer(app->pDevice, app->device, sizeof(glm::mat4) * transforms.size(), vk::BufferUsageFlagBits::eStorageBuffer,
					 vk::MemoryPropertyFlagBits::eHostVisible, "transform-" + name, transformBuffer);

		createBuffer(app->pDevice, app->device, sizeof(glm::mat4) * normalTransforms.size(), vk::BufferUsageFlagBits::eStorageBuffer,
					 vk::MemoryPropertyFlagBits::eHostVisible, "normal-transform-" + name, normalTransformBuffer);
	}
	fillDeviceBuffer(app->device, transformBuffer.mem, transforms);
	fillDeviceBuffer(app->device, normalTransformBuffer.mem, normalTransforms);
}

void Scene::setCenterOfMassTransforms()
{
	for (Mesh<Vertex>& m : meshes)
	{
		glm::vec3 centerOfMass = glm::vec3(0.0f);
		for (Vertex& v : m.vertices)
		{
			centerOfMass += v.position;
		}
		centerOfMass /= m.vertices.size();
		comDictionary[m.name] = centerOfMass;
	}

	comDictionary["Groudon_polygon1_2"] = comDictionary["Groudon_polygon0_1"];
	comDictionary["BistroExterior_Bistro_Research_Exterior_Linde_Tree_Large_linde_tree_large_4051_121"] = comDictionary["BistroExterior_Bistro_Research_Exterior_Linde_Tree_Large_linde_tree_large_4051_120"];
	comDictionary["BistroExterior_Bistro_Research_Exterior_Linde_Tree_Large_linde_tree_large_4051_122"] = comDictionary["BistroExterior_Bistro_Research_Exterior_Linde_Tree_Large_linde_tree_large_4051_120"];
}

void Scene::createBrightnessTexture(AppResources* app)
{
	brightness.format = vk::Format::eR32G32B32A32Sfloat;
	createImage(app, "brightness-texture",
				(app->swapChainExtent.width + 15) / 16,
				(app->swapChainExtent.height + 15) / 16,
				1,
				vk::ImageType::e2D,
				brightness.format,
				vk::ImageLayout::eUndefined,
				vk::ImageTiling::eOptimal,
				vk::ImageCreateFlagBits(),
				vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
				vk::MemoryPropertyFlagBits::eDeviceLocal,
				brightness.image,
				brightness.mem, vk::SampleCountFlagBits::e1);

	createImageView(app, brightness.image, vk::ImageViewType::e2D, 1, 0,
					{ vk::ComponentSwizzle::eIdentity,
					 vk::ComponentSwizzle::eIdentity,
					 vk::ComponentSwizzle::eIdentity,
					 vk::ComponentSwizzle::eIdentity },
					brightness.format, vk::ImageAspectFlagBits::eColor, brightness.imageView);

	vk::SamplerCreateInfo sampler = {};
	sampler.magFilter = vk::Filter::eNearest;
	sampler.minFilter = vk::Filter::eNearest;
	sampler.mipmapMode = vk::SamplerMipmapMode::eNearest;
	sampler.addressModeU = vk::SamplerAddressMode::eClampToBorder;
	sampler.addressModeV = sampler.addressModeU;
	sampler.addressModeW = sampler.addressModeU;
	sampler.mipLodBias = 0.0f;
	sampler.compareOp = vk::CompareOp::eNever;
	sampler.minLod = 0.0f;
	sampler.maxLod = 1;
	sampler.borderColor = vk::BorderColor::eIntTransparentBlack;
	sampler.maxAnisotropy = 1.0f;
	sampler.anisotropyEnable = VK_FALSE;
	app->device.createSampler(&sampler, nullptr, &brightness.sampler);
}

void Scene::loadMeshes(const std::string& name, const aiScene* scene, AppResources* app, bool loadNormalMaps, glm::vec3 sceneUp, bool mirror)
{
	if (textures.size() <= 0)
		textures.push_back(loadTexture2D(app, "res/textures/empty.png"));
	if (materials.size() <= 0)
		materials.push_back(Material());

	meshes.reserve(scene->mNumMeshes);
	for (size_t i = 0; i < scene->mNumMeshes; ++i)
	{
		const aiMesh* model = scene->mMeshes[i];
		const aiMaterial* mtl = scene->mMaterials[model->mMaterialIndex];

		Material mat;
		aiString diffuseTexturePath;
		aiString specularTexturePath;
		aiString normalTexturePath;
		float opacity = 0.0f;
		if (mtl->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS)
		{
			mat.opacity = opacity;
		}
		float roughness = 0.0f;
		if (mtl->Get(AI_MATKEY_SHININESS, roughness) == AI_SUCCESS)
		{
			mat.roughness = glm::sqrt(2.0f / (roughness + 2.0f));
		}
		aiVector3D diffuseColor;
		if (mtl->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == AI_SUCCESS)
		{
			mat.diffuseColor = glm::vec4(diffuseColor.x, diffuseColor.y, diffuseColor.z, 1.0f);
		}

		if ((mtl->GetTextureCount(aiTextureType_DIFFUSE) > 0) && (mtl->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseTexturePath, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS))
		{
			SampledTexture texture;
			texture.name = directory + diffuseTexturePath.data;

			size_t texIndex;
			for (texIndex = 1; texIndex < textures.size(); ++texIndex)
			{
				if (textures[texIndex] == texture)
				{
					break;
				}
			}

			if (texIndex >= textures.size())
			{
				textures.push_back(texture);
				textures[texIndex] = loadTexture2D(app, textures[texIndex].name);
			}

			mat.diffuseTextureID = texIndex;
		}

		if ((mtl->GetTextureCount(aiTextureType_SPECULAR) > 0) && (mtl->GetTexture(aiTextureType_SPECULAR, 0, &specularTexturePath, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS))
		{
			SampledTexture texture;
			texture.name = directory + specularTexturePath.data;

			size_t texIndex;
			for (texIndex = 1; texIndex < textures.size(); ++texIndex)
			{
				if (textures[texIndex] == texture)
				{
					break;
				}
			}

			if (texIndex >= textures.size())
			{
				textures.push_back(texture);
				textures[texIndex] = loadTexture2D(app, textures[texIndex].name);
			}

			mat.specularTextureID = texIndex;
		}

		if (loadNormalMaps && (mtl->GetTextureCount(aiTextureType_NORMALS) > 0) && (mtl->GetTexture(aiTextureType_NORMALS, 0, &normalTexturePath, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS))
		{
			SampledTexture texture;
			texture.name = directory + normalTexturePath.data;

			size_t texIndex;
			for (texIndex = 1; texIndex < textures.size(); ++texIndex)
			{
				if (textures[texIndex] == texture)
				{
					break;
				}
			}

			if (texIndex >= textures.size())
			{
				textures.push_back(texture);
				textures[texIndex] = loadTexture2D(app, textures[texIndex].name);
			}

			mat.normalMapTextureID = texIndex;
		}

		// change if other pipeline is needed (f.e. transparent materials)
		uint32_t pipelineID = 0;
		materials.push_back(mat);

		//-----------Meshes---------------------//
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
				if (!mirror)
				{
					indices.push_back(face.mIndices[0]);
					indices.push_back(face.mIndices[1]);
					indices.push_back(face.mIndices[2]);
				}
				else
				{
					indices.push_back(face.mIndices[0]);
					indices.push_back(face.mIndices[2]);
					indices.push_back(face.mIndices[1]);
				}
			}
		}

		std::vector<Vertex> vertices;
		vertices.reserve(model->mNumVertices);
		for (size_t j = 0; j < model->mNumVertices; ++j)
		{
			Vertex v;
			if (sceneUp == glm::vec3(0.0f, 1.0f, 0.0f))
			{
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
				if (model->HasTangentsAndBitangents())
				{
					v.tangent = glm::vec3(model->mTangents[j].x, model->mTangents[j].y, model->mTangents[j].z);
					v.bitangent = glm::vec3(model->mBitangents[j].x, model->mBitangents[j].y, model->mBitangents[j].z);
				}
			}
			else if (sceneUp == glm::vec3(0.0f, 0.0f, 1.0f))
			{
				if (model->HasPositions())
				{
					v.position = glm::vec3(model->mVertices[j].y, model->mVertices[j].z, model->mVertices[j].x);
				}
				else
				{
					throw std::exception(("no vertex position! " + std::string(scene->mName.data)).c_str());
				}
				if (model->HasNormals())
				{
					v.normal = glm::vec3(model->mNormals[j].y, model->mNormals[j].z, model->mNormals[j].x);
				}
				if (model->HasTangentsAndBitangents())
				{
					v.tangent = glm::vec3(model->mTangents[j].y, model->mTangents[j].z, model->mTangents[j].x);
					v.bitangent = glm::vec3(model->mBitangents[j].y, model->mBitangents[j].z, model->mBitangents[j].x);
				}
			}
			else if (sceneUp == glm::vec3(1.0f, 0.0f, 0.0f))
			{
				if (model->HasPositions())
				{
					v.position = glm::vec3(model->mVertices[j].z, model->mVertices[j].x, model->mVertices[j].y);
				}
				else
				{
					throw std::exception(("no vertex position! " + std::string(scene->mName.data)).c_str());
				}
				if (model->HasNormals())
				{
					v.normal = glm::vec3(model->mNormals[j].z, model->mNormals[j].x, model->mNormals[j].y);
				}
				if (model->HasTangentsAndBitangents())
				{
					v.tangent = glm::vec3(model->mTangents[j].z, model->mTangents[j].x, model->mTangents[j].y);
					v.bitangent = glm::vec3(model->mBitangents[j].z, model->mBitangents[j].x, model->mBitangents[j].y);
				}
			}
			else
			{
				throw std::exception("illegal up vector");
			}
			if (mirror)
			{
				float x = v.position.x;
				v.position.x = v.position.z;
				v.position.z = x;
				v.texCoord.x = 1.0f - v.texCoord.x;
				float nx = v.normal.x;
				v.normal.x = v.normal.z;
				v.normal.z = nx;
				float tx = v.tangent.x;
				v.tangent.x = v.tangent.z;
				v.tangent.z = tx;
				float bx = v.bitangent.x;
				v.bitangent.x = v.bitangent.z;
				v.bitangent.z = bx;
			}

			if (model->HasTextureCoords(0))
			{
				v.texCoord = glm::vec2(model->mTextureCoords[0][j].x, model->mTextureCoords[0][j].y);
			}
			v.materialID = materials.size() - 1;
			vertices.push_back(v);
		}

		std::string n_name = name + "_" + std::string(model->mName.data) + "_" + std::to_string(model->mMaterialIndex);
		meshes.emplace_back(n_name, vertices, indices);
		transformDictionary[n_name] = glm::mat4(1.0f);
		comDictionary[n_name] = glm::vec3(0.0f);
		//std::cout << n_name << "\n";
	}
}

void Scene::loadLights(const std::string& name, const aiScene* scene, AppResources* app, glm::vec3 sceneUp, bool mirror)
{
	if (lights.size() <= 0)
	{
		lights.push_back({ "Sun", { Sun::position, Sun::color, Sun::direction, std::max(Sun::intensity * Sun::direction.y, 0.0f), LIGHT_SOURCE::L_DIRECTIONAL, true } });
		lights.push_back({ "Moon", { Moon::position, Moon::color, Moon::direction, std::max(Moon::intensity * Moon::direction.y, 0.0f), LIGHT_SOURCE::L_DIRECTIONAL, true } });
		lights.push_back({ "Torch", { app->windowObj->gui_control.camera.getPosition() + app->windowObj->gui_control.camera.getLookAt() * 0.2f + app->windowObj->gui_control.camera.getSide() * 0.2f, Torch::color, glm::vec3(0.0f), Torch::intensity, LIGHT_SOURCE::L_POINT, false } });
	}

	for (size_t i = 0; i < scene->mNumLights; ++i)
	{
		const aiLight* light = scene->mLights[i];
		LightObj lobj;
		lobj.name = name + "_" + light->mName.C_Str();
		auto& l = lobj.light;
		glm::vec4 pos = M * glm::vec4(light->mPosition.x, light->mPosition.y, light->mPosition.z, 1.0f);
		l.position = glm::vec3(pos) / pos.w;
		l.direction = M * glm::vec4(-glm::vec3(light->mDirection.x, light->mDirection.y, light->mDirection.z), 0.0f);
		if (mirror)
		{
			float tmp = l.position.x;
			l.position.x = l.position.z;
			l.position.z = tmp;
			tmp = l.direction.x;
			l.direction.x = l.direction.z;
			l.direction.z = tmp;
		}

		glm::vec3 color = glm::vec3(light->mColorDiffuse.r, light->mColorDiffuse.g, light->mColorDiffuse.b);
		l.color = glm::normalize(color);
		l.on = true;
		switch (light->mType)
		{
			case aiLightSource_DIRECTIONAL:
				l.type = LIGHT_SOURCE::L_DIRECTIONAL;
				l.intensity = glm::length(color) * 0.02f;
				break;
			case aiLightSource_POINT:
				l.type = LIGHT_SOURCE::L_POINT;
				l.intensity = glm::length(color) * 0.08f;
				break;
			default:
				l.type = LIGHT_SOURCE::L_POINT;
				break;
		}

		lights.push_back(lobj);
	}
}
