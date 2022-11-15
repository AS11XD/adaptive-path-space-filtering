
vec3 performPrePassWithRTX(vec3 diffuseColor, vec3 specularColor, vec3 position, vec3 normal, vec3 tangent, vec3 bitangent, float roughness, float metallic, uint materialID, uint triangleID, float depth) 
{
    vec3 color = vec3(0.0);
    rayQueryEXT rayQuery;
    vec3 dir = position - ubo.camPos.xyz;
    vec3 direction = dir;
    vec3 n_position = ubo.camPos.xyz;
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
        //color += texture(t_skybox, direction).rgb;
    }
    else 
    {
        // closest hit reflections
        int instanceID = rayQueryGetIntersectionInstanceIdEXT (rayQuery, true);
        uint triangle = rayQueryGetIntersectionPrimitiveIndexEXT (rayQuery, true);
        vec2 barycentrics = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);
        Vertex v = getInterpolatedVertex(triangle, instanceID, barycentrics);
        Material mat = materials[v.materialID];
        color += pow(texture(textures[mat.diffuseTextureID], v.texCoord).rgb, vec3(p.gamma));
        uvec2 tc = uvec2(gl_FragCoord.xy);
        if (!p.keepCurrentLines && tc.x == p.width / 2 && tc.y == p.height / 2) {
            LineVertex l0, l1;
            l0.position = v.position;
            l0.color = vec3(0.0, 1.0, 0.0);
            l1.position = v.position + normalize(v.normal) * 0.4;
            l1.color = vec3(0.0, 1.0, 0.0);
            appendLine(l0, l1);
            l0.position = v.position;
            l0.color = vec3(1.0, 0.0, 0.0);
            l1.position = v.position + normalize(v.tangent) * 0.4;
            l1.color = vec3(1.0, 0.0, 0.0);
            appendLine(l0, l1);
            l0.position = v.position;
            l0.color = vec3(0.0, 0.0, 1.0);
            l1.position = v.position + normalize(v.bitangent) * 0.4;
            l1.color = vec3(0.0, 0.0, 1.0);
            appendLine(l0, l1);
        }
        normal = v.normal;
    }
    
    return color;
}
