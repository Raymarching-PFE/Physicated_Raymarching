#include "vulkan_renderer.h"

#include <fstream>
#include <set>
#include <random>
#include <glm/gtx/string_cast.hpp>

#define NUMBER_OF_UBO 1

#if defined(__clang__) || defined(__GNUC__)
    #define TracyFunction __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
    #define TracyFunction __FUNCSIG__
    #define TRACY_IMPORTS
#endif

#include "tracy/Tracy.hpp"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"

//#include "binaryTree.h"


void VulkanRenderer::Run()
{
    InitWindow();
    m_modelPaths = LoadPLYFilePaths("point_clouds/");
    m_modelCache.LoadAllModelsInCache(m_modelPaths);

    //LoadGeneratedPoint();

    InitVulkan();
    InitImGui();
    MainLoop();
    Cleanup();
}


// System utilities / callbacks
void VulkanRenderer::CheckVkResult(VkResult err)
{
    if (err == 0)
        return;

    std::cerr << "\033[31m" << "[vulkan] Error: VkResult = " << err << "\033[0m" << '\n'; // Red

    if (err < 0)
        abort();
}

void VulkanRenderer::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    VulkanRenderer* app = static_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
    app->m_framebufferResized = true;
}

void VulkanRenderer::MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    VulkanRenderer* app = static_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));

    if (app->m_isCursorCaptured)
    {
        if (app->m_firstMouse)
        {
            app->m_lastX = xpos;
            app->m_lastY = ypos;
            app->m_firstMouse = false;
        }

        float xoffset = xpos - app->m_lastX;
        float yoffset = app->m_lastY - ypos;
        app->m_lastX = xpos;
        app->m_lastY = ypos;

        float sensitivity = 0.1f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        app->m_yaw += xoffset;
        app->m_pitch -= yoffset;

        if (app->m_pitch > 89.0f) app->m_pitch = 89.0f;
        if (app->m_pitch < -89.0f) app->m_pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(app->m_yaw)) * cos(glm::radians(app->m_pitch));
        front.y = sin(glm::radians(app->m_pitch));
        front.z = sin(glm::radians(app->m_yaw)) * cos(glm::radians(app->m_pitch));
        app->m_cameraFront = glm::normalize(front);
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRenderer::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    switch (messageSeverity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: // Green
            std::cerr << "\033[32m" << "Validation layer: " << pCallbackData->pMessage << "\033[0m" << '\n';
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: // Light blue
            std::cerr << "\033[34m" << "Validation layer: " << pCallbackData->pMessage << "\033[0m" << '\n';
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: // Yellow
            std::cerr << "\033[33m" << "Validation layer: " << pCallbackData->pMessage << "\033[0m" << '\n';
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: // Red
            std::cerr << "\033[31m" << "Validation layer: " << pCallbackData->pMessage << "\033[0m" << '\n';
            break;
        default:
            break;
    }

    return VK_FALSE;
}


// General initialization
void VulkanRenderer::InitWindow()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_window = glfwCreateWindow(WIDTH, HEIGHT, "VulkanRenderer", nullptr, nullptr);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, FramebufferResizeCallback);
}

void VulkanRenderer::InitVulkan()
{
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain();
    CreateImageViews();
    CreateRenderPass();

#if COMPUTE
    CreateTextureSampler();
    CreateDescriptorSetLayout();
    CreateComputeDescriptorSetLayout();
    CreateGraphicsPipeline();
    CreateComputePipeline();

    //SendBinaryTreeToCompute();

    CreateCommandPool();
    CreateFramebuffers();
    LoadModel(m_modelPaths[m_currentModelIndex]);
    CreateUniformBuffers();

    // LoadGeneratedPoint();

    CreateDescriptorPool();
    CreateStorageImage();
    CreateDescriptorSets();
    CreateComputeDescriptorSets();
    CreateCommandBuffers();
    CreateComputeCommandBuffers();
#else
    CreateTextureSampler();
    CreateDescriptorSetLayout();
    CreateGraphicsPipeline();
    CreateCommandPool();
    CreateFramebuffers();
    LoadModel(m_modelPaths[m_currentModelIndex]);
    //LoadGeneratedPoint();
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSets();
    CreateCommandBuffers();
#endif

    CreateSyncObjects();

    glfwSetCursorPosCallback(m_window, MouseCallback);
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void VulkanRenderer::MainLoop()
{
    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();
        glfwPostEmptyEvent();
        ProcessInput(m_window);

        MainImGui();

        BeginFrame();
        DrawFrame();
        EndFrame();
    }
}

