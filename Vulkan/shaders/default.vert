#version 450

layout(location = 0) out vec3 outDirection;
layout(location = 1) out vec2 texCoord;


#include "includes/push_constants_ext.frag"

layout(binding = 0) uniform UniformBufferObject {
    mat4 VP;
    vec4 camPos;
} ubo;

void main() {
    vec3 POSITION = (gl_VertexIndex == 0) ? vec3(-1.0, -1.0, 0.0) : ((gl_VertexIndex == 1) ? vec3(3.0, -1.0, 0.0) : vec3(-1.0, 3.0, 0.0));

    vec4 tempPosition = ubo.VP * vec4(POSITION, 1.0);
    outDirection = -ubo.camPos.xyz + tempPosition.xyz / tempPosition.w;
    gl_Position = vec4(POSITION, 1.0);
    texCoord = (POSITION.xy + 1.0) * 0.5;
}