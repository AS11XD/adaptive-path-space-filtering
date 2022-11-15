
vec3 performSampleValidation(vec3 diffuseColor, vec3 specularColor, vec3 position, vec3 normal, vec3 tangent, vec3 bitangent, float roughness, float metallic, uint materialID, uint triangleID, float depth) {
    vec3 dir = vec3(0.0);
    float pdf = 1.0;
    uint prev = p.frameCount;
    
    switch (p.sampleStrategyBRDF)
    {
        case BRDFSS_UNIFORM:
            dir = sampleUniformDirection(prev);
            pdf = pdfUniform();
            break;
        case BRDFSS_DIFFUSE:
            dir = sampleDiffuseDirection(prev);
            pdf = diffusePDF(dir);
            break;
        case BRDFSS_SPECULAR_GGX:
        {
            vec2 rnd = vec2(random(prev), random(prev));
            vec3 view = normalize(vec3(0.5, 0.5, 0.5));
            dir = sampleSpecularGGXDirection(view, prev, 0.1);
            pdf = pdfGGXVNDF(view, dir, 0.1);
            break;
        }
        case BRDFSS_DIFFUSE_SPECULAR:
        {
            vec3 view = normalize(vec3(0.5, 0.5, 0.5));
            dir = sampleCombinedDirection(view, prev, 0.1);
            pdf = pdfCombined(view, dir, 0.1);
            break;
        }
        default:
            break;
    }
    
    return (pdf > 0.0 && dir.z > 0.0) ? vec3(dir / pdf) / (4.0 * PI) : vec3(0.0);
}