void VulkanRenderer::Cleanup()
{
    vkDeviceWaitIdle(m_device);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    CleanupSwapChain();

#if COMPUTE
    // Compute-specific pipelines
    vkDestroyPipeline(m_device, m_computePipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_computePipelineLayout, nullptr);
#endif

    vkDestroyPipeline(m_device, m_graphicsComputePipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_graphicsComputePipelineLayout, nullptr);

    // Graphics pipeline
    vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);

    vkDestroyRenderPass(m_device, m_renderPass, nullptr);

    for (size_t j = 0; j < NUMBER_OF_UBO; ++j)
    {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            if (m_uniformBuffers[i + j * NUMBER_OF_UBO] != VK_NULL_HANDLE)
                vkDestroyBuffer(m_device, m_uniformBuffers[i + j * NUMBER_OF_UBO], nullptr);
            if (m_uniformBuffersMemory[i + j * NUMBER_OF_UBO] != VK_NULL_HANDLE)
                vkFreeMemory(m_device, m_uniformBuffersMemory[i + j * NUMBER_OF_UBO], nullptr);
        }
    }

    // Descriptor layouts and pool
    if (m_descriptorPool != VK_NULL_HANDLE)
        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

    if (m_descriptorSetLayout != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

#if COMPUTE
    if (m_computeDescriptorSetLayout != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(m_device, m_computeDescriptorSetLayout, nullptr);
#endif

    // --- Shared Resources ---
    if (m_textureSampler != VK_NULL_HANDLE)
        vkDestroySampler(m_device, m_textureSampler, nullptr);

    if (m_textureImageView != VK_NULL_HANDLE)
        vkDestroyImageView(m_device, m_textureImageView, nullptr);

    DestroyMeshBuffers();

#if COMPUTE
    DestroyBinaryTreeResources();
#endif

    // --- Sync Objects (common to both modes if ImGui/swapchain is used) ---
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        if (m_renderFinishedSemaphores[i] != VK_NULL_HANDLE)
            vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
        if (m_imageAvailableSemaphores[i] != VK_NULL_HANDLE)
            vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
        if (m_inFlightFences[i] != VK_NULL_HANDLE)
            vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
#if COMPUTE
        if (m_computeFinishedSemaphores[i] != VK_NULL_HANDLE)
            vkDestroySemaphore(m_device, m_computeFinishedSemaphores[i], nullptr);
        if (m_computeInFlightFences[i] != VK_NULL_HANDLE)
            vkDestroyFence(m_device, m_computeInFlightFences[i], nullptr);
        if (m_transitionFinishedSemaphores[i] != VK_NULL_HANDLE)
            vkDestroySemaphore(m_device, m_transitionFinishedSemaphores[i], nullptr);
        for (TransitionCmd& cmd : m_transitionCommandBuffers[i])
        {
            if (cmd.fence != VK_NULL_HANDLE)
                vkDestroyFence(m_device, cmd.fence, nullptr);
        }
#endif
    }

    // --- Storage Image (used for compute rendering output) ---
#if COMPUTE
    if (m_storageImageView != VK_NULL_HANDLE)
        vkDestroyImageView(m_device, m_storageImageView, nullptr);
    if (m_storageImage != VK_NULL_HANDLE)
        vkDestroyImage(m_device, m_storageImage, nullptr);
    if (m_storageImageMemory != VK_NULL_HANDLE)
        vkFreeMemory(m_device, m_storageImageMemory, nullptr);
#endif

    // --- Core Vulkan Cleanup ---
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    vkDestroyDevice(m_device, nullptr);

    if (enableValidationLayers)
        DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);

    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void VulkanRenderer::RecreateSwapChain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_window, &width, &height);

    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(m_window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(m_device);

    CleanupSwapChain();

    CreateSwapChain();
    CreateImageViews();
    CreateFramebuffers();

    ImGui_ImplVulkan_SetMinImageCount(m_minImageCount);
}


// ImGui
void VulkanRenderer::InitImGui() const
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(m_window, true);

    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = m_instance;
    initInfo.PhysicalDevice = m_physicalDevice;
    initInfo.Device = m_device;
    initInfo.QueueFamily = m_queueFamily;
    initInfo.Queue = m_graphicsQueue;
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = m_descriptorPool;
    initInfo.RenderPass = m_renderPass;
    initInfo.Subpass = 0;
    initInfo.MinImageCount = m_minImageCount;
    initInfo.ImageCount = m_imageCount;
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.Allocator = nullptr;
    initInfo.CheckVkResultFn = CheckVkResult;

    ImGui_ImplVulkan_Init(&initInfo);
}

void VulkanRenderer::MainImGui()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Physicated Raymarching");

        ProcessInput(m_window);

        if (!m_modelPaths.empty())
        {
            std::vector<const char*> modelNames;
            for (const std::string& path : m_modelPaths)
                modelNames.push_back(path.c_str());

            static int selectedIndex = m_currentModelIndex;
            if (ImGui::Combo("Select .ply file", &selectedIndex, modelNames.data(), static_cast<int>(modelNames.size())))
            {
                if (selectedIndex != m_currentModelIndex)
                {
                    m_currentModelIndex = selectedIndex;
                    ReloadModel(m_modelPaths[m_currentModelIndex]);
                }
            }
        }

        ImGui::Text("Number of points: %zu", m_vertexNb);

        ImGuiIO& io = ImGui::GetIO();
        float clampedFPS = std::min(io.Framerate, 144.0f);
        ImGui::Text("Application average: %.3f ms/frame (%.1f FPS)", 1000.0f / clampedFPS, clampedFPS);

    ImGui::End();

    ImGui::Render();
}


// Instance / Device / Surface / Debug
void VulkanRenderer::CreateInstance()
{
    if (enableValidationLayers && !CheckValidationLayerSupport())
        throw std::runtime_error("Validation layers requested, but not available!");

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Physicated Raymarching";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    const std::vector<const char*> extensions = GetRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        PopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
        throw std::runtime_error("Failed to create instance!");
}

void VulkanRenderer::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;
}

void VulkanRenderer::SetupDebugMessenger()
{
    if constexpr (!enableValidationLayers)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    PopulateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
        throw std::runtime_error("Failed to set up debug messenger!");
}

void VulkanRenderer::CreateSurface()
{
    if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface!");
}

void VulkanRenderer::PickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (deviceCount == 0)
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    for (const VkPhysicalDevice& device : devices)
    {
        if (IsDeviceSuitable(device))
        {
            m_physicalDevice = device;
            break;
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE)
        throw std::runtime_error("Failed to find a suitable GPU!");
}

