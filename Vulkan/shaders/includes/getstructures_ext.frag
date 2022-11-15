
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

Vertex getVertex(uint idx) {
    VertexSSBO vssbo = v[idx];
    Vertex v;
    v.position = vec3(vssbo.position[0], vssbo.position[1], vssbo.position[2]);
    v.normal = vec3(vssbo.normal[0], vssbo.normal[1], vssbo.normal[2]);
    v.texCoord = vec2(vssbo.texCoord[0], vssbo.texCoord[1]);
    v.materialID = vssbo.materialID;
    v.tangent = vec3(vssbo.tangent[0], vssbo.tangent[1], vssbo.tangent[2]);
    v.bitangent = vec3(vssbo.bitangent[0], vssbo.bitangent[1], vssbo.bitangent[2]);
    return v;
}

Vertex getInterpolatedVertex(uint triangle, uint instanceID, vec2 barycentrics) {
    Vertex v;
    vec3 bar = vec3(barycentrics, 1.0 - barycentrics.x - barycentrics.y);
    Vertex v2 = getVertex(indices[3 * triangle + offsets[instanceID] + 0]);
    Vertex v0 = getVertex(indices[3 * triangle + offsets[instanceID] + 1]);
    Vertex v1 = getVertex(indices[3 * triangle + offsets[instanceID] + 2]);
    
    v.position = v0.position * bar.x + v1.position * bar.y + v2.position * bar.z;
    v.normal = v0.normal * bar.x + v1.normal * bar.y + v2.normal * bar.z;
    v.texCoord = v0.texCoord * bar.x + v1.texCoord * bar.y + v2.texCoord * bar.z;
    v.materialID = v0.materialID;
    mat4 _M = mat4(M[v.materialID]);
    vec4 temp_pos = _M * vec4(v.position, 1.0);
    v.position = temp_pos.xyz / temp_pos.w;
    mat3 _N = mat3(N[v.materialID]);
    v.trisNormal = normalize(_N * cross(v0.position -  v1.position, v0.position -  v2.position));
    v.normal = normalize(_N * v.normal);
    v.tangent = v0.tangent * bar.x + v1.tangent * bar.y + v2.tangent * bar.z;
    v.tangent = normalize(_N * v.tangent);
    v.bitangent = v0.bitangent * bar.x + v1.bitangent * bar.y + v2.bitangent * bar.z;
    v.bitangent = normalize(_N * v.bitangent);
    
    if (p.drawNormalMaps) {
        mat3 tangent_space = mat3(v.tangent, v.bitangent, v.normal);
        Material mat = materials[v.materialID];
        if (mat.normalTextureID != 0) {
            vec3 tangent_space_normal = normalize(vec3(texture(textures[mat.normalTextureID], v.texCoord).rg * 2.0 - 1.0, 1.0));
            v.normal = normalize(tangent_space * tangent_space_normal);
            v.tangent = normalize(v.tangent - dot(v.normal, v.tangent) * v.normal);
            v.bitangent = normalize(v.bitangent - dot(v.normal, v.bitangent) * v.normal - dot(v.tangent, v.bitangent) * v.tangent);
        }
    }
    return v;
}

Light getLight(uint idx) {
    LightSSBO lssbo = lights[idx];
    Light l;
    l.position = vec3(lssbo.position[0], lssbo.position[1], lssbo.position[2]);
    l.color = vec3(lssbo.color[0], lssbo.color[1], lssbo.color[2]);
    l.direction = vec3(lssbo.direction[0], lssbo.direction[1], lssbo.direction[2]);
    l.intensity = lssbo.intensity;
    l.type = lssbo.type;
    l.on = lssbo.on;
    return l;    
}
