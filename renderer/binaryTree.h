#pragma once

#include <glm/glm.hpp>

struct Node
{
    glm::vec3 position = glm::vec3(-1, -1, -1);

    // Children
    Node* left = nullptr;
    Node* right = nullptr;

    // Debug only
    int generation = -1;
};

class BinaryTree
{
public:
    BinaryTree();
    ~BinaryTree();

private:
    void CreateStructureNodes(int CurrGen, int maxGen, Node* root);
    void ViewNode(Node* node);
    void ViewNodeRecursive(Node* node);

    float MedianX(std::vector<glm::vec3> data);


   Node* root;

    int partition(std::vector<float> &vec, int low, int high);
    void quickSort(std::vector<float> &vec, int low, int high);

};