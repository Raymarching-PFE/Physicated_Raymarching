#pragma once

#include <glm/glm.hpp>

#include <array>

constexpr int MAX_POINTS_PER_LEAVES = 16;

struct alignas(16) GPUNode {
	glm::vec4 boxPos;         // .xyz used
	glm::vec4 boxSize;        // .xyz used
	glm::ivec4 children;      // .x = left, .y = right

	glm::vec4 cloudPoints[16]; // .xyz = point, .w = unused
};

struct Node
{
   float slice = -1;

   glm::vec3 boxPos = glm::vec3(-1, -1, -1);
   glm::vec3 boxSize = glm::vec3(-1, -1, -1);

   // Children
   Node *left = nullptr;
   Node *right = nullptr;

   int mortonNumber = 1;

   //Only when leaf
   // std::vector<glm::vec3> cloudPoints;
   std::array<glm::vec3, MAX_POINTS_PER_LEAVES> cloudPoints;
   // std::array<unsigned int, MAX_POINTS_PER_LEAVES> cloudPoints;
};

std::vector<glm::vec3> FakeDataGenerator(int numberOfValues, float min = -1, float max = 1);

class BinaryTree
{
public:
	BinaryTree(std::vector<glm::vec3>& pointCloudPoints);
	BinaryTree() {};

   ~BinaryTree();

   std::vector<GPUNode> GPUReadyBuffer;

   // double arrays system
   // the first one is the tree and the cloudpoints in the nodes are offset for the next array
   // the second array is an array with all the points

   //TODO define sizes better
   // size is 2^(gen+1) -1
   // on order avec les morton number -> index
   // std::array<Node, 1> ToGPUArrayTree;

   //TODO opti ? on pre-tri les elements?
   // size is number of point of pointcloud
   // les nodes font ref a des index de cet array
   // std::array<glm::vec3, 1> ToGPUArrayPoints;

private:

   // USELESS ?
   glm::vec3* FillGPUPointsArray(std::vector<glm::vec3>& pointCloudPoints);
   std::vector<Node> FillGPUArray(Node* root, std::vector<glm::vec3>& pointCloudPoints);
   void FillGPUArrayRecursive(Node *node, std::vector<glm::vec3>& pointCloudPoints, std::vector<Node>& toReturn);

   std::vector<glm::vec3> GetBox(std::vector<glm::vec3> data);

   // Node* GetNodeFromMorton(int mortonNumber, Node* _root);

   std::vector<std::vector<glm::vec3> > FillUpTree(const std::vector<glm::vec3> &data, Node *node, int deepness);
   void FillUpTreeRecursive(const std::vector<glm::vec3> &data, Node *node, int deepness);

   void CreateStructureNodes(int CurrGen, int maxGen, Node *node, int currentMortenNumber);

   void PrintNode(const Node *node);
   void PrintNodeRecursive(Node *node);

   float FindOptimalSlice(std::vector<glm::vec3> data, int deepness);

   Node *root;

   Node* GetNearestBoxesRecursive(glm::vec3 point, float radius, int deepness, Node* node);

   glm::vec3 GetNearestPoint(glm::vec3 point, float radius, int deepness, Node* node);

   bool CheckBoxSphereIntersection(Node *node, glm::vec3 point, float radius);

   std::vector<glm::vec3> GetPointsInBoxRecursive(Node* node, std::vector<glm::vec3> points);

   // Quick sort algorythm
   int Partition(std::vector<float> &vec, int low, int high);

   void QuickSort(std::vector<float> &vec, int low, int high);
};