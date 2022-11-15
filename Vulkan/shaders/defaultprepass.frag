#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 world_position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 tex_coord;
layout(location = 3) in flat uint material_ID;
layout(location = 4) in vec3 tangent;
layout(location = 5) in vec3 bitangent;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec4 frag_position;
layout(location = 2) out vec4 frag_normal;
layout(location = 3) out vec4 frag_tangent;
layout(location = 4) out vec4 frag_bitangent;

#include "includes/material_ext.frag"

layout(push_constant) uniform PushStruct {
    bool drawNormalMaps;
    uint width;
    uint height;
    bool keepCurrentLines;
} p;

struct DrawIndirect {
    uint vertexCount;
    uint instanceCount;
    uint firstVertex;
    uint firstInstance;
};

struct LineVertexSSBO {
    float position[3];
    float color[3];
};

struct LineVertex {
    vec3 position;
    vec3 color;
};

layout(binding = 1) buffer materialBuffer { Material materials[]; };
layout(binding = 2) uniform sampler2D textures[];
layout(binding = 5) buffer drawIndirectLinesBuffer { DrawIndirect drawIndirectLines; };
layout(binding = 6) buffer linesBuffer { LineVertexSSBO lines[]; };

void appendLine(LineVertex l0, LineVertex l1) {
    LineVertexSSBO lssbo0, lssbo1;
    lssbo0.position[0] = l0.position.x;
    lssbo0.position[1] = l0.position.y;
    lssbo0.position[2] = l0.position.z;
    lssbo0.color[0] = l0.color.x;
    lssbo0.color[1] = l0.color.y;
    lssbo0.color[2] = l0.color.z;
    lssbo1.position[0] = l1.position.x;
    lssbo1.position[1] = l1.position.y;
    lssbo1.position[2] = l1.position.z;
    lssbo1.color[0] = l1.color.x;
    lssbo1.color[1] = l1.color.y;
    lssbo1.color[2] = l1.color.z;
    uint idx = atomicAdd(drawIndirectLines.vertexCount, 2);
    lines[idx + 0] = lssbo0;    
    lines[idx + 1] = lssbo1;   
}

void computeNormal(inout vec3 n_normal, inout vec3 n_tangent, inout vec3 n_bitangent, in uint normalTexID) {
    
    if (p.drawNormalMaps && normalTexID != 0) {
        // vec3 dp1 = dFdx(world_position); 
        // vec3 dp2 = dFdy(world_position); 
        // vec2 duv1 = dFdx(tex_coord); 
        // vec2 duv2 = dFdy(tex_coord);
        // vec3 dp2perp = cross( dp2, n_normal );
        // vec3 dp1perp = cross( n_normal, dp1 );
        // float dirCorrection = (duv1.y * duv2.x - duv1.x * duv2.y) < 0.0 ? -1.0 : 1.0;
        // dirCorrection *= 0.5;
        // vec3 T = (dp2perp * duv1.x + dp1perp * duv2.x) * dirCorrection;
        // vec3 B = (dp2perp * duv1.y + dp1perp * duv2.y) * dirCorrection;
        // float invdet = 1.0 / dot(dp1, dp2perp);
        // T *= invdet;
        // B *= invdet;
        //mat3 tangent_space = mat3(T, B, n_normal);
        
        mat3 tangent_space = mat3(n_tangent, n_bitangent, n_normal);
        //uvec2 tc = uvec2(gl_FragCoord.xy);
        //if (!p.keepCurrentLines && tc.x == p.width / 2 && tc.y == p.height / 2) {
        //    LineVertex l0, l1;
        //    l0.position = world_position;
        //    l0.color = vec3(0.0, 1.0, 0.0);
        //    l1.position = world_position + n_normal * 0.4;
        //    l1.color = vec3(0.0, 1.0, 0.0);
        //    appendLine(l0, l1);
        //    l0.position = world_position;
        //    l0.color = vec3(1.0, 0.0, 0.0);
        //    l1.position = world_position + tangent * 0.4;
        //    l1.color = vec3(1.0, 0.0, 0.0);
        //    appendLine(l0, l1);
        //    l0.position = world_position;
        //    l0.color = vec3(0.0, 0.0, 1.0);
        //    l1.position = world_position + bitangent * 0.4;
        //    l1.color = vec3(0.0, 0.0, 1.0);
        //    appendLine(l0, l1);
        //}
        vec3 tangent_space_normal = normalize(vec3(texture(textures[normalTexID], tex_coord).rg * 2.0 - 1.0, 1.0));
        n_normal = normalize(tangent_space * tangent_space_normal);
        n_tangent = normalize(n_tangent - dot(n_normal, n_tangent) * n_normal);
        n_bitangent = normalize(n_bitangent - dot(n_normal, n_bitangent) * n_normal - dot(n_tangent, n_bitangent) * n_tangent);
    }
}

void main() {
    uint diffuseTexID = materials[material_ID].diffuseTextureID;
    uint specularTexID = materials[material_ID].specularTextureID;
    uint normalTexID = materials[material_ID].normalTextureID;
    vec4 diffuseColor = (diffuseTexID == 0) ? materials[material_ID].diffuseColor : texture(textures[diffuseTexID], tex_coord);
    vec4 specularColor = (specularTexID == 0) ? vec4(0.0) : texture(textures[specularTexID], tex_coord);
    float roughness = (specularTexID == 0) ? materials[material_ID].roughness : specularColor.g;
    float metallic = (specularTexID == 0) ? 0.0 : specularColor.b;
    vec3 n_normal = normalize(normal);
    vec3 n_tangent = normalize(tangent);
    vec3 n_bitangent = normalize(bitangent);
    
    computeNormal(n_normal, n_tangent, n_bitangent, normalTexID);

    float opacity = materials[material_ID].opacity;
    if (diffuseColor.a < 0.1)
        discard;

    frag_color = vec4(diffuseColor.rgb, 1.0);
    frag_position = vec4(world_position, uintBitsToFloat(material_ID));
    frag_normal = vec4(n_normal, uintBitsToFloat(uint(gl_PrimitiveID)));
    frag_tangent = vec4(n_tangent, roughness);
    frag_bitangent = vec4(n_bitangent, metallic);
    //frag_color = vec4(vec3(opacity), 1.0);
    //frag_color = vec4((n_normal + 1.0) * 0.5, 1.0);
    //frag_color = vec4(world_position, 1.0);
}