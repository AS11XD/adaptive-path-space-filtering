#version 450

layout (location = 0) in vec3 worldSpacePosition;
layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 fragPosition;
layout (location = 2) out vec4 fragNormal;
layout (location = 3) out vec4 fragTangent;
layout (location = 4) out vec4 fragBitangent;

layout(binding = 3) uniform samplerCube skyBoxCubemap;

layout(push_constant) uniform PushStruct {
	mat4 M;
	bool toneMapping;
} p;

vec4 fromSRGB(vec4 sRGB)
{
    bvec4 cutoff = lessThan(sRGB, vec4(0.04045));
    vec4 higher = pow((sRGB + vec4(0.055)) / vec4(1.055), vec4(2.4));
    vec4 lower = sRGB / vec4(12.92);

    return mix(higher, lower, cutoff);
}

void main()
{
	vec3 col = (p.toneMapping) ? fromSRGB(vec4(texture(skyBoxCubemap, worldSpacePosition).rgb, 1.0)).rgb : texture(skyBoxCubemap, worldSpacePosition).rgb;
	fragColor = vec4(pow(col, vec3(1.0 / 2.2)), 1.0);
	fragPosition = vec4(0.0);
	fragNormal = vec4(0.0);
	fragTangent = vec4(0.0);
	fragBitangent = vec4(0.0);
}