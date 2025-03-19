#include <iostream>
#include <cstdlib>

#include "vulkan_renderer.h"
#include "binaryTree.h"

int
main()
{
   // BinaryTree binary_tree;

   // return EXIT_SUCCESS;

   VulkanRenderer app;

   try
   {
      app.Run();
   }
   catch (const std::exception &e)
   {
      std::cerr << e.what() << '\n';
      return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}
