vec3 calculateItColor(uint iteration) 
{
    vec3 iterColors[11] = 
    {
        vec3(0.001462, 0.000466, 0.013866),
        vec3(0.087411, 0.044556, 0.224813),
        vec3(0.258234, 0.038571, 0.406485),
        vec3(0.416331, 0.090203, 0.432943),
        vec3(0.578304, 0.148039, 0.404411),
        vec3(0.735683, 0.215906, 0.330245),
        vec3(0.865006, 0.316822, 0.226055),
        vec3(0.954506, 0.468744, 0.099874),
        vec3(0.987622, 0.64532, 0.039886),
        vec3(0.964394, 0.843848, 0.273391),
        vec3(0.988362, 0.998364, 0.644924)
    };

    return iterColors[iteration];
}

vec3 performAdaptivePathSpaceFiltering2(vec3 diffuseColor, vec3 specularColor, vec3 position, vec3 normal, vec3 tangent, vec3 bitangent, float roughness, float metallic, uint materialID, uint triangleID, float depth, float brightness) 
{
    rayQueryEXT rayQuery;
    Material mat0 = materials[materialID];
    
    Intersect currentIt;
    currentIt.v.position = position;
    currentIt.v.normal = normal;
    currentIt.v.tangent = tangent;
    currentIt.v.bitangent = bitangent;

    vec3 dir = normalize(currentIt.v.position - ubo.camPos.xyz);
    uint prev = p.frameCount;

    Vertex v2 = getVertex(indices[3 * triangleID + 0]);
    Vertex v0 = getVertex(indices[3 * triangleID + 1]);
    Vertex v1 = getVertex(indices[3 * triangleID + 2]);
    mat3 _N = mat3(N[materialID]);
    currentIt.v.trisNormal = normalize(_N * cross(v0.position -  v1.position, v0.position -  v2.position));
    
    bool uJitter = p.gridJitter;
    uvec2 currentHMFP = hashMapCellFingerprint(uJitter, prev, currentIt.v.position, currentIt.v.normal, currentIt.v.tangent, currentIt.v.bitangent, -dir, 0);
    
    switch (p.psfMode)
    {
        case PSFM_DEFAULT:
            break;
        case PSFM_HASH_MAP_OCCUPATION:
            break;
        case PSFM_DEBUG_HASH_CELLS:
            return vec3(hash31(float(currentHMFP.x)));
            break;
        case PSFM_DEBUG_HASH_FINGERPRINTS:
            return vec3(hash31(float(currentHMFP.y)));
            break;
        default:
            break;
    }    
  
    HashMapCell currentHMC = getHashMapCell2(currentHMFP);
    updateHashMapTimestamp(currentHMFP);

    currentIt.rough = roughness;
    currentIt.metal = metallic;
    float epsUVfHM = 0.0;
    float epsPSR = 0.0;
    float variance = 0.0;
    bool terminated = false;
    float alpha = 1.0;
    vec3 throughputSeparate = vec3(1.0);
    vec3 contribution = vec3(0.0);
    currentIt.diffCol = diffuseColor;
    currentIt.specCol = specularColor;

    if (currentHMC.key == 0) {
        epsPSR = 1.0;
    }
    else 
    {
        caluclateEps(epsUVfHM, epsPSR, calculateVariance(currentHMC));
    }
        
    //epsPSR = subgroupAdd(epsPSR) / subgroupAdd(1.0);
    //
    bool pathSurvived = (p.usePrimaryMethods) ? random(prev) < epsPSR : true;
    //pathSurvived = subgroupBroadcastFirst(pathSurvived);
    uint terminateIt = p.iterations;

    for (uint i = 0; i < p.iterations + 1; ++i) 
    {    
        if (i != 0 && !pathSurvived)
        {
            break;
        }

        
        if (currentHMC.key != 0 && !terminated)
        {
            // check variance
            if (currentHMC.counter >= p.maxSamples)
            {
                vec3 prefix = throughputSeparate * getAlbedo(currentIt);
                float throughput = (prefix.x + prefix.y + prefix.z) / 3.0;
                float varianceLocal = throughput * throughput * calculateVariance(currentHMC);
                if (variance + varianceLocal > p.varianceConst)
                {
                    alpha = sqrt((p.varianceConst - variance) / varianceLocal);
                    contribution += (1.0 - alpha) * throughputSeparate * getRadiance(currentHMC) * getAlbedo(currentIt);
                    
                    terminated = true;
                    variance = p.varianceConst;
                    terminateIt = i;
                }
                else
                {
                    variance += varianceLocal;
                }                
            }
        }
            
        vec3 contributionNEE = traceLightRays(rayQuery, prev, currentIt.diffCol, currentIt.specCol, currentIt.v, -dir, currentIt.rough, currentIt.metal);
        
        contribution += alpha * throughputSeparate * contributionNEE;
    
        
        // indirect
        float tmp;
        dir = -dir;
        vec3 bsdfWeight = sampleBRDFGetWeight(prev, dir, currentIt.diffCol, currentIt.specCol, currentIt.rough, currentIt.metal, currentIt.v, tmp);
        throughputSeparate *= bsdfWeight;
    
        if (dot(currentIt.v.trisNormal, dir) <= 0.0 || any(lessThan(throughputSeparate, vec3(0.0)))) 
        {
            break;
        }
    
        Intersect nextIT;
        nextIT = currentIt;
        if (traceRay(rayQuery, nextIT.v.position, dir, nextIT.v, 10000.0)) 
        {
            Material mat = materials[nextIT.v.materialID];
            vec3 baseColor = pow(texture(textures[mat.diffuseTextureID], nextIT.v.texCoord).rgb, vec3(p.gamma));
            vec2 metrough = texture(textures[mat.specularTextureID], nextIT.v.texCoord).gb;
            nextIT.rough = (mat.specularTextureID == 0) ? mat.roughness : metrough.x;
            nextIT.rough = clamp(nextIT.rough, 0.01, 1.0);
            nextIT.metal = (mat.specularTextureID == 0) ? 0.0 : metrough.y;
            nextIT.diffCol = mix(baseColor, vec3(0.0), nextIT.metal);
            nextIT.specCol = mix(vec3(0.04), baseColor, nextIT.metal);
        }
        else 
        {
            vec3 envmapCol = (p.sampleEnvironmentMap) ? sampleSkyBox(dir) : vec3(0.0);
            contribution += alpha * throughputSeparate * envmapCol;
    
            if (i == 0 || pathSurvived)
            {
                insertHashMap(currentHMFP, (contributionNEE + bsdfWeight * envmapCol) / getAlbedo(currentIt), brightness);
            }
            break;
        }

        uvec2 nextHMFP = hashMapCellFingerprint(uJitter, prev, nextIT.v.position, nextIT.v.normal, nextIT.v.tangent, nextIT.v.bitangent, -dir, i + 1);
        HashMapCell nextHMC = getHashMapCell2(nextHMFP);
        vec3 hmVal = contributionNEE;
        if (nextHMC.counter >= p.maxSamples && nextHMC.key != 0)
        {
            hmVal += bsdfWeight * getRadiance(nextHMC) * getAlbedo(nextIT);
        }
        if (i == 0 || pathSurvived)
        {
            if (!(any(isnan(hmVal))) && !(any(isinf(hmVal))) && !(any(lessThan(hmVal, vec3(0.0)))))
                insertHashMap(currentHMFP, hmVal / getAlbedo(currentIt), brightness);    
        }
        currentHMFP = nextHMFP;
        currentHMC = nextHMC;
        currentIt = nextIT;

        if (terminated) 
        {
            alpha = 0.0;
        }
    }
     
    if (pathSurvived) 
    {
        if (!p.usePrimaryMethods || random(prev) > epsUVfHM)
        {
            // set psf mask
            output2 = vec4(contribution, 1.0);
        }
    }
    
    atomicAdd(averagePathDepth, terminateIt);
    output3 = vec4(calculateItColor(terminateIt) , 1.0);
    
    return vec3(0.0);
}