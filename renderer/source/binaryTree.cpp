#include "binaryTree.h"

#include <vector>
#include <iostream>

#include <bitset>


std::vector<glm::vec3> FakeDataGenerator(int numberOfValues = 3, float min, float max)
{
   std::vector<glm::vec3> toReturn;

   // static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

   //std::cout << "Points :" << std::endl;
   for (int i = 0; i < numberOfValues; i++)
   {
      // Generate
      glm::vec3 value = glm::vec3(min + static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * (max - min),
                                  min + static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * (max - min),
                                  min + static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * (max - min));
      toReturn.push_back(value);

      // Print it
      //std::cout << "[" << value.x << ", " << value.y << ", " << value.z << "]" << std::endl;
   }
   //std::cout << std::endl;

   return toReturn;
}

BinaryTree::BinaryTree(std::vector<glm::vec3> &pointCloudPoints)
{
   generatedPoints = pointCloudPoints;

   // Get generation
   // int generation = 0;
   // while (pow(2, generation) < pointCloudPoints.size() / MAX_POINTS_PER_LEAVES)
   //    generation++;

   int generation = std::ceil(std::log2(pointCloudPoints.size() / static_cast<double>(MAX_POINTS_PER_LEAVES)));

   //std::cout << "Elements : " << pointCloudPoints.size() << ", Gen : " << generation << std::endl;

   // Create architecture from generation
   Node *root = new Node();
   root->slice = 0;
   root->boxPos = glm::vec3(0, 0, 0);
   root->boxSize = glm::vec3(0, 0, 0);
   root->left = nullptr;
   root->right = nullptr;
   // root->generation = 0;
   CreateStructureNodes(0, generation, root, 1);

   // Get root box
   std::vector<glm::vec3> rootbox = GetBox(pointCloudPoints);

   root->boxPos = rootbox[0];
   root->boxSize = rootbox[1];

   //
   // std::vector<float> min = {pointCloudPoints[0].x, pointCloudPoints[0].y, pointCloudPoints[0].z};
   // std::vector<float> max = {pointCloudPoints[0].x, pointCloudPoints[0].y, pointCloudPoints[0].z};
   //
   // for (int i = 1; i < pointCloudPoints.size(); i++)
   // {
   //    for (int j = 0; j < 3; j++)
   //    {
   //       if (pointCloudPoints[i][j] < min[j])
   //          min[j] = pointCloudPoints[i][j];
   //       if (pointCloudPoints[i][j] > max[j])
   //          max[j] = pointCloudPoints[i][j];
   //    }
   // }
   // root->boxPos = glm::vec3(min[0], min[1], min[2]);
   // root->boxSize = glm::vec3(max[0] - min[0], max[1] - min[1], max[2] - min[2]);

   // std::cout << "Box corner : [" << root->boxPos[0] << ", " << root->boxPos[1] << ", " << root->boxPos[2] <<
   //       "], size : [" << root->boxSize[0] << ", " << root->boxSize[1] << ", " << root->boxSize[2] << "]" << std::endl;

   // Give values for structure nodes
   FillUpTreeRecursive(pointCloudPoints, root, 0);

   // view tree
   // PrintNodeRecursive(root);

   // Node* result =  GetNodeFromMorton(6, root);
   // PrintNode(result);

   glm::vec3 nearestPoint = GetNearestPoint(glm::vec3(50, 50, 50), 1, 0, root);


   //std::cout << "nearestPoint : " << nearestPoint[0] << ", " << nearestPoint[1] << ", " << nearestPoint[2] << std::endl;

   // glm::vec3* pointsArray = FillGPUPointsArray();

   std::vector<Node> buffer = FillGPUArray(root, pointCloudPoints);

   // std::cout << "Buffer morton: " << buffer.size() << std::endl;
   // for (int i = 0; i < buffer.size(); i++)
   // {
   //    std::cout << buffer[i].mortonNumber << ", ";
   // }

   //std::cout << std::endl;
   // std::cout << "Buffer points: " << buffer.size() << std::endl;
   // for (int i = 0; i < buffer.size(); i++)
   // {
   //    for (int j = 0; j < MAX_POINTS_PER_LEAVES; j++)
   //       std::cout << buffer.data()[i].cloudPoints[j].x << ", " << buffer.data()[i].cloudPoints[j].y << ", " << buffer.
   //             data()[i].cloudPoints[j].z << std::endl;
   //    std::cout << std::endl;
   // }

   GPUReadyBuffer.resize(buffer.size());
   for (int i = 0; i < buffer.size(); i++)
   {
      // Unchanged
       GPUReadyBuffer[i].boxPos = glm::vec4(buffer[i].boxPos, -1);
      GPUReadyBuffer[i].boxSize = glm::vec4(buffer[i].boxSize, -1);

      for(int j =0; j < 15; j++)
        GPUReadyBuffer[i].cloudPoints[j] = glm::vec4(buffer[i].cloudPoints[j], -1);

      // left/right -> int index
       if (buffer[i].left != nullptr)
          GPUReadyBuffer[i].children.x = buffer[i].left->mortonNumber;
       else
          GPUReadyBuffer[i].children.x = 0;

       if (buffer[i].right != nullptr)
          GPUReadyBuffer[i].children.y = buffer[i].right->mortonNumber;
       else
          GPUReadyBuffer[i].children.y = 0;
   }

   //std::cout << "GPU buffer : " << GPUReadyBuffer.size() << std::endl;
   //for (int i = 0; i < GPUReadyBuffer.size(); i++)
   //{
   //   //std::cout << GPUReadyBuffer[i].mortonNumber << ", ";
   //   //std::cout << GPUReadyBuffer[i].left << "-" << GPUReadyBuffer[i].right << ", ";
   //   //std::cout << "x:" << GPUReadyBuffer[i].boxPos.x << "y:" << GPUReadyBuffer[i].boxPos.y << "z:" << GPUReadyBuffer[i].boxPos.z << ", ";
   //   //std::cout << "x:" << GPUReadyBuffer[i].boxSize.x << "y:" << GPUReadyBuffer[i].boxSize.y << "z:" << GPUReadyBuffer[i].boxSize.z << ", ";

   //   //std::cout << "x:" << GPUReadyBuffer[i].cloudPoints[0].x << "y:" << GPUReadyBuffer[i].cloudPoints[0].y << "z:" << GPUReadyBuffer[i].cloudPoints[0].z << ", ";
   //}
}

