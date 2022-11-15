
// ggx diffuse and specular brdf from https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
vec3 F_Schlick (in vec3 f0 , in float f90 , in float u )
{
    return f0 + ( f90 - f0 ) * pow (1.0 - u , 5.0);
}

float Fr_DisneyDiffuse(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
    float energyBias = mix(0, 0.5, linearRoughness);
    float energyFactor = mix(1.0, 1.0 / 1.51, linearRoughness);
    float fd90 = energyBias + 2.0 * LdotH * LdotH * linearRoughness;
    vec3 f0 = vec3(1.0);
    float lightScatter = F_Schlick(f0 , fd90 , NdotL).r;
    float viewScatter = F_Schlick(f0 , fd90 , NdotV).r;

    return lightScatter * viewScatter * energyFactor ;
}

float V_SmithGGXCorrelated(float NdotL, float NdotV, float alphaG)
{
    // Original formulation of G_SmithGGX Correlated
    // lambda_v = ( -1 + sqrt ( alphaG2 * (1 - NdotL2 ) / NdotL2 + 1)) * 0.5 f;
    // lambda_l = ( -1 + sqrt ( alphaG2 * (1 - NdotV2 ) / NdotV2 + 1)) * 0.5 f;
    // G_SmithGGXCorrelated = 1 / (1 + lambda_v + lambda_l );
    // V_SmithGGXCorrelated = G_SmithGGXCorrelated / (4.0 f * NdotL * NdotV );

    // This is the optimize version
    float alphaG2 = alphaG * alphaG ;
    // Caution : the " NdotL *" and " NdotV *" are explicitely inversed , this is not a mistake .
    float Lambda_GGXV = NdotL * sqrt (( - NdotV * alphaG2 + NdotV ) * NdotV + alphaG2 );
    float Lambda_GGXL = NdotV * sqrt (( - NdotL * alphaG2 + NdotL ) * NdotL + alphaG2 );
    
    return 0.5 / ( Lambda_GGXV + Lambda_GGXL );
}

float D_GGX(float NdotH, float m)
{
    float m2 = m * m ;
    float f = ( NdotH * m2 - NdotH ) * NdotH + 1.0;
    return m2 / (f * f * PI);
}

vec3 specularGGXBRDF(vec3 Ve, vec3 Ne, vec3 Le, float roughness, float metallic) 
{
    float NdotV = abs(dot(Ne , Ve)) + N_DOT_V_OFFSET; // avoid artifact
    vec3 He = normalize (Ve + Le);
    float LdotH = clamp(dot(Le, He), 0.0, 1.0);
    float NdotH = clamp(dot(Ne, He), 0.0, 1.0);
    float NdotL = clamp(dot(Ne, Le), 0.0, 1.0);

    // TODO what shall I pick!!!!
    vec3 f0 = vec3(0.04 * (1.0 - metallic) + metallic);
    float f90 = 1.0;
    //vec3 f0 = vec3(0.8);
	//float f90 = 1.0;
    //vec3 f0 = vec3(0.16 * (1.0 - roughness) * (1.0 - roughness));
    //float f90 = clamp(50.0 * dot(f0, vec3(0.33)), 0.0, 1.0);

    vec3 F = F_Schlick(f0, f90, LdotH);
    float Vis = V_SmithGGXCorrelated(NdotV, NdotL, roughness);
    float D = D_GGX (NdotH, roughness);
    return D * F * Vis;
}

vec3 specularGGXBRDF(vec3 Ve, vec3 Ne, vec3 Le, vec3 albedo, float roughness, float metallic) 
{
    return specularGGXBRDF(Ve, Ne, Le, roughness, metallic) * albedo;
}

vec3 diffuseGGXBRDF(vec3 Ve, vec3 Ne, vec3 Le, float roughness, float metallic) 
{
    float NdotV = abs(dot(Ne , Ve)) + N_DOT_V_OFFSET; // avoid artifact
    vec3 He = normalize (Ve + Le);
    float LdotH = clamp(dot(Le, He), 0.0, 1.0);
    float NdotL = clamp(dot(Ne, Le), 0.0, 1.0);
    
    float linearRoughness = sqrt(roughness);
    return vec3(Fr_DisneyDiffuse(NdotV, NdotL, LdotH, linearRoughness) / PI);
}

