
float oneOverXSqFunction(float x, float xStart, float xEnd, float varianceThresholds[2])
{
	float xVal = (x - xStart) / (xEnd - xStart);
	xVal -= ((varianceThresholds[1] + varianceThresholds[0]) * 0.5) / (xEnd - xStart);
	float scal = (varianceThresholds[1] - varianceThresholds[0]) / (xEnd - xStart);
	xVal /= scal * 2.0;
	float y = -(1.0 - scal * 0.2) / (xVal * xVal * 16.0 + 1.0) + 1.0;
	y = clamp(y, 0.0, 1.0);
	return y;
}

float smoothLinearFunction(float x, float xStart, float xEnd, float varianceThresholds[2])
{
	float xVal = (x - xStart) / (xEnd - xStart);
	float scal = (varianceThresholds[1] - varianceThresholds[0]) / (xEnd - xStart);
	float mid = (varianceThresholds[1] + varianceThresholds[0]) * 0.5 / (xEnd - xStart);
	float y = smoothstep(0.0, (1.0 - scal * 0.2), (xVal - mid) / (scal * 5.0) + (1.0 - scal * 0.2) * 0.5);
	y = clamp(y, 0.0, 1.0);
    return y;
}

float stepFunction(float x, float xStart, float xEnd, float varianceThreshold[2])
{
	return (x >= varianceThreshold[0] && x <= varianceThreshold[1]) ? 0.0 : 1.0;
}

float linearFunction(float x, float xStart, float xEnd, float varianceThreshold[2])
{
	return clamp((x - varianceThreshold[0]) / (varianceThreshold[1] - varianceThreshold[0]), 0.0, 1.0);
}

float oneFunction(float x, float xStart, float xEnd, float varianceThreshold[2])
{
	return 1.0;
}

float zeroFunction(float x, float xStart, float xEnd, float varianceThreshold[2])
{
	return 0.0;
}

struct Intersect
{
    Vertex v;
    vec3 diffCol;
    vec3 specCol;
    float rough;
    float metal;    
};

vec3 getAlbedo(in Intersect i)
{
    return max(i.diffCol + i.specCol, vec3(0.001));
}

void caluclateEps(inout float epsUVfHM, inout float epsPSR, in float variance)
{
    float xStart = viColors[0].value;
    float xEnd = viColors[p.varianceInterpolationSize - 1].value;
    switch (p.varianceFuncitonUVfHM)
    {
        case VFT_ONE_OVER_X_SQUARED:
            epsUVfHM = oneOverXSqFunction(clamp(variance, xStart, xEnd), xStart, xEnd, p.varianceThresholdsUVfHM);
            break;
        case VFT_SMOOTH_LINEAR:
            epsUVfHM = smoothLinearFunction(clamp(variance, xStart, xEnd), xStart, xEnd, p.varianceThresholdsUVfHM);
            break;
        case VFT_STEP:
            epsUVfHM = stepFunction(clamp(variance, xStart, xEnd), xStart, xEnd, p.varianceThresholdsUVfHM);
            break;
        case VFT_LINEAR:
            epsUVfHM = linearFunction(clamp(variance, xStart, xEnd), xStart, xEnd, p.varianceThresholdsUVfHM);
            break;
        case VFT_ONE:
            epsUVfHM = oneFunction(clamp(variance, xStart, xEnd), xStart, xEnd, p.varianceThresholdsUVfHM);
            break;
        case VFT_ZERO:
            epsUVfHM = zeroFunction(clamp(variance, xStart, xEnd), xStart, xEnd, p.varianceThresholdsUVfHM);
            break;
        default:
            break;
    }
    switch (p.varianceFunctionPSR)
    {
        case VFT_ONE_OVER_X_SQUARED:
            epsPSR = oneOverXSqFunction(clamp(variance, xStart, xEnd), xStart, xEnd, p.varianceThresholdsPSR);
            break;
        case VFT_SMOOTH_LINEAR:
            epsPSR = smoothLinearFunction(clamp(variance, xStart, xEnd), xStart, xEnd, p.varianceThresholdsPSR);
            break;
        case VFT_STEP:
            epsPSR = stepFunction(clamp(variance, xStart, xEnd), xStart, xEnd, p.varianceThresholdsPSR);
            break;
        case VFT_LINEAR:
            epsPSR = linearFunction(clamp(variance, xStart, xEnd), xStart, xEnd, p.varianceThresholdsPSR);
            break;
        case VFT_ONE:
            epsPSR = oneFunction(clamp(variance, xStart, xEnd), xStart, xEnd, p.varianceThresholdsPSR);
            break;
        case VFT_ZERO:
            epsPSR = zeroFunction(clamp(variance, xStart, xEnd), xStart, xEnd, p.varianceThresholdsPSR);
            break;
        default:
            break;
    }
}

vec3 performAdaptivePathSpaceFiltering1(vec3 diffuseColor, vec3 specularColor, vec3 position, vec3 normal, vec3 tangent, vec3 bitangent, float roughness, float metallic, uint materialID, uint triangleID, float depth, float brightness) 
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
  
    HashMapCell hmCell = getHashMapCell2(hashMapCellFP);
    updateHashMapTimestamp(hashMapCellFP);

    float epsUVfHM = 0.0;
    float epsPSR = 0.0;
    if (hmCell.key == 0) {
        // TODO do something else in this case as the cell could not be found
        epsPSR = 1.0;
    }
    else 
    {
        float variance = calculateVariance(hmCell);
        caluclateEps(epsUVfHM, epsPSR, variance);
    }
    
    //epsPSR = subgroupAdd(epsPSR) / subgroupAdd(1.0);
    //
    bool pathSurvived = random(prev) < epsPSR;
    //pathSurvived = subgroupBroadcastFirst(pathSurvived);
    

    if (p.showPTComparison || pathSurvived)
    {
        vec3 diffRad, specRad;
        traceLightRaysSeparate(rayQuery, prev, diffRad, specRad, v, -dir, roughness, metallic);
        
        vec3 diffCol = vec3(1.0);
        vec3 specCol = vec3(1.0);
        uint iterations = p.iterations;
        vec3 throughput = vec3(1.0);
        vec3 throughputD = vec3(1.0);
        vec3 throughputS = vec3(1.0);
        float rough = roughness;
        float metal = metallic;
        for (uint i = 0; i < iterations; ++i) {
            float tmp;
            dir = -dir;
            vec3 throughputDiffuse = vec3(1.0);
            vec3 throughputSpecular = vec3(1.0);
            sampleBRDF(prev, throughputDiffuse, throughputSpecular, dir, rough, metal, v, tmp);
            if (i == 0)
            {
                throughputD = throughputDiffuse;
                throughputS = throughputSpecular;
            }
            else 
            {
                throughput *= throughputSpecular * specCol + throughputDiffuse * diffCol;
            }
        
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

        
        if (pathSurvived) 
        {        
            insertHashMap(hashMapCellFP, color * (throughputD + throughputS) + diffRad + specRad, brightness);
            output2 = vec4(color * (throughputD * diffuseColor + throughputS * specularColor) + diffRad * diffuseColor + specRad * specularColor, 1.0 - epsUVfHM);
        }

        output3 = vec4(color * (throughputD * diffuseColor + throughputS * specularColor) + diffRad * diffuseColor + specRad * specularColor, 1.0);
    }
    
    return vec3(0.0);
}