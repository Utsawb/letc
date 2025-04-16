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
#include "XR.hh"

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

struct App
{
    letc::XrContext xrCtx;

    std::unique_ptr<letc::Instance> instance;
    vkfw::UniqueWindow window;
    vk::UniqueSurfaceKHR surface;

    std::unique_ptr<letc::Device> device;
    std::unique_ptr<letc::Allocator> allocator;
    std::unique_ptr<letc::Swapchain> swapchain;
    vk::Queue queue;

    vk::UniqueCommandPool commandPool;
    std::vector<vk::UniqueCommandBuffer> commandBuffers;
    uint32_t m_currentImageIndex = 0;
    vk::UniqueFence imageFence;
    std::vector<vk::UniqueFence> commandFences;

    GlobalUniforms globalUniforms;
    std::unique_ptr<letc::Buffer> globalUniformsBuffer;

    std::vector<Light> lights;
    std::unique_ptr<letc::Buffer> lightsBuffer;

    std::unique_ptr<letc::Camera> camera;
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

        letc::DeviceBuilder deviceBuilder;
        deviceBuilder.addExtensions(xrCtx.getVkDeviceExt());
        deviceBuilder.requestDevice(xrCtx.getVkPhysicalDevice(*instance));
        deviceBuilder.requestQueue("graphics", vk::QueueFlagBits::eGraphics);
        device = std::make_unique<letc::Device>(*instance, deviceBuilder);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(device->device);

        // allocator initialization
        allocator = std::make_unique<letc::Allocator>(*instance, *device);

        // swapchain + queue initialization
        swapchain = std::make_unique<letc::Swapchain>(*window, *surface, *device);
        queue = device->queues["graphics"].queue;

        xrCtx.createSession(*instance, device->physicalDevice, *device, device->queues["graphics"].queueFamilyIndex, 0);

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

        // Flat camera
        camera = std::make_unique<letc::Camera>(*allocator, glm::vec4{0.0f, 0.0f, 2.0f, 1.0f},
                                                glm::vec4{0.0f, 0.0f, 0.0f, 1.0f}, glm::vec4{0.0f, 1.0f, 0.0f, 1.0f},
                                                60.0f, (float)window->getWidth() / (float)window->getHeight());

