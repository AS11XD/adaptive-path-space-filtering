#version 450

layout(location = 0) in vec3 worldSpacePosition;
layout(location = 1) in vec4 moonSpacePosition;
layout(location = 0) out vec4 fragColor;

layout(binding = 2) uniform sampler2D moonTex;

layout(push_constant) uniform PushStruct {
	float time;
	float sunMoonSize;
	float sunMoonImageSize;
	float moonLuminance;
    mat4 moonVP;
	mat4 M;
	vec3 moonPos;
	vec3 sunPos;
	vec3 cameraPos;
	vec3 moonCol;
	vec3 sunCol;
} p;

void main()
{
	vec3 sunCol = p.sunCol;
	vec3 moonCol = p.moonCol;
	vec4 sky_c_b1 = vec4(0.2, 0.3, 0.6, 1.0);
	vec4 sky_c_b2 = vec4(0.2, 0.6, 0.9, 1.0);
	vec4 sky_c_r = vec4(0.6, 0.2, 0.1, 0.0);
	vec4 sky_c = vec4(0.0);
	float ang = dot(normalize(p.sunPos), vec3(0.0, 1.0, 0.0));
	float ang2 = dot(normalize(worldSpacePosition - p.cameraPos), vec3(0.0, 1.0, 0.0));
	sky_c = (ang >= -0.2) ? mix(sky_c_r, mix(sky_c_b2, sky_c_b1, ang2), (ang + 0.2) / 1.2) : 
							mix(sky_c_r, vec4(0.0), clamp((-ang - 0.2) / 0.8 * 5.0, 0.0, 1.0));
	
	vec4 sun_c = vec4(0.0);
	if (length(worldSpacePosition - p.sunPos) < p.sunMoonSize)
		sun_c = vec4(sunCol * sunCol, (p.sunMoonSize - length(worldSpacePosition - p.sunPos)) / 40.0) * exp((30.0 - length(worldSpacePosition - p.sunPos)) / 15.0);
	
	vec4 moon_c = vec4(0.0);
	if (length(worldSpacePosition - p.moonPos) < p.sunMoonSize) {
		moon_c = vec4(moonCol * moonCol, (p.sunMoonSize - length(worldSpacePosition - p.moonPos)) / 40.0) * exp((30.0 - length(worldSpacePosition - p.moonPos)) / 15.0);
		if (length(worldSpacePosition - p.moonPos) < p.sunMoonImageSize) {
			vec3 uv = moonSpacePosition.xyz / moonSpacePosition.w;
			uv = (uv + 1.0) * 0.5;
			vec4 moon_tex = texture(moonTex, uv.xy);
			moon_c = (moon_tex.a > 0.0) ? vec4(moon_tex.rgb + moonCol * p.moonLuminance, moon_tex.a) : moon_c;
		}
	}

	fragColor = mix(sky_c, vec4(sun_c.rgb + moon_c.rgb, 1.0), sun_c.a + moon_c.a);
	fragColor = mix(fragColor, vec4(0.2, 0.2, 0.3, 1.0), smoothstep(-90.0, -120.0, worldSpacePosition.y));
}