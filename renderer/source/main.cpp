#include "vulkan_renderer.h"

int main()
{
   VulkanRenderer app;

    try
    {
        app.Run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "\033[31m" << e.what() << '\n'; // Red

        return EXIT_FAILURE;
    }

   return EXIT_SUCCESS;
}
