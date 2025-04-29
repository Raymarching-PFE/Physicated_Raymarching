#pragma once

#include <glm/glm.hpp>

constexpr int MAX_POINTS_PER_LEAVES = 4;

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
   std::vector<glm::vec3> cloudPoints;
};

std::vector<glm::vec3> FakeDataGenerator(int numberOfValues, float min = -1, float max = 1);

class BinaryTree
{
public:
   BinaryTree(std::vector<glm::vec3> pointCloudPoints);

   ~BinaryTree();

   std::vector<glm::vec3> generatedPoints;

private:

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