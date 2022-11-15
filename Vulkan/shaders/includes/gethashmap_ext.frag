
void insertHashMap(uvec2 cellFP, vec3 radiance, float brightness)
{
    uint cell = cellFP.x;
    uint memVal = atomicCompSwap(hashMap[cell].key, 0, cellFP.y);
    while (memVal != 0 && memVal != cellFP.y)
    {
        cell++;
        cell %= p.hashMapSize;        
        if (cell == (cellFP.x + 100) % p.hashMapSize)
            break;
        memVal = atomicCompSwap(hashMap[cell].key, 0, cellFP.y);
    }
    hashMap[cell].timestamp = p.frameCount;
    atomicAdd(hashMap[cell].counter, 1);
    atomicAdd(hashMap[cell].radiance[0], radiance.x);
    atomicAdd(hashMap[cell].radiance[1], radiance.y);
    atomicAdd(hashMap[cell].radiance[2], radiance.z);
    float n_brightness = max(brightness, 0.01);
    float avgCol = (radiance.x + radiance.y + radiance.z) / (3.0);
    atomicAdd(hashMap[cell].mom2, avgCol * avgCol);
}

void updateHashMapTimestamp(uvec2 cellFP)
{
    uint cell = cellFP.x;
    uint memVal = atomicCompSwap(hashMap[cell].key, 0, cellFP.y);
    while (memVal != 0 && memVal != cellFP.y)
    {
        cell++;
        cell %= p.hashMapSize;        
        if (cell == (cellFP.x + 100) % p.hashMapSize)
            break;
        memVal = atomicCompSwap(hashMap[cell].key, 0, cellFP.y);
    }
    hashMap[cell].timestamp = p.frameCount;
}

HashMapCell getHashMapCell(uvec2 hashMapCellFP)
{
    uint cell = hashMapCellFP.x;
    while (hashMap[cell].key != hashMapCellFP.y && hashMap[cell].key != 0)
    {
        cell++;
        cell %= p.hashMapSize;
        if (cell == (hashMapCellFP.x + 100) % p.hashMapSize)
            break;
    }
    return hashMap[cell];
}

HashMapCell getHashMapCell2(uvec2 hashMapCellFP)
{
    uint cell = hashMapCellFP.x;
    while (hashMap2[cell].key != hashMapCellFP.y && hashMap2[cell].key != 0)
    {
        cell++;
        cell %= p.hashMapSize;
        if (cell == (hashMapCellFP.x + 100) % p.hashMapSize)
            break;
    }
    return hashMap2[cell];
}

vec3 getRadiance(in HashMapCell hmCell)
{
    return vec3(hmCell.radiance[0], hmCell.radiance[1], hmCell.radiance[2]) / hmCell.counter;
}

float calculateVariance(in HashMapCell hmCell)
{
    vec3 rad = getRadiance(hmCell);
    float mom1 = (rad.x + rad.y + rad.z) / 3.0;
    float variance = max((hmCell.mom2 / hmCell.counter - mom1 * mom1), 0.0);
    return variance;        
}