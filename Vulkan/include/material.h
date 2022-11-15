#pragma once

struct Material
{
	float opacity = 1.0f;
	float roughness = 0.1f;
	uint32_t diffuseTextureID = 0;
	uint32_t specularTextureID = 0;
	uint32_t normalMapTextureID = 0;
	float pad[3];
	glm::vec4 diffuseColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

	bool operator==(const Material& other) const
	{
		return opacity == other.opacity &&
			diffuseColor == other.diffuseColor &&
			normalMapTextureID == other.normalMapTextureID &&
			specularTextureID == other.specularTextureID &&
			diffuseTextureID == other.diffuseTextureID;
	}
};