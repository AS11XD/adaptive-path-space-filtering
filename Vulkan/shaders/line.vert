#version 450

layout(location = 0) in vec3 POSITION;
layout(location = 1) in vec3 COLOR;

layout(location = 0) out vec3 outColor;

layout(push_constant) uniform PushStruct {
    uint tmp;
} p;

layout(binding = 0) uniform UniformBufferObject {
    mat4 VP;
} ubo;

void main() {
    gl_Position = ubo.VP * vec4(POSITION, 1.0);
    outColor = COLOR;
}