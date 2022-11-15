#version 450

layout(location = 0) in vec3 worldSpacePosition;
layout(location = 1) in vec4 cloudSpacePosition;

layout(push_constant) uniform PushStruct {
	float time;
	float sunMoonSize;
	float sunMoonImageSize;
	float moonLuminance;
    mat4 cloudVP;
	mat4 M;
	vec3 moonPos;
	vec3 sunPos;
	vec3 cameraPos;
} p;

layout(location = 0) out vec4 fragColor;

//	Classic Perlin 2D Noise 
//	by Stefan Gustavson
//  https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83

vec2 fade(vec2 t) {return t*t*t*(t*(t*6.0-15.0)+10.0);}
vec4 permute(vec4 x){return mod(((x*34.0)+1.0)*x, 289.0);}

float cnoise(vec2 P){
  vec4 Pi = floor(P.xyxy) + vec4(0.0, 0.0, 1.0, 1.0);
  vec4 Pf = fract(P.xyxy) - vec4(0.0, 0.0, 1.0, 1.0);
  Pi = mod(Pi, 289.0); // To avoid truncation effects in permutation
  vec4 ix = Pi.xzxz;
  vec4 iy = Pi.yyww;
  vec4 fx = Pf.xzxz;
  vec4 fy = Pf.yyww;
  vec4 i = permute(permute(ix) + iy);
  vec4 gx = 2.0 * fract(i * 0.0243902439) - 1.0; // 1/41 = 0.024...
  vec4 gy = abs(gx) - 0.5;
  vec4 tx = floor(gx + 0.5);
  gx = gx - tx;
  vec2 g00 = vec2(gx.x,gy.x);
  vec2 g10 = vec2(gx.y,gy.y);
  vec2 g01 = vec2(gx.z,gy.z);
  vec2 g11 = vec2(gx.w,gy.w);
  vec4 norm = 1.79284291400159 - 0.85373472095314 * 
    vec4(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11));
  g00 *= norm.x;
  g01 *= norm.y;
  g10 *= norm.z;
  g11 *= norm.w;
  float n00 = dot(g00, vec2(fx.x, fy.x));
  float n10 = dot(g10, vec2(fx.y, fy.y));
  float n01 = dot(g01, vec2(fx.z, fy.z));
  float n11 = dot(g11, vec2(fx.w, fy.w));
  vec2 fade_xy = fade(Pf.xy);
  vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade_xy.x);
  float n_xy = mix(n_x.x, n_x.y, fade_xy.y);
  return 2.3 * n_xy;
}

//https://thebookofshaders.com/13/
float fbm (in vec2 st, in uint octaves) {
    // Initial values
    float value = 0.0;
    float amplitude = 0.5;
    //
    // Loop of octaves
    for (int i = 0; i < octaves; i++) {
        value += amplitude * cnoise(st);
        st *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

void main()
{
    vec3 uv = cloudSpacePosition.xyz / cloudSpacePosition.w;
    uv = (uv + 1.0) * 0.5;
	uv *= 0.6;
	vec4 color = vec4(0.0);
    color += - 0.8 * fbm(uv.xy * 4.0 + vec2(1.2) * p.time, 3);
    color += 0.8 * fbm(uv.xy * 5.0 + vec2(1.0) * p.time, 5);

	vec3 dir = worldSpacePosition - p.cameraPos;
	
	fragColor = color;
	fragColor = mix(fragColor, vec4(0.0), smoothstep(p.moonLuminance * 0.5, p.moonLuminance * 0.9, length(dir)));
}