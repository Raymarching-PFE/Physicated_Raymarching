#include "binaryTree.h"

#include <vector>
#include <iostream>

BinaryTree::BinaryTree()
{
    std::vector<glm::vec3> sample =
    {
        glm::vec3(0.83f, 0.47f, 0.1f),
        glm::vec3(0.65f, -0.48f, 0.2f),
        glm::vec3(0.11f, 0.96f, 0.3f),
        glm::vec3(0.042f, -0.03f, 0.4f),
        glm::vec3(-0.13f, -0.98f, 0.5f),
        glm::vec3(-0.64f, 0.74f, 0.6f),
        glm::vec3(-0.77f, -0.62f, 0.7f),
        glm::vec3(-0.72f, 0.06f, 0.8f)
    };

    // get how many generations
    int gen = 0;
    while (pow(2, gen) < sample.size())
    {
        gen++;
    }

    std::cout << "ELements : " << sample.size() <<  ", Gen : "<< gen << std::endl;

    // create architecture from generation
    Node root = {glm::vec3(0, 0, 0), nullptr, nullptr};
    CreateStructureNodes(0, gen, root);

    // view tree
    ViewNode(root);
    ViewNode(root->right);


    // create root nodes
//    for(int i = 0; i < sample.size(); i++)
//    {
//        root.position = glm::vec3(sample[i].x, sample[i].y, sample[i].z);
//
//        // root node must be median between all x values nodes
//
//
//    }
}

void BinaryTree::ViewNode(Node node)
    {
    bool leftExist = node.left != nullptr;
    bool rightExist = node.right != nullptr;


    std::cout << "node : (" << node.position.x << ", " << node.position.y << ", " << node.position.z << "), " <<
     (leftExist?"left is true":"left is false") << ", " << (rightExist?"right is true":"right is false") << std::endl;

  }

void BinaryTree::CreateStructureNodes(int CurrGen, int maxGen, Node root)
{

  // first use case
//  if(currGen == 0)
//  {
//      Node newRoot = {glm::vec3(0, 0, 0), nullptr, nullptr};
//      root = newRoot;
//
//      if(currGen == maxGen)
//          return;
//      else
//          CreateStructureNodes(currGen, maxGen, root);
//  }
// else
// {
     Node newRight = {glm::vec3(0, 0, 0), nullptr, nullptr};
     Node newLeft = {glm::vec3(0, 0, 0), nullptr, nullptr};
     root.right = &newRight;
     root.left = &newLeft;

// }

    CurrGen++;

    if(CurrGen == maxGen)
        return;
    else
      {
        CreateStructureNodes(CurrGen, maxGen, *root.right);
        CreateStructureNodes(CurrGen, maxGen, *root.left);
}
}

BinaryTree::~BinaryTree()
{
  //TODO free allocations
}

//void BinaryTree::AddNode(Node* node)
//{
//
//}