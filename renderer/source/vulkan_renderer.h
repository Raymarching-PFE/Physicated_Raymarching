#pragma once

#include <optional>
#include <vector>
#include <chrono>
#include <filesystem>
#include <backends/imgui_impl_vulkan.h>
#include <shaderc/shaderc.hpp>

#include "model_parser.h"
#include "binaryTree.h"
#include "tracy/TracyVulkan.hpp"


constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
constexpr int MAX_FRAMES_IN_FLIGHT = 1;

constexpr int MAX_NODES_SSBO = 2048;

const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef NDEBUG
    constexpr bool enableValidationLayers = false;
#else
    constexpr bool enableValidationLayers = true;
#endif


inline VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    const auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

inline void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    const auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
        func(instance, debugMessenger, pAllocator);
}

inline bool CompileShaderFromFile(const std::string& _path, shaderc_shader_kind _stage, std::vector<uint32_t>& _out)
{
    // Read File
    std::string code;
    {
        std::fstream fStream(_path, std::ios_base::in);

        if (!fStream.is_open())
        {
            std::cerr << "\033[31m" << "Failed to open shader file " << _path << "\033[0m" << '\n'; // Red
            return false;
        }

        std::stringstream sstream;
        sstream << fStream.rdbuf();

        fStream.close();

        code = sstream.str();
    }

    // Compile
    static shaderc::Compiler compiler;

    shaderc::CompileOptions options;

#if NDEBUG
    options.SetOptimizationLevel(shaderc_optimization_level_zero);
#else
    options.SetOptimizationLevel(shaderc_optimization_level_performance);
#endif

    const shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(code, _stage, _path.c_str(), options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        std::cerr << "\033[31m" << "Compile Shader " << _path << " failed!" << "\033[0m" << '\n'; // Red
        std::cerr << "\033[31m" << "Errors: " << result.GetNumErrors() + '\t' << "Warnings: " << result.GetNumWarnings() << "\033[0m" << '\n'; // Red
        std::cerr << "\033[31m" << result.GetErrorMessage() << '\n'; // Red
        return false;
    }
    else if (result.GetNumWarnings())
    {
        std::cerr << "\033[33m" << "Compile Shader " << _path << " success with " << result.GetNumWarnings() << " warnings" << "\033[0m" << '\n'; // Yellow
        std::cerr << "\033[33m" << result.GetErrorMessage() << '\n'; // Yellow
    }
    else
        std::cerr << "\033[32m" << "Compile Shader " << _path << " success" << "\033[0m" << '\n'; // Green

    _out = { result.cbegin(), result.cend() };

    return true;
}

