#pragma once

#include <array>
#include <optional>
#include <vector>
#include <chrono>
#include <shaderc/shaderc.hpp>

#include "model_parser.h"


constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

constexpr uint32_t PARTICLE_COUNT = 8192;

// const std::string TEXTURE_PATH = "textures/viking_room.png";
const std::string TEXTURE_PATH = "textures/red.png";

constexpr int MAX_FRAMES_IN_FLIGHT = 1;

constexpr uint32_t NUMBER_OF_UBO = 1;

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

struct UniformBufferObject
{
	alignas(16) float time;
    alignas(16) glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, -3.0f);
    alignas(16) glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    alignas(16) glm::vec4 spheresArray[8];// w values are for sizes
    alignas(16) int sphereNumber;
};

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
    bool m_firstMouse = true;

    float m_deltaTime = 0.0f;
    float m_lastFrame = 0.0f;

	std::chrono::high_resolution_clock::time_point lastTime;

    float GetDeltaTime();
    void ProcessInput(GLFWwindow* window);
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);

    GLFWwindow* m_window = nullptr;

    VkInstance m_instance = nullptr;
    VkDebugUtilsMessengerEXT m_debugMessenger = nullptr;
    VkSurfaceKHR m_surface = nullptr;

    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkDevice m_device = nullptr;

    VkQueue m_graphicsQueue = nullptr;
    VkQueue m_computeQueue = nullptr;
    VkQueue m_presentQueue = nullptr;

    VkSwapchainKHR m_swapChain = nullptr;
    std::vector<VkImage> m_swapChainImages;
    VkFormat m_swapChainImageFormat = {};
    VkExtent2D m_swapChainExtent = {};
    std::vector<VkImageView> m_swapChainImageViews;
    std::vector<VkFramebuffer> m_swapChainFramebuffers;

    VkRenderPass m_renderPass = nullptr;
    VkDescriptorSetLayout m_descriptorSetLayout = nullptr;
    VkPipelineLayout m_pipelineLayout = nullptr;
    VkPipeline m_graphicsPipeline = nullptr;
    VkPipeline m_graphicsComputePipeline = nullptr;
    VkPipelineLayout m_graphicsComputePipelineLayout = nullptr;

    VkShaderModule m_vertexShader = VK_NULL_HANDLE;
    VkShaderModule m_fragmentShader = VK_NULL_HANDLE;
    VkShaderModule m_computeShader = VK_NULL_HANDLE;

    VkCommandPool m_commandPool = nullptr;

    VkImage m_colorImage = nullptr;
    VkDeviceMemory m_colorImageMemory = nullptr;
    VkImageView m_colorImageView = nullptr;

    VkImage m_depthImage = nullptr;
    VkDeviceMemory m_depthImageMemory = nullptr;
    VkImageView m_depthImageView = nullptr;

    uint32_t m_mipLevels = 0;
    VkImage m_textureImage = nullptr;
    VkDeviceMemory m_textureImageMemory = nullptr;
    VkImageView m_textureImageView = nullptr;
    VkSampler m_textureSampler = nullptr;

    VkBuffer m_vertexBuffer = nullptr;
    VkDeviceMemory m_vertexBufferMemory = nullptr;
    VkBuffer m_indexBuffer = nullptr;
    VkBuffer m_QuadIndexBuffer = nullptr;
    VkDeviceMemory m_indexBufferMemory = nullptr;
    VkDeviceMemory m_QuadindexBufferMemory = nullptr;

    std::vector<VkBuffer> m_uniformBuffers;
    std::vector<VkDeviceMemory> m_uniformBuffersMemory;
    std::vector<void*> m_uniformBuffersMapped;

    VkDescriptorPool m_descriptorPool = nullptr;
    std::vector<VkDescriptorSet> m_descriptorSets;
    std::vector<VkDescriptorSet> m_computeDescriptorSets;
    VkDescriptorSetLayout m_computeDescriptorSetLayout = nullptr;
    VkPipeline m_computePipeline = nullptr;
    VkPipelineLayout m_computePipelineLayout = nullptr;

    std::vector<VkCommandBuffer> m_commandBuffers;
    std::vector<VkCommandBuffer> m_computeCommandBuffers;

    std::vector<VkBuffer> m_shaderStorageBuffers;
    std::vector<VkDeviceMemory> m_shaderStorageBuffersMemory;

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkSemaphore> m_computeFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    std::vector<VkFence> m_computeInFlightFences;
    uint32_t m_currentFrame = 0;
    uint32_t m_imageIndex = 0;

    bool m_framebufferResized = false;

    float m_lastFrameTime = 0.0f;
    double m_lastTime = 0.0f;

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
    void CreateComputeDescriptorSetLayout();
    void CreateDescriptorSetLayout();
    void CreateGraphicsPipeline();
    void CreateGraphicsComputePipeline();
    void CreateComputePipeline();
    void CreateFramebuffers();
    void CreateCommandPool();
    void CreateShaderStorageBuffers();
    void CreateColorResources();
    void CreateDepthResources();
    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
    VkFormat FindDepthFormat() const;
    static bool HasStencilComponent(VkFormat format);
    void CreateTextureImage();
    void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) const;
    VkSampleCountFlagBits GetMaxUsableSampleCount() const;
    void CreateTextureImageView();
    void CreateTextureSampler();
    VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const;
    void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const;
    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) const;
    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;
    void LoadModel();
    void LoadGeneratedPoint();

    void CreateVertexBuffer();
    void CreateIndexBuffer();
    void CreateUniformBuffers();
    void CreateDescriptorPool();
    void CreateDescriptorSets();
    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const;
    VkCommandBuffer BeginSingleTimeCommands() const;
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer) const;
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    void CreateComputeDescriptorSets();
    void CreateCommandBuffers();
    void RecordComputeCommandBuffer(VkCommandBuffer commandBuffer) const;
    void CreateComputeCommandBuffers();    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) const;
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
    bool IsDeviceSuitable(VkPhysicalDevice device) const;
    static bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
    static std::vector<const char*> GetRequiredExtensions();
    static bool CheckValidationLayerSupport();
    static std::vector<char> ReadFile(const std::string& filename);
    
    VKAPI_ATTR static VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
};