
struct LightSSBO {
    float[3] position;
    float[3] color;
    float[3] direction;
    float intensity;
    uint type;
    bool on;
};

struct Light {
    vec3 position;
    vec3 color;
    vec3 direction;
    float intensity;
    uint type;
    bool on;
};