inline std::vector<std::string> LoadPLYFilePaths(const std::string& directoryPath)
{
    std::vector<std::string> filePaths;

    for (const auto& entry : std::filesystem::directory_iterator(directoryPath))
    {
        if (entry.path().extension() == ".ply")
            filePaths.push_back(entry.path().string());
    }

    return filePaths;
}

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsAndComputeFamily;
    std::optional<uint32_t> presentFamily;

    bool IsComplete() const
    {
        return graphicsAndComputeFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct alignas(16) SSBOData
{
    alignas(16) GPUNode SSBONodes[MAX_NODES_SSBO];
};

struct UniformBufferObject
{
    // Groupe 1 : flags (regroupés dans un vec4)
    alignas(16) glm::vec4 settings1;
    // x = lighting (int)
    // y = boxDebug (int)
    // z = randomColor (int)
    // w = unused

    // Groupe 2 : floats divers
    alignas(16) glm::vec4 settings2;
    // x = sphereRadius
    // y = time
    // z = blendingFactor
    // w = far

    // Groupe 3 : reflectivity et padding
    alignas(16) glm::vec4 settings3;
    // x = reflectivity
    // yzw = unused

    alignas(16) glm::vec4 lightingDir;
    alignas(16) glm::vec4 objectColor;

    alignas(16) glm::vec4 cameraPos = glm::vec4(0.0f, 0.0f, -3.0f, 0.0f);
    alignas(16) glm::vec4 cameraFront = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
#if !COMPUTE
    alignas(16) glm::vec4 spheresArray[8];// w values are for sizes
    alignas(16) glm::ivec4 sphereNumber;
#endif
};

class VulkanRenderer
{
public:
    void Run();

private:

#pragma region VARIABLES
    TracyVkCtx m_graphicTracyVkCtx = nullptr;
#if COMPUTE
    TracyVkCtx m_computeTracyVkCtx = nullptr;
#endif

    // Sphere Size
    float m_sphereRadius = 0.3f;

    float m_blendingFactor = 0.002f;
    float m_far = 100;

    bool m_lighting = true;
    bool m_boxDebug = false;
    bool m_randomColor = false;
    float m_reflectivity = 0.0f;
    glm::vec3 m_lightingDir = glm::vec3(1.0, -1.0, -1.0);
    glm::vec3 m_objectColor = glm::vec3(1.0, 0.0, 0.0);

    // Camera
    glm::vec3 m_cameraPos = glm::vec3(0.0f, 0.0f, -3.0f);
    glm::vec3 m_cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 m_cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    // Mouse
    float m_yaw = -90.0f;
    float m_pitch = 0.0f;
    float m_lastX = 0.0f;
    float m_lastY = 0.0f;
    bool  m_firstMouse = true;
    bool  m_isCursorCaptured = true;

    // ImGui
    bool m_vsyncEnabled = true;
    float m_cameraSpeed = 2.0f;

    // Binary tree
    GPUNode myNodes[100];

    // Application
    int                      m_currentModelIndex = 0;
    std::vector<std::string> m_modelPaths;

    float m_deltaTime = 0.016f;
    std::chrono::high_resolution_clock::time_point m_lastTime;

    GLFWwindow* m_window = nullptr;

    VkBuffer m_ssboBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_ssboMemory = VK_NULL_HANDLE;
    BinaryTree m_binaryTree;

    // Vulkan base
    VkInstance               m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR             m_surface = VK_NULL_HANDLE;

    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice         m_device = VK_NULL_HANDLE;

    // Queues
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue  = VK_NULL_HANDLE;

    // Swapchain
    VkSwapchainKHR             m_swapChain = VK_NULL_HANDLE;
    std::vector<VkImage>       m_swapChainImages;
    VkFormat                   m_swapChainImageFormat = {};
    VkExtent2D                 m_swapChainExtent = {};
    std::vector<VkImageView>   m_swapChainImageViews;
    std::vector<VkFramebuffer> m_swapChainFramebuffers;

    // Render
    VkRenderPass          m_renderPass = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout      m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline            m_graphicsPipeline = VK_NULL_HANDLE;

    VkPipelineLayout      m_graphicsComputePipelineLayout = VK_NULL_HANDLE;
    VkPipeline            m_graphicsComputePipeline = VK_NULL_HANDLE;

    VkDescriptorPool             m_descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_descriptorSets;

    VkCommandPool                m_commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_commandBuffers;

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence>     m_inFlightFences;

    uint32_t m_currentFrame = 0;
    uint32_t m_imageIndex   = 0;
    bool     m_framebufferResized = false;

    // Uniforms
    std::vector<VkBuffer>       m_uniformBuffers;
    std::vector<VkDeviceMemory> m_uniformBuffersMemory;
    std::vector<void*>          m_uniformBuffersMapped;

    // Geometry
    VkBuffer        m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory  m_vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer        m_indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory  m_indexBufferMemory = VK_NULL_HANDLE;
    VkBuffer        m_quadIndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory  m_quadIndexBufferMemory = VK_NULL_HANDLE;

    std::vector<Vertex>     m_vertices;
    std::vector<uint32_t>   m_indices;
    size_t                  m_vertexNb = 0;

    // Shader modules
    VkShaderModule m_vertexShader   = VK_NULL_HANDLE;
    VkShaderModule m_fragmentShader = VK_NULL_HANDLE;

    // Texture
    VkImageView m_textureImageView = VK_NULL_HANDLE;
    VkSampler   m_textureSampler   = VK_NULL_HANDLE;

    // Model loading
    ModelCache m_modelCache;

    // Queue family
    uint32_t m_minImageCount = 0;
    uint32_t m_imageCount    = 0;
    uint32_t m_queueFamily   = (uint32_t)-1;

#if COMPUTE
    // Compute pipeline
    VkShaderModule   m_computeShader = VK_NULL_HANDLE;
    VkPipelineLayout m_computePipelineLayout = VK_NULL_HANDLE;
    VkPipeline       m_computePipeline = VK_NULL_HANDLE;

    VkQueue m_computeQueue = VK_NULL_HANDLE;

    // Descriptor set for compute
    VkDescriptorSetLayout           m_computeDescriptorSetLayout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet>    m_computeDescriptorSets;

    // Command buffers & sync for compute
    std::vector<VkCommandBuffer>    m_computeCommandBuffers;
    std::vector<VkSemaphore>        m_computeFinishedSemaphores;
    std::vector<VkFence>            m_computeInFlightFences;
    std::vector<VkSemaphore>        m_transitionFinishedSemaphores;

    struct TransitionCmd
    {
        VkCommandBuffer buffer;
        VkFence fence;
    };

    std::array<TransitionCmd, 2> m_transitionCommandBuffers[MAX_FRAMES_IN_FLIGHT];

    // Storage image (compute output)
    VkImage        m_storageImage = VK_NULL_HANDLE;
    VkDeviceMemory m_storageImageMemory = VK_NULL_HANDLE;
    VkImageView    m_storageImageView = VK_NULL_HANDLE;
    VkImageLayout  m_storageImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    // Node buffer (used for compute tree)
    VkBuffer              m_nodeBuffer = VK_NULL_HANDLE;
    VkDeviceMemory        m_nodeBufferMemory = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_nodeDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool      m_nodeDescriptorPool      = VK_NULL_HANDLE;

    // Submission
    VkSubmitInfo m_computeSubmitInfo = {};
#endif
#pragma endregion

#pragma region FUNCTIONS
    // System utilities / callbacks
    static void CheckVkResult(VkResult err);
    static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
    static VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                             const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

    // General initialization
    void InitWindow();
    void InitVulkan();
    void MainLoop();
    void Cleanup();
    void RecreateSwapChain();

    // Tracy
    void InitTracy();

    // ImGui
    void InitImGui() const;
    void MainImGui();
    void ShowCustomMetricsWindow();

    // Instance / Device / Surface / Debug
    void CreateInstance();
    static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void SetupDebugMessenger();
    void CreateSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    bool IsDeviceSuitable(const VkPhysicalDevice device);
    static bool CheckDeviceExtensionSupport(const VkPhysicalDevice device);
    QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice device);
    static std::vector<const char*> GetRequiredExtensions();
    bool CheckValidationLayerSupport();

    // Swapchain
    void CreateSwapChain();
    void CreateImageViews();
    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) const;
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;
    static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
    void CleanupSwapChain() const;

    // Render Pass / Pipelines
    void CreateRenderPass();
    void CreateGraphicsPipeline();
    void CreateDescriptorSetLayout();
    void CreateDescriptorSets();
    static bool HasStencilComponent(VkFormat format);
    void CreateFramebuffers();
    void CreateCommandPool();

    // Shaders
    VkShaderModule CreateShaderModule(const std::vector<char>& code) const;
    static std::vector<char> ReadFile(const std::string& filename);

    // Buffers & Memory
    void CreateVertexBuffer();
    void CreateIndexBuffer();
    void CreateUniformBuffers();
    void CreateDescriptorPool();
    void CreateCommandBuffers();
    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) const;
    void UpdateUniformBuffer(uint32_t currentImage) const;
    void CreateSyncObjects();
    void CreateTextureSampler();
    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const;
    VkCommandBuffer BeginSingleTimeCommands() const;
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer) const;
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    // Images
    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) const;

    // Render
    void BeginFrame();
    void DrawFrame();
    void EndFrame();

    // Models & Binary tree
    void LoadModel(const std::string& path);
    void ReloadModel(const std::string& path);
    void DestroyModelResources();

    // Inputs & Timings
    float GetDeltaTime();
    void ProcessInput(GLFWwindow* window);

    // Compute
    #if COMPUTE
    void CreateStorageImage();
    void CreateComputePipeline();
    void CreateComputeDescriptorSetLayout();
    void CreateComputeDescriptorSets();
    void CreateComputeCommandBuffers();
    void RecordComputeCommandBuffer(VkCommandBuffer commandBuffer) const;
	void CreateSSBOBuffer();
    void ComputeTransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, VkQueue queue, VkSemaphore waitOn, VkSemaphore signalOut, uint32_t index);
    void DestroyBinaryTreeResources();
    #endif
#pragma endregion
};