void VulkanRenderer::CreateLogicalDevice()
{
    QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsAndComputeFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.sampleRateShading = VK_TRUE; // enable sample shading feature for the device

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
        createInfo.enabledLayerCount = 0;

    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
        throw std::runtime_error("Failed to create logical device!");

#if COMPUTE
    vkGetDeviceQueue(m_device, indices.graphicsAndComputeFamily.value(), 0, &m_computeQueue);
#endif

    vkGetDeviceQueue(m_device, indices.graphicsAndComputeFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
}

bool VulkanRenderer::IsDeviceSuitable(const VkPhysicalDevice device)
{
    const QueueFamilyIndices indices = FindQueueFamilies(device);

    const bool extensionsSupported = CheckDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        const SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.IsComplete() && extensionsSupported && swapChainAdequate;
}

bool VulkanRenderer::CheckDeviceExtensionSupport(const VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const VkExtensionProperties& extension : availableExtensions)
        requiredExtensions.erase(extension.extensionName);

    return requiredExtensions.empty();
}

QueueFamilyIndices VulkanRenderer::FindQueueFamilies(const VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const VkQueueFamilyProperties& queueFamily : queueFamilies)
    {
#if COMPUTE
        if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT))
            indices.graphicsAndComputeFamily = i;
#else
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphicsAndComputeFamily = i;
#endif

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);

        if (presentSupport)
            indices.presentFamily = i;

        if (indices.IsComplete())
            break;

        ++i;
    }

    m_queueFamily = indices.graphicsAndComputeFamily.value();

    return indices;
}

std::vector<const char*> VulkanRenderer::GetRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
}

bool VulkanRenderer::CheckValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers)
    {
        bool layerFound = false;

        for (const VkLayerProperties& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
            return false;
    }

    return true;
}


// Swapchain
void VulkanRenderer::CreateSwapChain()
{
    const SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_physicalDevice);

    const VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    const VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    const VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        imageCount = swapChainSupport.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    m_minImageCount = swapChainSupport.capabilities.minImageCount;
    m_imageCount = imageCount;

    const QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice);
    const uint32_t queueFamilyIndices[] = {indices.graphicsAndComputeFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsAndComputeFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
        throw std::runtime_error("Failed to create swap chain!");

    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
    m_swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());

    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;
}

void VulkanRenderer::CreateImageViews()
{
    m_swapChainImageViews.resize(m_swapChainImages.size());

    for (size_t i = 0; i < m_swapChainImages.size(); ++i)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_device, &createInfo, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create image views!");
    }
}

SwapChainSupportDetails VulkanRenderer::QuerySwapChainSupport(const VkPhysicalDevice device) const
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkExtent2D VulkanRenderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;
    else
    {
        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);

        VkExtent2D actualExtent =
        {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

VkSurfaceFormatKHR VulkanRenderer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const VkSurfaceFormatKHR& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;
    }

    return availableFormats[0];
}

VkPresentModeKHR VulkanRenderer::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const VkPresentModeKHR& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return availablePresentMode;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

void VulkanRenderer::CleanupSwapChain() const
{
    for (VkFramebuffer framebuffer : m_swapChainFramebuffers)
        vkDestroyFramebuffer(m_device, framebuffer, nullptr);

    for (VkImageView imageView : m_swapChainImageViews)
        vkDestroyImageView(m_device, imageView, nullptr);

    vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
}


// Render Pass / Pipelines
void VulkanRenderer::CreateRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 1> attachments = {colorAttachment};
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
        throw std::runtime_error("Failed to create render pass!");
}

void VulkanRenderer::CreateGraphicsPipeline()
{
    // Vertex Shader
    {
        std::vector<uint32_t> shCode;

        CompileShaderFromFile("shaders/basic_Raymarching.vert", shaderc_vertex_shader, shCode);

        const VkShaderModuleCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0u,
            .codeSize = static_cast<uint32_t>(shCode.size()) * sizeof(uint32_t),
            .pCode = shCode.data(),
        };

        const VkResult vrShaderCompile = vkCreateShaderModule(m_device, &createInfo, nullptr, &m_vertexShader);
        if (vrShaderCompile != VK_SUCCESS)
        {
            std::cerr << "\033[31m" << "Create Vertex Shader failed!" << "\033[0m" << '\n'; // Red
            std::cerr << "\033[31m" << "Error code: " << vrShaderCompile << "\033[0m" << '\n'; // Red
        }
        else
            std::cerr << "\033[32m" << "Create Vertex Shader success" << "\033[0m" << '\n'; // Green
    }

    // Fragment Shader
    {
        std::vector<uint32_t> shCode;

#if COMPUTE
        CompileShaderFromFile("shaders/compute_Raymarching.frag", shaderc_fragment_shader, shCode);
#else
        CompileShaderFromFile("shaders/basic_Raymarching.frag", shaderc_fragment_shader, shCode);
#endif

        const VkShaderModuleCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0u,
            .codeSize = static_cast<uint32_t>(shCode.size()) * sizeof(uint32_t),
            .pCode = shCode.data(),
        };

        const VkResult vrShaderCompile = vkCreateShaderModule(m_device, &createInfo, nullptr, &m_fragmentShader);
        if (vrShaderCompile != VK_SUCCESS)
        {
            std::cerr << "\033[31m" << "Create Vertex Shader failed!" << "\033[0m" << '\n'; // Red
            std::cerr << "\033[31m" << "Error code: " << vrShaderCompile << "\033[0m" << '\n'; // Red
        }
        else
            std::cerr << "\033[32m" << "Create Fragment Shader success" << "\033[0m" << '\n'; // Green
    }

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = m_vertexShader;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = m_fragmentShader;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // === Pipeline Configuration ===
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // === Pipeline Layout ===
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;

    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create pipeline layout!");

    // === Graphics Pipeline ===
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create graphics pipeline!");

    vkDestroyShaderModule(m_device, m_fragmentShader, nullptr);
    vkDestroyShaderModule(m_device, m_vertexShader, nullptr);
}

void VulkanRenderer::CreateDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create graphics descriptor set layout!");
}

