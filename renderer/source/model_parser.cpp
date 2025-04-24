#include "model_parser.h"

#include <unordered_map>

#pragma region MODEL PARSER
std::vector<std::array<double, 3>> ModelParser::GetPlyVertexPos(happly::PLYData* plyObj, const std::string& vertexElementName)
{
	return plyObj->getVertexPositions();
}

std::vector<double> ModelParser::GetPlyProperty(happly::PLYData* plyObj, const std::string& propName, const std::string& vertexElementName)
{
	return plyObj->getElement(vertexElementName).getProperty<double>(propName);
}
#pragma endregion

#pragma region MODEL CACHE

void ModelCache::LoadAllModelsInCache(const std::vector<std::string>& paths)
{
	for (const std::string& path : paths)
		LoadModelInCache(path);
}

bool ModelCache::LoadModelInCache(const std::string& path)
{
	auto it = m_cache.find(path);

	if (it != m_cache.end())
		return true;

	return LoadModelFromFile(path);
}

const CachedModel& ModelCache::GetModelFromCache(const std::string& path) const
{
	auto it = m_cache.find(path);

	if (it != m_cache.end())
		return it->second;

	throw std::runtime_error("Model not find in cache");
}

bool ModelCache::LoadModelFromFile(const std::string& path)
{
	happly::PLYData plyObj(path);
	plyObj.validate();

	std::vector<std::array<double, 3>> vertexPos = ModelParser::GetPlyVertexPos(&plyObj);
	std::vector<std::vector<size_t>> faceIndices = plyObj.getFaceIndices();

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	CachedModel cachedModel;

	for (const std::vector<size_t>& face : faceIndices)
	{
		for (size_t index : face)
		{
			Vertex vertex{};
			vertex.pos.x = static_cast<float>(vertexPos[index][0]);
			vertex.pos.y = static_cast<float>(vertexPos[index][1]);
			vertex.pos.z = static_cast<float>(vertexPos[index][2]);
			vertex.texCoord = {0.f, 0.f};
			vertex.color = {1.0f, 1.0f, 1.0f};

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(cachedModel.m_cachedVertices.size());
				cachedModel.m_cachedVertices.push_back(vertex);
			}

			cachedModel.m_cachedIndices.push_back(uniqueVertices[vertex]);
		}
	}

	cachedModel.m_cachedVertexCount = plyObj.getElement("vertex").count;

	m_cache[path] = cachedModel;

	return true;
}
#pragma endregion