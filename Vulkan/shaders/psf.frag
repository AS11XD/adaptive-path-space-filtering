#version 460
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_ray_query : enable
#extension GL_EXT_control_flow_attributes : require
#extension GL_EXT_shader_atomic_float : enable

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
layout(location = 1) out vec4 varianceOut;
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

void main() {    
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
    vec4 mask = texture(t_ptMask, texCoord);
    varianceOut = calculateVarColor(0.0);
    if (p.psfMode == PSFM_HASH_MAP_OCCUPATION) {
        uvec2 tc = uvec2(gl_FragCoord.xy);
        float coord = float(tc.x + tc.y * p.width) / (p.width * p.height);

        float check = hashMap[uint(coord * p.hashMapSize)].key > 0 ? 1.0 : 0.0;
        vec3 color = vec3(check, 0.0, 0.0);
        returnColor(color, position, normal, materialID, triangleID, depth, brightness);
        return;
    }

    if (materialID == 0) {
        returnColor(baseColor, position, normal, materialID, triangleID, depth, brightness);
        return;
    }
    
    uint prev = p.frameCount;
    uvec2 hashMapCellFP = hashMapCellFingerprint(p.gridJitter, prev, position, normal, tangent, bitangent, normalize(-position + ubo.camPos.xyz), 0);
    //vec3 color = vec3(((cell + p.hashMapSize) - hashMapCellFP.x) % p.hashMapSize) / 100.0;
    HashMapCell hmCell = getHashMapCell(hashMapCellFP);
    
    if (mask.r >= 0.0 && mask.g >= 0.0 && mask.b >= 0.0 && mask.a >= 0.0) {
        if (hmCell.key != 0) {
            float variance = calculateVariance(hmCell);
            varianceOut = calculateVarColor(variance);
            vec3 rad = getRadiance(hmCell);
            vec3 color = max(baseColor, 0.001) * rad;
            returnColor(mix(color, mask.rgb, mask.a), position, normal, materialID, triangleID, depth, brightness);
        }
        else
        {
            returnColor(mask.rgb, position, normal, materialID, triangleID, depth, brightness);
        }
        return;
    }
    
    if (hmCell.key == 0) {
        returnColor(vec3(0.0), position, normal, materialID, triangleID, depth, brightness);
        return;
    }
        
    vec3 rad = getRadiance(hmCell);
    vec3 color = max(baseColor, 0.001) * rad;
    float variance = calculateVariance(hmCell);
    //vec3 color = vec3(hash31(float(hashMap[cell].key)));
    varianceOut = calculateVarColor(variance);
   
    returnColor(color, position, normal, materialID, triangleID, depth, brightness);
}