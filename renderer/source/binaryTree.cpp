#include "binaryTree.h"

#include <vector>
#include <iostream>

BinaryTree::BinaryTree()
{
    // Generate fake data
    std::vector<glm::vec3> fakeData =
    {
        glm::vec3(0.1f, 0.0f, 0.0f),
        glm::vec3(0.5f, 1.0f, 1.0f),
        glm::vec3(0.3f, 2.0f, 2.0f)
    };

    for (int i = 0; i < fakeData.size(); i++)
        std::cout << "Data[" << i << "] : (" << fakeData[i].x << ", " << fakeData[i].y << ", " << fakeData[i].z <<")" << std::endl;

    // Get generation
    int generation = 0;
    while (pow(2, generation) < fakeData.size())
        generation++;

    std::cout << "Elements : " << fakeData.size() << ", Gen : " << generation << std::endl;

    // create architecture from generation
    Node* root = new Node({glm::vec3(0, 0, 0), nullptr, nullptr, 0});
    CreateStructureNodes(0, generation, root);

    // Give values for structures nodes
    root->position.x = MedianX(fakeData);

    // view tree
    ViewNodeRecursive(root);
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

    return vec[(vec.size()-1)/2];
}


void BinaryTree::ViewNode(Node* node)
{
    std::cout << "Node : (" << node->position.x << ", " << node->position.y << ", " << node->position.z << "), " <<
        (node->left != nullptr ? "TRUE" : "FALSE") << ", " << (node->right != nullptr ? "TRUE" : "FALSE") << std::endl;
}

void BinaryTree::ViewNodeRecursive(Node* node)
{
    ViewNode(node);

    if (node->left != nullptr)
        ViewNodeRecursive(node->left);
    if (node->right != nullptr)
        ViewNodeRecursive(node->right);
}

void BinaryTree::CreateStructureNodes(int CurrGen, int maxGen, Node* root)
{
    CurrGen++;

    root->left = new Node( {glm::vec3(CurrGen, 0, 0), nullptr, nullptr, CurrGen});
    root->right = new Node({glm::vec3(CurrGen, 0, 0), nullptr, nullptr, CurrGen});

    if(CurrGen != maxGen)
    {
        CreateStructureNodes(CurrGen, maxGen, root->right);
        CreateStructureNodes(CurrGen, maxGen, root->left);
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

    for (int j = low; j <= high - 1; j++) {

        // If current element is smaller than or
        // equal to pivot
        if (vec[j] <= pivot) {
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
    if (low < high) {

        // pi is Partitioning Index, arr[p] is now at
        // right place
        int pi = partition(vec, low, high);

        // Separately sort elements before and after the
        // Partition Index pi
        quickSort(vec, low, pi - 1);
        quickSort(vec, pi + 1, high);
    }
}