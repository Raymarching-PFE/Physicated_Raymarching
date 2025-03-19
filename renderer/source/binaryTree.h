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
};

class BinaryTree
{
public:
   BinaryTree();

   ~BinaryTree();

private:
   std::vector<std::vector<glm::vec3> > FillUpTree(const std::vector<glm::vec3> &data, Node *root, int deepness);

   void FillUpTreeRecursive(const std::vector<glm::vec3> &data, Node *root, int deepness);

   void CreateStructureNodes(int CurrGen, int maxGen, Node *root);

   void ViewNode(Node *node);

   void ViewNodeRecursive(Node *node);

   float Median(std::vector<glm::vec3> data, int deepness);

   Node *root;

   // Quick sort algorythme
   int partition(std::vector<float> &vec, int low, int high);

   void quickSort(std::vector<float> &vec, int low, int high);

public:
   std::vector<glm::vec3> generatedPoints;
   
};
