#pragma once

#include <array>
#include <cstring>
#include <glm/ext.hpp>
#include <iostream>
#include <vector>

#include <glm/glm.hpp>

#include <vulkan/vulkan.hpp>

#define XR_USE_GRAPHICS_API_VULKAN
#include <openxr/openxr_platform.h>

#include <openxr.hpp>

namespace letc
{
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

    /*
     *  For now this will be a very, very, hard coded interface, just enough
     *  to get me through my class project. I would love to support it more,
     *  but my interests lie more on the Vulkan side. Maybe once I am content
     *  with the Vulkan interface I can come back and give this some love.
     */
    struct XrContext
    {
        xr::DispatchLoaderDynamic disp{};
        xr::UniqueDynamicInstance instance{};
        xr::SystemId systemId{};

        xr::UniqueDynamicSession session{};

        xr::Time predictedDisplayTime{};

        xr::UniqueDynamicSpace leftHandSpace{};
        xr::UniqueDynamicSpace rightHandSpace{};

        xr::Path leftHandPath{};
        xr::Path rightHandPath{};

        xr::UniqueDynamicActionSet actionSet{};
        xr::UniqueDynamicAction triggerAction{};
        xr::UniqueDynamicAction joystickAction{};
        xr::UniqueDynamicAction faceButtonAction{};
        xr::UniqueDynamicAction handPoseAction{};

        std::vector<xr::ViewConfigurationView> viewConfigViews{};
        xr::UniqueDynamicSpace referenceSpace{};
        long long swapchainFormat = (long long)vk::Format::eR8G8B8A8Srgb;
        std::array<xr::UniqueDynamicSwapchain, 2> swapchains;
        std::array<std::vector<xr::SwapchainImageVulkanKHR>, 2> swapchainImages;
        std::array<std::vector<vk::UniqueImageView>, 2> swapchainImageViews;

        xr::EventDataBuffer eventDataBuffer{};
        xr::SessionState sessionState = xr::SessionState::Unknown;

