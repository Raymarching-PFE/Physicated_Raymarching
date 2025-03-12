#include "model_parser.h"

#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"


void ModelParser::LoadObjModel()
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, OBJ_PATH.c_str()))
		throw std::runtime_error(warn + err);

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	for (const tinyobj::shape_t& shape : shapes)
	{
		for (const tinyobj::index_t& index : shape.mesh.indices)
		{
			Vertex vertex{};

			vertex.pos =
			{
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			if (index.texcoord_index >= 0)
			{
				vertex.texCoord =
				{
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};
			}

			vertex.color = {1.0f, 1.0f, 1.0f};

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(_vertices.size());
				_vertices.push_back(vertex);
			}

			_indices.push_back(uniqueVertices[vertex]);
		}
	}
}


std::vector<std::array<double, 3>> ModelParser::GetPlyVertexPos(happly::PLYData* plyObj, const std::string& vertexElementName)
{
	return plyObj->getVertexPositions();
}

std::vector<double> ModelParser::GetPlyProperty(happly::PLYData* plyObj, const std::string& propName, const std::string& vertexElementName)
{
	return plyObj->getElement(vertexElementName).getProperty<double>(propName);
}

void ModelParser::LoadPlyModel(happly::PLYData* plyObj)
{
	std::vector<std::array<double, 3>> tmp = GetPlyVertexPos(plyObj);

	for (std::array<double, 3> pos : tmp)
	{
		Vertex vertex{};

		vertex.pos.x = static_cast<float>(pos[0]);
		vertex.pos.y = static_cast<float>(pos[1]);
		vertex.pos.z = static_cast<float>(pos[2]);

		vertex.texCoord = {0.f, 0.f};

		vertex.color = {1.0f, 1.0f, 1.0f};

		_vertices.push_back(vertex);

		std::unordered_map<Vertex, uint32_t> uniqueVertices{};

		if (uniqueVertices.count(vertex) == 0)
		{
			uniqueVertices[vertex] = static_cast<uint32_t>(_vertices.size());
			_vertices.push_back(vertex);
		}

		_indices.push_back(uniqueVertices[vertex]);
	}
}
