#pragma once

#include "Allocator.hh"
#include "pch.hh"

#include "Camera.hh"
#include "Image.hh"

// moving all the xr code into its own class cause its just boilerplate

struct XrContext
{
    xr::DispatchLoaderDynamic dispatchLoader;
    xr::UniqueDynamicInstance instance;
    xr::SystemId systemId;

    xr::UniqueDynamicSession session;
    xr::UniqueDynamicSwapchain swapchain[2];

    xr::Time predictedDisplayTime = xr::Time{};
    xr::UniqueDynamicSpace leftHandSpace;
    xr::UniqueDynamicSpace rightHandSpace;
    xr::Path leftHandPath;
    xr::Path rightHandPath;
    xr::UniqueDynamicActionSet actionSet;
    xr::UniqueDynamicAction triggerAction;
    xr::UniqueDynamicAction joystickAction;
    xr::UniqueDynamicAction handPoseAction;

    xr::UniqueDynamicAction aButtonAction;
    xr::UniqueDynamicAction bButtonAction;
    xr::UniqueDynamicAction xButtonAction;
    xr::UniqueDynamicAction yButtonAction;

    std::vector<xr::ViewConfigurationView> viewConfigurationViews;
    xr::UniqueDynamicSpace space;
    long long swapchainFormat;
    std::vector<xr::SwapchainImageVulkanKHR, std::allocator<xr::SwapchainImageVulkanKHR>> swapchainImageVk[2];
    std::vector<vk::UniqueImageView> swapchainImageViews[2];
    xr::EventDataBuffer eventDataBuffer{};
    xr::SessionState sessionState = xr::SessionState::Unknown;

    std::vector<std::unique_ptr<letc::Camera>> cameras;

    std::unique_ptr<letc::Image> depthImage[2];
    std::unique_ptr<letc::ImageView> depthImageView[2];

    XrContext()
    {
        dispatchLoader = xr::DispatchLoaderDynamic{};
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

        instance = xr::createInstanceUnique(xrInstanceInfo, dispatchLoader);
        assertThrow(instance, "failed to create xr instance");
        dispatchLoader = xr::DispatchLoaderDynamic{*instance};

        xr::SystemGetInfo xrSystemInfo{};
        xrSystemInfo.formFactor = xr::FormFactor::HeadMountedDisplay;
        xrSystemInfo.next = nullptr;
        systemId = instance->getSystem(xrSystemInfo);
    }

    auto getGraphicsDevice(const vk::Instance &vkInstance)
    {
        auto xrGraphicsRequirements = instance->getVulkanGraphicsRequirements2KHR(systemId, dispatchLoader);
        auto xrGraphicsDevice = instance->getVulkanGraphicsDevice2KHR(
            xr::VulkanGraphicsDeviceGetInfoKHR{systemId, vkInstance, nullptr}, dispatchLoader);
        // auto xrGraphicsDeviceExtensions = xrInstance->getVulkanDeviceExtensionsKHR(xrSystemId, xrDispatchLoader);

        return xrGraphicsDevice;
    }

