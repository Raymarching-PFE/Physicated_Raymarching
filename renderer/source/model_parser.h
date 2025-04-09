#pragma once

#include <array>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"

#include "happly.h"


struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription GetBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const
	{
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};

template<> struct std::hash<Vertex>
{
	size_t operator()(Vertex const& vertex) const
	{
		return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
	}
};


class ModelParser
{
public:
	static std::vector<std::array<double, 3>> GetPlyVertexPos(happly::PLYData* plyObj, const std::string& vertexElementName = "vertex");
	static std::vector<double> GetPlyProperty(happly::PLYData* plyObj, const std::string& propName, const std::string& vertexElementName = "vertex");
};


struct CachedModel
{
	std::vector<Vertex> m_cachedVertices;
	std::vector<uint32_t> m_cachedIndices;
	size_t m_cachedVertexCount;
};

class ModelCache
{
public:
	 void LoadAllModelsInCache(const std::vector<std::string>& paths);
    bool LoadModelInCache(const std::string& path);
    const CachedModel& GetModelFromCache(const std::string& path) const;

private:
    std::unordered_map<std::string, CachedModel> m_cache;
    bool LoadModelFromFile(const std::string& path);
};