void BinaryTree::FillGPUArrayRecursive(Node *node, std::vector<glm::vec3> &pointCloudPoints,
                                       std::vector<Node> &toReturn)
{
   if (node->mortonNumber >= toReturn.size())
   {
      std::cout << "MORTON BROKEN ALED" << std::endl;
      return;
   }
   toReturn[node->mortonNumber] = *node;


   if (node->left != nullptr)
      FillGPUArrayRecursive(node->left, pointCloudPoints, toReturn);
   if (node->right != nullptr)
      FillGPUArrayRecursive(node->right, pointCloudPoints, toReturn);
}

std::vector<Node> BinaryTree::FillGPUArray(Node *root, std::vector<glm::vec3> &pointCloudPoints)
{
   int generation = std::ceil(std::log2(pointCloudPoints.size() / static_cast<double>(MAX_POINTS_PER_LEAVES)));

   std::vector<Node> toReturn;
   // toReturn.resize(pow(2, generation+1) -1);
   toReturn.resize((1 << (generation + 1)));

   FillGPUArrayRecursive(root, pointCloudPoints, toReturn);
   return toReturn;
}

glm::vec3 *BinaryTree::FillGPUPointsArray(std::vector<glm::vec3> &pointCloudPoints)
{
   return pointCloudPoints.data();
}

// Node* BinaryTree::GetNodeFromMorton(int mortonNumber, Node* _root)
// {
//    std::cout << "looking for Morton number : " << mortonNumber << std::endl;
//
//    // Get generations
//    int gen = 0;
//
//    int mortenCopy = mortonNumber;
//    for (int i = 0; i < 16; i++)
//    {
//       // check if the first bit is 1
//       if (mortenCopy > 16384)
//       {
//          gen = 15 - i;
//          break;
//       }
//       mortenCopy = mortenCopy << 1;
//    }
//
//    std::cout << "gen : " << gen << std::endl;
//
//    // retrieve the node
//    Node* toReturn = _root;
//    for (int i = 0 ; i < gen; i++)
//    {
//       if (mortonNumber % 2 == 0)
//          toReturn = toReturn->left;
//       else
//          toReturn = toReturn->right;
//       mortonNumber = mortonNumber >> 1;
//    }
//
//    // std::cout << "node : " << std::endl;
//    // PrintNode(toReturn);
//
//    return toReturn;
// }

std::vector<glm::vec3> BinaryTree::GetPointsInBoxRecursive(Node *node, std::vector<glm::vec3> points)
{
   points.insert(points.end(), node->cloudPoints.begin(), node->cloudPoints.end());

   if (node->left != nullptr)
   {
      points = GetPointsInBoxRecursive(node->left, points);
   }
   if (node->right != nullptr)
   {
      points = GetPointsInBoxRecursive(node->right, points);
   }

   return points;
}

