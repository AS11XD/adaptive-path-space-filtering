#version 460
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_ray_query : enable
#extension GL_EXT_control_flow_attributes : require
#extension GL_EXT_shader_atomic_float : enable
#extension GL_KHR_shader_subgroup_vote : enable
#extension GL_KHR_shader_subgroup_arithmetic : enable
#extension GL_KHR_shader_subgroup_ballot : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 VP;
    vec4 camPos;
} ubo;

#include "includes/push_constants_ext.frag"
#include "includes/constants_ext.frag"
#include "includes/random_ext.frag"
#include "includes/hashmap_ext.frag"
#include "includes/srgb_ext.frag"
#include "includes/vertex_ext.frag"
#include "includes/material_ext.frag"
#include "includes/light_ext.frag"
#include "includes/line_ext.frag"
#include "includes/brdf_ext.frag"

layout(location = 0) in vec3 inDirection;
layout(location = 1) in vec2 texCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 output2;
layout(location = 2) out vec4 output3;

struct DrawIndirect {
    uint vertexCount;
    uint instanceCount;
    uint firstVertex;
    uint firstInstance;
};

struct VIColor
{
    float value; 
    vec4 color;
};

layout(binding = 1) buffer materialBuffer { Material materials[]; };
layout(binding = 2) uniform sampler2D textures[];
layout(binding = 3) uniform sampler2D t_color;
layout(binding = 4) uniform sampler2D t_position;
layout(binding = 5) uniform sampler2D t_normal;
layout(binding = 6) uniform sampler2D t_tangent;
layout(binding = 7) uniform sampler2D t_bitangent;
layout(binding = 8) uniform sampler2D t_depth;
layout(binding = 9) uniform samplerCube t_skybox;
layout(binding = 10) buffer lightBuffer { LightSSBO lights[]; };
layout(binding = 11) buffer vertexBuffer { VertexSSBO v[]; };
layout(binding = 12) buffer indexBuffer { uint indices[]; };
layout(binding = 13) buffer offsetBuffer { uint offsets[]; };
layout(binding = 14) buffer transformBuffer { mat4 M[]; };
layout(binding = 15) buffer normalTransformBuffer { mat4 N[]; };
layout(binding = 16) uniform accelerationStructureEXT accelerationStructure;
layout(binding = 17) buffer drawIndirectLinesBuffer { DrawIndirect drawIndirectLines; };
layout(binding = 18) buffer linesBuffer { LineVertexSSBO lines[]; };
layout(binding = 19) buffer hashMapBuffer { HashMapCell hashMap[]; };
layout(binding = 20) buffer hashMapBuffer2 { HashMapCell hashMap2[]; };
layout(binding = 21) uniform sampler2D t_brightness;
layout(binding = 22) buffer varInterpolBuffer { VIColor viColors[]; };
layout(binding = 23) uniform sampler2D t_ptMask;
layout(binding = 24) buffer averagePathDepthBuffer { uint averagePathDepth; };

#include "includes/getstructures_ext.frag"
#include "includes/gethashmap_ext.frag"
#include "includes/returncolor_ext.frag"
#include "includes/varcolor_ext.frag"
#include "includes/raytracingutil_ext.frag"
#include "includes/lightsampling_ext.frag"
#include "includes/raytracing_ext.frag"
#include "includes/rtxprepass_ext.frag"
#include "includes/pathtracing_ext.frag"
#include "includes/pathspacefiltering_ext.frag"
#include "includes/adaptivepathspacefiltering1_ext.frag"
#include "includes/adaptivepathspacefiltering2_ext.frag"
#include "includes/samplevalidation_ext.frag"

void main() {    
    output2 = vec4(-1.0);
    output3 = vec4(0.0);
    vec3 baseColor = pow(texture(t_color, texCoord).rgb, vec3(p.gamma));
    vec4 positionMID = texture(t_position, texCoord);
    vec4 normalTID = texture(t_normal, texCoord);
    uint materialID = floatBitsToUint(positionMID.w);
    uint triangleID = floatBitsToUint(normalTID.w);
    vec3 position = positionMID.xyz;
    vec3 normal = normalize(normalTID.xyz);
    vec4 tangentR = texture(t_tangent, texCoord);
    vec3 tangent = normalize(tangentR.rgb);
    float roughness = clamp(tangentR.a, 0.01, 1.0);
    vec4 bitangentM = texture(t_bitangent, texCoord);
    vec3 bitangent = normalize(bitangentM.rgb);
    float metallic = bitangentM.a;
    vec3 diffuseColor = mix(baseColor, vec3(0.0), metallic);
    vec3 specularColor = mix(vec3(0.04), baseColor, metallic);
    float depth = texture(t_depth, texCoord).r;
    float brightness = texture(t_brightness, texCoord).r;

    if (p.renderState != RS_G_BUFFER &&
        p.renderMode == RM_SAMPLE_VALIDATION &&
        gl_FragCoord.x >= p.width / 2 - 20.0 && gl_FragCoord.x <= p.width / 2 + 20.0 &&
        gl_FragCoord.y >= p.height / 2 - 20.0 && gl_FragCoord.y <= p.height / 2 + 20.0) {
        
        returnColor(vec3(0.5), position, normal, materialID, triangleID, depth, brightness);
        return;
    }

    if (materialID == 0) {
        output3 = vec4(diffuseColor, 1.0);
        returnColor(diffuseColor, position, normal, materialID, triangleID, depth, brightness);
        return;
    }
    
    vec3 color = vec3(1.0);
    if (p.renderState != RS_G_BUFFER) {
        switch (p.renderMode) {
            case RM_DIFFUSE_COLOR:
                color = diffuseColor;
                break;
            case RM_SPECULAR_COLOR:
                color = specularColor;
                break;
            case RM_RTX_PREPASS:
                color = performPrePassWithRTX(diffuseColor, specularColor, position, normal, tangent, bitangent, roughness, metallic, materialID, triangleID, depth);
                break;
            case RM_RAY_TRACING:
                color = performRayTracing(diffuseColor, specularColor, position, normal, tangent, bitangent, roughness, metallic, materialID, triangleID, depth);
                break;
            case RM_PATH_TRACING:
                color = performPathTracing(diffuseColor, specularColor, position, normal, tangent, bitangent, roughness, metallic, materialID, triangleID, depth);
                break;
            case RM_PATH_SPACE_FILTERING:
                color = performPathSpaceFiltering(diffuseColor, specularColor, position, normal, tangent, bitangent, roughness, metallic, materialID, triangleID, depth, brightness);
                break;
            case RM_ADAPTIVE_PATH_SPACE_FILTERING_1:
                color = performAdaptivePathSpaceFiltering1(diffuseColor, specularColor, position, normal, tangent, bitangent, roughness, metallic, materialID, triangleID, depth, brightness);
                break;
            case RM_ADAPTIVE_PATH_SPACE_FILTERING_2:
                color = performAdaptivePathSpaceFiltering2(diffuseColor, specularColor, position, normal, tangent, bitangent, roughness, metallic, materialID, triangleID, depth, brightness);
                break;
            case RM_SAMPLE_VALIDATION:
                color = performSampleValidation(diffuseColor, specularColor, position, normal, tangent, bitangent, roughness, metallic, materialID, triangleID, depth);
                break;
            default:
                color = vec3(1.0);
                break;
        }
    }
    returnColor(color, position, normal, materialID, triangleID, depth, brightness);
}