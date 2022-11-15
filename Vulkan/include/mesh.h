#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <string>

struct Vertex
{
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec2 texCoord = glm::vec2(0.0f, 1.0f);
	uint32_t  materialID = 0;
	glm::vec3 tangent = glm::vec3(0.0f);
	glm::vec3 bitangent = glm::vec3(0.0f);
};

struct LineVertex
{
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 color = glm::vec3(0.0f);
};

template<typename T>
class Mesh
{
public:

	std::string name;
	std::vector<T> vertices;
	std::vector<uint32_t> indices;

	Mesh();
	Mesh(std::string _name, std::vector<T>& _vertices, std::vector<uint32_t>& _indices);
	~Mesh();
};


template <typename T>
Mesh<T>::Mesh()
{
}

template <typename T>
Mesh<T>::Mesh(std::string _name, std::vector<T>& _vertices, std::vector<uint32_t>& _indices)
{
	if (_vertices.size() <= 0)
	{
		throw std::exception((_name + " no zero vertex count allowed\n").c_str());
	}
	if (_indices.size() <= 0)
	{
		throw std::exception((_name + " no zero index count allowed\n").c_str());
	}

	name = _name;
	vertices = _vertices;
	indices = _indices;
}

template <typename T>
Mesh<T>::~Mesh()
{
}