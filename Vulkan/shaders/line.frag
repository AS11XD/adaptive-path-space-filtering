#version 450

layout(location = 0) in vec3 color;

layout(location = 0) out vec4 frag_color;

layout(push_constant) uniform PushStruct {
    uint tmp;
} p;

void main() {
    frag_color = vec4(color.rgb, 1.0);
}