void VulkanRenderer::CreateDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate descriptor sets!");

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        // --- UBO : binding 0 ---
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffers[i * NUMBER_OF_UBO];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

#if COMPUTE
        // --- Sampler image (binding 1) — on utilise _storageImageView ---
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_storageImageView;
        imageInfo.sampler = m_textureSampler;
        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
#else
        std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

#endif
        // Binding 0 : UBO
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = m_descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

#if COMPUTE
        // Binding 1 : Combined image sampler (texture)
        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = m_descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

#endif
        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

bool VulkanRenderer::HasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void VulkanRenderer::CreateFramebuffers()
{
    m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

    for (size_t i = 0; i < m_swapChainImageViews.size(); ++i)
    {
        std::array<VkImageView, 1> attachments =
        {
            m_swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_swapChainExtent.width;
        framebufferInfo.height = m_swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create framebuffer!");
    }
}

void VulkanRenderer::CreateCommandPool()
{
    const QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily.value();

    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create command pool!");
}


// Shaders
VkShaderModule VulkanRenderer::CreateShaderModule(const std::vector<char>& code) const
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        throw std::runtime_error("Failed to create shader module!");

    return shaderModule;
}

std::vector<char> VulkanRenderer::ReadFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        throw std::runtime_error("Failed to open file!");

    const size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}


// Buffers & Memory
void VulkanRenderer::CreateVertexBuffer()
{
    DestroyMeshBuffers();

    const VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(m_device, stagingBufferMemory);

    CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory);

    CopyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);
}

void VulkanRenderer::CreateIndexBuffer()
{
    DestroyMeshBuffers();

    std::vector<uint32_t> quadIndices = { 0, 1, 2, 2, 3, 0 };

    const VkDeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();
    const VkDeviceSize quadBufferSize = sizeof(uint32_t) * quadIndices.size();

    //std::cout << "Index buffer size: " << bufferSize << std::endl;
    //std::cout << "Index buffer size: " << quadBufferSize << std::endl;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    VkBuffer quadStagingBuffer;
    VkDeviceMemory quadStagingBufferMemory;
    CreateBuffer(quadBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, quadStagingBuffer, quadStagingBufferMemory);

    void* data;
    vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_indices.data(), (size_t) bufferSize);
    vkUnmapMemory(m_device, stagingBufferMemory);

    // Copie des indices dans le buffer
    void* quadData;
    vkMapMemory(m_device, quadStagingBufferMemory, 0, quadBufferSize, 0, &quadData);
    memcpy(quadData, quadIndices.data(), quadBufferSize);
    vkUnmapMemory(m_device, quadStagingBufferMemory);

    CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory);
    CreateBuffer(quadBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_quadIndexBuffer, m_quadIndexBufferMemory);

    CopyBuffer(stagingBuffer, m_indexBuffer, bufferSize);
    CopyBuffer(quadStagingBuffer, m_quadIndexBuffer, quadBufferSize);

    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);

    vkDestroyBuffer(m_device, quadStagingBuffer, nullptr);
    vkFreeMemory(m_device, quadStagingBufferMemory, nullptr);
}

void VulkanRenderer::CreateUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    m_uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    m_uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_uniformBuffers[i], m_uniformBuffersMemory[i]);
        vkMapMemory(m_device, m_uniformBuffersMemory[i], 0, bufferSize, 0, &m_uniformBuffersMapped[i]);
    }
}

void VulkanRenderer::CreateDescriptorPool()
{
    // Constantes lisibles et ajustables
    constexpr uint32_t DESCRIPTORS_PER_TYPE = 64;         // Assez large pour ImGui + app
    constexpr uint32_t MAX_IMGUI_OVERHEAD   = 64;         // Pour les besoins internes d'ImGui
    constexpr uint32_t MAX_DESCRIPTOR_SETS  = MAX_FRAMES_IN_FLIGHT * 4 + MAX_IMGUI_OVERHEAD;

    std::array<VkDescriptorPoolSize, 4> poolSizes{};
    poolSizes[0] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         DESCRIPTORS_PER_TYPE };
    poolSizes[1] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, DESCRIPTORS_PER_TYPE };
    poolSizes[2] = { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          DESCRIPTORS_PER_TYPE };
    poolSizes[3] = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         DESCRIPTORS_PER_TYPE };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = MAX_DESCRIPTOR_SETS;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create descriptor pool!");
}

void VulkanRenderer::CreateCommandBuffers()
{
    m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

    if (vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers!");
}

void VulkanRenderer::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) const
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        throw std::runtime_error("Failed to begin recording graphics command buffer!");

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_swapChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_swapChainExtent.width);
        viewport.height = static_cast<float>(m_swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = m_swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // Dessin
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[m_currentFrame], 0, nullptr);
        vkCmdDraw(commandBuffer, 6, 1, 0, 0);  // Quad complet

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to record command buffer!");
}

