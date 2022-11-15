
vec3 sampleSkyBox(vec3 dir) {
    vec3 col = (p.tonemapping) ? fromSRGB(vec4(texture(t_skybox, dir).rgb, 1.0)).rgb / 0.2 : texture(t_skybox, dir).rgb;
    col = pow(col, vec3(p.gamma / 1.6));
    return col;        
}

float lightPdf() 
{
    return 1.0 / p.lightCount;
}

void traceLightRaysSeparate(rayQueryEXT rayQuery, inout uint prev, out vec3 diffuseColor, out vec3 specularColor, Vertex v, vec3 view, float roughness, float metallic) {
    diffuseColor = vec3(0.0);
    specularColor = vec3(0.0);
    Light light = getLight(randomUint(prev) % p.lightCount);
    vec3 direction = vec3(0.0);
    float far = 10000.0;
    float falloff = 1.0;
    switch(light.type) {
        case L_DIRECTIONAL:
            direction = normalize(light.direction);
            far = 10000.0;
            falloff = 1.0;
            break;
        case L_POINT:
            direction = normalize(light.position - v.position);
            float distsq = length(light.position - v.position);
            distsq *= distsq;
            falloff = distsq;
            far = length(light.position - v.position) - 0.0001;
            //uvec2 tc = uvec2(gl_FragCoord.xy);
            //if (!p.keepCurrentLines && tc.x == p.width / 2 && tc.y == p.height / 2) {
            //    LineVertex l0, l1;
            //    l0.position = position;
            //    l0.color = vec3(0.0, 1.0, 0.0);
            //    l1.position = light.position;
            //    l1.color = vec3(0.0, 1.0, 0.0);
            //    appendLine(l0, l1);
            //}
                         
            break;
        default:
            return;
    }

    if (dot(v.trisNormal, direction) <= 0.0) {
            return;
    }

    vec3 lightCol = 1.0 / lightPdf() * light.intensity * light.color / falloff;
    specularColor = specularGGXBRDF(normalize(view), v.normal, direction, roughness, metallic) * lightCol;
    diffuseColor = diffuseGGXBRDF(normalize(view), v.normal, direction, roughness, metallic) * lightCol;

    if (traceRay(rayQuery, v.position, direction, v, far)) {
        specularColor = vec3(0.0);
        diffuseColor = vec3(0.0);
    }
}

vec3 traceLightRays(rayQueryEXT rayQuery, inout uint prev, vec3 diffuseColor, vec3 specularColor, Vertex v, vec3 view, float roughness, float metallic) {
    vec3 diffCol, specCol;
    traceLightRaysSeparate(rayQuery, prev, diffCol, specCol, v, view, roughness, metallic);
    return diffCol * diffuseColor + specCol * specularColor;
}
