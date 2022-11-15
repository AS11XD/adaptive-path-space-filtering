#version 450

layout (location = 0) in vec3 worldSpacePosition;
layout (location = 0) out vec4 fragColor;

layout(binding = 1) uniform samplerCube cubemap;

void main()
{
	fragColor = vec4(texture(cubemap, worldSpacePosition).rgb, 1.0);
}