    auto sessionCreate(const letc::Allocator &allocator, const vk::Instance &vkInstance,
                       const vk::PhysicalDevice &physicalDevice, const vk::Device &device,
                       const uint32_t &queueFamilyIndex)
    {
        xr::GraphicsBindingVulkanKHR xrGraphicsBinding{};
        xrGraphicsBinding.next = nullptr;
        xrGraphicsBinding.instance = vkInstance;
        xrGraphicsBinding.physicalDevice = physicalDevice;
        xrGraphicsBinding.device = device;
        xrGraphicsBinding.queueFamilyIndex = queueFamilyIndex;
        xrGraphicsBinding.queueIndex = 0;

        xr::SessionCreateInfo xrSessionInfo{};
        xrSessionInfo.next = &xrGraphicsBinding;
        xrSessionInfo.createFlags = xr::SessionCreateFlagBits::None;
        xrSessionInfo.systemId = systemId;
        session = instance->createSessionUnique(xrSessionInfo, dispatchLoader);

        {
            leftHandPath = instance->stringToPath("/user/hand/left", dispatchLoader);
            rightHandPath = instance->stringToPath("/user/hand/right", dispatchLoader);

            // Create an action set.
            xr::ActionSetCreateInfo actionSetInfo{};
            std::strcpy(actionSetInfo.actionSetName, "gameplay");
            std::strcpy(actionSetInfo.localizedActionSetName, "Gameplay");
            actionSetInfo.priority = 0;
            actionSet = instance->createActionSetUnique(actionSetInfo, dispatchLoader);

            std::vector<xr::Path> subactionPaths = {leftHandPath, rightHandPath};

            // Create the trigger action (boolean input).
            xr::ActionCreateInfo triggerActionInfo{};
            std::strcpy(triggerActionInfo.actionName, "trigger_press");
            std::strcpy(triggerActionInfo.localizedActionName, "Trigger Press");
            triggerActionInfo.actionType = xr::ActionType::BooleanInput;
            triggerActionInfo.countSubactionPaths = static_cast<uint32_t>(subactionPaths.size());
            triggerActionInfo.subactionPaths = subactionPaths.data();
            triggerAction = actionSet->createActionUnique(triggerActionInfo, dispatchLoader);

            // Create the joystick action (vector2f input).
            xr::ActionCreateInfo joystickActionInfo{};
            std::strcpy(joystickActionInfo.actionName, "joystick_move");
            std::strcpy(joystickActionInfo.localizedActionName, "Joystick Move");
            joystickActionInfo.actionType = xr::ActionType::Vector2FInput;
            joystickActionInfo.countSubactionPaths = static_cast<uint32_t>(subactionPaths.size());
            joystickActionInfo.subactionPaths = subactionPaths.data();
            joystickAction = actionSet->createActionUnique(joystickActionInfo, dispatchLoader);

            // Create the A button action (boolean input, typically right hand)
            xr::ActionCreateInfo aButtonActionInfo{};
            std::strcpy(aButtonActionInfo.actionName, "a_button_press");
            std::strcpy(aButtonActionInfo.localizedActionName, "A Button Press");
            aButtonActionInfo.actionType = xr::ActionType::BooleanInput;
            aButtonActionInfo.countSubactionPaths =
                static_cast<uint32_t>(subactionPaths.size()); // Still need both subactions for potential remapping
            aButtonActionInfo.subactionPaths = subactionPaths.data();
            aButtonAction = actionSet->createActionUnique(aButtonActionInfo, dispatchLoader);
            assertThrow(aButtonAction, "Failed to create A button action");

            // Create the B button action (boolean input, typically right hand)
            xr::ActionCreateInfo bButtonActionInfo{};
            std::strcpy(bButtonActionInfo.actionName, "b_button_press");
            std::strcpy(bButtonActionInfo.localizedActionName, "B Button Press");
            bButtonActionInfo.actionType = xr::ActionType::BooleanInput;
            bButtonActionInfo.countSubactionPaths = static_cast<uint32_t>(subactionPaths.size());
            bButtonActionInfo.subactionPaths = subactionPaths.data();
            bButtonAction = actionSet->createActionUnique(bButtonActionInfo, dispatchLoader);
            assertThrow(bButtonAction, "Failed to create B button action");

            // Create the X button action (boolean input, typically left hand)
            xr::ActionCreateInfo xButtonActionInfo{};
            std::strcpy(xButtonActionInfo.actionName, "x_button_press");
            std::strcpy(xButtonActionInfo.localizedActionName, "X Button Press");
            xButtonActionInfo.actionType = xr::ActionType::BooleanInput;
            xButtonActionInfo.countSubactionPaths = static_cast<uint32_t>(subactionPaths.size());
            xButtonActionInfo.subactionPaths = subactionPaths.data();
            xButtonAction = actionSet->createActionUnique(xButtonActionInfo, dispatchLoader);
            assertThrow(xButtonAction, "Failed to create X button action");

            // Create the Y button action (boolean input, typically left hand)
            xr::ActionCreateInfo yButtonActionInfo{};
            std::strcpy(yButtonActionInfo.actionName, "y_button_press");
            std::strcpy(yButtonActionInfo.localizedActionName, "Y Button Press");
            yButtonActionInfo.actionType = xr::ActionType::BooleanInput;
            yButtonActionInfo.countSubactionPaths = static_cast<uint32_t>(subactionPaths.size());
            yButtonActionInfo.subactionPaths = subactionPaths.data();
            yButtonAction = actionSet->createActionUnique(yButtonActionInfo, dispatchLoader);
            assertThrow(yButtonAction, "Failed to create Y button action");

            // Create a pose action to capture hand position data.
            xr::ActionCreateInfo handPoseActionInfo{};
            std::strcpy(handPoseActionInfo.actionName, "hand_pose");
            std::strcpy(handPoseActionInfo.localizedActionName, "Hand Pose");
            handPoseActionInfo.actionType = xr::ActionType::PoseInput;
            handPoseActionInfo.countSubactionPaths = static_cast<uint32_t>(subactionPaths.size());
            handPoseActionInfo.subactionPaths = subactionPaths.data();
            handPoseAction = actionSet->createActionUnique(handPoseActionInfo, dispatchLoader);

            // Suggest bindings for a common controller profile (example: Oculus Touch).
            xr::Path oculusTouchInteractionProfile =
                instance->stringToPath("/interaction_profiles/oculus/touch_controller", dispatchLoader);
            std::vector<xr::ActionSuggestedBinding> bindings;
            bindings.push_back(
                {*triggerAction, instance->stringToPath("/user/hand/left/input/trigger", dispatchLoader)});
            bindings.push_back(
                {*triggerAction, instance->stringToPath("/user/hand/right/input/trigger", dispatchLoader)});
            bindings.push_back(
                {*joystickAction, instance->stringToPath("/user/hand/left/input/thumbstick", dispatchLoader)});
            bindings.push_back(
                {*joystickAction, instance->stringToPath("/user/hand/right/input/thumbstick", dispatchLoader)});
            bindings.push_back(
                {*handPoseAction, instance->stringToPath("/user/hand/left/input/aim/pose", dispatchLoader)});
            bindings.push_back(
                {*handPoseAction, instance->stringToPath("/user/hand/right/input/aim/pose", dispatchLoader)});
            // Added individual face button bindings
            bindings.push_back(
                {*aButtonAction, instance->stringToPath("/user/hand/right/input/a/click", dispatchLoader)});
            bindings.push_back(
                {*bButtonAction, instance->stringToPath("/user/hand/right/input/b/click", dispatchLoader)});
            bindings.push_back(
                {*xButtonAction, instance->stringToPath("/user/hand/left/input/x/click", dispatchLoader)});
            bindings.push_back(
                {*yButtonAction, instance->stringToPath("/user/hand/left/input/y/click", dispatchLoader)});

            xr::InteractionProfileSuggestedBinding suggestedBinding{};
            suggestedBinding.interactionProfile = oculusTouchInteractionProfile;
            suggestedBinding.suggestedBindings = bindings.data();
            suggestedBinding.countSuggestedBindings = static_cast<uint32_t>(bindings.size());
            instance->suggestInteractionProfileBindings(suggestedBinding, dispatchLoader);

            {
                xr::ActionSpaceCreateInfo actionSpaceInfo{};
                actionSpaceInfo.action = *handPoseAction;
                // Use an identity pose for the action space (adjust if you need an offset).
                actionSpaceInfo.poseInActionSpace.orientation = xr::Quaternionf{0, 0, 0, 1};
                actionSpaceInfo.poseInActionSpace.position = xr::Vector3f{0, 0, 0};

                // Create left hand action space.
                actionSpaceInfo.subactionPath = leftHandPath;
                leftHandSpace = session->createActionSpaceUnique(actionSpaceInfo, dispatchLoader);

                // Create right hand action space.
                actionSpaceInfo.subactionPath = rightHandPath;
                rightHandSpace = session->createActionSpaceUnique(actionSpaceInfo, dispatchLoader);
            }

            // Attach the action set to the session.
            xr::SessionActionSetsAttachInfo attachInfo{};
            std::vector<xr::ActionSet> actionSets = {actionSet->get()};
            attachInfo.countActionSets = static_cast<uint32_t>(actionSets.size());
            attachInfo.actionSets = actionSets.data();
            session->attachSessionActionSets(attachInfo);
        }

        auto xrViewConfig = instance->enumerateViewConfigurationsToVector(systemId, dispatchLoader);
        auto stereo = std::find_if(xrViewConfig.begin(), xrViewConfig.end(), [](xr::ViewConfigurationType &vc) {
            return vc == xr::ViewConfigurationType::PrimaryStereo;
        });
        assertThrow(stereo != xrViewConfig.end(), "failed to find stereo headset");

        viewConfigurationViews = instance->enumerateViewConfigurationViewsToVector(systemId, *stereo, dispatchLoader);

        xr::ReferenceSpaceCreateInfo xrSpaceInfo{};
        xrSpaceInfo.next = nullptr;
        xrSpaceInfo.referenceSpaceType = xr::ReferenceSpaceType::Local;
        xrSpaceInfo.poseInReferenceSpace = xr::Posef{{0, 0, 0, 1}, {0, 0, 0}};
        space = session->createReferenceSpaceUnique(xrSpaceInfo, dispatchLoader);

        auto xrSwapchainFormat = session->enumerateSwapchainFormatsToVector(dispatchLoader);
        auto it = std::find_if(xrSwapchainFormat.begin(), xrSwapchainFormat.end(),
                               [this](int64_t &f) { return f == (long long)vk::Format::eR8G8B8A8Srgb; });
        assertThrow(it != xrSwapchainFormat.end(), "failed to find swapchain format");
        this->swapchainFormat = *it;
        // std::cout << std::format("Prefered format: {}\n", *it);

        xr::SwapchainCreateInfo xrSwapchainInfo{};
        xrSwapchainInfo.next = nullptr;
        xrSwapchainInfo.createFlags = xr::SwapchainCreateFlagBits::None;
        xrSwapchainInfo.usageFlags = xr::SwapchainUsageFlagBits::ColorAttachment |
                                     xr::SwapchainUsageFlagBits::TransferSrc | xr::SwapchainUsageFlagBits::TransferDst;
        xrSwapchainInfo.format = (long)vk::Format::eR8G8B8A8Srgb;
        xrSwapchainInfo.sampleCount = 1;
        xrSwapchainInfo.width = viewConfigurationViews.at(0).recommendedImageRectWidth;
        xrSwapchainInfo.height = viewConfigurationViews.at(0).recommendedImageRectHeight;
        xrSwapchainInfo.faceCount = 1;
        xrSwapchainInfo.arraySize = 1;
        xrSwapchainInfo.mipCount = 1;

        swapchain[0] = session->createSwapchainUnique(xrSwapchainInfo, dispatchLoader);
        swapchain[1] = session->createSwapchainUnique(xrSwapchainInfo, dispatchLoader);

        swapchainImageVk[0] =
            swapchain[0]->enumerateSwapchainImagesToVector<xr::SwapchainImageVulkanKHR>(dispatchLoader);
        swapchainImageVk[1] =
            swapchain[1]->enumerateSwapchainImagesToVector<xr::SwapchainImageVulkanKHR>(dispatchLoader);

        swapchainImageViews[0].resize(swapchainImageVk[0].size());
        swapchainImageViews[1].resize(swapchainImageVk[1].size());

        for (size_t i = 0; i < swapchainImageViews[0].size(); i++)
        {
            vk::ImageSubresourceRange imageSubresourceRange{};
            imageSubresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
            imageSubresourceRange.setBaseMipLevel(0);
            imageSubresourceRange.setLevelCount(1);
            imageSubresourceRange.setBaseArrayLayer(0);
            imageSubresourceRange.setLayerCount(1);

            swapchainImageViews[0][i] = device.createImageViewUnique(vk::ImageViewCreateInfo{}
                                                                         .setImage(swapchainImageVk[0][i].image)
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

            swapchainImageViews[1][i] = device.createImageViewUnique(vk::ImageViewCreateInfo{}
                                                                         .setImage(swapchainImageVk[1][i].image)
                                                                         .setFormat(vk::Format::eR8G8B8A8Srgb)
                                                                         .setViewType(vk::ImageViewType::e2D)
                                                                         .setComponents(vk::ComponentMapping{})
                                                                         .setSubresourceRange(imageSubresourceRange));
        }

        // XR cameras
        for (int i = 0; i < 2; ++i)
        {
            cameras.push_back(std::make_unique<letc::Camera>(
                allocator, glm::vec4{0.0f, 0.0f, 2.0f, 1.0f}, glm::vec4{0.0f, 0.0f, 0.0f, 1.0f},
                glm::vec4{0.0f, 1.0f, 0.0f, 1.0f}, 60.0f,
                static_cast<float>(viewConfigurationViews.at(i).recommendedImageRectWidth) /       // Use index i here
                    static_cast<float>(viewConfigurationViews.at(i).recommendedImageRectHeight))); // Use index i here
        }

        depthImage[0] =
            letc::laconic::depthImage(allocator.allocator, viewConfigurationViews.at(0).recommendedImageRectWidth,
                                      viewConfigurationViews.at(0).recommendedImageRectHeight);
        depthImage[1] =
            letc::laconic::depthImage(allocator.allocator, viewConfigurationViews.at(0).recommendedImageRectWidth,
                                      viewConfigurationViews.at(0).recommendedImageRectHeight);
        depthImageView[0] = letc::laconic::depthImageView(device, depthImage[0]->image);
        depthImageView[1] = letc::laconic::depthImageView(device, depthImage[1]->image);

        // Poll for events until the session is ready
        while (true)
        {
            instance->pollEvent(eventDataBuffer, dispatchLoader);

            if (eventDataBuffer.type == xr::StructureType::EventDataSessionStateChanged)
            {
                auto &event = *reinterpret_cast<xr::EventDataSessionStateChanged *>(&eventDataBuffer);
                if (event.session == *session && event.state == xr::SessionState::Ready)
                {
                    break;
                }
            }
        }

        // Begin the session
        xr::SessionBeginInfo beginInfo{};
        beginInfo.primaryViewConfigurationType = xr::ViewConfigurationType::PrimaryStereo;
        session->beginSession(beginInfo, dispatchLoader);
    }

    auto updateLogic()
    {
        instance->pollEvent(eventDataBuffer, dispatchLoader);

        struct
        {
            glm::mat4 leftHand, rightHand = glm::mat4(1.0f);
            bool leftTriggerPressed, rightTriggerPressed = false;
            glm::vec2 leftJoystick, rightJoystick = glm::vec2(0.0);
            bool a, b, x, y = false;
        } eventStatus;

        // --- Query OpenXR action states ---
        XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
        XrActiveActionSet activeActionSet{actionSet->get(), XR_NULL_PATH};
        syncInfo.countActiveActionSets = 1;
        syncInfo.activeActionSets = &activeActionSet;
        session->syncActions(syncInfo);

        xr::Time predictedDisplayTime = this->predictedDisplayTime;

        xr::ActionStateGetInfo triggerActionStateInfo{};
        triggerActionStateInfo.action = *triggerAction;
        triggerActionStateInfo.subactionPath = leftHandPath;
        xr::ActionStateBoolean leftTriggerState =
            session->getActionStateBoolean(triggerActionStateInfo, dispatchLoader);
        triggerActionStateInfo.subactionPath = rightHandPath;
        xr::ActionStateBoolean rightTriggerState =
            session->getActionStateBoolean(triggerActionStateInfo, dispatchLoader);

        xr::SpaceLocation leftLocation;
        if (predictedDisplayTime != 0.0f)
        {
            leftLocation = space->locateSpace(*leftHandSpace, predictedDisplayTime, dispatchLoader);
            glm::mat4 leftTranslation =
                glm::translate(glm::mat4(1.0f), glm::vec3(leftLocation.pose.position.x, leftLocation.pose.position.y,
                                                          leftLocation.pose.position.z));
            glm::mat4 leftRotation =
                glm::mat4_cast(-glm::quat(leftLocation.pose.orientation.w, leftLocation.pose.orientation.x,
                                          leftLocation.pose.orientation.y, leftLocation.pose.orientation.z));
            eventStatus.leftHand = glm::inverse(leftTranslation * leftRotation);
        }
        if (leftTriggerState.isActive && leftTriggerState.currentState)
        {
            if (leftLocation.locationFlags & xr::SpaceLocationFlagBits::PositionValid)
            {
                eventStatus.leftTriggerPressed = true;
            }
        }
        xr::SpaceLocation rightLocation;
        if (predictedDisplayTime != 0.0f)
        {
            rightLocation = space->locateSpace(*rightHandSpace, predictedDisplayTime, dispatchLoader);
            glm::mat4 rightTranslation =
                glm::translate(glm::mat4(1.0f), glm::vec3(rightLocation.pose.position.x, rightLocation.pose.position.y,
                                                          rightLocation.pose.position.z));
            glm::mat4 rightRotation =
                glm::mat4_cast(-glm::quat(rightLocation.pose.orientation.w, rightLocation.pose.orientation.x,
                                          rightLocation.pose.orientation.y, rightLocation.pose.orientation.z));
            eventStatus.rightHand = glm::inverse(rightTranslation * rightRotation);
        }
        if (rightTriggerState.isActive && rightTriggerState.currentState)
        {
            if (rightLocation.locationFlags & xr::SpaceLocationFlagBits::PositionValid)
            {
                eventStatus.rightTriggerPressed = true;
            }
        }

        xr::ActionStateGetInfo joystickActionStateInfo{};
        joystickActionStateInfo.action = *joystickAction;
        joystickActionStateInfo.subactionPath = leftHandPath;
        xr::ActionStateVector2f leftJoystickState =
            session->getActionStateVector2f(joystickActionStateInfo, dispatchLoader);
        joystickActionStateInfo.subactionPath = rightHandPath;
        xr::ActionStateVector2f rightJoystickState =
            session->getActionStateVector2f(joystickActionStateInfo, dispatchLoader);
        if (leftJoystickState.isActive &&
            (leftJoystickState.currentState.x != 0.0f || leftJoystickState.currentState.y != 0.0f))
        {
            glm::vec2 leftJoystickPos{leftJoystickState.currentState.x, leftJoystickState.currentState.y};
            eventStatus.leftJoystick = leftJoystickPos;
        }
        if (rightJoystickState.isActive &&
            (rightJoystickState.currentState.x != 0.0f || rightJoystickState.currentState.y != 0.0f))
        {
            glm::vec2 rightJoystickPos{rightJoystickState.currentState.x, rightJoystickState.currentState.y};
            eventStatus.rightJoystick = rightJoystickPos;
        }

        // Query state for A button (Right Hand)
        xr::ActionStateGetInfo aButtonStateInfo{};
        aButtonStateInfo.action = *aButtonAction;
        aButtonStateInfo.subactionPath = rightHandPath; // Query specific hand for this button
        xr::ActionStateBoolean aButtonState = session->getActionStateBoolean(aButtonStateInfo, dispatchLoader);
        if (aButtonState.isActive && aButtonState.currentState)
        {
            eventStatus.a = true;
        }

        // Query state for B button (Right Hand)
        xr::ActionStateGetInfo bButtonStateInfo{};
        bButtonStateInfo.action = *bButtonAction;
        bButtonStateInfo.subactionPath = rightHandPath; // Query specific hand
        xr::ActionStateBoolean bButtonState = session->getActionStateBoolean(bButtonStateInfo, dispatchLoader);
        if (bButtonState.isActive && bButtonState.currentState)
        {
            eventStatus.b = true;
        }

        // Query state for X button (Left Hand)
        xr::ActionStateGetInfo xButtonStateInfo{};
        xButtonStateInfo.action = *xButtonAction;
        xButtonStateInfo.subactionPath = leftHandPath; // Query specific hand
        xr::ActionStateBoolean xButtonState = session->getActionStateBoolean(xButtonStateInfo, dispatchLoader);
        if (xButtonState.isActive && xButtonState.currentState)
        {
            eventStatus.x = true;
        }

        // Query state for Y button (Left Hand)
        xr::ActionStateGetInfo yButtonStateInfo{};
        yButtonStateInfo.action = *yButtonAction;
        yButtonStateInfo.subactionPath = leftHandPath; // Query specific hand
        xr::ActionStateBoolean yButtonState = session->getActionStateBoolean(yButtonStateInfo, dispatchLoader);
        if (yButtonState.isActive && yButtonState.currentState)
        {
            eventStatus.y = true;
        }

        return eventStatus;
    }

    xr::FrameState frameState;
    auto startFrame()
    {
        // Wait for the next XR frame
        frameState = session->waitFrame(nullptr, dispatchLoader);
        predictedDisplayTime = frameState.predictedDisplayTime;

        if (!frameState.shouldRender)
        {
            std::clog << std::format("Skipping frame\n");
            session->beginFrame(nullptr, dispatchLoader);

            xr::FrameEndInfo xrFrameEndInfo{};
            xrFrameEndInfo.displayTime = frameState.predictedDisplayTime;
            xrFrameEndInfo.environmentBlendMode = xr::EnvironmentBlendMode::Opaque;
            xrFrameEndInfo.layerCount = 0;
            xrFrameEndInfo.layers = nullptr;
            session->endFrame(xrFrameEndInfo, dispatchLoader);
            return false;
        }

        session->beginFrame(nullptr, dispatchLoader);

        return true;
    }

    std::vector<xr::View> views;
    auto getViews()
    {
        const uint32_t viewCount = 2;
        xr::ViewLocateInfo viewLocateInfo{};
        viewLocateInfo.viewConfigurationType = xr::ViewConfigurationType::PrimaryStereo;
        viewLocateInfo.displayTime = frameState.predictedDisplayTime;
        viewLocateInfo.space = *space;
        xr::ViewState viewState{};
        views =
            session->locateViewsToVector(viewLocateInfo, reinterpret_cast<XrViewState *>(&viewState), dispatchLoader);

        std::array<glm::mat4, 2> eyes;

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

                cameras.at(i)->uniform.view = viewMatrix;
                cameras.at(i)->uniform.proj = proj;
                cameras.at(i)->cpy();

                eyes[i] = eyeMatrix;
            }
        }