void VulkanRenderer::UpdateUniformBuffer(uint32_t currentImage) const
{
    UniformBufferObject ubo{};
    ubo.cameraPos = m_cameraPos;
    ubo.cameraFront = m_cameraFront;
    ubo.time = static_cast<float>(glfwGetTime());

    // ubo.spheresArray[0] = glm::vec4(GeneratedPoint[0].x, GeneratedPoint[0].y, GeneratedPoint[0].z, 0.0f);
    // ubo.spheresArray[1] = glm::vec4(GeneratedPoint[1].x, GeneratedPoint[1].y, GeneratedPoint[1].z, 0.0f);
    // ubo.spheresArray[2] = glm::vec4(GeneratedPoint[2].x, GeneratedPoint[2].y, GeneratedPoint[2].z, 0.0f);
    // ubo.spheresArray[3] = glm::vec4(GeneratedPoint[3].x, GeneratedPoint[3].y, GeneratedPoint[3].z, 0.0f);
    // ubo.spheresArray[4] = glm::vec4(GeneratedPoint[4].x, GeneratedPoint[4].y, GeneratedPoint[4].z, 0.0f);
    // ubo.spheresArray[5] = glm::vec4(GeneratedPoint[5].x, GeneratedPoint[5].y, GeneratedPoint[5].z, 0.0f);
    // ubo.spheresArray[6] = glm::vec4(GeneratedPoint[6].x, GeneratedPoint[6].y, GeneratedPoint[6].z, 0.0f);
    // ubo.spheresArray[7] = glm::vec4(GeneratedPoint[7].x, GeneratedPoint[7].y, GeneratedPoint[7].z, 0.0f);

#if COMPUTE
#else

    ubo.spheresArray[0] = glm::vec4(0.0f, 0.0f, -7.0f, 0.5f);// center
    ubo.spheresArray[1] = glm::vec4(-3.0f, -1.5f, -7.0f, 0.5f);// min
    ubo.spheresArray[2] = glm::vec4(3.0f, 1.5f, -5.0f, 0.5f);// max
    ubo.spheresArray[3] = glm::vec4(3.0f, 0.0f, -7.0f, 0.5f);
    ubo.spheresArray[4] = glm::vec4(-1.0f, 0.0f, -7.0f, 0.5f);
    ubo.spheresArray[5] = glm::vec4(-2.0f, 0.0f, -7.0f, 0.5f);
    ubo.spheresArray[6] = glm::vec4(-3.0f, 0.0f, -7.0f, 0.5f);
    ubo.spheresArray[7] = glm::vec4(-4.0f, 0.0f, -7.0f, 0.5f);
#endif
    ubo.sphereNumber = 6;
	//std::cout << "Time: " << ubo.time << std::endl;
	//std::cout << "Position along time: " << 2.5f + 7.5f * sin(ubo.time) << std::endl;
    //std::cout << "Frame: " << m_currentFrame << ", Time: " << ubo.time << std::endl;

#if COMPUTE

    size_t arrayMaxSize = 2526;

   // Update binary tree data
    // TODO put it in a SRV buffer instead
   std::copy(m_binary_tree.GPUReadyBuffer.begin(), m_binary_tree.GPUReadyBuffer.begin() + std::min(m_binary_tree.GPUReadyBuffer.size(), arrayMaxSize), ubo.nodes);
#endif

    memcpy(m_uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void VulkanRenderer::CreateSyncObjects()
{
#if COMPUTE
    m_computeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_computeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    m_transitionFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
#endif

    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
#if COMPUTE
        if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_computeFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_transitionFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_device, &fenceInfo, nullptr, &m_computeInFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create compute synchronization objects for a frame!");
        }
#endif
        if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create graphics synchronization objects for a frame!");
        }
    }
}

void VulkanRenderer::CreateTextureSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS)
        throw std::runtime_error("Failed to create texture sampler!");
}

void VulkanRenderer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to create buffer!");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate buffer memory!");

    vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
}

VkCommandBuffer VulkanRenderer::BeginSingleTimeCommands() const
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VulkanRenderer::EndSingleTimeCommands(VkCommandBuffer commandBuffer) const
{
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to end command buffer!");

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
        throw std::runtime_error("Failed to submit command buffer!");

    vkQueueWaitIdle(m_graphicsQueue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

void VulkanRenderer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    EndSingleTimeCommands(commandBuffer);
}

uint32_t VulkanRenderer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}


