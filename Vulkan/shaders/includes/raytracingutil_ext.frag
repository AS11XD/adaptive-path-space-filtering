
bool isIntersectionOpaque(uint triangle, uint instanceID, vec2 barycentrics) {
    Vertex v = getInterpolatedVertex(triangle, instanceID, barycentrics);
    Material mat = materials[v.materialID];
    return texture(textures[mat.diffuseTextureID], v.texCoord).a >= 0.1;
}

bool traceRay(rayQueryEXT rayQuery, vec3 origin, vec3 direction, out Vertex v, float maxLength) {
    rayQueryInitializeEXT(rayQuery, accelerationStructure, 0, 0xFF, origin + RAY_OFFSET_EPS * direction, 0.0, direction, maxLength - RAY_OFFSET_EPS);
    
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
        // miss
        return false;
    }
    else 
    {
        // closest hit
        int instanceID = rayQueryGetIntersectionInstanceIdEXT (rayQuery, true);
        uint triangle = rayQueryGetIntersectionPrimitiveIndexEXT (rayQuery, true);
        vec2 barycentrics = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);
        v = getInterpolatedVertex(triangle, instanceID, barycentrics);
        return true;
    }
}