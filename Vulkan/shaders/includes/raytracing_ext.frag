
vec3 performRayTracing(vec3 diffuseColor, vec3 specularColor, vec3 position, vec3 normal, vec3 tangent, vec3 bitangent, float roughness, float metallic, uint materialID, uint triangleID, float depth) 
{
    vec3 color = vec3(0.0);
    rayQueryEXT rayQuery;
    uint iterations = p.iterations;
    vec3 dir = position - ubo.camPos.xyz;
    vec3 direction = reflect(normalize(dir), normalize(normal));
    vec3 n_position = position;
    uint prev = p.frameCount;
    for (int i = 0; i < iterations; ++i) {
        //Light l = getLight(0);
        //rayQueryInitializeEXT(rayQuery, accelerationStructure, gl_RayFlagsNoOpaqueEXT, 0xFF, n_position + 0.0001 * direction, 0.0, direction, 10000.0);
        rayQueryInitializeEXT(rayQuery, accelerationStructure, 0, 0xFF, n_position + RAY_OFFSET_EPS * direction, 0.0, direction, 10000.0);
        
        while(rayQueryProceedEXT(rayQuery)) 
        {
            // any hit
            int instanceID = rayQueryGetIntersectionInstanceIdEXT (rayQuery, false);
            uint triangle = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, false);
            vec2 barycentrics = rayQueryGetIntersectionBarycentricsEXT(rayQuery, false);
            if (isIntersectionOpaque(triangle, instanceID, barycentrics))
                rayQueryConfirmIntersectionEXT(rayQuery);
        }
        uint intersectionType = rayQueryGetIntersectionTypeEXT(rayQuery, true);
        if (intersectionType == gl_RayQueryCommittedIntersectionNoneEXT) 
        {
            // miss refelcted towards skybox
            //color += diffuseColor;
            // sample skybox
            color += sampleSkyBox(direction);
            break;
        }
        else 
        {
            // closest hit reflections
            int instanceID = rayQueryGetIntersectionInstanceIdEXT (rayQuery, true);
            uint triangle = rayQueryGetIntersectionPrimitiveIndexEXT (rayQuery, true);
            vec2 barycentrics = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);
            Vertex v = getInterpolatedVertex(triangle, instanceID, barycentrics);
            Material mat = materials[v.materialID];
            direction = normalize(reflect(normalize(direction), normalize(v.normal)));
            n_position = v.position;
            if (i == iterations - 1) 
            {
                vec3 baseColor = pow(texture(textures[mat.diffuseTextureID], v.texCoord).rgb, vec3(p.gamma));
                vec2 metrough = texture(textures[mat.specularTextureID], v.texCoord).gb;
                float rough = (mat.specularTextureID == 0) ? mat.roughness : metrough.x;
                float metal = (mat.specularTextureID == 0) ? 0.0 : metrough.y;
                vec3 diffCol = mix(baseColor, vec3(0.0), metal);
                vec3 specCol = mix(vec3(0.04), baseColor, metal);
                color += traceLightRays(rayQuery, prev, diffCol, specCol, v, direction, rough, metal);
            }
        }
    }
    return color;
}