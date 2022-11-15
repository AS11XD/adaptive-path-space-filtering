#version 450

layout(location = 0) in vec3 POSITION;
layout(location = 1) in vec3 NORMAL;
layout(location = 2) in vec2 TEX_COORD;
layout(location = 3) in uint MAT_ID;
layout(location = 4) in vec3 TANGENT;
layout(location = 5) in vec3 BITANGENT;

layout(location = 0) out vec3 world_position;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec2 tex_coord;
layout(location = 3) out flat uint material_ID;
layout(location = 4) out vec3 tangent;
layout(location = 5) out vec3 bitangent;

layout(push_constant) uniform PushStruct {
    bool drawNormalMaps;
    uint width;
    uint height;
    bool keepCurrentLines;
} p;

layout(binding = 0) uniform UniformBufferObject {
    mat4 VP;
} ubo;

layout(binding = 3) buffer transformBuffer { mat4 T[]; };
layout(binding = 4) buffer normalTransformBuffer { mat4 N[]; };

void main() {
    mat4 M = T[MAT_ID];
    vec4 temp_position = M * vec4(POSITION, 1.0);
    world_position = temp_position.xyz / temp_position.w;
    gl_Position = ubo.VP * M * vec4(POSITION, 1.0);
    mat3 _N = mat3(N[MAT_ID]);
    normal = normalize(_N * NORMAL);
    tex_coord = TEX_COORD;
    tangent = _N * TANGENT;
    bitangent = _N * BITANGENT;
    material_ID = MAT_ID;
}