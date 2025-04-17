#include "Allocator.hh"
#include "Buffer.hh"
#include "Camera.hh"
#include "Descriptor.hh"
#include "Device.hh"
#include "Image.hh"
#include "Material.hh"
#include "Model.hh"
#include "Pipeline.hh"
#include "Points.hh"
#include "Swapchain.hh"
#include "Window.hh"

std::filesystem::path resourcePath = "../resources/";

struct GlobalUniforms
{
    float time;
    float frame;
};

struct Light
{
    glm::vec4 position = {0.0f, 0.0f, 0.0f, 1.0f};
    glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
};

inline std::vector<std::string> split(std::string str, std::string delim = " ")
{
    std::vector<std::string> result;
    for (std::size_t pos; (pos = str.find(delim)) != std::string::npos;)
    {
        result.emplace_back(str.substr(0, pos));
        str = str.substr(pos + delim.size());
    }
    result.emplace_back(str);
    return result;
}

struct App
{
    xr::DispatchLoaderDynamic xrDispatchLoader;
    xr::UniqueDynamicInstance xrInstance;
    xr::SystemId xrSystemId;

    std::unique_ptr<letc::Instance> instance;
    vkfw::UniqueWindow window;
    vk::UniqueSurfaceKHR surface;

    std::unique_ptr<letc::Device> device;
    std::unique_ptr<letc::Allocator> allocator;
    vk::Queue queue;

    std::unique_ptr<letc::DescriptorManager> descriptorManager;

    xr::UniqueDynamicSession xrSession;
    xr::UniqueDynamicSwapchain xrSwapchain[2];

    xr::Time xrPredictedDisplayTime = xr::Time{};
    xr::UniqueDynamicSpace leftHandSpace;
    xr::UniqueDynamicSpace rightHandSpace;
    xr::Path leftHandPath;
    xr::Path rightHandPath;
    xr::UniqueDynamicActionSet actionSet;
    xr::UniqueDynamicAction triggerAction;
    xr::UniqueDynamicAction joystickAction;
    xr::UniqueDynamicAction faceButtonAction;
    xr::UniqueDynamicAction handPoseAction;

    std::vector<xr::ViewConfigurationView> xrViewConfigurationViews;
    xr::UniqueDynamicSpace xrSpace;
    long long xrSwapchainFormat;
    std::vector<xr::SwapchainImageVulkanKHR, std::allocator<xr::SwapchainImageVulkanKHR>> xrSwapchainImageVk[2];
    std::vector<vk::UniqueImageView> swapchainImageViews[2];
    xr::EventDataBuffer xrEventDataBuffer{};
    xr::SessionState xrSessionState = xr::SessionState::Unknown;

    vk::UniqueCommandPool commandPool;
    std::vector<vk::UniqueCommandBuffer> commandBuffers;
    uint32_t m_currentImageIndex = 0;
    vk::UniqueFence imageFence;
    std::vector<vk::UniqueFence> commandFences;

    GlobalUniforms globalUniforms;
    std::unique_ptr<letc::Buffer> globalUniformsBuffer;

    std::vector<Light> lights;
    std::unique_ptr<letc::Buffer> lightsBuffer;

    std::vector<std::unique_ptr<letc::Camera>> xrCameras;

    std::vector<letc::Model> models;
    std::vector<letc::Model::UniformBuffer> modelUniforms{};
    std::unique_ptr<letc::Buffer> modelUniformsBuffer;

    std::unique_ptr<letc::ModelRenderer> modelRenderer;

    std::unique_ptr<letc::Points> points;
    std::unique_ptr<letc::PointsRenderer> pointsRenderer;

    std::unique_ptr<letc::Image> depthImage;         // Renamed from depthBuffer for consistency
    std::unique_ptr<letc::ImageView> depthImageView; // Renamed for consistency

    std::unique_ptr<letc::Image> xrDepthImage;         // Renamed from xrDepthBuffer for consistency
    std::unique_ptr<letc::ImageView> xrDepthImageView; // Renamed for consistency

    double lastMouseX, lastMouseY;