        // XR cameras
        for (int i = 0; i < 2; ++i)
        {
            xrCameras.push_back(std::make_unique<letc::Camera>(
                *allocator, glm::vec4{0.0f, 0.0f, 2.0f, 1.0f}, glm::vec4{0.0f, 0.0f, 0.0f, 1.0f},
                glm::vec4{0.0f, 1.0f, 0.0f, 1.0f}, 60.0f,
                static_cast<float>(xrCtx.viewConfigViews.at(i).recommendedImageRectWidth) /       // Use index i here
                    static_cast<float>(xrCtx.viewConfigViews.at(i).recommendedImageRectHeight))); // Use index i here
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
        modelRenderer = std::make_unique<letc::ModelRenderer>(*allocator, *device, *swapchain,
                                                              readFile(resourcePath / "pbr.vert.spv"),
                                                              readFile(resourcePath / "pbr.frag.spv"));

        // *** Update ModelRenderer's descriptor sets ***
        modelRenderer->material->updateDescriptorBufferInfo(0, 0, globalUniformsBuffer->buffer, 0,
                                                            sizeof(GlobalUniforms));
        modelRenderer->material->updateDescriptorBufferInfo(0, 1, lightsBuffer->buffer, 0,
                                                            sizeof(Light) * lights.size());
        // Initial camera binding (will be updated per frame)
        modelRenderer->material->updateDescriptorBufferInfo(0, 2, *camera->buffer, 0, sizeof(letc::Camera::Uniform));
        // Binding for the SSBO
        modelRenderer->material->updateDescriptorBufferInfo(1, 0, modelUniformsBuffer->buffer, 0,
                                                            sizeof(letc::Model::UniformBuffer) * modelUniforms.size());
        modelRenderer->material->updateDescriptorSets(); // Update all sets once

        points = std::make_unique<letc::Points>(*allocator);
        pointsRenderer = std::make_unique<letc::PointsRenderer>(*allocator, *device, *swapchain,
                                                                readFile(resourcePath / "points.vert.spv"),
                                                                readFile(resourcePath / "points.frag.spv"));

        // depth buffer initialization (flat)
        depthImage = letc::laconic::depthImage(allocator->allocator, window->getWidth(), window->getHeight());
        depthImageView = letc::laconic::depthImageView(*device, *depthImage);

        xrDepthImage =
            letc::laconic::depthImage(allocator->allocator, xrCtx.viewConfigViews.at(0).recommendedImageRectWidth,
                                      xrCtx.viewConfigViews.at(0).recommendedImageRectHeight);
        xrDepthImageView = letc::laconic::depthImageView(*device, *xrDepthImage);

        std::tie(lastMouseX, lastMouseY) = window->getCursorPos();
        window->callbacks()->on_scroll = [this](vkfw::Window const &, double x, double y) {
            camera->zoom(static_cast<float>(y));
        };

        xrCtx.waitForSessionStart();
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
        xrCtx.pollEvents();

        if (window->getKey(vkfw::Key::eQ))
        {
            window->setShouldClose(true);
        }

        auto [mouseX, mouseY] = window->getCursorPos();
        if (window->getMouseButton(vkfw::MouseButton::eLeft))
        {
            float orbitSensitivity = 0.05f;
            float deltaX = static_cast<float>(mouseX - lastMouseX);
            float deltaY = static_cast<float>(mouseY - lastMouseY);

            camera->orbit(deltaX * orbitSensitivity, deltaY * orbitSensitivity);
        }
        lastMouseX = mouseX;
        lastMouseY = mouseY;
        camera->updateView();

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

        glm::mat4 leftHand, rightHand;
        xrCtx.getState(leftHand, rightHand);
        models.at(2).uniform.model = leftHand;
        models.at(3).uniform.model = rightHand;

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
        camera->cpy(); // Flat camera
        modelUniformsBuffer->cpy(modelUniforms.data(), sizeof(letc::Model::UniformBuffer) * models.size());

        // *** No need to update modelRenderer descriptor sets here unless ***
        // *** global/light/model SSBO buffer bindings change, which they don't per frame ***
        modelRenderer->material->updateDescriptorSets(); // Remove this
        points->cpy();
    }