// Images
void VulkanRenderer::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) const
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL)
{
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
{
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
{
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

        // S'assurer que le aspectMask est correct pour le depth (avec ou sans stencil)
        if (HasStencilComponent(format))
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        else
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL)
    {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    else
        throw std::invalid_argument("Unsupported layout transition!");

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage,
        destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    EndSingleTimeCommands(commandBuffer);
}


// Render
void VulkanRenderer::BeginFrame()
{
#if COMPUTE
    vkWaitForFences(m_device, 1, &m_computeInFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(m_device, 1, &m_computeInFlightFences[m_currentFrame]);
    vkResetCommandBuffer(m_computeCommandBuffers[m_currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
#endif

    vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);
    vkResetCommandBuffer(m_commandBuffers[m_currentFrame], /*VkCommandBufferResetFlagBits*/ 0);

    VkResult result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &m_imageIndex);

#if COMPUTE
    // On retransforme l'image en GENERAL pour le compute à venir
    ComputeTransitionImageLayout(
        m_storageImage,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        m_storageImageLayout,
        VK_IMAGE_LAYOUT_GENERAL,
        1,
        m_computeQueue,
        VK_NULL_HANDLE,
        VK_NULL_HANDLE,
        1
    );

    m_storageImageLayout = VK_IMAGE_LAYOUT_GENERAL; // met à jour le layout courant

#endif

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        RecreateSwapChain();
        return;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        throw std::runtime_error("Failed to acquire swap chain image!");
}

void VulkanRenderer::DrawFrame()
{
    UpdateUniformBuffer(m_currentFrame);

#if COMPUTE
     // --- 1. Record and submit compute ---
    RecordComputeCommandBuffer(m_computeCommandBuffers[m_currentFrame]);

    VkSubmitInfo computeSubmitInfo{};
    computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    computeSubmitInfo.commandBufferCount = 1;
    computeSubmitInfo.pCommandBuffers = &m_computeCommandBuffers[m_currentFrame];
    computeSubmitInfo.signalSemaphoreCount = 1;
    computeSubmitInfo.pSignalSemaphores = &m_computeFinishedSemaphores[m_currentFrame];

    if (vkQueueSubmit(m_computeQueue, 1, &computeSubmitInfo, m_computeInFlightFences[m_currentFrame]) != VK_SUCCESS)
        throw std::runtime_error("Failed to submit compute command buffer!");

    // --- 2. Transition image layout: GENERAL → SHADER_READ_ONLY_OPTIMAL
    ComputeTransitionImageLayout(
        m_storageImage,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        m_storageImageLayout,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        1,
        m_graphicsQueue,
        m_computeFinishedSemaphores[m_currentFrame],
        m_transitionFinishedSemaphores[m_currentFrame],
        0
    );

    m_storageImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // met à jour le layout courant

    // --- 3. Record and submit draw command buffer
    RecordCommandBuffer(m_commandBuffers[m_currentFrame], m_imageIndex);

    VkSemaphore waitSemaphores[] = {
        m_transitionFinishedSemaphores[m_currentFrame],
        m_imageAvailableSemaphores[m_currentFrame]
    };
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSubmitInfo graphicsSubmitInfo{};
    graphicsSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    graphicsSubmitInfo.waitSemaphoreCount = 2;
    graphicsSubmitInfo.pWaitSemaphores = waitSemaphores;
    graphicsSubmitInfo.pWaitDstStageMask = waitStages;
    graphicsSubmitInfo.commandBufferCount = 1;
    graphicsSubmitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];
    graphicsSubmitInfo.signalSemaphoreCount = 1;
    graphicsSubmitInfo.pSignalSemaphores = &m_renderFinishedSemaphores[m_currentFrame];

    if (vkQueueSubmit(m_graphicsQueue, 1, &graphicsSubmitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
        throw std::runtime_error("Failed to submit draw command buffer!");

#else
    // --- Graphics only ---
    RecordCommandBuffer(m_commandBuffers[m_currentFrame], m_imageIndex);

    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };

    VkSubmitInfo graphicsSubmitInfo{};
    graphicsSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    graphicsSubmitInfo.waitSemaphoreCount = 1;
    graphicsSubmitInfo.pWaitSemaphores = waitSemaphores;
    graphicsSubmitInfo.pWaitDstStageMask = waitStages;
    graphicsSubmitInfo.commandBufferCount = 1;
    graphicsSubmitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];
    graphicsSubmitInfo.signalSemaphoreCount = 1;
    graphicsSubmitInfo.pSignalSemaphores = &m_renderFinishedSemaphores[m_currentFrame];

    if (vkQueueSubmit(m_graphicsQueue, 1, &graphicsSubmitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
        throw std::runtime_error("Failed to submit draw command buffer!");
#endif
}

void VulkanRenderer::EndFrame()
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &m_renderFinishedSemaphores[m_currentFrame];

    const VkSwapchainKHR swapChains[] = { m_swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &m_imageIndex;

    VkResult result = vkQueuePresentKHR(m_presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized)
    {
        m_framebufferResized = false;
        RecreateSwapChain();
    }
    else if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to present swap chain image!");

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

// Models & Binary tree
void VulkanRenderer::LoadModel(const std::string& path)
{
    if (!m_modelCache.LoadModelInCache(path))
    {
        std::cerr << "Error loading model from cache: " << path << std::endl;
        return;
    }

    const CachedModel& cachedModel = m_modelCache.GetModelFromCache(path);

    m_vertices = cachedModel.m_cachedVertices;
    m_indices = cachedModel.m_cachedIndices;
    m_vertexNb = cachedModel.m_cachedVertexCount;

#if COMPUTE
    // TODO fill binarytree with model vertices
    std::vector<glm::vec3> cloudPoints;
    for (int i  = 0; i < m_vertices.size(); i++)
    {
        cloudPoints.push_back(m_vertices[i].pos);
    }

    m_binary_tree = BinaryTree(cloudPoints);

#endif

    CreateVertexBuffer();
    CreateIndexBuffer();
}

void VulkanRenderer::ReloadModel(const std::string& path)
{
    vkDeviceWaitIdle(m_device);

    DestroyModelResources();
    LoadModel(path);
}

void VulkanRenderer::DestroyModelResources()
{
    if (m_indexBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
        vkFreeMemory(m_device, m_indexBufferMemory, nullptr);
        m_indexBuffer = VK_NULL_HANDLE;
        m_indexBufferMemory = VK_NULL_HANDLE;
    }

    if (m_vertexBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
        vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
        m_vertexBuffer = VK_NULL_HANDLE;
        m_vertexBufferMemory = VK_NULL_HANDLE;
    }

    if (m_quadIndexBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_device, m_quadIndexBuffer, nullptr);
        vkFreeMemory(m_device, m_quadIndexBufferMemory, nullptr);
        m_quadIndexBuffer = VK_NULL_HANDLE;
        m_quadIndexBufferMemory = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::DestroyMeshBuffers()
{
    if (m_vertexBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
        m_vertexBuffer = VK_NULL_HANDLE;
    }

    if (m_vertexBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
        m_vertexBufferMemory = VK_NULL_HANDLE;
    }

    if (m_indexBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
        m_indexBuffer = VK_NULL_HANDLE;
    }

    if (m_indexBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_device, m_indexBufferMemory, nullptr);
        m_indexBufferMemory = VK_NULL_HANDLE;
    }

    if (m_quadIndexBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_device, m_quadIndexBuffer, nullptr);
        m_quadIndexBuffer = VK_NULL_HANDLE;
    }

    if (m_quadIndexBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_device, m_quadIndexBufferMemory, nullptr);
        m_quadIndexBufferMemory = VK_NULL_HANDLE;
    }
}


// Inputs & Timings
float VulkanRenderer::GetDeltaTime()
{
    std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();

    std::chrono::duration<float> deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(currentTime - m_lastTime);

    m_lastTime = currentTime;

    return deltaTime.count();
}

void VulkanRenderer::ProcessInput(GLFWwindow* window)
{
    float cameraSpeed = 2.5f * GetDeltaTime();
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        m_cameraPos += cameraSpeed * m_cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        m_cameraPos -= cameraSpeed * m_cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        m_cameraPos -= glm::normalize(glm::cross(m_cameraFront, m_cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        m_cameraPos += glm::normalize(glm::cross(m_cameraFront, m_cameraUp)) * cameraSpeed;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        if (m_isCursorCaptured)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            m_isCursorCaptured = false;
        }
    }

    if (!m_isCursorCaptured)
    {
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            if (!ImGui::IsAnyItemHovered() && !ImGui::IsWindowHovered())
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                m_isCursorCaptured = true;
            }
        }
    }
}


// Compute
#if COMPUTE
void VulkanRenderer::CreateStorageImage()
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    imageInfo.extent.width = m_swapChainExtent.width;
    imageInfo.extent.height = m_swapChainExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage =
        VK_IMAGE_USAGE_STORAGE_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(m_device, &imageInfo, nullptr, &m_storageImage) != VK_SUCCESS)
        throw std::runtime_error("Failed to create storage image!");

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, m_storageImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_storageImageMemory) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate storage image memory!");

    vkBindImageMemory(m_device, m_storageImage, m_storageImageMemory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_storageImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_storageImageView) != VK_SUCCESS)
        throw std::runtime_error("Failed to create storage image view!");

    // Transition initiale : UNDEFINED -> GENERAL si tu veux commencer par le compute
    TransitionImageLayout(
        m_storageImage,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL,
        1
    );

    m_storageImageLayout = VK_IMAGE_LAYOUT_GENERAL;
}

