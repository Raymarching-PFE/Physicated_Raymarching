#pragma once

#include <glm/glm.hpp>

struct Node
{
   float slice = -1;

   glm::vec3 boxPos = glm::vec3(-1, -1, -1);
   glm::vec3 boxSize = glm::vec3(-1, -1, -1);

   // Children
   Node *left = nullptr;
   Node *right = nullptr;

   // Debug only
   int generation = -1;

   int mortonNumber = 1;
};

class BinaryTree
{
public:
   BinaryTree(int pointsNumber);

   ~BinaryTree();

   std::vector<glm::vec3> generatedPoints;

private:
   std::vector<std::vector<glm::vec3> > FillUpTree(const std::vector<glm::vec3> &data, Node *node, int deepness);
   void FillUpTreeRecursive(const std::vector<glm::vec3> &data, Node *node, int deepness);

   void CreateStructureNodes(int CurrGen, int maxGen, Node *node, int currentMortenNumber);

   void PrintNode(Node *node);
   void PrintNodeRecursive(Node *node);

   float FindOptimalSlice(std::vector<glm::vec3> data, int deepness);

   Node *root;

   Node* GetNearestBoxRecursive(glm::vec3 point, float radius, int deepness, Node* node);

   glm::vec3 GetNearestPointRecursive(glm::vec3 point, float radius, int deepness, Node* node);

   // Quick sort algorythm
   int Partition(std::vector<float> &vec, int low, int high);

   void QuickSort(std::vector<float> &vec, int low, int high);
};