glm::vec3 BinaryTree::GetNearestPoint(glm::vec3 point, float radius, int deepness, Node *node)
{
   // Get the smaller possible box where the sphere is in
   // Node *nearestBox = GetNearestBoxesRecursive(glm::vec3(50, 50, 50), 1, 0, root);

   // Get all the points inside the box recursivly to get the minimum amount of points to test
   std::vector<glm::vec3> cloudPoints;
   cloudPoints = GetPointsInBoxRecursive(node, cloudPoints);

   if (cloudPoints.size() < 1)
   {
      std::cout << "Failed to get point" << std::endl;
      return glm::vec3(-1, -1, -1);
   }

   // Return the nearest point inside the array you just made
   glm::vec3 nearestPoint = cloudPoints[0];

   glm::vec3 diff = cloudPoints[0] - point;
   float distSqr = dot(diff, diff);
   float minDist = distSqr;

   for (int i = 1; i < cloudPoints.size(); i++)
   {
      diff = cloudPoints[i] - point;
      distSqr = dot(diff, diff);

      if (distSqr < minDist)
      {
         nearestPoint = cloudPoints[i];
         minDist = distSqr;
      }
   }

   return nearestPoint;
}


std::vector<glm::vec3> BinaryTree::GetBox(std::vector<glm::vec3> data)
{
   glm::vec3 min = {data[0].x, data[0].y, data[0].z};
   glm::vec3 max = {data[0].x, data[0].y, data[0].z};

   for (int i = 1; i < data.size(); i++)
   {
      for (int j = 0; j < 3; j++)
      {
         if (data[i][j] < min[j])
            min[j] = data[i][j];
         if (data[i][j] > max[j])
            max[j] = data[i][j];
      }
   }

   std::vector<glm::vec3> box;
   box.push_back(min);

   max[0] -= min[0];
   max[1] -= min[1];
   max[2] -= min[2];

   box.push_back(max);

   return box;
}

void BinaryTree::FillUpTreeRecursive(const std::vector<glm::vec3> &data, Node *root, int deepness = 0)
{
   if (root == nullptr)
      return;
   if (data.size() < 1)
      return;
   if (data.size() <= MAX_POINTS_PER_LEAVES)
   {
      // Set box
      std::vector<glm::vec3> rootbox = GetBox(data);
      root->boxPos = rootbox[0];
      root->boxSize = rootbox[1];

      // Add points
      for (int i = 0; i < data.size(); i++)
      {
         root->cloudPoints[i] = data[i];
      }

      // No children, it is a leaf
      root->right = nullptr;
      root->left = nullptr;
      return;
   }

   std::vector<std::vector<glm::vec3> > result = FillUpTree(data, root, deepness);
   const std::vector<glm::vec3> leftNodes = result[0];
   const std::vector<glm::vec3> rightNodes = result[1];

   if (root->left != nullptr)
   {
      // Fill up box
      root->left->boxPos = root->boxPos;
      root->left->boxSize = root->boxSize;
      root->left->boxSize[deepness % 3] = root->slice - root->boxPos[deepness % 3];
      FillUpTreeRecursive(leftNodes, root->left, deepness + 1);
   }
   if (root->right != nullptr)
   {
      root->right->boxPos = root->boxPos;
      root->right->boxPos[deepness % 3] = root->slice;
      root->right->boxSize = root->boxSize;
      root->right->boxSize[deepness % 3] = root->slice - root->boxPos[deepness % 3];
      FillUpTreeRecursive(rightNodes, root->right, deepness + 1);
   }
}

