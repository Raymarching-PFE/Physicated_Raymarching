#pragma once

//
// #define GLM_FORCE_RADIANS
// #define GLM_FORCE_DEPTH_ZERO_TO_ONE
// #define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
// #include <glm/gtx/hash.hpp>
// #include <glm/gtc/matrix_transform.hpp>



 struct Node
 {
     glm::vec3 position;

     // Children
     Node* right;
     Node* left;
};


class BinaryTree {
public:
    BinaryTree();
    ~BinaryTree();



private:
    void CreateStructureNodes(int CurrGen, int maxGen, Node* root);
void ViewNode(Node* node);

   Node root;
};