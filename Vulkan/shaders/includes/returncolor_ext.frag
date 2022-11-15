void returnColor(vec3 color, vec3 position, vec3 normal, uint materialID, uint triangleID, float depth, float brightness) {
    switch (p.renderState) {
        case RS_COLOR:
            fragColor = (p.denoise || !p.tonemapping || p.referenceMode) ? vec4(color, 1.0) : fromLinear(vec4(color * p.exposure, 1.0));
            //fragColor = vec4(color, 1.0);
            break;
        case RS_G_BUFFER:
            vec2 tc2 = texCoord * 3.0;
            vec3 c = vec3(0.0);
            if (tc2.y > 0.5 && tc2.y <= 1.5) 
            {
                if (tc2.x <= 1.0)
                    c = texture(t_color, tc2 - vec2(0.0, 0.5)).rgb;
                else if (tc2.x <= 2.0)
                    c = texture(t_position, tc2 - vec2(1.0, 0.5)).rgb;
                else
                    c = (normalize(texture(t_normal, tc2 - vec2(2.0, 0.5)).rgb) + 1.0) * 0.5;
            }
            if (tc2.y > 1.5 && tc2.y <= 2.5)
            {                
                if (tc2.x > 0.5 && tc2.x <= 1.5)
                    c = vec3(hash31(float(floatBitsToUint(texture(t_position, tc2 - vec2(0.5, 1.5)).w))));
                if (tc2.x > 1.5 && tc2.x <= 2.5)
                    c = vec3(texture(t_depth, tc2 - vec2(1.5, 1.5)).r);
            }
            fragColor = vec4(c, 1.0);
            break;
        case RS_POSITION:
            fragColor = vec4(position, 1.0);
            break;
        case RS_NORMAL:
            fragColor = vec4((normalize(normal) + 1.0) * 0.5, 1.0);
            //uvec2 tc = uvec2(gl_FragCoord.xy);
            //if (!p.keepCurrentLines && tc.x == p.width / 2 && tc.y == p.height / 2) {
            //    LineVertex l0, l1;
            //    l0.position = position;
            //    l0.color = vec3(1.0, 0.0, 0.0);
            //    l1.position = position + normal * 0.4;
            //    l1.color = vec3(1.0, 0.0, 0.0);
            //    appendLine(l0, l1);
            //}
            //fragColor = vec4(normal.z < 0.5 ? 1.0 : 0.0, 0.0, 0.0, 1.0);
            //fragColor = vec4(normal, 1.0);
            break;
        case RS_MATERIAL:
            fragColor = vec4(vec3(hash31(float(materialID))), 1.0);
            break;
        case RS_DEPTH:
            fragColor = vec4(vec3(depth), 1.0);
            break;
        case RS_BRIGHTNESS:
            fragColor = vec4(vec3(brightness), 1.0);
            break;
        default:
            fragColor = vec4(1.0);
            break;
    }
}