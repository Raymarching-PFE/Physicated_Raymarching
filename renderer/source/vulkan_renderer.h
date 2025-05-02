#pragma once

#include <optional>
#include <algorithm>
#include <vector>
#include <chrono>
#include <filesystem>
#include <backends/imgui_impl_vulkan.h>
#include <shaderc/shaderc.hpp>

#include "model_parser.h"


constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
constexpr int MAX_FRAMES_IN_FLIGHT = 1;

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

struct UniformBufferObject
{
    //alignas(16) glm::mat4 model;
    //alignas(16) glm::mat4 view;
    //alignas(16) glm::mat4 proj;

	alignas(16) float time;
    alignas(16) glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, -3.0f);
    alignas(16) glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
};

#if COMPUTE
struct Particle
{
    glm::vec2 position;
    glm::vec2 velocity;
    glm::vec4 color;

    static VkVertexInputBindingDescription GetBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Particle);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Particle, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Particle, color);

        return attributeDescriptions;
    }
};
#endif

class VulkanRenderer
{
public:
    void Run();

private:
    glm::vec3 m_cameraPos = glm::vec3(0.0f, 0.0f, -3.0f);
    glm::vec3 m_cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 m_cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    float m_yaw = -90.0f;
    float m_pitch = 0.0f;
    float m_lastX = 0.0f;
    float m_lastY = 0.0f;
    bool  m_firstMouse = true;
    bool  m_isCursorCaptured = true;

    int                      m_currentModelIndex = 0;
    std::vector<std::string> m_modelPaths;

	std::chrono::high_resolution_clock::time_point m_lastTime;

    GLFWwindow* m_window = nullptr;

    VkInstance               m_instance = nullptr;
    VkDebugUtilsMessengerEXT m_debugMessenger = nullptr;
    VkSurfaceKHR             m_surface = nullptr;

    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice         m_device = nullptr;

    VkQueue m_graphicsQueue = nullptr;
    VkQueue m_presentQueue = nullptr;

    VkSwapchainKHR             m_swapChain = nullptr;
    std::vector<VkImage>       m_swapChainImages;
    VkFormat                   m_swapChainImageFormat = {};
    VkExtent2D                 m_swapChainExtent = {};
    std::vector<VkImageView>   m_swapChainImageViews;
    std::vector<VkFramebuffer> m_swapChainFramebuffers;

    VkRenderPass          m_renderPass = nullptr;
    VkDescriptorSetLayout m_descriptorSetLayout = nullptr;
    //VkDescriptorSetLayout m_computeDescriptorSetLayout = nullptr;
    VkPipelineLayout      m_pipelineLayout = nullptr;
    VkPipelineLayout      m_graphicsComputePipelineLayout = nullptr;
    VkPipeline            m_graphicsPipeline = nullptr;
    VkPipeline            m_graphicsComputePipeline = nullptr;

    VkDescriptorPool             m_descriptorPool = nullptr;
    std::vector<VkDescriptorSet> m_descriptorSets;

    VkCommandPool                m_commandPool = nullptr;
    std::vector<VkCommandBuffer> m_commandBuffers;

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence>     m_inFlightFences;

    uint32_t m_currentFrame = 0;
    uint32_t m_imageIndex = 0;

    bool m_framebufferResized = false;

    std::vector<VkBuffer>       m_uniformBuffers;
    std::vector<VkDeviceMemory> m_uniformBuffersMemory;
    std::vector<void*>          m_uniformBuffersMapped;

    VkBuffer        m_vertexBuffer = nullptr;
    VkDeviceMemory  m_vertexBufferMemory = nullptr;
    VkBuffer        m_indexBuffer = nullptr;
    VkDeviceMemory  m_indexBufferMemory = nullptr;
    VkBuffer        m_quadIndexBuffer = nullptr;
    VkDeviceMemory  m_quadIndexBufferMemory = nullptr;

    VkShaderModule m_vertexShader = VK_NULL_HANDLE;
    VkShaderModule m_fragmentShader = VK_NULL_HANDLE;

    uint32_t m_minImageCount = 0;
    uint32_t m_imageCount = 0;
    uint32_t m_queueFamily = (uint32_t)-1;

