#include "binaryTree.h"

#include <vector>
#include <iostream>

#include "tiny_obj_loader.h"

std::vector<glm::vec3> FakeDataGenerator(int numberOfValues = 3, int min = 1, int max = 100)
{
   std::vector<glm::vec3> toReturn;

   for (int i = 0; i < numberOfValues; i++)
   {
      toReturn.push_back(glm::vec3(min + rand() % (max - min), min + rand() % (max - min), min + rand() % (max - min)));
   }

   return toReturn;
}

BinaryTree::BinaryTree()
{
   // Generate fake data
   const std::vector<glm::vec3> fakeData = FakeDataGenerator(4);

   for (int i = 0; i < fakeData.size(); i++)
      std::cout << "Data[" << i << "] : (" << fakeData[i].x << ", " << fakeData[i].y << ", " << fakeData[i].z << ")" <<
            std::endl;

   // Get generation
   int generation = 0;
   while (pow(2, generation) < fakeData.size())
      generation++;

   std::cout << "Elements : " << fakeData.size() << ", Gen : " << generation << std::endl;

   // create architecture from generation
   Node *root = new Node({glm::vec3(0, 0, 0), nullptr, nullptr, 0});
   CreateStructureNodes(0, generation, root);

   // Give values for structures nodes
   SplitListFromStructureNodeRecursive(fakeData, root, 0);

   // view tree
   ViewNodeRecursive(root);
}

void BinaryTree::SplitListFromStructureNodeRecursive(const std::vector<glm::vec3> &data, Node *root, int deepness = 0)
{
   if (root == nullptr)
      return;
   if (data.size() < 1)
      return;
   if (data.size() == 1)
   {
      root->position = data[0];
      root->right = nullptr;
      root->left = nullptr;
      return;
   }

   std::vector<std::vector<glm::vec3> > result = SplitListFromStructureNode(data, root, deepness);
   const std::vector<glm::vec3> leftNodes = result[0];
   const std::vector<glm::vec3> rightNodes = result[1];

   if (root->left != nullptr)
      SplitListFromStructureNodeRecursive(leftNodes, root->left, deepness + 1);
   if (root->right != nullptr)
      SplitListFromStructureNodeRecursive(rightNodes, root->right, deepness + 1);
}

std::vector<std::vector<glm::vec3> > BinaryTree::SplitListFromStructureNode(
   const std::vector<glm::vec3> &data, Node *root, int deepness = 0)
{
   std::vector<std::vector<glm::vec3> > results;

   // root->position.x = MedianX(data);
   root->position[deepness % 3] = Median(data, deepness);

   std::vector<glm::vec3> leftNodes;
   std::vector<glm::vec3> rightNodes;

   for (int i = 0; i < data.size(); i++)
   {
      if (data[i][deepness % 3] <= root->position[deepness % 3])
      {
         leftNodes.push_back(data[i]);
      }
      else
      {
         rightNodes.push_back(data[i]);
      }
   }

   results.push_back(leftNodes);
   results.push_back(rightNodes);

   return results;
}

float BinaryTree::MedianX(std::vector<glm::vec3> data)
{
   // sort the x values
   std::vector<float> vec;
   for (int i = 0; i < data.size(); i++)
   {
      vec.push_back(data[i].x);
   }

   int n = vec.size();

   // Calling quicksort for the vector vec
   quickSort(vec, 0, n - 1);

   return vec[(vec.size() - 1) / 2];
}

float BinaryTree::Median(std::vector<glm::vec3> data, int deepness = 0)
{
   // sort the x values
   std::vector<float> vec;
   for (int i = 0; i < data.size(); i++)
   {
      if (deepness % 3 == 0)
         vec.push_back(data[i].x);
      if (deepness % 3 == 1)
         vec.push_back(data[i].y);
      if (deepness % 3 == 2)
         vec.push_back(data[i].z);
   }

   int n = vec.size();

   // Calling quicksort for the vector vec
   quickSort(vec, 0, n - 1);

   return vec[(vec.size() - 1) / 2];
}

void BinaryTree::ViewNode(Node *node)
{
   std::cout << "Node : (" << node->position.x << ", " << node->position.y << ", " << node->position.z << "), " <<
         (node->left != nullptr ? "TRUE" : "FALSE") << ", " << (node->right != nullptr ? "TRUE" : "FALSE") << std::endl;
}

void BinaryTree::ViewNodeRecursive(Node *node)
{
   ViewNode(node);

   if (node->left != nullptr)
      ViewNodeRecursive(node->left);
   if (node->right != nullptr)
      ViewNodeRecursive(node->right);
}

void BinaryTree::CreateStructureNodes(int CurrGen, int maxGen, Node *root)
{
   CurrGen++;

   root->left = new Node({glm::vec3(0, 0, 0), nullptr, nullptr, CurrGen});
   root->right = new Node({glm::vec3(0, 0, 0), nullptr, nullptr, CurrGen});

   if (CurrGen != maxGen)
   {
      CreateStructureNodes(CurrGen, maxGen, root->left);
      CreateStructureNodes(CurrGen, maxGen, root->right);
   }
}

BinaryTree::~BinaryTree()
{
   //TODO free allocations
}

// QuickSort

int BinaryTree::partition(std::vector<float> &vec, int low, int high)
{
   // Selecting last element as the pivot
   float pivot = vec[high];

   // Index of element just before the last element
   // It is used for swapping
   int i = (low - 1);

   for (int j = low; j <= high - 1; j++)
   {
      // If current element is smaller than or
      // equal to pivot
      if (vec[j] <= pivot)
      {
         i++;
         std::swap(vec[i], vec[j]);
      }
   }

   // Put pivot to its position
   std::swap(vec[i + 1], vec[high]);

   // Return the point of partition
   return (i + 1);
}

void BinaryTree::quickSort(std::vector<float> &vec, int low, int high)
{
   // Base case: This part will be executed till the starting
   // index low is lesser than the ending index high
   if (low < high)
   {
      // pi is Partitioning Index, arr[p] is now at
      // right place
      int pi = partition(vec, low, high);

      // Separately sort elements before and after the
      // Partition Index pi
      quickSort(vec, low, pi - 1);
      quickSort(vec, pi + 1, high);
   }
}