void VulkanRenderer::CreateComputePipeline()
{
    if (m_computePipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(m_device, m_computePipelineLayout, nullptr);
        m_computePipelineLayout = VK_NULL_HANDLE;
    }
    if (m_computePipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_device, m_computePipeline, nullptr);
        m_computePipeline = VK_NULL_HANDLE;
    }

    std::vector<uint32_t> shCode;

    CompileShaderFromFile("shaders/basic_Raymarching.comp", shaderc_compute_shader, shCode);

    const VkShaderModuleCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0u,
        .codeSize = static_cast<uint32_t>(shCode.size()) * sizeof(uint32_t),
        .pCode = shCode.data(),
    };

    const VkResult vrShaderCompile = vkCreateShaderModule(m_device, &createInfo, nullptr, &m_computeShader);
    if (vrShaderCompile != VK_SUCCESS)
    {
        std::cerr << "\033[31m" << "Create Compute Shader failed!" << "\033[0m" << '\n'; // Red
        std::cerr << "\033[31m" << "Error code: " << vrShaderCompile << "\033[0m" << '\n'; // Red
    }
    else
        std::cerr << "\033[32m" << "Create Compute Shader success" << "\033[0m" << '\n'; // Green

    VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
    computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStageInfo.module = m_computeShader;
    computeShaderStageInfo.pName = "main";

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_computeDescriptorSetLayout;

    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_computePipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create compute pipeline layout!");

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = m_computePipelineLayout;
    pipelineInfo.stage = computeShaderStageInfo;

    if (vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_computePipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create compute pipeline!");

    vkDestroyShaderModule(m_device, m_computeShader, nullptr);
}

void VulkanRenderer::CreateComputeDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding imageLayoutBinding{};
    imageLayoutBinding.binding = 1;
    imageLayoutBinding.descriptorCount = 1;
    imageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    imageLayoutBinding.pImmutableSamplers = nullptr;
    imageLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, imageLayoutBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_computeDescriptorSetLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create compute descriptor set layout!");
}

void VulkanRenderer::CreateComputeDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_computeDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    m_computeDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(m_device, &allocInfo, m_computeDescriptorSets.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate descriptor sets!");

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        VkDescriptorBufferInfo uniformBufferInfo{};
        uniformBufferInfo.buffer = m_uniformBuffers[i * NUMBER_OF_UBO];
        uniformBufferInfo.offset = 0;
        uniformBufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageInfo.imageView = m_storageImageView;
        imageInfo.sampler = VK_NULL_HANDLE;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = m_computeDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &uniformBufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = m_computeDescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

void VulkanRenderer::CreateComputeCommandBuffers()
{
    m_computeCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_computeCommandBuffers.size());

    std::vector<VkCommandBuffer> tempTransitionBuffers(MAX_FRAMES_IN_FLIGHT * 2);

    VkCommandBufferAllocateInfo transitionAllocInfo{};
    transitionAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    transitionAllocInfo.commandPool = m_commandPool;
    transitionAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    transitionAllocInfo.commandBufferCount = static_cast<uint32_t>(tempTransitionBuffers.size());

    if (vkAllocateCommandBuffers(m_device, &transitionAllocInfo, tempTransitionBuffers.data()) != VK_SUCCESS ||
        vkAllocateCommandBuffers(m_device, &allocInfo, m_computeCommandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate transition command buffers!");

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Important !

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            m_transitionCommandBuffers[i][j].buffer = tempTransitionBuffers[i * 2 + j];
            if (vkCreateFence(m_device, &fenceInfo, nullptr, &m_transitionCommandBuffers[i][j].fence) != VK_SUCCESS)
                throw std::runtime_error("Failed to create transition fence!");
        }
    }
}

void VulkanRenderer::RecordComputeCommandBuffer(VkCommandBuffer commandBuffer) const
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        throw std::runtime_error("Failed to begin recording compute command buffer!");

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayout, 0, 1, &m_computeDescriptorSets[m_currentFrame], 0, nullptr);

    vkCmdDispatch(commandBuffer, m_swapChainExtent.width / 16, m_swapChainExtent.height / 16, 1);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to record compute command buffer!");
}

