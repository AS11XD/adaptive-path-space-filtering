
struct Vertex {
    vec3 position;
    vec3 normal;
    vec3 trisNormal;
    vec2 texCoord;
    uint materialID;
    vec3 tangent;
    vec3 bitangent;
};

struct VertexSSBO {
    float position[3];
    float normal[3];
    float texCoord[2];
    uint materialID;
    float tangent[3];
    float bitangent[3];
};