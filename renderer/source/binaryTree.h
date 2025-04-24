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

   std::vector<glm::vec3> cloudPoints;
};

class BinaryTree
{
public:
   BinaryTree(int pointsNumber);

   ~BinaryTree();

   std::vector<glm::vec3> generatedPoints;

private:

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