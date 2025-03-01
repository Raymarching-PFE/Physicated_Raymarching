#include <iostream>
#include <cstdlib>

#include "vulkan_renderer.h"

//#if defined ( __clang__ ) || defined ( __GNUC__ )
//# define TracyFunction __PRETTY_FUNCTION__
//# elif defined ( _MSC_VER )
//# define TracyFunction __FUNCSIG__
//# endif
//# include <tracy/Tracy.hpp >

int main() 
{
    VulkanRenderer app;

    try 
    {
        app.Run();
    }
    catch (const std::exception& e) 
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}