std::vector<std::vector<glm::vec3> > BinaryTree::FillUpTree(const std::vector<glm::vec3> &data, Node *node,
                                                            int deepness = 0)
{
   std::vector<std::vector<glm::vec3> > results;

   node->slice = FindOptimalSlice(data, deepness);

   std::vector<glm::vec3> leftNodes;
   std::vector<glm::vec3> rightNodes;

   for (int i = 0; i < data.size(); i++)
   {
      if (data[i][deepness % 3] <= node->slice)
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

float BinaryTree::FindOptimalSlice(std::vector<glm::vec3> data, int deepness = 0)
{
   std::vector<float> vec;
   for (int i = 0; i < data.size(); i++)
   {
      vec.push_back(data[i][deepness % 3]);
   }

   int n = vec.size();

   // Calling quicksort for the vector vec
   QuickSort(vec, 0, n - 1);

   // Return median
   return (vec[(vec.size() - 1) / 2] + vec[(vec.size() - 1) / 2 + 1]) / 2;
}

void BinaryTree::PrintNode(const Node *node)
{
   std::bitset<16> bits(node->mortonNumber);

   std::cout << "Node" << std::endl << "Slice : " << node->slice << std::endl << "Box : (" << node->boxPos[0] << ", " <<
         node->boxPos[1] << ", " << node->boxPos[2] << ")," "(" << node->boxSize[0] << ", " << node->boxSize[1] << ", "
         << node->boxSize[2] << "), " << std::endl << "Children : " << (node->left != nullptr ? "TRUE" : "FALSE") <<
         ", " << (node->right != nullptr ? "TRUE" : "FALSE") << std::endl << "Morton : " << node->mortonNumber <<
         ", Morton(bits) : " << bits << std::endl

         // << "Number of cloud points : " << node->cloudPoints.size()

         // << "cloud points : " << node->cloudPoints[0] << ", " << node->cloudPoints[1] << ", " << node->cloudPoints[2]
         // << ", " << node->cloudPoints[3]

         << std::endl << std::endl;
}

void BinaryTree::PrintNodeRecursive(Node *node)
{
   PrintNode(node);

   if (node->left != nullptr)
      PrintNodeRecursive(node->left);
   if (node->right != nullptr)
      PrintNodeRecursive(node->right);
}

void BinaryTree::CreateStructureNodes(int CurrGen, int maxGen, Node *node, int currentMortenNumber)
{
   CurrGen++;

   currentMortenNumber = currentMortenNumber << 1;

   node->left = new Node({0, glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), nullptr, nullptr, currentMortenNumber});
   node->right = new Node({0, glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), nullptr, nullptr, currentMortenNumber + 1});

   if (CurrGen != maxGen)
   {
      CreateStructureNodes(CurrGen, maxGen, node->left, currentMortenNumber);
      CreateStructureNodes(CurrGen, maxGen, node->right, currentMortenNumber + 1);
   }
}

BinaryTree::~BinaryTree()
{
   //TODO free allocations
}

Node *BinaryTree::GetNearestBoxesRecursive(glm::vec3 point, float radius, int deepness, Node *node)
{
   if (node == nullptr)
   {
      std::cout << "FAILED : Node is null" << std::endl;
      return nullptr;
   }

   // if a leaf, return the leaf point
   if (node->left == nullptr && node->right == nullptr)
      return node;

   // if only one child remaining : return it
   if (node->left == nullptr)
      return node->right;
   else if (node->right == nullptr)
      return node->left;

   // check box-sphere intersection
   bool left = CheckBoxSphereIntersection(node->left, point, radius);
   bool right = CheckBoxSphereIntersection(node->right, point, radius);

   // if only one of them is good, GetNearestBoxRecursive in the intersecting box
   if (left | right)
   {
      if (left)
      {
         return GetNearestBoxesRecursive(point, radius, deepness + 1, node->left);
      }
      else
      {
         return GetNearestBoxesRecursive(point, radius, deepness + 1, node->right);
      }
   }

   // if both of them are colliding, return all the points inside the both boxes (get them with morten indexes i guess?)
   if (left && right)
   {
      return node;
   }

   // if nothing intersect, return null
   // i think we never go here... depending on the minimal cuts variant...
   if (!left && !right)
   {
      return nullptr;
   }
   return nullptr;
}

bool BinaryTree::CheckBoxSphereIntersection(Node *node, glm::vec3 point, float radius)
{
   // Naive method
   // TODO improve it

   std::vector<glm::vec3> boxPoints;

   boxPoints.push_back(node->boxPos);
   boxPoints.push_back(node->boxPos + glm::vec3(node->boxSize.x, 0, 0));
   boxPoints.push_back(node->boxPos + glm::vec3(0, node->boxSize.y, 0));
   boxPoints.push_back(node->boxPos + glm::vec3(0, 0, node->boxSize.z));

   boxPoints.push_back(node->boxPos + node->boxSize - glm::vec3(node->boxSize.x, 0, 0));
   boxPoints.push_back(node->boxPos + node->boxSize - glm::vec3(0, node->boxSize.y, 0));
   boxPoints.push_back(node->boxPos + node->boxSize - glm::vec3(0, 0, node->boxSize.z));
   boxPoints.push_back(node->boxPos + node->boxSize);

   for (int i = 0; i < 6; i++)
   {
      glm::vec3 diff = boxPoints[i] - point;
      float distSqr = dot(diff, diff);

      if (distSqr < radius)
      {
         return true;
      }
   }

   return false;
}

int BinaryTree::Partition(std::vector<float> &vec, int low, int high)
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

void BinaryTree::QuickSort(std::vector<float> &vec, int low, int high)
{
   // Base case: This part will be executed till the starting
   // index low is lesser than the ending index high
   if (low < high)
   {
      // pi is Partitioning Index, arr[p] is now at
      // right place
      int pi = Partition(vec, low, high);

      // Separately sort elements before and after the
      // Partition Index pi
      QuickSort(vec, low, pi - 1);
      QuickSort(vec, pi + 1, high);
   }
}
