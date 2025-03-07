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
   Node* root = new Node();
   root->slice = 0;
   root->boxPos = glm::vec3(0, 0, 0);
   root->boxSize = glm::vec3(0, 0, 0);
   root->left = nullptr;
   root->right = nullptr;
   root->generation = 0;
   CreateStructureNodes(0, generation, root);


   // Get root box
   std::vector<float> min = {fakeData[0].x, fakeData[0].y, fakeData[0].z};
   std::vector<float> max = {fakeData[0].x, fakeData[0].y, fakeData[0].z};

   for (int i = 1; i < fakeData.size(); i++)
   {
      for (int j = 0; j < 3; j++)
      {
         if (fakeData[i][j] < min[j])
            min[j] = fakeData[i][j];
         if (fakeData[i][j] > max[j])
            max[j] = fakeData[i][j];
      }
   }
   root->boxPos = glm::vec3(min[0], min[1], min[2]);
   root->boxSize = glm::vec3(max[0]-min[0], max[1]-min[1], max[2]-min[2]);

   std::cout << "Box corner : [" << root->boxPos[0] << ", " << root->boxPos[1] << ", " << root->boxPos[2] << "], size : [" <<
      root->boxSize[0] << ", " << root->boxSize[1] << ", " << root->boxSize[2] << "]" << std::endl;

   // Give values for structure nodes
   FillUpTreeRecursive(fakeData, root, 0);

   // view tree
   ViewNodeRecursive(root);
}

void BinaryTree::FillUpTreeRecursive(const std::vector<glm::vec3> &data, Node *root, int deepness = 0)
{
   if (root == nullptr)
      return;
   if (data.size() < 1)
      return;
   if (data.size() == 1)
   {
      root->boxPos = data[0];
      root->right = nullptr;
      root->left = nullptr;
      return;
   }

   std::vector<std::vector<glm::vec3> > result = FillUpTree(data, root, deepness);
   const std::vector<glm::vec3> leftNodes = result[0];
   const std::vector<glm::vec3> rightNodes = result[1];

   if (root->left != nullptr)
   {
      FillUpTreeRecursive(leftNodes, root->left, deepness + 1);
      // Fill up box
      root->left->boxPos = root->boxPos;
      root->left->boxSize = root->boxSize;
      root->left->boxSize[deepness % 3] = root->boxSize[deepness % 3] - root->slice;
   }
   if (root->right != nullptr)
   {
      FillUpTreeRecursive(rightNodes, root->right, deepness + 1);

      root->right->boxPos = root->boxPos;
      root->right->boxPos[deepness % 3] = root->boxPos[deepness % 3] + root->slice;
      root->right->boxSize = root->boxSize;
      root->right->boxSize[deepness % 3] = root->boxSize[deepness % 3] - root->slice;
   }
}

std::vector<std::vector<glm::vec3> > BinaryTree::FillUpTree(
   const std::vector<glm::vec3> &data, Node *root, int deepness = 0)
{
   std::vector<std::vector<glm::vec3> > results;

   root->slice = Median(data, deepness);

   std::vector<glm::vec3> leftNodes;
   std::vector<glm::vec3> rightNodes;

   for (int i = 0; i < data.size(); i++)
   {
      if (data[i][deepness % 3] <= root->slice)
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
   std::cout << "Node : Slice : " << node->slice <<
      ", BoxPos : (" << node->boxPos[0] << ", " << node->boxPos[1] << ", " << node->boxPos[2] <<
         "), BoxSize : (" << node->boxSize[0] << ", " << node->boxSize[1] << ", " << node->boxSize[2] << "), "<<
         (node->left != nullptr ? "TRUE" : "FALSE") <<
            ", " << (node->right != nullptr ? "TRUE" : "FALSE") <<
               ", generation : " << node->generation <<
                  std::endl;
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

   root->left = new Node({0, glm::vec3(0, 0, 0),
      glm::vec3(0, 0, 0), nullptr, nullptr, CurrGen});
   root->right = new Node({0, glm::vec3(0, 0, 0),
   glm::vec3(0, 0, 0), nullptr, nullptr, CurrGen});

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