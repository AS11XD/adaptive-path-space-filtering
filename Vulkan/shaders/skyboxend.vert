#version 450

layout(location = 0) in vec3 POSITION;
layout(location = 1) in vec3 NORMAL;
layout(location = 2) in vec2 TEX_COORD;
layout(location = 3) in uint MAT_ID;
layout(location = 4) in vec3 TANGENT;
layout(location = 5) in vec3 BITANGENT;

layout(push_constant) uniform PushStruct {
	mat4 M;
	bool toneMapping;
} p;

layout (location = 0) out vec3 worldSpacePosition;

void main() {
	worldSpacePosition = POSITION;
	gl_Position = p.M * vec4(POSITION, 1.0);
}