vec3 diffuseGGXBRDF(vec3 Ve, vec3 Ne, vec3 Le, vec3 albedo, float roughness, float metallic) 
{
    return diffuseGGXBRDF(Ve, Ne, Le, roughness, metallic) * albedo;
}

vec3 calculateDisneyDiffuse(vec3 view, vec3 albedoDiffuse, vec3 albedoSpecular, vec3 normal, vec3 lightDir, float roughness, float metallic) {    
    return specularGGXBRDF(view, normal, lightDir, albedoSpecular, roughness, metallic)
         + diffuseGGXBRDF(view, normal, lightDir, albedoDiffuse, roughness, metallic);
}

// https://jcgt.org/published/0007/04/01/paper.pdf
// Input Ve: view direction
// Input alpha_x, alpha_y: roughness parameters
// Input U1, U2: uniform random numbers
// Output Ne: normal sampled with PDF D_Ve(Ne) = G1(Ve) * max(0, dot(Ve, Ne)) * D(Ne) / Ve.z
vec3 sampleGGXVNDF(vec3 Ve, float alpha_x, float alpha_y, float U1, float U2)
{
    // Section 3.2: transforming the view direction to the hemisphere configuration
    vec3 Vh = normalize(vec3(alpha_x * Ve.x, alpha_y * Ve.y, Ve.z));
    // Section 4.1: orthonormal basis (with special case if cross product is zero)
    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    vec3 T1 = lensq > 0 ? vec3(-Vh.y, Vh.x, 0) * inversesqrt(lensq) : vec3(1,0,0);
    vec3 T2 = cross(Vh, T1);
    // Section 4.2: parameterization of the projected area
    float r = sqrt(U1);
    float phi = 2.0 * PI * U2;
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5 * (1.0 + Vh.z);
    t2 = (1.0 - s)*sqrt(1.0 - t1*t1) + s*t2;
    // Section 4.3: reprojection onto hemisphere
    vec3 Nh = t1*T1 + t2*T2 + sqrt(max(0.0, 1.0 - t1*t1 - t2*t2))*Vh;
    // Section 3.4: transforming the normal back to the ellipsoid configuration
    vec3 Ne = normalize(vec3(alpha_x * Nh.x, alpha_y * Nh.y, max(0.0, Nh.z)));
    return Ne;
}

float pdfGGXVNDF(vec3 Ve, vec3 Le, float roughness)
{
    vec3 Ne = normalize(Ve + Le);
    float alpha2 = roughness * roughness;
    //D component
    float denom = (Ne.x * Ne.x) / alpha2 +  (Ne.y * Ne.y) / alpha2 + Ne.z * Ne.z;
    float D = 1.0 / (PI * alpha2 * denom * denom);

    // not sure which one to use both give a similar result
    //float lambda_v = (-1.0 + sqrt(alpha2 * (1.0 - NdotL2 ) / NdotL2 + 1.0)) * 0.5;
    float lambda_v = (-1.0 + sqrt(1.0 + (alpha2 * Ve.x * Ve.x + alpha2 * Ve.y * Ve.y) / (Ve.z * Ve.z))) * 0.5;
	
    float G1 = 1.0 / (1.0 + lambda_v);
    float numeratorPDF = G1 * D / Ve.z;
    return numeratorPDF / 4.0;
}

vec3 diffuseBRDF(vec3 albedo) 
{
    return albedo / PI;
}

float diffusePDF(vec3 Le) 
{
    return max(Le.z, 0.0) / PI;
}

vec3 sampleSpecularGGXDirection(vec3 dir, inout uint prev, float rough)
{
    vec2 rnd = vec2(random(prev), random(prev));
    vec3 Ne = sampleGGXVNDF(dir, rough, rough, rnd.x, rnd.y);
    return reflect(-dir, Ne);
}