    size_t currentFrame = 0;
    App()
    {
        xrDispatchLoader = xr::DispatchLoaderDynamic{};

        auto xrDebugCallback =
            [](XrDebugUtilsMessageSeverityFlagsEXT messageSeverity, XrDebugUtilsMessageTypeFlagsEXT messageTypes,
               const XrDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) -> XrBool32 {
            std::cerr << "XrDebug: " << pCallbackData->messageId << ": " << pCallbackData->message << std::endl;
            return XR_FALSE;
        };

        xr::DebugUtilsMessengerCreateInfoEXT xrDebugInfo{};
        xrDebugInfo.messageSeverities = xr::DebugUtilsMessageSeverityFlagBitsEXT::AllBits;
        xrDebugInfo.messageTypes = xr::DebugUtilsMessageTypeFlagBitsEXT::AllBits;
        xrDebugInfo.userCallback = xrDebugCallback;
        xrDebugInfo.userData = nullptr;

        xr::ApplicationInfo xrAppInfo{};
        std::memcpy(xrAppInfo.applicationName, "Dev", 4);
        xrAppInfo.applicationVersion = 1;
        std::memcpy(xrAppInfo.engineName, "letc", 5);
        xrAppInfo.engineVersion = 1;
        xrAppInfo.apiVersion = xr::Version{XR_CURRENT_API_VERSION};

        std::vector<const char *> xrInstanceExtensions = {"XR_KHR_vulkan_enable2"};
        xr::InstanceCreateInfo xrInstanceInfo{};
        xrInstanceInfo.createFlags = xr::InstanceCreateFlagBits::None;
        xrInstanceInfo.applicationInfo = xrAppInfo;
        xrInstanceInfo.enabledExtensionCount = static_cast<uint32_t>(xrInstanceExtensions.size());
        xrInstanceInfo.enabledExtensionNames = xrInstanceExtensions.data();
        xrInstanceInfo.enabledApiLayerCount = 0;
        xrInstanceInfo.enabledApiLayerNames = nullptr;
        xrInstanceInfo.next = &xrDebugInfo;

        xrInstance = xr::createInstanceUnique(xrInstanceInfo, xrDispatchLoader);
        assertThrow(xrInstance, "failed to create xr instance");
        xrDispatchLoader = xr::DispatchLoaderDynamic{*xrInstance};

        xr::SystemGetInfo xrSystemInfo{};
        xrSystemInfo.formFactor = xr::FormFactor::HeadMountedDisplay;
        xrSystemInfo.next = nullptr;
        xrSystemId = xrInstance->getSystem(xrSystemInfo);

        // basic initialization
        VULKAN_HPP_DEFAULT_DISPATCHER.init();
        vkfw::init();

        // window and vulkan initialization
        vkfw::WindowHints windowHints{};
        window = vkfw::createWindowUnique(letc::WindowBuilder{});
        instance =
            std::make_unique<letc::Instance>(letc::InstanceBuilder{}
                                                 .setDebug(true)
                                                 .addInstanceExtension("VK_KHR_external_memory_capabilities")
                                                 .addInstanceExtension("VK_KHR_get_physical_device_properties2"));
        VULKAN_HPP_DEFAULT_DISPATCHER.init(instance->instance);

        // window surface creation
        surface = vkfw::createWindowSurfaceUnique(*instance, *window);
        assertThrow(surface, "failed to create surface");

        auto xrGraphicsRequirements = xrInstance->getVulkanGraphicsRequirements2KHR(xrSystemId, xrDispatchLoader);
        auto xrGraphicsDevice = xrInstance->getVulkanGraphicsDevice2KHR(
            xr::VulkanGraphicsDeviceGetInfoKHR{xrSystemId, instance->instance, nullptr}, xrDispatchLoader);
        // auto xrGraphicsDeviceExtensions = xrInstance->getVulkanDeviceExtensionsKHR(xrSystemId, xrDispatchLoader);

        letc::DeviceBuilder deviceBuilder;
        // deviceBuilder.addExtensions(split(xrGraphicsDeviceExtensions));
        deviceBuilder.requestDevice(xrGraphicsDevice);
        deviceBuilder.requestQueue("graphics", vk::QueueFlagBits::eGraphics);
        device = std::make_unique<letc::Device>(*instance, deviceBuilder);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(device->device);

        // allocator initialization
        allocator = std::make_unique<letc::Allocator>(*instance, *device);

        // swapchain + queue initialization
        queue = device->queues["graphics"].queue;

        descriptorManager = std::make_unique<letc::DescriptorManager>(*device);
        descriptorManager->addLayoutBinding("pbrGlobalLayout", 0, vk::DescriptorType::eUniformBuffer,
                                            vk::ShaderStageFlagBits::eAllGraphics);
        descriptorManager->addLayoutBinding("pbrGlobalLayout", 1, vk::DescriptorType::eStorageBuffer,
                                            vk::ShaderStageFlagBits::eAllGraphics);
        descriptorManager->addLayoutBinding("pbrGlobalLayout", 2, vk::DescriptorType::eUniformBuffer,
                                            vk::ShaderStageFlagBits::eAllGraphics);
        descriptorManager->createLayout("pbrGlobalLayout");

        descriptorManager->addLayoutBinding("pbrModelLayout", 0, vk::DescriptorType::eStorageBuffer,
                                            vk::ShaderStageFlagBits::eAllGraphics);
        descriptorManager->createLayout("pbrModelLayout");

        descriptorManager->createSet("pbrGlobalLayout", "pbrGlobalSet");
        descriptorManager->createSet("pbrModelLayout", "pbrModelSet");

        xr::GraphicsBindingVulkanKHR xrGraphicsBinding{};
        xrGraphicsBinding.next = nullptr;
        xrGraphicsBinding.instance = instance->instance;
        xrGraphicsBinding.physicalDevice = device->physicalDevice;
        xrGraphicsBinding.device = device->device;
        xrGraphicsBinding.queueFamilyIndex = device->queues["graphics"].queueFamilyIndex;
        xrGraphicsBinding.queueIndex = 0;

        xr::SessionCreateInfo xrSessionInfo{};
        xrSessionInfo.next = &xrGraphicsBinding;
        xrSessionInfo.createFlags = xr::SessionCreateFlagBits::None;
        xrSessionInfo.systemId = xrSystemId;
        xrSession = xrInstance->createSessionUnique(xrSessionInfo, xrDispatchLoader);

        {
            leftHandPath = xrInstance->stringToPath("/user/hand/left", xrDispatchLoader);
            rightHandPath = xrInstance->stringToPath("/user/hand/right", xrDispatchLoader);

            // Create an action set.
            xr::ActionSetCreateInfo actionSetInfo{};
            std::strcpy(actionSetInfo.actionSetName, "gameplay");
            std::strcpy(actionSetInfo.localizedActionSetName, "Gameplay");
            actionSetInfo.priority = 0;
            actionSet = xrInstance->createActionSetUnique(actionSetInfo, xrDispatchLoader);

            // Create subaction paths for left and right hands.
            xr::Path leftHandPath = xrInstance->stringToPath("/user/hand/left", xrDispatchLoader);
            xr::Path rightHandPath = xrInstance->stringToPath("/user/hand/right", xrDispatchLoader);
            std::vector<xr::Path> subactionPaths = {leftHandPath, rightHandPath};

            // Create the trigger action (boolean input).
            xr::ActionCreateInfo triggerActionInfo{};
            std::strcpy(triggerActionInfo.actionName, "trigger_press");
            std::strcpy(triggerActionInfo.localizedActionName, "Trigger Press");
            triggerActionInfo.actionType = xr::ActionType::BooleanInput;
            triggerActionInfo.countSubactionPaths = static_cast<uint32_t>(subactionPaths.size());
            triggerActionInfo.subactionPaths = subactionPaths.data();
            triggerAction = actionSet->createActionUnique(triggerActionInfo, xrDispatchLoader);

            // Create the joystick action (vector2f input).
            xr::ActionCreateInfo joystickActionInfo{};
            std::strcpy(joystickActionInfo.actionName, "joystick_move");
            std::strcpy(joystickActionInfo.localizedActionName, "Joystick Move");
            joystickActionInfo.actionType = xr::ActionType::Vector2FInput;
            joystickActionInfo.countSubactionPaths = static_cast<uint32_t>(subactionPaths.size());
            joystickActionInfo.subactionPaths = subactionPaths.data();
            joystickAction = actionSet->createActionUnique(joystickActionInfo, xrDispatchLoader);

            // Create the face button action (boolean input).
            xr::ActionCreateInfo faceButtonActionInfo{};
            std::strcpy(faceButtonActionInfo.actionName, "face_button_press");
            std::strcpy(faceButtonActionInfo.localizedActionName, "Face Button Press");
            faceButtonActionInfo.actionType = xr::ActionType::BooleanInput;
            faceButtonActionInfo.countSubactionPaths = static_cast<uint32_t>(subactionPaths.size());
            faceButtonActionInfo.subactionPaths = subactionPaths.data();
            faceButtonAction = actionSet->createActionUnique(faceButtonActionInfo, xrDispatchLoader);

            // Create a pose action to capture hand position data.
            xr::ActionCreateInfo handPoseActionInfo{};
            std::strcpy(handPoseActionInfo.actionName, "hand_pose");
            std::strcpy(handPoseActionInfo.localizedActionName, "Hand Pose");
            handPoseActionInfo.actionType = xr::ActionType::PoseInput;
            handPoseActionInfo.countSubactionPaths = static_cast<uint32_t>(subactionPaths.size());
            handPoseActionInfo.subactionPaths = subactionPaths.data();
            handPoseAction = actionSet->createActionUnique(handPoseActionInfo, xrDispatchLoader);

            // Suggest bindings for a common controller profile (example: Oculus Touch).
            xr::Path oculusTouchInteractionProfile =
                xrInstance->stringToPath("/interaction_profiles/oculus/touch_controller", xrDispatchLoader);
            std::vector<xr::ActionSuggestedBinding> bindings;
            bindings.push_back(
                {*triggerAction, xrInstance->stringToPath("/user/hand/left/input/trigger", xrDispatchLoader)});
            bindings.push_back(
                {*triggerAction, xrInstance->stringToPath("/user/hand/right/input/trigger", xrDispatchLoader)});
            bindings.push_back(
                {*joystickAction, xrInstance->stringToPath("/user/hand/left/input/thumbstick", xrDispatchLoader)});
            bindings.push_back(
                {*joystickAction, xrInstance->stringToPath("/user/hand/right/input/thumbstick", xrDispatchLoader)});
            bindings.push_back(
                {*faceButtonAction, xrInstance->stringToPath("/user/hand/left/input/x", xrDispatchLoader)});
            bindings.push_back(
                {*faceButtonAction, xrInstance->stringToPath("/user/hand/right/input/a", xrDispatchLoader)});
            bindings.push_back(
                {*handPoseAction, xrInstance->stringToPath("/user/hand/left/input/aim/pose", xrDispatchLoader)});
            bindings.push_back(
                {*handPoseAction, xrInstance->stringToPath("/user/hand/right/input/aim/pose", xrDispatchLoader)});

            xr::InteractionProfileSuggestedBinding suggestedBinding{};
            suggestedBinding.interactionProfile = oculusTouchInteractionProfile;
            suggestedBinding.suggestedBindings = bindings.data();
            suggestedBinding.countSuggestedBindings = static_cast<uint32_t>(bindings.size());
            xrInstance->suggestInteractionProfileBindings(suggestedBinding, xrDispatchLoader);

            {
                xr::ActionSpaceCreateInfo actionSpaceInfo{};
                actionSpaceInfo.action = *handPoseAction;
                // Use an identity pose for the action space (adjust if you need an offset).
                actionSpaceInfo.poseInActionSpace.orientation = xr::Quaternionf{0, 0, 0, 1};
                actionSpaceInfo.poseInActionSpace.position = xr::Vector3f{0, 0, 0};

                // Create left hand action space.
                actionSpaceInfo.subactionPath = leftHandPath;
                leftHandSpace = xrSession->createActionSpaceUnique(actionSpaceInfo, xrDispatchLoader);

                // Create right hand action space.
                actionSpaceInfo.subactionPath = rightHandPath;
                rightHandSpace = xrSession->createActionSpaceUnique(actionSpaceInfo, xrDispatchLoader);
            }

            // Attach the action set to the session.
            xr::SessionActionSetsAttachInfo attachInfo{};
            std::vector<xr::ActionSet> actionSets = {actionSet->get()};
            attachInfo.countActionSets = static_cast<uint32_t>(actionSets.size());
            attachInfo.actionSets = actionSets.data();
            xrSession->attachSessionActionSets(attachInfo);
        }

        auto xrViewConfig = xrInstance->enumerateViewConfigurationsToVector(xrSystemId, xrDispatchLoader);
        auto stereo = std::find_if(xrViewConfig.begin(), xrViewConfig.end(), [](xr::ViewConfigurationType &vc) {
            return vc == xr::ViewConfigurationType::PrimaryStereo;
        });
        assertThrow(stereo != xrViewConfig.end(), "failed to find stereo headset");

        xrViewConfigurationViews =
            xrInstance->enumerateViewConfigurationViewsToVector(xrSystemId, *stereo, xrDispatchLoader);

        xr::ReferenceSpaceCreateInfo xrSpaceInfo{};
        xrSpaceInfo.next = nullptr;
        xrSpaceInfo.referenceSpaceType = xr::ReferenceSpaceType::Stage;
        xrSpaceInfo.poseInReferenceSpace = xr::Posef{{0, 0, 0, 1}, {0, 0, 0}};
        xrSpace = xrSession->createReferenceSpaceUnique(xrSpaceInfo, xrDispatchLoader);

        auto xrSwapchainFormat = xrSession->enumerateSwapchainFormatsToVector(xrDispatchLoader);
        auto it = std::find_if(xrSwapchainFormat.begin(), xrSwapchainFormat.end(),
                               [this](int64_t &f) { return f == (long long)vk::Format::eR8G8B8A8Srgb; });
        assertThrow(it != xrSwapchainFormat.end(), "failed to find swapchain format");
        this->xrSwapchainFormat = *it;
        // std::cout << std::format("Prefered format: {}\n", *it);

        xr::SwapchainCreateInfo xrSwapchainInfo{};
        xrSwapchainInfo.next = nullptr;
        xrSwapchainInfo.createFlags = xr::SwapchainCreateFlagBits::None;
        xrSwapchainInfo.usageFlags = xr::SwapchainUsageFlagBits::ColorAttachment |
                                     xr::SwapchainUsageFlagBits::TransferSrc | xr::SwapchainUsageFlagBits::TransferDst;
        xrSwapchainInfo.format = (long)vk::Format::eR8G8B8A8Srgb;
        xrSwapchainInfo.sampleCount = 1;
        xrSwapchainInfo.width = xrViewConfigurationViews.at(0).recommendedImageRectWidth;
        xrSwapchainInfo.height = xrViewConfigurationViews.at(0).recommendedImageRectHeight;
        xrSwapchainInfo.faceCount = 1;
        xrSwapchainInfo.arraySize = 1;
        xrSwapchainInfo.mipCount = 1;

        xrSwapchain[0] = xrSession->createSwapchainUnique(xrSwapchainInfo, xrDispatchLoader);
        xrSwapchain[1] = xrSession->createSwapchainUnique(xrSwapchainInfo, xrDispatchLoader);

        xrSwapchainImageVk[0] =
            xrSwapchain[0]->enumerateSwapchainImagesToVector<xr::SwapchainImageVulkanKHR>(xrDispatchLoader);
        xrSwapchainImageVk[1] =
            xrSwapchain[1]->enumerateSwapchainImagesToVector<xr::SwapchainImageVulkanKHR>(xrDispatchLoader);

        swapchainImageViews[0].resize(xrSwapchainImageVk[0].size());
        swapchainImageViews[1].resize(xrSwapchainImageVk[1].size());

        for (size_t i = 0; i < swapchainImageViews[0].size(); i++)
        {
            vk::ImageSubresourceRange imageSubresourceRange{};
            imageSubresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
            imageSubresourceRange.setBaseMipLevel(0);
            imageSubresourceRange.setLevelCount(1);
            imageSubresourceRange.setBaseArrayLayer(0);
            imageSubresourceRange.setLayerCount(1);

            swapchainImageViews[0][i] =
                device->device.createImageViewUnique(vk::ImageViewCreateInfo{}
                                                         .setImage(xrSwapchainImageVk[0][i].image)
                                                         .setFormat(vk::Format::eR8G8B8A8Srgb)
                                                         .setViewType(vk::ImageViewType::e2D)
                                                         .setComponents(vk::ComponentMapping{})
                                                         .setSubresourceRange(imageSubresourceRange));
        }

        for (size_t i = 0; i < swapchainImageViews[1].size(); i++)
        {
            vk::ImageSubresourceRange imageSubresourceRange{};
            imageSubresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
            imageSubresourceRange.setBaseMipLevel(0);
            imageSubresourceRange.setLevelCount(1);
            imageSubresourceRange.setBaseArrayLayer(0);
            imageSubresourceRange.setLayerCount(1);

            swapchainImageViews[1][i] =
                device->device.createImageViewUnique(vk::ImageViewCreateInfo{}
                                                         .setImage(xrSwapchainImageVk[1][i].image)
                                                         .setFormat(vk::Format::eR8G8B8A8Srgb)
                                                         .setViewType(vk::ImageViewType::e2D)
                                                         .setComponents(vk::ComponentMapping{})
                                                         .setSubresourceRange(imageSubresourceRange));
        }

        // command buffer initialization
        commandPool =
            device->device.createCommandPoolUnique(vk::CommandPoolCreateInfo{}
                                                       .setQueueFamilyIndex(device->queues["graphics"].queueFamilyIndex)
                                                       .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer));
        commandBuffers =
            std::move(device->device.allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{}
                                                                      .setCommandBufferCount(3)
                                                                      .setCommandPool(commandPool.get())
                                                                      .setLevel(vk::CommandBufferLevel::ePrimary)));

        // fence initialization
        imageFence = device->device.createFenceUnique(vk::FenceCreateInfo{});
        commandFences.push_back(device->device.createFenceUnique(vk::FenceCreateInfo{}));
        commandFences.push_back(device->device.createFenceUnique(vk::FenceCreateInfo{}));
        commandFences.push_back(device->device.createFenceUnique(vk::FenceCreateInfo{}));

        // data initialization
        globalUniforms = {0.0f, 0.0f};
        globalUniformsBuffer = std::make_unique<letc::Buffer>(
            *allocator, sizeof(GlobalUniforms), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

        float s = 4.0f;
        float h = 2.0f; // height above the origin
        lights.push_back({{-s, h, -s, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}});
        lights.push_back({{-s, h, s, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}});
        lights.push_back({{s, h, s, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}});
        lights.push_back({{s, h, -s, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}});

        lightsBuffer =
            std::make_unique<letc::Buffer>(*allocator, sizeof(Light) * lights.size(),
                                           vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

        // XR cameras
        for (int i = 0; i < 2; ++i)
        {
            xrCameras.push_back(std::make_unique<letc::Camera>(
                *allocator, glm::vec4{0.0f, 0.0f, 2.0f, 1.0f}, glm::vec4{0.0f, 0.0f, 0.0f, 1.0f},
                glm::vec4{0.0f, 1.0f, 0.0f, 1.0f}, 60.0f,
                static_cast<float>(xrViewConfigurationViews.at(i).recommendedImageRectWidth) /       // Use index i here
                    static_cast<float>(xrViewConfigurationViews.at(i).recommendedImageRectHeight))); // Use index i here
        }

        models.emplace_back(*allocator, resourcePath / "Avocado.glb");
        models.emplace_back(*allocator, resourcePath / "pointy.glb");
        models.emplace_back(*allocator, resourcePath / "Avocado.glb"); // Left Hand Model placeholder?
        models.emplace_back(*allocator, resourcePath / "Avocado.glb"); // Right Hand Model placeholder?
        // Remove extra models if not needed, or adjust indices later
        models.emplace_back(*allocator, resourcePath / "Avocado.glb");
        models.emplace_back(*allocator, resourcePath / "Avocado.glb");

        float modelScalingFactor = 10.0f;
        models.at(0).uniform.model *= glm::scale(models.at(0).uniform.model, glm::vec3(modelScalingFactor));
        models.at(1).uniform.model *= glm::scale(models.at(1).uniform.model, glm::vec3(modelScalingFactor));
        // Scale hand models if necessary (models 2 and 3)

        std::for_each(models.begin(), models.end(), [](letc::Model &m) { m.cpyAttributes(); });

        std::for_each(models.begin(), models.end(),
                      [this](const letc::Model &m) { modelUniforms.push_back(m.uniform); });
        // *** Use eStorageBuffer for SSBO ***
        modelUniformsBuffer =
            std::make_unique<letc::Buffer>(*allocator, sizeof(letc::Model::UniformBuffer) * models.size(),
                                           vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

        // *** Initialize ModelRenderer ***
        modelRenderer = std::make_unique<letc::ModelRenderer>(
            *allocator, *device, readFile(resourcePath / "pbr.vert.spv"), readFile(resourcePath / "pbr.frag.spv"));

        // *** Update ModelRenderer's descriptor sets ***
        modelRenderer->material->updateDescriptorBufferInfo(0, 0, globalUniformsBuffer->buffer, 0,
                                                            sizeof(GlobalUniforms));
        modelRenderer->material->updateDescriptorBufferInfo(0, 1, lightsBuffer->buffer, 0,
                                                            sizeof(Light) * lights.size());
        // Binding for the SSBO
        modelRenderer->material->updateDescriptorBufferInfo(1, 0, modelUniformsBuffer->buffer, 0,
                                                            sizeof(letc::Model::UniformBuffer) * modelUniforms.size());
        modelRenderer->material->updateDescriptorSets(); // Update all sets once

        points = std::make_unique<letc::Points>(*allocator);
        pointsRenderer =
            std::make_unique<letc::PointsRenderer>(*allocator, *device, readFile(resourcePath / "points.vert.spv"),
                                                   readFile(resourcePath / "points.frag.spv"));

        // depth buffer initialization (flat)
        depthImage = letc::laconic::depthImage(allocator->allocator, window->getWidth(), window->getHeight());
        depthImageView = letc::laconic::depthImageView(*device, *depthImage);

        xrDepthImage =
            letc::laconic::depthImage(allocator->allocator, xrViewConfigurationViews.at(0).recommendedImageRectWidth,
                                      xrViewConfigurationViews.at(0).recommendedImageRectHeight);
        xrDepthImageView = letc::laconic::depthImageView(*device, *xrDepthImage);

        std::tie(lastMouseX, lastMouseY) = window->getCursorPos();

        // Poll for events until the session is ready
        while (true)
        {
            xrInstance->pollEvent(xrEventDataBuffer, xrDispatchLoader);

            if (xrEventDataBuffer.type == xr::StructureType::EventDataSessionStateChanged)
            {
                auto &event = *reinterpret_cast<xr::EventDataSessionStateChanged *>(&xrEventDataBuffer);
                if (event.session == *xrSession && event.state == xr::SessionState::Ready)
                {
                    break;
                }
            }
        }

        // Begin the session
        xr::SessionBeginInfo beginInfo{};
        beginInfo.primaryViewConfigurationType = xr::ViewConfigurationType::PrimaryStereo;
        xrSession->beginSession(beginInfo, xrDispatchLoader);
    }

    void triggersPressed(const glm::vec3 &handPosition)
    {
        std::cout << "Trigger pressed! Hand position: (" << handPosition.x << ", " << handPosition.y << ", "
                  << handPosition.z << ")\n";
        points->points.push_back(handPosition);
    }

    void joystickMoved(const glm::vec2 &joystickPosition)
    {
        std::cout << "Joystick moved! Position: (" << joystickPosition.x << ", " << joystickPosition.y << ")\n";
    }

    const std::chrono::duration<long long, std::milli> BUTTON_DEBOUNCE_TIME = std::chrono::milliseconds(100);
    std::chrono::time_point<std::chrono::steady_clock> lastButtonPressTime;
    char lastButtonPressed = '\0';
    void faceButtonPressed(const char button)
    {
        auto now = std::chrono::steady_clock::now();
        if (now - lastButtonPressTime >= BUTTON_DEBOUNCE_TIME || button != lastButtonPressed)
        {
            points->points.clear();

            switch (button)
            {
            case 'A':
                std::cout << "Face button A pressed." << std::endl;
                // TODO: Add functionality for button A
                break;
            case 'X':
                std::cout << "Face button X pressed." << std::endl;
                // TODO: Add functionality for button X
                break;
            case 'Y':
                std::cout << "Face button Y pressed." << std::endl;
                // TODO: Add functionality for button Y
                break;
            case 'B':
                std::cout << "Face button B pressed." << std::endl;
                // TODO: Add functionality for button B
                break;
            default:
                std::cout << "Unknown face button pressed: " << button << std::endl;
                break;
            }
            lastButtonPressTime = now;
            lastButtonPressed = button;
        }
    }

    void updateLogic()
    {
        vkfw::pollEvents();
        xrInstance->pollEvent(xrEventDataBuffer, xrDispatchLoader);

        if (window->getKey(vkfw::Key::eQ))
        {
            window->setShouldClose(true);
        }

        globalUniforms.time = static_cast<float>(vkfw::getTime());
        globalUniforms.frame = static_cast<float>(currentFrame++);

        models.at(0).uniform.model = glm::rotate(models.at(0).uniform.model, 0.01f, glm::vec3(0.0f, 1.0f, 0.0f));

        // rotate the lights around the y axis
        for (size_t i = 0; i < lights.size(); ++i)
        {
            float angle = globalUniforms.time + static_cast<float>(i) * glm::pi<float>() / 2.0f;
            lights[i].position.x = 4.0f * cos(angle); // Using 4.0f like non-xr, adjust if needed
            lights[i].position.z = 4.0f * sin(angle); // Using 4.0f like non-xr, adjust if needed
        }

        // --- Query OpenXR action states ---
        XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
        XrActiveActionSet activeActionSet{actionSet->get(), XR_NULL_PATH};
        syncInfo.countActiveActionSets = 1;
        syncInfo.activeActionSets = &activeActionSet;
        xrSyncActions(xrSession->get(), &syncInfo);

        xr::Time predictedDisplayTime = xrPredictedDisplayTime;

        xr::ActionStateGetInfo triggerActionStateInfo{};
        triggerActionStateInfo.action = *triggerAction;
        triggerActionStateInfo.subactionPath = leftHandPath;
        xr::ActionStateBoolean leftTriggerState =
            xrSession->getActionStateBoolean(triggerActionStateInfo, xrDispatchLoader);
        triggerActionStateInfo.subactionPath = rightHandPath;
        xr::ActionStateBoolean rightTriggerState =
            xrSession->getActionStateBoolean(triggerActionStateInfo, xrDispatchLoader);

        xr::SpaceLocation leftLocation;
        if (predictedDisplayTime != 0.0f)
        {
            leftLocation = xrSpace->locateSpace(*leftHandSpace, predictedDisplayTime, xrDispatchLoader);
            glm::mat4 leftTranslation =
                glm::translate(glm::mat4(1.0f), glm::vec3(leftLocation.pose.position.x, leftLocation.pose.position.y,
                                                          leftLocation.pose.position.z));
            glm::mat4 leftRotation =
                glm::mat4_cast(-glm::quat(leftLocation.pose.orientation.w, leftLocation.pose.orientation.x,
                                          leftLocation.pose.orientation.y, leftLocation.pose.orientation.z));
            models.at(2).uniform.model = glm::inverse(leftTranslation * leftRotation);
        }
        if (leftTriggerState.isActive && leftTriggerState.currentState)
        {
            if (leftLocation.locationFlags & xr::SpaceLocationFlagBits::PositionValid)
            {
                triggersPressed(glm::vec3(models.at(2).uniform.model[3]));
            }
        }
        xr::SpaceLocation rightLocation;
        if (predictedDisplayTime != 0.0f)
        {
            rightLocation = xrSpace->locateSpace(*rightHandSpace, predictedDisplayTime, xrDispatchLoader);
            glm::mat4 rightTranslation =
                glm::translate(glm::mat4(1.0f), glm::vec3(rightLocation.pose.position.x, rightLocation.pose.position.y,
                                                          rightLocation.pose.position.z));
            glm::mat4 rightRotation =
                glm::mat4_cast(-glm::quat(rightLocation.pose.orientation.w, rightLocation.pose.orientation.x,
                                          rightLocation.pose.orientation.y, rightLocation.pose.orientation.z));
            models.at(3).uniform.model = glm::inverse(rightTranslation * rightRotation);
        }
        if (rightTriggerState.isActive && rightTriggerState.currentState)
        {
            if (rightLocation.locationFlags & xr::SpaceLocationFlagBits::PositionValid)
            {
                glm::vec3 rightHandPosition{rightLocation.pose.position.x, rightLocation.pose.position.y,
                                            rightLocation.pose.position.z};
                triggersPressed(glm::vec3(models.at(3).uniform.model[3]));
            }
        }

        xr::ActionStateGetInfo joystickActionStateInfo{};
        joystickActionStateInfo.action = *joystickAction;
        joystickActionStateInfo.subactionPath = leftHandPath;
        xr::ActionStateVector2f leftJoystickState =
            xrSession->getActionStateVector2f(joystickActionStateInfo, xrDispatchLoader);
        joystickActionStateInfo.subactionPath = rightHandPath;
        xr::ActionStateVector2f rightJoystickState =
            xrSession->getActionStateVector2f(joystickActionStateInfo, xrDispatchLoader);
        if (leftJoystickState.isActive &&
            (leftJoystickState.currentState.x != 0.0f || leftJoystickState.currentState.y != 0.0f))
        {
            glm::vec2 leftJoystickPos{leftJoystickState.currentState.x, leftJoystickState.currentState.y};
            joystickMoved(leftJoystickPos);
        }
        if (rightJoystickState.isActive &&
            (rightJoystickState.currentState.x != 0.0f || rightJoystickState.currentState.y != 0.0f))
        {
            glm::vec2 rightJoystickPos{rightJoystickState.currentState.x, rightJoystickState.currentState.y};
            joystickMoved(rightJoystickPos);
        }

        xr::ActionStateGetInfo faceButtonActionStateInfo{};
        faceButtonActionStateInfo.action = *faceButtonAction;
        faceButtonActionStateInfo.subactionPath = leftHandPath;
        xr::ActionStateBoolean leftFaceButtonState =
            xrSession->getActionStateBoolean(faceButtonActionStateInfo, xrDispatchLoader);
        faceButtonActionStateInfo.subactionPath = rightHandPath;
        xr::ActionStateBoolean rightFaceButtonState =
            xrSession->getActionStateBoolean(faceButtonActionStateInfo, xrDispatchLoader);
        if (leftFaceButtonState.isActive && leftFaceButtonState.currentState)
        {
            faceButtonPressed('X'); // Left hand binding mapped to "X"
        }
        if (rightFaceButtonState.isActive && rightFaceButtonState.currentState)
        {
            faceButtonPressed('A'); // Right hand binding mapped to "A"
        }

        // --- Update modelUniforms SSBO data ---
        for (size_t i = 0; i < models.size(); ++i)
        {
            modelUniforms[i].model = models[i].uniform.model;
            glm::mat4 invModel = glm::inverse(models[i].uniform.model);
            if (glm::determinant(models[i].uniform.model) != 0.0f)
            {
                modelUniforms[i].modelInvTranspose = glm::transpose(invModel);
            }
            else
            {
                modelUniforms[i].modelInvTranspose = glm::mat4(1.0f); // Identity or handle error
            }
        }
    }

    void syncData()
    {
        globalUniformsBuffer->cpy(&globalUniforms, sizeof(GlobalUniforms));
        lightsBuffer->cpy(lights.data(), sizeof(Light) * lights.size());
        modelUniformsBuffer->cpy(modelUniforms.data(), sizeof(letc::Model::UniformBuffer) * models.size());

        descriptorManager->attachBuffer("pbrGlobalSet", 0, globalUniformsBuffer->buffer, sizeof(GlobalUniforms));
        descriptorManager->attachBuffer("pbrGlobalSet", 1, lightsBuffer->buffer, sizeof(Light) * lights.size());
        descriptorManager->attachBuffer("pbrModelSet", 0, modelUniformsBuffer->buffer,
                                        sizeof(letc::Model::UniformBuffer) * models.size());

        // *** No need to update modelRenderer descriptor sets here unless ***
        // *** global/light/model SSBO buffer bindings change, which they don't per frame ***
        modelRenderer->material->updateDescriptorSets(); // Remove this
        points->cpy();
    }

    void xrFrame()
    {
        // Wait for the next XR frame
        xr::FrameState xrFrameState = xrSession->waitFrame(nullptr, xrDispatchLoader);
        xrPredictedDisplayTime = xrFrameState.predictedDisplayTime;

        if (!xrFrameState.shouldRender)
        {
            std::clog << std::format("Skipping frame\n");
            xrSession->beginFrame(nullptr, xrDispatchLoader);

            xr::FrameEndInfo xrFrameEndInfo{};
            xrFrameEndInfo.displayTime = xrFrameState.predictedDisplayTime;
            xrFrameEndInfo.environmentBlendMode = xr::EnvironmentBlendMode::Opaque;
            xrFrameEndInfo.layerCount = 0;
            xrFrameEndInfo.layers = nullptr;
            xrSession->endFrame(xrFrameEndInfo, xrDispatchLoader);
            return;
        }

        xrSession->beginFrame(nullptr, xrDispatchLoader);

        const uint32_t viewCount = 2;
        xr::ViewLocateInfo viewLocateInfo{};
        viewLocateInfo.viewConfigurationType = xr::ViewConfigurationType::PrimaryStereo;
        viewLocateInfo.displayTime = xrFrameState.predictedDisplayTime;
        viewLocateInfo.space = *xrSpace;
        xr::ViewState viewState{};
        std::vector<xr::View> views = xrSession->locateViewsToVector(
            viewLocateInfo, reinterpret_cast<XrViewState *>(&viewState), xrDispatchLoader);

        if (views.size() >= 2)
        {
            // Process both eyes in a loop.
            for (size_t i = 0; i < 2; ++i)
            {
                float nearZ = 0.1f;
                float farZ = 50.0f;
                float left = glm::tan(views[i].fov.angleLeft) * nearZ;
                float right = glm::tan(views[i].fov.angleRight) * nearZ;
                float bottom = glm::tan(views[i].fov.angleDown) * nearZ;
                float top = glm::tan(views[i].fov.angleUp) * nearZ;
                glm::mat4 proj = glm::frustum(left, right, bottom, top, nearZ, farZ);

                glm::mat4 clipConversion = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                                                     0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

                proj = clipConversion * proj;

                glm::vec3 pos(views[i].pose.position.x, views[i].pose.position.y, views[i].pose.position.z);
                glm::quat orient(views[i].pose.orientation.w, views[i].pose.orientation.x, views[i].pose.orientation.y,
                                 views[i].pose.orientation.z);

                glm::mat4 eyeMatrix = glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(orient);
                glm::mat4 viewMatrix = glm::inverse(eyeMatrix);

                xrCameras.at(i)->uniform.view = viewMatrix;
                xrCameras.at(i)->uniform.proj = proj;
                xrCameras.at(i)->cpy();

                modelUniforms.at(i + 4).model = eyeMatrix;
            }

            modelUniformsBuffer->cpy(modelUniforms.data(), sizeof(letc::Model::UniformBuffer) * models.size());
        }

        std::vector<xr::CompositionLayerProjectionView> projectionViews(viewCount);

        for (uint32_t eye = 0; eye < viewCount; ++eye)
        {
            uint32_t xrImageIndex = xrSwapchain[eye]->acquireSwapchainImage(nullptr, xrDispatchLoader);

            xr::SwapchainImageWaitInfo waitInfo{xr::Duration::infinite()};
            xrSwapchain[eye]->waitSwapchainImage(waitInfo, xrDispatchLoader);

            uint32_t eyeWidth = xrViewConfigurationViews.at(eye).recommendedImageRectWidth;
            uint32_t eyeHeight = xrViewConfigurationViews.at(eye).recommendedImageRectHeight;

            // *** Update ModelRenderer camera binding for the current eye ***
            modelRenderer->material->updateDescriptorBufferInfo(0, 2, *xrCameras[eye]->buffer, 0,
                                                                sizeof(letc::Camera::Uniform));
            modelRenderer->material->updateDescriptorSet(0); // Only update set 0 (camera)

            descriptorManager->attachBuffer("pbrGlobalSet", 2, xrCameras[eye]->buffer->buffer,
                                            sizeof(letc::Camera::Uniform));

            // Update PointsRenderer camera binding
            pointsRenderer->material->updateDescriptorBufferInfo(0, 0, *xrCameras[eye]->buffer, 0,
                                                                 sizeof(letc::Camera::Uniform));
            pointsRenderer->material->updateDescriptorSet(0);

            vk::UniqueCommandBuffer &commandBuffer = commandBuffers.at(2);
            commandBuffer->reset(vk::CommandBufferResetFlagBits::eReleaseResources);
            commandBuffer->begin(vk::CommandBufferBeginInfo{}.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

            vk::Rect2D scissor({0, 0}, {eyeWidth, eyeHeight});
            commandBuffer->setScissor(0, 1, &scissor);
            vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(eyeWidth), static_cast<float>(eyeHeight), 0.0f, 1.0f);
            commandBuffer->setViewport(0, 1, &viewport);

            // color attachment layout transition
            letc::laconic::transitionImageLayout(
                *commandBuffer, xrSwapchainImageVk[eye][xrImageIndex].image, vk::ImageAspectFlagBits::eColor,
                vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, vk::AccessFlags{},
                vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eTopOfPipe,
                vk::PipelineStageFlagBits::eColorAttachmentOutput);
            // depth attachment layout transition (XR depth image)
            letc::laconic::transitionImageLayout(
                *commandBuffer, *xrDepthImage, vk::ImageAspectFlagBits::eDepth,
                vk::ImageLayout::eUndefined, // Use xrDepthImage
                vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::AccessFlags{},
                vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eEarlyFragmentTests);

            vk::RenderingInfo renderingInfo{};
            renderingInfo.setRenderArea({{0, 0}, {eyeWidth, eyeHeight}});
            renderingInfo.setLayerCount(1);

            vk::RenderingAttachmentInfo colorAttachment{};
            colorAttachment.setImageView(swapchainImageViews[eye][xrImageIndex].get());
            colorAttachment.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);
            colorAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
            colorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
            colorAttachment.setClearValue(
                vk::ClearValue{}.setColor(vk::ClearColorValue{}.setFloat32({0.1176f, 0.1176f, 0.1804f, 1.0f})));

            vk::RenderingAttachmentInfo depthAttachment{};
            depthAttachment.setImageView(*xrDepthImageView);
            depthAttachment.setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
            depthAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
            depthAttachment.setStoreOp(vk::AttachmentStoreOp::eDontCare);
            depthAttachment.setClearValue(vk::ClearValue{}.setDepthStencil({1.0f, 0}));

            renderingInfo.setColorAttachmentCount(1);
            renderingInfo.setPColorAttachments(&colorAttachment);
            renderingInfo.setPDepthAttachment(&depthAttachment);

            commandBuffer->beginRendering(renderingInfo);

            // *** Render models using ModelRenderer and push constants ***
            modelRenderer->pipeline->bind(commandBuffer.get());
            // modelRenderer->material->bind(commandBuffer.get(), *modelRenderer->pipeline);

            auto sets = descriptorManager->organizeSets("pbrGlobalSet", "pbrModelSet");
            commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, modelRenderer->pipeline->layout, 0,
                                              sets.size(), sets.data(), 0, nullptr);

            for (uint32_t i = 0; i < models.size(); ++i)
            {
                commandBuffer->pushConstants(modelRenderer->pipeline->layout, vk::ShaderStageFlagBits::eAllGraphics, 0,
                                             sizeof(uint32_t), &i);
                models[i].draw(*commandBuffer);
            }

            pointsRenderer->pipeline->bind(commandBuffer.get());
            pointsRenderer->material->bind(commandBuffer.get(), *pointsRenderer->pipeline);
            points->draw(*commandBuffer);

            commandBuffer->endRendering();

            // color attachment layout transition
            letc::laconic::transitionImageLayout(
                *commandBuffer, xrSwapchainImageVk[eye][xrImageIndex].image, vk::ImageAspectFlagBits::eColor,
                vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
                vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eMemoryRead,
                vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe);

            commandBuffer->end();

            vk::Fence commandFence = commandFences.at(eye).get();
            assertThrow(device->device.resetFences(1, &commandFence) == vk::Result::eSuccess,
                        "failed to reset command fence");
            queue.submit(vk::SubmitInfo{}.setCommandBufferCount(1).setPCommandBuffers(&commandBuffer.get()),
                         commandFence);
            assertThrow(device->device.waitForFences(1, &commandFence, VK_TRUE, 5000000000) == vk::Result::eSuccess,
                        "failed to wait for command fence");

            xr::SwapchainImageReleaseInfo releaseInfo{};
            xrSwapchain[eye]->releaseSwapchainImage(nullptr, xrDispatchLoader);

            projectionViews[eye].pose = views[eye].pose;
            projectionViews[eye].fov = views[eye].fov;
            projectionViews[eye].subImage.swapchain = xrSwapchain[eye].get();
            projectionViews[eye].subImage.imageRect = xr::Rect2Di{
                xr::Offset2Di{0, 0}, xr::Extent2Di{static_cast<int32_t>(eyeWidth), static_cast<int32_t>(eyeHeight)}};
            projectionViews[eye].subImage.imageArrayIndex = 0;
        }

        xr::CompositionLayerProjection projectionLayer{};
        projectionLayer.space = *xrSpace;
        projectionLayer.viewCount = static_cast<uint32_t>(projectionViews.size());
        projectionLayer.views = projectionViews.data();

        xr::FrameEndInfo xrFrameEndInfo{};
        xrFrameEndInfo.displayTime = xrFrameState.predictedDisplayTime;
        xrFrameEndInfo.environmentBlendMode = xr::EnvironmentBlendMode::Opaque;
        std::vector<const xr::CompositionLayerBaseHeader *> layers = {
            reinterpret_cast<const xr::CompositionLayerBaseHeader *>(&projectionLayer)};
        xrFrameEndInfo.layerCount = static_cast<uint32_t>(layers.size());
        xrFrameEndInfo.layers = layers.data();

        xrSession->endFrame(xrFrameEndInfo, xrDispatchLoader);
    }

    ~App()
    {
        device->device.waitIdle();
    }
};

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        resourcePath = std::filesystem::path(argv[1]);
    }

    App app{};
    while (!app.window->shouldClose())
    {
        app.updateLogic();
        app.syncData();
        app.xrFrame();
    }
    return 0;
}