    ModelCache              m_modelCache;
    std::vector<Vertex>     m_vertices;
    std::vector<uint32_t>   m_indices;
    size_t                  m_vertexNb = 0;

#if COMPUTE
    VkShaderModule m_computeShader = VK_NULL_HANDLE;

    VkQueue m_computeQueue = nullptr;

    VkDescriptorSetLayout m_computeDescriptorSetLayout = nullptr;
    VkPipelineLayout      m_computePipelineLayout = nullptr;
    VkPipeline            m_computePipeline = nullptr;

    std::vector<VkBuffer>       m_shaderStorageBuffers;
    std::vector<VkDeviceMemory> m_shaderStorageBuffersMemory;

    std::vector<VkDescriptorSet> m_computeDescriptorSets;
    std::vector<VkCommandBuffer> m_computeCommandBuffers;

    std::vector<VkSemaphore> m_computeFinishedSemaphores;
    std::vector<VkFence>     m_computeInFlightFences;
#endif

    float GetDeltaTime();
    void ProcessInput(GLFWwindow* window);
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);

    static void CheckVkResult(VkResult err);
    void InitImGui() const;
    void MainImGui();

    void InitWindow();
    static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
    void InitVulkan();
    void MainLoop();
    void CleanupSwapChain() const;
    void Cleanup() const;
    void RecreateSwapChain();
    void CreateInstance();
    static void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void SetupDebugMessenger();
    void CreateSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapChain();
    void CreateImageViews();
    void CreateRenderPass();
    void CreateGraphicsPipeline();
    void CreateFramebuffers();
    void CreateCommandPool();
    VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const;
    void CreateUniformBuffers();
    void CreateDescriptorPool();
    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const;
    VkCommandBuffer BeginSingleTimeCommands() const;
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer) const;
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    void CreateCommandBuffers();
    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) const;
    void CreateSyncObjects();
    void UpdateUniformBuffer(uint32_t currentImage) const;
    void BeginFrame();
    void DrawFrame() const;
    void EndFrame();
    VkShaderModule CreateShaderModule(const std::vector<char>& code) const;
    static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;
    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) const;
    bool IsDeviceSuitable(VkPhysicalDevice device);
    static bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
    static std::vector<const char*> GetRequiredExtensions();
    static bool CheckValidationLayerSupport();
    static std::vector<char> ReadFile(const std::string& filename);

    void LoadModel(const std::string& path);
    void CreateVertexBuffer();
    void CreateIndexBuffer();

    void ReloadModel(const std::string& path);
    void DestroyModelResources();

#if COMPUTE
    void CreateShaderStorageBuffers();
    void CreateComputePipeline();
    void CreateComputeDescriptorSetLayout();
    void CreateComputeDescriptorSets();
    void CreateComputeCommandBuffers();
    void RecordComputeCommandBuffer(VkCommandBuffer commandBuffer) const;
    void CreateDescriptorSets();
#else
    void CreateDescriptorSets();
#endif
    void CreateDescriptorSetLayout();
    
    VKAPI_ATTR static VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

    void CreateStorageImage();
    void CreateComputeResources();
    void CreateGraphicsComputePipeline();
    void CreateColorResources();
    void CreateDepthResources();
    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
    VkFormat FindDepthFormat() const;
	static bool HasStencilComponent(VkFormat format);
    VkSampleCountFlagBits GetMaxUsableSampleCount() const;
    void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const;
    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) const;
    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;
    void CreateTextureSampler();

    VkImage _storageImage;
    VkDeviceMemory _storageImageMemory;
    VkImageView _storageImageView;
	VkSubmitInfo _computeSubmitInfo;
    VkDescriptorPool _computeDescriptorPool;

    VkImage m_colorImage = nullptr;
    VkDeviceMemory m_colorImageMemory = nullptr;
    VkImageView m_colorImageView = nullptr;

    VkImage m_depthImage = nullptr;
    VkDeviceMemory m_depthImageMemory = nullptr;
    VkImageView m_depthImageView = nullptr;

    VkBuffer m_QuadIndexBuffer = nullptr;
    VkDeviceMemory m_QuadindexBufferMemory = nullptr;

    VkImageView m_textureImageView = nullptr;
    VkSampler m_textureSampler = nullptr;
};