vec3 sampleDiffuseDirection(inout uint prev)
{
    vec2 rnd = vec2(random(prev), random(prev));
    float sqrndx = sqrt(rnd.x);
    return normalize(vec3(cos(2.0 * PI * rnd.y) * sqrndx, sin(2.0 * PI * rnd.y) * sqrndx, sqrt(1.0 - rnd.x)));
}

vec3 sampleUniformDirection(inout uint prev) 
{   
    vec2 rnd = vec2(random(prev), random(prev));
    float z = rnd.x;
    float tmp = sqrt(1.0 - z*z);
    float sinPhi = sin(2.0 * PI * rnd.y);
    float cosPhi = cos(2.0 * PI * rnd.y);
    return vec3(cosPhi * tmp, sinPhi * tmp, z);
}

float pdfUniform()
{
    return 1.0 / (2.0 * PI);
}

vec3 sampleCombinedDirection(vec3 dir, inout uint prev, float rough) 
{
    return (random(prev) > 0.5) ? sampleDiffuseDirection(prev) : sampleSpecularGGXDirection(dir, prev, rough);
}

float pdfCombined(vec3 dir, vec3 Le, float rough) 
{
    float specPDF = pdfGGXVNDF(dir, Le, rough);
    float diffPDF = diffusePDF(Le);
    return (specPDF + diffPDF) * 0.5;
}

void sampleBRDF(inout uint prev, inout vec3 throughputDiffuse, inout vec3 throughputSpecular, inout vec3 dir, float rough, float metal, Vertex v, out float pdf)
{
    mat3 tangent_space = mat3(v.tangent, v.bitangent, v.normal);
    dir = normalize(transpose(tangent_space) * dir);
    if (dir.z <= 0.0) {
        throughputSpecular *= 0.0;
        throughputDiffuse *= 0.0;
        return;
    }
    vec3 Le;
    vec3 brdf;
    
    switch (p.sampleStrategyBRDF)
    {
        case BRDFSS_UNIFORM:
            Le = sampleUniformDirection(prev);
            pdf = pdfUniform();
            break;
        case BRDFSS_DIFFUSE:
            Le = sampleDiffuseDirection(prev);
            pdf = diffusePDF(Le);
            break;
        case BRDFSS_SPECULAR_GGX:
            Le = sampleSpecularGGXDirection(dir, prev, rough);
            pdf = pdfGGXVNDF(dir, Le, rough);
            break;
        case BRDFSS_DIFFUSE_SPECULAR:
            Le = sampleCombinedDirection(dir, prev, rough);
            pdf = pdfCombined(dir, Le, rough);
            break;
        default:
            break;
    }
    if (Le.z <= 0.0) {
        throughputSpecular *= 0.0;
        throughputDiffuse *= 0.0;
        return;
    }

    vec3 brdfDiffuse = diffuseGGXBRDF(dir, vec3(0.0, 0.0, 1.0), Le, rough, metal);
    vec3 brdfSpecular = specularGGXBRDF(dir, vec3(0.0, 0.0, 1.0), Le, rough, metal);

    float cospdf = max(Le.z, 0.0) / pdf;
    throughputDiffuse *= brdfDiffuse * cospdf;
    throughputSpecular *= brdfSpecular * cospdf;
    dir = normalize(tangent_space * Le); 
}

vec3 sampleBRDFGetWeight(inout uint prev, inout vec3 dir, vec3 diffColor, vec3 specColor, float rough, float metal, Vertex v, out float pdf)
{
    vec3 throughputDiffuse = vec3(1.0);
    vec3 throughputSpecular = vec3(1.0);
    sampleBRDF(prev, throughputDiffuse, throughputSpecular, dir, rough, metal, v, pdf);
    return throughputSpecular * specColor + throughputDiffuse * diffColor;
}

void sampleBRDF(inout uint prev, inout vec3 throughput, inout vec3 dir, vec3 diffColor, vec3 specColor, float rough, float metal, Vertex v, out float pdf)
{
    vec3 throughputDiffuse = vec3(1.0);
    vec3 throughputSpecular = vec3(1.0);
    sampleBRDF(prev, throughputDiffuse, throughputSpecular, dir, rough, metal, v, pdf);
    throughput *= throughputSpecular * specColor + throughputDiffuse * diffColor;
}