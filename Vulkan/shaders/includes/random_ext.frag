
//https://www.shadertoy.com/view/4djSRW
vec3 hash31(float p)
{
   vec3 p3 = fract(vec3(p) * vec3(.1031, .1030, .0973));
   p3 += dot(p3, p3.yzx+33.33);
   return fract((p3.xxy+p3.yzz)*p3.zyx); 
}

//https://www.shadertoy.com/view/4djSRW
float hash13(vec3 p3)
{
	p3  = fract(p3 * .1031);
    p3 += dot(p3, p3.zyx + 31.32);
    return fract((p3.x + p3.y) * p3.z);
}

//https://www.shadertoy.com/view/4djSRW
float hash14(vec4 p4)
{
	p4 = fract(p4  * vec4(.1031, .1030, .0973, .1099));
    p4 += dot(p4, p4.wzxy+33.33);
    return fract((p4.x + p4.y) * (p4.z + p4.w));
}

uint tea(uint val0, uint val1)
{
  uint v0 = val0;
  uint v1 = val1;
  uint s0 = 0;

  [[unroll]]
  for(uint n = 0; n < 16; n++)
  {
    s0 += 0x9e3779b9;
    v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
    v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
  }

  return v0;
}

float random(inout uint prev)
{
    uint seedX = uint(gl_FragCoord.x);
    uint seedY = uint(gl_FragCoord.y);
    uint seed = bitfieldInsert(seedX, seedY, 16, 16);
    prev = tea(seed, prev);
    return (uintBitsToFloat((prev & 0x007FFFFFu) | 0x3F800000u) - 1.0);
    //return vec3(prev & 0x00FFFFFF) / vec3(0x00FFFFFF);
}

uint randomUint(inout uint prev)
{
    uint seedX = uint(gl_FragCoord.x);
    uint seedY = uint(gl_FragCoord.y);
    uint seed = bitfieldInsert(seedX, seedY, 16, 16);
    prev = tea(seed, prev);
    return prev;
}

/**
 * Generate a uniformly distributed random point on the unit-sphere.
 *
 * After:
 * http://mathworld.wolfram.com/SpherePointPicking.html
 */
vec3 spherePoint(vec3 rand)
{
    float ang1 = (rand.x + 1.0) * PI; // [-1..1) -> [0..2*PI)
    float u = rand.y; // [-1..1), cos and acos(2v-1) cancel each other out, so we arrive at [-1..1)
    float u2 = u * u;
    float sqrt1MinusU2 = sqrt(1.0 - u2);
    float x = sqrt1MinusU2 * cos(ang1);
    float y = sqrt1MinusU2 * sin(ang1);
    float z = u;
    return vec3(x, y, z);
}

vec3 hemiSpherePoint(vec3 rand, vec3 n)
{
    vec3 rand2 = rand * 2.0 - 1.0; // [0..1) -> [-1..1)
    vec3 v = spherePoint(rand2);
    return v * sign(dot(v, n));
}

uint getOctahedronFaceIndex(vec3 direction)
{
    uint idx = 0;
    if (direction.x > 0.0)
        idx |= 1;
    if (direction.y > 0.0)
        idx |= 2;
    if (direction.z > 0.0)
        idx |= 4;
    return idx;
}

int level_of_detail(vec3 position) 
{
    vec3 camPos = ubo.camPos.xyz;
    float dist = length(camPos - position);
    return int(log2(dist));
}

vec3 jitterDisk(inout uint prev, vec3 tangent, vec3 bitangent) 
{
    float r = random(prev);
    float phi = random(prev) * 2.0 * PI;
    vec2 uniformDisk = r * vec2(cos(phi), sin(phi));
    return tangent * uniformDisk.x + bitangent * uniformDisk.y;
}

ivec3 getDiscretePosition(bool useJitter, inout uint prev, vec3 position, vec3 tangent, vec3 bitangent)
{
    int lod = level_of_detail(position);
    float uj = (useJitter) ? 1.0 : 0.0;
    vec3 n_pos = position + uj * jitterDisk(prev, tangent, bitangent) * p.gridScale * exp2(lod);
    int n_lod = level_of_detail(n_pos);
    return ivec3(floor(vec3(n_pos / (p.gridScale * exp2(n_lod)))));
}

uvec2 hashMapCellFingerprint(bool useJitter, inout uint prev, vec3 position, vec3 normal, vec3 tangent, vec3 bitangent, vec3 incidentDirection, uint indirection) 
{
    ivec3 positionDiscrete = getDiscretePosition(useJitter, prev, position, tangent, bitangent);
    uint hash = tea(positionDiscrete.x, positionDiscrete.y);
    hash = tea(hash, positionDiscrete.z);
    hash = tea(hash, indirection);

    uint normalDiscrete = getOctahedronFaceIndex(normal);
    //uint incidentDirectionDiscrete = getOctahedronFaceIndex(incidentDirection);
    //uint hash2 = tea(normalDiscrete, incidentDirectionDiscrete);
    uint hash2 = tea(normalDiscrete, positionDiscrete.x);
    hash2 = tea(hash2, positionDiscrete.y);
    hash2 = tea(hash2, positionDiscrete.z);
    hash2 = tea(hash2, indirection);

    return uvec2(hash % p.hashMapSize, hash2 % p.hashMapSize);
}
