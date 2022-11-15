vec4 calculateVarColor(float variance) 
{
    if (variance < 0.0) 
        return vec4(1.0, 0.0, 1.0, 1.0);
    if (isnan(variance)) 
        return vec4(0.0, 1.0, 1.0, 1.0);
    if (isinf(variance)) 
        return vec4(1.0, 1.0, 1.0, 1.0);

    float n_variance = clamp(variance, viColors[0].value, viColors[p.varianceInterpolationSize - 1].value);
    for (uint i = 0; i < p.varianceInterpolationSize - 1; ++i)
    {
        VIColor vic = viColors[i];
        VIColor vic1 = viColors[i + 1];
        if (n_variance >= vic.value && n_variance <= vic1.value) 
        {
            float valsize = (vic1.value - vic.value);
            return mix(vic.color, vic1.color, (n_variance - vic.value) / valsize);
        }
    }

    return vec4(0.0);
}