void VulkanRenderer::ComputeTransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, VkQueue queue, VkSemaphore waitOn, VkSemaphore signalOut, uint32_t index)
{
    if (oldLayout == newLayout)
        return;

    TransitionCmd& cmd = m_transitionCommandBuffers[m_currentFrame][index];

    // Attendre que le GPU ait fini d'utiliser ce command buffer
    vkWaitForFences(m_device, 1, &cmd.fence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_device, 1, &cmd.fence);
    vkResetCommandBuffer(cmd.buffer, 0);

    // Début d'enregistrement
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(cmd.buffer, &beginInfo) != VK_SUCCESS)
        throw std::runtime_error("Failed to begin recording transition command buffer!");

    // Configuration de la barrière d'image
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags srcStage{}, dstStage{};

    if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL)
    {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        throw std::invalid_argument("Unsupported layout transition in ComputeTransitionImageLayout.");
    }

    vkCmdPipelineBarrier(
        cmd.buffer,
        srcStage, dstStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    if (vkEndCommandBuffer(cmd.buffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to record transition command buffer!");

    // Préparation de la soumission
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd.buffer;

    if (waitOn != VK_NULL_HANDLE)
    {
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &waitOn;
        submitInfo.pWaitDstStageMask = &srcStage;
    }

    if (signalOut != VK_NULL_HANDLE)
    {
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &signalOut;
    }

    if (vkQueueSubmit(queue, 1, &submitInfo, cmd.fence) != VK_SUCCESS)
        throw std::runtime_error("Failed to submit transition command buffer!");
}

void VulkanRenderer::DestroyBinaryTreeResources()
{
    if (m_nodeBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_device, m_nodeBuffer, nullptr);
        m_nodeBuffer = VK_NULL_HANDLE;
    }

    if (m_nodeBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_device, m_nodeBufferMemory, nullptr);
        m_nodeBufferMemory = VK_NULL_HANDLE;
    }

    if (m_nodeDescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(m_device, m_nodeDescriptorPool, nullptr);
        m_nodeDescriptorPool = VK_NULL_HANDLE;
    }

    if (m_nodeDescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_device, m_nodeDescriptorSetLayout, nullptr);
        m_nodeDescriptorSetLayout = VK_NULL_HANDLE;
    }
}
//
//void VulkanRenderer::SendBinaryTreeToCompute()
//{
//    DestroyBinaryTreeResources();
//
//    // Create fake data
//    std::vector<glm::vec3> fakePoints = FakeDataGenerator(100, -1, 1);
//    BinaryTree fakeTree(fakePoints);
//
//    // Create buffer to send
//    VkBufferCreateInfo bufferInfo{};
//    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//    bufferInfo.size = sizeof(GPUNode) * fakeTree.GPUReadyBuffer.size();
//    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
//    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//
//    vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_nodeBuffer);
//
//    // Allocate GPU memory
//    VkMemoryRequirements memRequirements;
//    vkGetBufferMemoryRequirements(m_device, m_nodeBuffer, &memRequirements);
//
//    VkMemoryAllocateInfo allocInfo{};
//    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//    allocInfo.allocationSize = memRequirements.size;
//    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits,
//        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
//
//    vkAllocateMemory(m_device, &allocInfo, nullptr, &m_nodeBufferMemory);
//
//    // Link to memory
//    vkBindBufferMemory(m_device, m_nodeBuffer, m_nodeBufferMemory, 0);
//
//    // Send to GPU
//    void* data;
//    vkMapMemory(m_device, m_nodeBufferMemory, 0, fakeTree.GPUReadyBuffer.size(), 0, &data);
//    memcpy(data, fakeTree.GPUReadyBuffer.data(), fakeTree.GPUReadyBuffer.size());
//    vkUnmapMemory(m_device, m_nodeBufferMemory);
//
//    // Descriptor set layout
//    VkDescriptorSetLayoutBinding nodeLayoutBinding{};
//    nodeLayoutBinding.binding = 0;
//    nodeLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
//    nodeLayoutBinding.descriptorCount = 1;
//    nodeLayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
//
//    VkDescriptorBufferInfo bufferInfoD{};
//    bufferInfoD.buffer = m_nodeBuffer;
//    bufferInfoD.offset = 0;
//    bufferInfoD.range = sizeof(GPUNode) * fakeTree.GPUReadyBuffer.size();
//
//    VkDescriptorSetLayoutCreateInfo layoutInfo{};
//    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
//    layoutInfo.bindingCount = 1;
//    layoutInfo.pBindings = &nodeLayoutBinding;
//
//    vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_nodeDescriptorSetLayout);
//
//    // 2. Créer un pool + allouer
//    VkDescriptorPoolSize poolSize{};
//    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
//    poolSize.descriptorCount = 1;
//
//    VkDescriptorPoolCreateInfo poolInfo{};
//    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
//    poolInfo.poolSizeCount = 1;
//    poolInfo.pPoolSizes = &poolSize;
//    poolInfo.maxSets = 1;
//
//    vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_nodeDescriptorPool);
//
//    VkDescriptorSetAllocateInfo allocInfo_2{};
//    allocInfo_2.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
//    allocInfo_2.descriptorPool = m_nodeDescriptorPool;
//    allocInfo_2.descriptorSetCount = 1;
//    allocInfo_2.pSetLayouts = &m_nodeDescriptorSetLayout;
//
//    VkDescriptorSet descriptorSet;
//    vkAllocateDescriptorSets(m_device, &allocInfo_2, &descriptorSet);
//
//    VkWriteDescriptorSet write{};
//    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//    write.dstSet = descriptorSet;
//    write.dstBinding = 0;
//    write.dstArrayElement = 0;
//    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
//    write.descriptorCount = 1;
//    write.pBufferInfo = &bufferInfoD;
//
//    vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
//}
#endif