    void xrFrame()
    {
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
            modelRenderer->material->bind(commandBuffer.get(), *modelRenderer->pipeline); // Bind all descriptor sets
            for (uint32_t i = 0; i < models.size(); ++i)                                  // Render all models
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

    void flatFrame()
    {
        auto [result, imageIndex] =
            device->device.acquireNextImageKHR(*swapchain, 5000000000, nullptr, imageFence.get());
        assertThrow(result == vk::Result::eSuccess, "failed to acquire next image: " + vk::to_string(result));
        m_currentImageIndex = imageIndex;

        // *** Update ModelRenderer camera binding for the flat view ***
        modelRenderer->material->updateDescriptorBufferInfo(0, 2, *camera->buffer, 0, sizeof(letc::Camera::Uniform));
        modelRenderer->material->updateDescriptorSet(0); // Only update set 0 (camera)

        pointsRenderer->material->updateDescriptorBufferInfo(0, 0, *camera->buffer, 0, sizeof(letc::Camera::Uniform));
        pointsRenderer->material->updateDescriptorSet(0);

        vk::UniqueCommandBuffer &commandBuffer = commandBuffers.at(2);
        commandBuffer->reset(vk::CommandBufferResetFlagBits::eReleaseResources);
        commandBuffer->begin(vk::CommandBufferBeginInfo{}.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        commandBuffer->setScissor(
            0, 1,
            &vk::Rect2D{}.setOffset({0, 0}).setExtent(
                {static_cast<uint32_t>(window->getWidth()), static_cast<uint32_t>(window->getHeight())}));
        commandBuffer->setViewport(0, 1,
                                   &vk::Viewport{}
                                        .setX(0.0f)
                                        .setY(0.0f)
                                        .setWidth(static_cast<float>(window->getWidth()))
                                        .setHeight(static_cast<float>(window->getHeight()))
                                        .setMinDepth(0.0f)
                                        .setMaxDepth(1.0f));

        // color attachment layout transition
        letc::laconic::transitionImageLayout(
            *commandBuffer, swapchain->images.at(m_currentImageIndex), vk::ImageAspectFlagBits::eColor,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, vk::AccessFlags{},
            vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eColorAttachmentOutput);
        // depth attachment layout transition
        letc::laconic::transitionImageLayout(
            *commandBuffer, *depthImage, vk::ImageAspectFlagBits::eDepth, vk::ImageLayout::eUndefined, // Use depthImage
            vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::AccessFlags{},
            vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eEarlyFragmentTests);

        vk::RenderingInfo renderingInfo{};
        renderingInfo.setRenderArea(vk::Rect2D{}.setOffset({0, 0}).setExtent(
            {static_cast<uint32_t>(window->getWidth()), static_cast<uint32_t>(window->getHeight())}));
        renderingInfo.setLayerCount(1);

        vk::RenderingAttachmentInfo colorAttachment{};
        colorAttachment.setImageView(swapchain->imageViews.at(m_currentImageIndex));
        colorAttachment.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);
        colorAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
        colorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
        colorAttachment.setClearValue(
            vk::ClearValue{}.setColor(vk::ClearColorValue{}.setFloat32({0.1176f, 0.1176f, 0.1804f, 1.0f})));

        vk::RenderingAttachmentInfo depthAttachment{};
        depthAttachment.setImageView(*depthImageView);
        depthAttachment.setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
        depthAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
        depthAttachment.setStoreOp(vk::AttachmentStoreOp::eDontCare);
        depthAttachment.setClearValue(vk::ClearDepthStencilValue{1.0f, 0});

        renderingInfo.setColorAttachmentCount(1);
        renderingInfo.setPColorAttachments(&colorAttachment);
        renderingInfo.setPDepthAttachment(&depthAttachment);

        commandBuffer->beginRendering(renderingInfo);

        // *** Render models using ModelRenderer and push constants ***
        modelRenderer->pipeline->bind(commandBuffer.get());
        modelRenderer->material->bind(commandBuffer.get(), *modelRenderer->pipeline);

        for (uint32_t i = 0; i < models.size(); ++i) // Render all models
        {
            // Push the model index
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
            *commandBuffer, swapchain->images.at(m_currentImageIndex), vk::ImageAspectFlagBits::eColor,
            vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eMemoryRead,
            vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe);

        commandBuffer->end();

        vk::Fence &commandFence = commandFences.at(2).get();
        assertThrow(device->device.resetFences(1, &commandFence) == vk::Result::eSuccess,
                    "failed to reset command fence");

        queue.submit(vk::SubmitInfo{}.setCommandBufferCount(1).setPCommandBuffers(&commandBuffer.get()), commandFence);

        assertThrow(device->device.waitForFences(1, &commandFence, VK_TRUE, 5000000000) == vk::Result::eSuccess,
                    "failed to wait for command fence");

        assertThrow(device->device.waitForFences(1, &imageFence.get(), VK_TRUE, 5000000000) == vk::Result::eSuccess,
                    "failed to wait for image fence");

        assertThrow(queue.presentKHR(vk::PresentInfoKHR{}
                                         .setWaitSemaphoreCount(0)
                                         .setPWaitSemaphores(nullptr)
                                         .setSwapchainCount(1)
                                         .setPSwapchains(&swapchain->swapchain)
                                         .setPImageIndices(&m_currentImageIndex)
                                         .setPNext(nullptr)) == vk::Result::eSuccess,
                    "failed to present image");

        assertThrow(device->device.resetFences(1, &imageFence.get()) == vk::Result::eSuccess,
                    "failed to reset image fence");
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
        app.flatFrame();
    }
    return 0;
}
