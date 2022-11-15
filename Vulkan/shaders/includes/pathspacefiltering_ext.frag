
vec3 performPathSpaceFiltering(vec3 diffuseColor, vec3 specularColor, vec3 position, vec3 normal, vec3 tangent, vec3 bitangent, float roughness, float metallic, uint materialID, uint triangleID, float depth, float brightness) 
{
    vec3 color = vec3(0.0);
    rayQueryEXT rayQuery;
    Material mat0 = materials[materialID];
    
    Vertex v;
    v.position = position;
    v.normal = normal;
    v.tangent = tangent;
    v.bitangent = bitangent;

    vec3 dir = normalize(v.position - ubo.camPos.xyz);
    uint prev = p.frameCount;

    Vertex v2 = getVertex(indices[3 * triangleID + 0]);
    Vertex v0 = getVertex(indices[3 * triangleID + 1]);
    Vertex v1 = getVertex(indices[3 * triangleID + 2]);
    mat3 _N = mat3(N[materialID]);
    v.trisNormal = normalize(_N * cross(v0.position -  v1.position, v0.position -  v2.position));
    
    bool uJitter = p.gridJitter;
    uvec2 hashMapCellFP = hashMapCellFingerprint(uJitter, prev, v.position, v.normal, v.tangent, v.bitangent, -dir, 0);
    
    switch (p.psfMode)
    {
        case PSFM_DEFAULT:
            break;
        case PSFM_HASH_MAP_OCCUPATION:
            break;
        case PSFM_DEBUG_HASH_CELLS:
            return vec3(hash31(float(hashMapCellFP.x)));
            break;
        case PSFM_DEBUG_HASH_FINGERPRINTS:
            return vec3(hash31(float(hashMapCellFP.y)));
            break;
        default:
            break;
    }   
    
    vec3 diffRad, specRad;
    traceLightRaysSeparate(rayQuery, prev, diffRad, specRad, v, -dir, roughness, metallic);
    color += diffRad + specRad;
    
    vec3 diffCol = vec3(1.0);
    vec3 specCol = vec3(1.0);
    uint iterations = p.iterations;
    vec3 throughput = vec3(1.0);
    float rough = roughness;
    float metal = metallic;
    for (uint i = 0; i < iterations; ++i) {
        float tmp;
        dir = -dir;
        sampleBRDF(prev, throughput, dir, diffCol, specCol, rough, metal, v, tmp);
    
        if (dot(v.trisNormal, dir) <= 0.0 || all(lessThanEqual(throughput, vec3(0.0)))) {
            break;
        }
    
        if (traceRay(rayQuery, v.position, dir, v, 10000.0)) {
            Material mat = materials[v.materialID];
            vec3 baseColor = pow(texture(textures[mat.diffuseTextureID], v.texCoord).rgb, vec3(p.gamma));
            vec2 metrough = texture(textures[mat.specularTextureID], v.texCoord).gb;
            rough = (mat.specularTextureID == 0) ? mat.roughness : metrough.x;
            rough = clamp(rough, 0.01, 1.0);
            metal = (mat.specularTextureID == 0) ? 0.0 : metrough.y;
            diffCol = mix(baseColor, vec3(0.0), metal);
            specCol = mix(vec3(0.04), baseColor, metal);
            color += throughput * traceLightRays(rayQuery, prev, diffCol, specCol, v, -dir, rough, metal);
        }
        else {
            color += (p.sampleEnvironmentMap) ? throughput * sampleSkyBox(dir) : vec3(0.0);
            break;
        }
    }

    insertHashMap(hashMapCellFP, color, brightness);
    
    return vec3(0.0);
}