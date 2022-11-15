#pragma once

#include <string>
#include <vulkan/vulkan.hpp>
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include <glm/glm.hpp>
#include "sampledTexture.h"
#include "material.h"
#include "light.h"
#include "initialization.h"
#include "defaultPrePassGraphicsPipeline.h"
#include "defaultDeferredGraphicsPipeline.h"
#include "lineGraphicsPipeline.h"
#include "meshMerger.h"
#include "accelerationStructure.h"
#include "hashMapEvictionPipeline.h"
#include "filterBrightnessPipeline.h"

class Scene
{
	static const unsigned int sceneLoaderFlags =
		aiProcess_CalcTangentSpace |
		aiProcess_FlipUVs |
		aiProcess_ImproveCacheLocality |
		aiProcess_PreTransformVertices |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType |
		aiProcess_OptimizeMeshes |
		aiProcess_ValidateDataStructure;

public:
	struct HashMapCell
	{
		uint32_t key = 0;
		uint32_t counter = 0;
		uint32_t timestamp = 0;
		float mom2 = 0.0f;
		float radiance[3];
	};

	struct VarianceInterpolationColors
	{
		float value;
		float padding[3];
		glm::vec4 color;
	};

	Scene(std::string path, AppResources* app, GuiControl* guiControl, SampledTexture* renderedCubemap, bool loadNormalMaps = true, glm::mat4 _M = glm::mat4(1.0f), glm::vec3 sceneUp = glm::vec3(0.0f, 1.0f, 0.0f), bool mirror = false);
	Scene(std::vector<std::string>& paths, AppResources* app, GuiControl* guiControl, SampledTexture* renderedCubemap, bool loadNormalMaps = true, glm::mat4 _M = glm::mat4(1.0f), glm::vec3 sceneUp = glm::vec3(0.0f, 1.0f, 0.0f), bool mirror = false);
	~Scene();
	void cleanUp(AppResources* app, bool full);
	void createPipelines(AppResources* app);
	void createScene(AppResources* app, GuiControl* guiControl, SampledTexture* renderedCubemap, const std::string& name);
	void updateUniformData(AppResources* app, GuiControl* guiControl);
	void generateAccelerationStructure(AppResources* app, std::vector<glm::mat4>& _transforms);
	void rebuildAccelerationStructure(AppResources* app, vk::CommandBuffer* cb);
	void clearHashMap(vk::CommandBuffer* cb, AppResources* app);
	void preRenderPass(vk::CommandBuffer* cb, AppResources* app);
	void deferredRenderPass(vk::CommandBuffer* cb, AppResources* app);
	void psfPass(vk::CommandBuffer* cb, AppResources* app);
	void renderLineVisualization(vk::CommandBuffer* cb, AppResources* app);
	void clearHashMap(AppResources* app);
	uint32_t getHashMapOccupation(AppResources* app);
	uint32_t getAveragePathDepth(AppResources* app);
	uint32_t getHashMapSize();
	std::unordered_map<std::string, glm::mat4> transformDictionary;
	std::unordered_map<std::string, glm::vec3> comDictionary;
	std::vector<glm::mat4> transforms;
	std::vector<glm::mat4> normalTransforms;
	std::vector<SampledTexture> textures;
	std::vector<Material> materials;
	std::vector<LightObj> lights;
	std::vector<Mesh<Vertex>> meshes;
	MeshMerger<Vertex> meshMerger;
	Buffer materialBuffer;
	SampledTexture brightness;
	FilterBrightnessPipeline filterBrightnessPipeline;
private:
	HashMapEvictionPipeline hashMapEvictionPipeline;
	DefaultPrePassGraphicsPipeline defaultPrePassGraphicsPipeline;
	DefaultDeferredGraphicsPipeline defaultDeferredGraphicsPipeline;
	LineGraphicsPipeline lineGraphicsPipeline;
	DefaultPrePassGraphicsPipeline::UniformBuffer uniformBufferPrepass;
	DefaultDeferredGraphicsPipeline::UniformBuffer uniformBufferDeferred;
	Buffer transformBuffer;
	Buffer normalTransformBuffer;
	Buffer lightBuffer;
	glm::mat4 M;
	AccelerationStructure accelerationStructure;
	Buffer drawIndirectLinesBuffer;
	Buffer linesBuffer;
	Buffer hashMapBuffer;
	Buffer hashMapBuffer2;
	Buffer hashMapOccupationBuffer;
	Buffer varianceInterpolationBuffer;
	uint32_t hashMapSize = 10000000;
	Buffer averagePathDepthBuffer;

	std::string directory;
	glm::mat4 getTransformationFromNode(aiNode& root, const aiString& name);
	void setTransformations(AppResources* app, const std::string& name, bool init);
	void setCenterOfMassTransforms();
	void createBrightnessTexture(AppResources* app);
	void loadMeshes(const std::string& name, const aiScene* scene, AppResources* app, bool loadNormalMaps, glm::vec3 sceneUp, bool mirror);
	void loadLights(const std::string& name, const aiScene* scene, AppResources* app, glm::vec3 sceneUp, bool mirror);
	std::string findBaseDir(std::string filename);
	void destroyTextures(AppResources* app);
};