        return eyes;
    }

    auto startRendering()
    {
        struct RenderingState
        {
            uint32_t imageIndex;
            uint32_t width, height = 0;
            letc::Camera *camera = nullptr;
            vk::Image swapchainImage = nullptr;
            vk::ImageView swapchainImageView = nullptr;
            vk::Image depthImage = nullptr;
            vk::ImageView depthImageView = nullptr;
        };
        std::vector<RenderingState> renderingState(2);

        for (int i = 0; i < 2; ++i)
        {
            renderingState[i].imageIndex = swapchain[i]->acquireSwapchainImage(nullptr, dispatchLoader);

            renderingState[i].width = viewConfigurationViews.at(i).recommendedImageRectWidth;
            renderingState[i].height = viewConfigurationViews.at(i).recommendedImageRectHeight;

            renderingState[i].camera = cameras.at(i).get();

            renderingState[i].swapchainImage = swapchainImageVk[i][renderingState[i].imageIndex].image;
            renderingState[i].swapchainImageView = swapchainImageViews[i][renderingState[i].imageIndex].get();

            renderingState[i].depthImage = depthImage[i]->image;
            renderingState[i].depthImageView = depthImageView[i]->imageView;
        }

        xr::SwapchainImageWaitInfo waitInfo{};
        waitInfo.timeout = xr::Duration::infinite(); // Or a reasonable timeout
        swapchain[0]->waitSwapchainImage(waitInfo, dispatchLoader);
        swapchain[1]->waitSwapchainImage(waitInfo, dispatchLoader);

        return renderingState;
    }

    auto endRendering()
    {
        std::array<xr::CompositionLayerProjectionView, 2> projectionViews;
        xr::SwapchainImageReleaseInfo releaseInfo{};

        for (int i = 0; i < 2; ++i)
        {
            swapchain[i]->releaseSwapchainImage(nullptr, dispatchLoader);
            projectionViews[i].pose = views[i].pose;
            projectionViews[i].fov = views[i].fov;
            projectionViews[i].subImage.swapchain = swapchain[i].get();
            projectionViews[i].subImage.imageRect = xr::Rect2Di{
                xr::Offset2Di{0, 0},
                xr::Extent2Di{static_cast<int32_t>(viewConfigurationViews.at(i).recommendedImageRectWidth),
                              static_cast<int32_t>(viewConfigurationViews.at(i).recommendedImageRectHeight)}};
            projectionViews[i].subImage.imageArrayIndex = 0;
        }

        xr::CompositionLayerProjection projectionLayer{};
        projectionLayer.space = *space;
        projectionLayer.viewCount = static_cast<uint32_t>(projectionViews.size());
        projectionLayer.views = projectionViews.data();

        xr::FrameEndInfo xrFrameEndInfo{};
        xrFrameEndInfo.displayTime = frameState.predictedDisplayTime;
        xrFrameEndInfo.environmentBlendMode = xr::EnvironmentBlendMode::Opaque;
        std::vector<const xr::CompositionLayerBaseHeader *> layers = {
            reinterpret_cast<const xr::CompositionLayerBaseHeader *>(&projectionLayer)};
        xrFrameEndInfo.layerCount = static_cast<uint32_t>(layers.size());
        xrFrameEndInfo.layers = layers.data();

        session->endFrame(xrFrameEndInfo, dispatchLoader);
    }
};
