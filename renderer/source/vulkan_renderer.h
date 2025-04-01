#pragma once

#include <array>
#include <optional>
#include <vector>
#include <iostream>
#include <chrono>

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
    //alignas(16) glm::mat4 model;
    //alignas(16) glm::mat4 view;
    //alignas(16) glm::mat4 proj;

	alignas(16) float time;
    alignas(16) glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, -3.0f);
    alignas(16) glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    //alignas(16) glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

};

class VulkanRenderer
{
public:
    void Run();

private:

    glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, -3.0f);
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    float yaw = -90.0f;
    float pitch = 0.0f;
    float lastX, lastY;
    bool firstMouse = true;

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

	std::chrono::high_resolution_clock::time_point lastTime;

    float GetDeltaTime();
    void ProcessInput(GLFWwindow* window);
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);

    GLFWwindow* _window = nullptr;

    VkInstance _instance = nullptr;
    VkDebugUtilsMessengerEXT _debugMessenger = nullptr;
    VkSurfaceKHR _surface = nullptr;

    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkSampleCountFlagBits _msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkDevice _device = nullptr;

    VkQueue _graphicsQueue = nullptr;
    VkQueue _computeQueue = nullptr;
    VkQueue _presentQueue = nullptr;

    VkSwapchainKHR _swapChain = nullptr;
    std::vector<VkImage> _swapChainImages;
    VkFormat _swapChainImageFormat = {};
    VkExtent2D _swapChainExtent = {};
    std::vector<VkImageView> _swapChainImageViews;
    std::vector<VkFramebuffer> _swapChainFramebuffers;

    VkRenderPass _renderPass = nullptr;
    VkDescriptorSetLayout _descriptorSetLayout = nullptr;
    VkPipelineLayout _pipelineLayout = nullptr;
    VkPipeline _graphicsPipeline = nullptr;
    VkPipeline _graphicsComputePipeline = nullptr;
    VkPipelineLayout _graphicsComputePipelineLayout = nullptr;

    VkCommandPool _commandPool = nullptr;

    VkImage _colorImage = nullptr;
    VkDeviceMemory _colorImageMemory = nullptr;
    VkImageView _colorImageView = nullptr;

    VkImage _depthImage = nullptr;
    VkDeviceMemory _depthImageMemory = nullptr;
    VkImageView _depthImageView = nullptr;

    uint32_t _mipLevels = 0;
    VkImage _textureImage = nullptr;
    VkDeviceMemory _textureImageMemory = nullptr;
    VkImageView _textureImageView = nullptr;
    VkSampler _textureSampler = nullptr;

    VkBuffer _vertexBuffer = nullptr;
    VkDeviceMemory _vertexBufferMemory = nullptr;
    VkBuffer _indexBuffer = nullptr;
    VkBuffer _QuadIndexBuffer = nullptr;
    VkDeviceMemory _indexBufferMemory = nullptr;
    VkDeviceMemory _QuadindexBufferMemory = nullptr;

    std::vector<VkBuffer> _uniformBuffers;
    std::vector<VkDeviceMemory> _uniformBuffersMemory;
    std::vector<void*> _uniformBuffersMapped;

    VkDescriptorPool _descriptorPool = nullptr;
    std::vector<VkDescriptorSet> _descriptorSets;
    std::vector<VkDescriptorSet> _computeDescriptorSets;
    VkDescriptorSetLayout _computeDescriptorSetLayout = nullptr;
    VkPipeline _computePipeline = nullptr;
    VkPipelineLayout _computePipelineLayout = nullptr;

    std::vector<VkCommandBuffer> _commandBuffers;
    std::vector<VkCommandBuffer> _computeCommandBuffers;

    std::vector<VkBuffer> _shaderStorageBuffers;
    std::vector<VkDeviceMemory> _shaderStorageBuffersMemory;

    std::vector<VkSemaphore> _imageAvailableSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    std::vector<VkSemaphore> _computeFinishedSemaphores;
    std::vector<VkFence> _inFlightFences;
    std::vector<VkFence> _computeInFlightFences;
    uint32_t _currentFrame = 0;
    uint32_t _imageIndex = 0;

    bool _framebufferResized = false;

    float _lastFrameTime = 0.0f;
    double _lastTime = 0.0f;

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
    static void LoadModel();
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

    void CreateStorageImage();
    void CreateComputeResources();
    void UpdateFrame();

    VkImage _storageImage;
    VkDeviceMemory _storageImageMemory;
    VkImageView _storageImageView;
	VkSubmitInfo _computeSubmitInfo;
    VkDescriptorPool _computeDescriptorPool;
};