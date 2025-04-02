#pragma once

#include "model_properties.h"
#include "happly.h"


const std::string OBJ_PATH = "point_clouds/bunny.obj";
const std::string PLY_PATH = "point_clouds/bunny.ply";


class ModelParser
{
public:
	static void LoadObjModel();

	static void LoadPlyModel(happly::PLYData* plyObj);
	static std::vector<std::array<double, 3>> GetPlyVertexPos(happly::PLYData* plyObj, const std::string& vertexElementName = "vertex");
	static std::vector<double> GetPlyProperty(happly::PLYData* plyObj, const std::string& propName, const std::string& vertexElementName = "vertex");
};