        static XrBool32 debugCallback(XrDebugUtilsMessageSeverityFlagsEXT messageSeverity,
                                      XrDebugUtilsMessageTypeFlagsEXT messageTypes,
                                      const XrDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
        {
            std::cerr << "XrDebug: " << pCallbackData->messageId << ": " << pCallbackData->message << std::endl;
            return XR_FALSE;
        }

        XrContext()
        {
            disp = xr::DispatchLoaderDynamic{};

            xr::DebugUtilsMessengerCreateInfoEXT dumci{};
            dumci.messageSeverities = xr::DebugUtilsMessageSeverityFlagBitsEXT::AllBits;
            dumci.messageTypes = xr::DebugUtilsMessageTypeFlagBitsEXT::AllBits;
            dumci.userCallback = debugCallback;
            dumci.userData = nullptr;

            xr::ApplicationInfo appInfo{};
            std::memcpy(appInfo.applicationName, "Dev", 4);
            appInfo.applicationVersion = 1;
            std::memcpy(appInfo.engineName, "letc", 5);
            appInfo.engineVersion = 1;
            appInfo.apiVersion = xr::Version{XR_CURRENT_API_VERSION};

            std::vector<const char *> extensions = {"XR_KHR_vulkan_enable2"};
            xr::InstanceCreateInfo instanceInfo{};
            instanceInfo.createFlags = xr::InstanceCreateFlagBits::None;
            instanceInfo.applicationInfo = appInfo;
            instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            instanceInfo.enabledExtensionNames = extensions.data();
            instanceInfo.enabledApiLayerCount = 0;
            instanceInfo.enabledApiLayerNames = nullptr;
            instanceInfo.next = &dumci;
            instance = xr::createInstanceUnique(instanceInfo, disp);

            disp = xr::DispatchLoaderDynamic{*instance};

            xr::SystemGetInfo systemInfo{};
            systemInfo.formFactor = xr::FormFactor::HeadMountedDisplay;
            systemInfo.next = nullptr;
            systemId = instance->getSystem(systemInfo);
        }

        vk::PhysicalDevice getVkPhysicalDevice(const vk::Instance &vkInstance)
        {
            xr::VulkanGraphicsDeviceGetInfoKHR vgdgi{};
            vgdgi.systemId = systemId;
            vgdgi.vulkanInstance = vkInstance;
            vgdgi.next = nullptr;
            return instance->getVulkanGraphicsDevice2KHR(vgdgi, disp);
        }

        std::vector<std::string> getVkDeviceExt()
        {
            return split(instance->getVulkanDeviceExtensionsKHR(systemId, disp));
        }

        void createSession(const vk::Instance &vkInstance, const vk::PhysicalDevice &vkPhysicalDevice,
                           const vk::Device &vkDevice, const uint32_t &vkQueueFamilyIndex, const uint32_t &vkQueueIndex)
        {
            xr::GraphicsBindingVulkanKHR gbv{};
            gbv.next = nullptr;
            gbv.instance = vkInstance;
            gbv.physicalDevice = vkPhysicalDevice;
            gbv.device = vkDevice;
            gbv.queueFamilyIndex = vkQueueFamilyIndex;
            gbv.queueIndex = vkQueueIndex;

            xr::SessionCreateInfo sci{};
            sci.next = &gbv;
            sci.createFlags = xr::SessionCreateFlagBits::None;
            sci.systemId = systemId;
            session = instance->createSessionUnique(sci, disp);

            leftHandPath = instance->stringToPath("/user/hand/left", disp);
            rightHandPath = instance->stringToPath("/user/hand/right", disp);

            // Create an action set.
            xr::ActionSetCreateInfo actionSetInfo{};
            std::strcpy(actionSetInfo.actionSetName, "gameplay");
            std::strcpy(actionSetInfo.localizedActionSetName, "Gameplay");
            actionSetInfo.priority = 0;
            actionSet = instance->createActionSetUnique(actionSetInfo, disp);

            // Create subaction paths for left and right hands.
            xr::Path leftHandPath = instance->stringToPath("/user/hand/left", disp);
            xr::Path rightHandPath = instance->stringToPath("/user/hand/right", disp);
            std::vector<xr::Path> subactionPaths = {leftHandPath, rightHandPath};

            // Create the trigger action (boolean input).
            xr::ActionCreateInfo triggerActionInfo{};
            std::strcpy(triggerActionInfo.actionName, "trigger_press");
            std::strcpy(triggerActionInfo.localizedActionName, "Trigger Press");
            triggerActionInfo.actionType = xr::ActionType::BooleanInput;
            triggerActionInfo.countSubactionPaths = static_cast<uint32_t>(subactionPaths.size());
            triggerActionInfo.subactionPaths = subactionPaths.data();
            triggerAction = actionSet->createActionUnique(triggerActionInfo, disp);

            // Create the joystick action (vector2f input).
            xr::ActionCreateInfo joystickActionInfo{};
            std::strcpy(joystickActionInfo.actionName, "joystick_move");
            std::strcpy(joystickActionInfo.localizedActionName, "Joystick Move");
            joystickActionInfo.actionType = xr::ActionType::Vector2FInput;
            joystickActionInfo.countSubactionPaths = static_cast<uint32_t>(subactionPaths.size());
            joystickActionInfo.subactionPaths = subactionPaths.data();
            joystickAction = actionSet->createActionUnique(joystickActionInfo, disp);

            // Create the face button action (boolean input).
            xr::ActionCreateInfo faceButtonActionInfo{};
            std::strcpy(faceButtonActionInfo.actionName, "face_button_press");
            std::strcpy(faceButtonActionInfo.localizedActionName, "Face Button Press");
            faceButtonActionInfo.actionType = xr::ActionType::BooleanInput;
            faceButtonActionInfo.countSubactionPaths = static_cast<uint32_t>(subactionPaths.size());
            faceButtonActionInfo.subactionPaths = subactionPaths.data();
            faceButtonAction = actionSet->createActionUnique(faceButtonActionInfo, disp);

            // Create a pose action to capture hand position data.
            xr::ActionCreateInfo handPoseActionInfo{};
            std::strcpy(handPoseActionInfo.actionName, "hand_pose");
            std::strcpy(handPoseActionInfo.localizedActionName, "Hand Pose");
            handPoseActionInfo.actionType = xr::ActionType::PoseInput;
            handPoseActionInfo.countSubactionPaths = static_cast<uint32_t>(subactionPaths.size());
            handPoseActionInfo.subactionPaths = subactionPaths.data();
            handPoseAction = actionSet->createActionUnique(handPoseActionInfo, disp);

            // Suggest bindings for a common controller profile (example: Oculus Touch).
            xr::Path oculusTouchInteractionProfile =
                instance->stringToPath("/interaction_profiles/oculus/touch_controller", disp);
            std::vector<xr::ActionSuggestedBinding> bindings;
            bindings.push_back({*triggerAction, instance->stringToPath("/user/hand/left/input/trigger", disp)});
            bindings.push_back({*triggerAction, instance->stringToPath("/user/hand/right/input/trigger", disp)});
            bindings.push_back({*joystickAction, instance->stringToPath("/user/hand/left/input/thumbstick", disp)});
            bindings.push_back({*joystickAction, instance->stringToPath("/user/hand/right/input/thumbstick", disp)});
            bindings.push_back({*faceButtonAction, instance->stringToPath("/user/hand/left/input/x", disp)});
            bindings.push_back({*faceButtonAction, instance->stringToPath("/user/hand/right/input/a", disp)});
            bindings.push_back({*handPoseAction, instance->stringToPath("/user/hand/left/input/aim/pose", disp)});
            bindings.push_back({*handPoseAction, instance->stringToPath("/user/hand/right/input/aim/pose", disp)});

            xr::InteractionProfileSuggestedBinding suggestedBinding{};
            suggestedBinding.interactionProfile = oculusTouchInteractionProfile;
            suggestedBinding.suggestedBindings = bindings.data();
            suggestedBinding.countSuggestedBindings = static_cast<uint32_t>(bindings.size());
            instance->suggestInteractionProfileBindings(suggestedBinding, disp);

            {
                xr::ActionSpaceCreateInfo actionSpaceInfo{};
                actionSpaceInfo.action = *handPoseAction;
                // Use an identity pose for the action space (adjust if you need an offset).
                actionSpaceInfo.poseInActionSpace.orientation = xr::Quaternionf{0, 0, 0, 1};
                actionSpaceInfo.poseInActionSpace.position = xr::Vector3f{0, 0, 0};

                // Create left hand action space.
                actionSpaceInfo.subactionPath = leftHandPath;
                leftHandSpace = session->createActionSpaceUnique(actionSpaceInfo, disp);

                // Create right hand action space.
                actionSpaceInfo.subactionPath = rightHandPath;
                rightHandSpace = session->createActionSpaceUnique(actionSpaceInfo, disp);
            }

            // Attach the action set to the session.
            xr::SessionActionSetsAttachInfo attachInfo{};
            std::vector<xr::ActionSet> actionSets = {*actionSet};
            attachInfo.countActionSets = static_cast<uint32_t>(actionSets.size());
            attachInfo.actionSets = actionSets.data();
            session->attachSessionActionSets(attachInfo);

            auto viewConfig = instance->enumerateViewConfigurationsToVector(systemId, disp);
            auto stereo = std::ranges::find_if(
                viewConfig, [](const auto &vc) { return vc == xr::ViewConfigurationType::PrimaryStereo; });
            assert(stereo != viewConfig.end());
            viewConfigViews = instance->enumerateViewConfigurationViewsToVector(systemId, *stereo, disp);

            xr::ReferenceSpaceCreateInfo rsci{};
            rsci.next = nullptr;
            rsci.referenceSpaceType = xr::ReferenceSpaceType::Stage;
            rsci.poseInReferenceSpace = xr::Posef{{0, 0, 0, 1}, {0, 0, 0}};
            referenceSpace = session->createReferenceSpaceUnique(rsci, disp);

            // fake call cause the layers get madd
            session->enumerateSwapchainFormatsToVector(disp);

            xr::SwapchainCreateInfo swapci{};
            swapci.next = nullptr;
            swapci.createFlags = xr::SwapchainCreateFlagBits::None;
            swapci.usageFlags = xr::SwapchainUsageFlagBits::ColorAttachment | xr::SwapchainUsageFlagBits::TransferSrc |
                                xr::SwapchainUsageFlagBits::TransferDst;
            swapci.format = swapchainFormat;
            swapci.sampleCount = 1;
            swapci.width = viewConfigViews.at(0).recommendedImageRectWidth;
            swapci.height = viewConfigViews.at(0).recommendedImageRectHeight;
            swapci.faceCount = 1;
            swapci.arraySize = 1;
            swapci.mipCount = 1;

            swapchains[0] = session->createSwapchainUnique(swapci, disp);
            swapchains[1] = session->createSwapchainUnique(swapci, disp);
            swapchainImages[0] = swapchains[0]->enumerateSwapchainImagesToVector<xr::SwapchainImageVulkanKHR>(disp);
            swapchainImages[1] = swapchains[1]->enumerateSwapchainImagesToVector<xr::SwapchainImageVulkanKHR>(disp);

            for (std::size_t i = 0; i < swapchainImages[0].size(); ++i)
            {
                vk::ImageSubresourceRange isr{};
                isr.setAspectMask(vk::ImageAspectFlagBits::eColor);
                isr.setBaseMipLevel(0);
                isr.setLevelCount(1);
                isr.setBaseArrayLayer(0);
                isr.setLayerCount(1);

                vk::ImageViewCreateInfo ivci{};
                ivci.setImage(swapchainImages[0][i].image);
                ivci.setFormat((vk::Format)swapchainFormat);
                ivci.setViewType(vk::ImageViewType::e2D);
                ivci.setComponents(vk::ComponentMapping{});
                ivci.setSubresourceRange(isr);

                swapchainImageViews[0][i] = vkDevice.createImageViewUnique(ivci);
                ivci.setImage(swapchainImages[1][i].image);
                swapchainImageViews[1][i] = vkDevice.createImageViewUnique(ivci);
            }
        }

        void pollEvents()
        {
            instance->pollEvent(eventDataBuffer, disp);
        }

        void waitForSessionStart()
        {
            while (true)
            {
                instance->pollEvent(eventDataBuffer, disp);

                if (eventDataBuffer.type == xr::StructureType::EventDataSessionStateChanged)
                {
                    auto &event = *(xr::EventDataSessionStateChanged *)(&eventDataBuffer);
                    if (event.session == *session && event.state == xr::SessionState::Ready)
                    {
                        break;
                    }
                }
            }

            xr::SessionBeginInfo sbi{};
            sbi.primaryViewConfigurationType = xr::ViewConfigurationType::PrimaryStereo;
            session->beginSession(sbi, disp);
        }

        void getState(glm::mat4 &leftHand, glm::mat4 &rightHand)
        {
            // --- Query OpenXR action states ---
            XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
            XrActiveActionSet activeActionSet{actionSet->get(), XR_NULL_PATH};
            syncInfo.countActiveActionSets = 1;
            syncInfo.activeActionSets = &activeActionSet;
            xrSyncActions(*session, &syncInfo);

            xr::Time predictedDisplayTime = this->predictedDisplayTime;

            xr::ActionStateGetInfo triggerActionStateInfo{};
            triggerActionStateInfo.action = *triggerAction;
            triggerActionStateInfo.subactionPath = leftHandPath;
            xr::ActionStateBoolean leftTriggerState = session->getActionStateBoolean(triggerActionStateInfo, disp);
            triggerActionStateInfo.subactionPath = rightHandPath;
            xr::ActionStateBoolean rightTriggerState = session->getActionStateBoolean(triggerActionStateInfo, disp);

            xr::SpaceLocation leftLocation;
            if (predictedDisplayTime != 0.0f)
            {
                leftLocation = referenceSpace->locateSpace(*leftHandSpace, predictedDisplayTime, disp);
                glm::mat4 leftTranslation = glm::translate(glm::mat4(1.0f), glm::vec3(leftLocation.pose.position.x,
                                                                                      leftLocation.pose.position.y,
                                                                                      leftLocation.pose.position.z));
                glm::mat4 leftRotation =
                    glm::mat4_cast(-glm::quat(leftLocation.pose.orientation.w, leftLocation.pose.orientation.x,
                                              leftLocation.pose.orientation.y, leftLocation.pose.orientation.z));
                leftHand = glm::inverse(leftTranslation * leftRotation);
            }
            if (leftTriggerState.isActive && leftTriggerState.currentState)
            {
                if (leftLocation.locationFlags & xr::SpaceLocationFlagBits::PositionValid)
                {
                    // trigger pressed
                }
            }
            xr::SpaceLocation rightLocation;
            if (predictedDisplayTime != 0.0f)
            {
                rightLocation = referenceSpace->locateSpace(*rightHandSpace, predictedDisplayTime, disp);
                glm::mat4 rightTranslation = glm::translate(glm::mat4(1.0f), glm::vec3(rightLocation.pose.position.x,
                                                                                       rightLocation.pose.position.y,
                                                                                       rightLocation.pose.position.z));
                glm::mat4 rightRotation =
                    glm::mat4_cast(-glm::quat(rightLocation.pose.orientation.w, rightLocation.pose.orientation.x,
                                              rightLocation.pose.orientation.y, rightLocation.pose.orientation.z));
                rightHand = glm::inverse(rightTranslation * rightRotation);
            }
            if (rightTriggerState.isActive && rightTriggerState.currentState)
            {
                if (rightLocation.locationFlags & xr::SpaceLocationFlagBits::PositionValid)
                {
                    // trigger pressed
                }
            }

            xr::ActionStateGetInfo joystickActionStateInfo{};
            joystickActionStateInfo.action = *joystickAction;
            joystickActionStateInfo.subactionPath = leftHandPath;
            xr::ActionStateVector2f leftJoystickState = session->getActionStateVector2f(joystickActionStateInfo, disp);
            joystickActionStateInfo.subactionPath = rightHandPath;
            xr::ActionStateVector2f rightJoystickState = session->getActionStateVector2f(joystickActionStateInfo, disp);
            if (leftJoystickState.isActive &&
                (leftJoystickState.currentState.x != 0.0f || leftJoystickState.currentState.y != 0.0f))
            {
                glm::vec2 leftJoystickPos{leftJoystickState.currentState.x, leftJoystickState.currentState.y};
                // joystick moved
            }
            if (rightJoystickState.isActive &&
                (rightJoystickState.currentState.x != 0.0f || rightJoystickState.currentState.y != 0.0f))
            {
                glm::vec2 rightJoystickPos{rightJoystickState.currentState.x, rightJoystickState.currentState.y};
                // joystick moved
            }

            xr::ActionStateGetInfo faceButtonActionStateInfo{};
            faceButtonActionStateInfo.action = *faceButtonAction;
            faceButtonActionStateInfo.subactionPath = leftHandPath;
            xr::ActionStateBoolean leftFaceButtonState =
                session->getActionStateBoolean(faceButtonActionStateInfo, disp);
            faceButtonActionStateInfo.subactionPath = rightHandPath;
            xr::ActionStateBoolean rightFaceButtonState =
                session->getActionStateBoolean(faceButtonActionStateInfo, disp);
            if (leftFaceButtonState.isActive && leftFaceButtonState.currentState)
            {
                // left facebutton pressed
            }
            if (rightFaceButtonState.isActive && rightFaceButtonState.currentState)
            {
                // right facebutton pressed
            }
        }

        bool prepareDraw()
        {
            // Wait for the next XR frame
            xr::FrameState frameState = session->waitFrame(nullptr, disp);
            predictedDisplayTime = frameState.predictedDisplayTime;

            if (!frameState.shouldRender)
            {
                std::clog << std::format("Skipping frame\n");
                session->beginFrame(nullptr, disp);

                xr::FrameEndInfo xrFrameEndInfo{};
                xrFrameEndInfo.displayTime = frameState.predictedDisplayTime;
                xrFrameEndInfo.environmentBlendMode = xr::EnvironmentBlendMode::Opaque;
                xrFrameEndInfo.layerCount = 0;
                xrFrameEndInfo.layers = nullptr;
                session->endFrame(xrFrameEndInfo, disp);
                return false;
            }

            session->beginFrame(nullptr, disp);

            const uint32_t viewCount = 2;
            xr::ViewLocateInfo viewLocateInfo{};
            viewLocateInfo.viewConfigurationType = xr::ViewConfigurationType::PrimaryStereo;
            viewLocateInfo.displayTime = frameState.predictedDisplayTime;
            viewLocateInfo.space = *referenceSpace;
            xr::ViewState viewState{};
            std::vector<xr::View> views =
                session->locateViewsToVector(viewLocateInfo, reinterpret_cast<XrViewState *>(&viewState), disp);

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

                    glm::mat4 clipConversion = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                                         1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

                    proj = clipConversion * proj;

                    glm::vec3 pos(views[i].pose.position.x, views[i].pose.position.y, views[i].pose.position.z);
                    glm::quat orient(views[i].pose.orientation.w, views[i].pose.orientation.x,
                                     views[i].pose.orientation.y, views[i].pose.orientation.z);

                    glm::mat4 eyeMatrix = glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(orient);
                    glm::mat4 viewMatrix = glm::inverse(eyeMatrix);

                    xrCameras.at(i)->uniform.view = viewMatrix;
                    xrCameras.at(i)->uniform.proj = proj;
                    xrCameras.at(i)->cpy();

                    modelUniforms.at(i + 4).model = eyeMatrix;
                }

                modelUniformsBuffer->cpy(modelUniforms.data(), sizeof(letc::Model::UniformBuffer) * models.size());
            }

            return true;
        }
    };

}; // namespace letc
