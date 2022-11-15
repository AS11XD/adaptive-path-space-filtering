#version 450 

layout(location = 0) in vec3 POSITION;
layout(location = 1) in vec3 NORMAL;
layout(location = 2) in vec2 TEX_COORD;
layout(location = 3) in uint MAT_ID;
layout(location = 4) in vec3 TANGENT;
layout(location = 5) in vec3 BITANGENT;

layout(push_constant) uniform PushStruct {
	float time;
	float sunMoonSize;
	float sunMoonImageSize;
	float moonLuminance;
    mat4 cloudVP;
	mat4 M;
	vec3 moonPos;
	vec3 sunPos;
	uvec4 cameraPos;
} p;

layout(location = 0) out vec3 worldSpacePosition;
layout(location = 1) out vec4 cloudSpacePosition;

layout(binding = 0) uniform matrix_buffer { mat4 matrices[6]; };

void main() {
	vec4 ws = p.M * vec4(POSITION, 1.0);
	worldSpacePosition = ws.xyz / ws.w;
	cloudSpacePosition = p.cloudVP * vec4(worldSpacePosition, 1.0);
	gl_Position = matrices[p.cameraPos.w] * p.M * vec4(POSITION, 1.0);	
}