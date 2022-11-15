#pragma once

#include "computePipeline.h"
#include "utils.h"

class HashMapEvictionPipeline : public ComputePipeline
{
public:
	struct PushStruct
	{
		uint32_t hashMapSize;
		uint32_t frameCount;
		uint32_t evictionTime;
		uint32_t maxSamples;
		uint32_t calcHashMapOccupation;
		uint32_t pad[3];
	};

	HashMapEvictionPipeline();
	~HashMapEvictionPipeline();

	void setHashMaps(Buffer* _hashMap, Buffer* _hashMap2, uint32_t _maxHashMapSize);
	void setHashMapOccupation(Buffer* _hashMapOccupation);
	void cleanUpPipeline(AppResources* app, bool full) override;
	void updateDescriptorSets(AppResources* app);

protected:
	void bindPushConstants(vk::CommandBuffer* cb) override;
	void createPipeline(AppResources* app, bool full) override;
	void createDescriptorPool(AppResources* app) override;
	void createDescriptorSet(AppResources* app) override;

	PushStruct pushConstants;
	uint32_t maxHashMapSize;
	Buffer* hashMap;
	Buffer* hashMap2;
	Buffer* hashMapOccupation;
};
