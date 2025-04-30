#include <letc/letc.hh>

#include "model.hh"

std::filesystem::path resourcePath = "../resources";

struct FrameData
{
    float time;
    uint32_t frame; // Use uint32_t for frame count
};

struct LightData
{
    glm::vec4 position{0.0f};
    glm::vec4 color{1.0f};
};

auto main(int argc, char *argv[]) -> int
{
    if (argc > 1)
    {
        resourcePath = argv[1];
    }

    // --- Initialization (mostly unchanged) ---
    auto window = letc::WindowBuilder{}.setWidth(1280).setHeight(720).setTitle("Dynamic Rendering Example").build();
    auto instance = letc::InstanceBuilder{}
                        .addExtension(vk::KHRSurfaceExtensionName)
                        .addExtension(vk::KHRGetPhysicalDeviceProperties2ExtensionName)
                        .build();
    auto device = letc::DeviceBuilder{}
                      .addExtension(vk::KHRSwapchainExtensionName)
                      .addExtension(vk::KHRDynamicRenderingExtensionName)
                      .addExtension(vk::KHRSeparateDepthStencilLayoutsExtensionName)
                      .addExtension(vk::KHRCreateRenderpass2ExtensionName)   // Dependency for separate_depth_stencil
                      .addExtension(vk::KHRDepthStencilResolveExtensionName) // Dependency for dynamic_rendering
                      .addExtension(vk::KHRMultiviewExtensionName)
                      .addExtension(vk::KHRMaintenance2ExtensionName)
                      .requestQueues("graphics", vk::QueueFlagBits::eGraphics)
                      // Removed compute queue request as it's not used in this basic setup
                      // .requestQueues("compute", vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer, 2)
                      .setDeviceFeatures([](letc::DeviceBuilder::FeatureChain &f) {
                          // Ensure necessary features are enabled
                          f.get<vk::PhysicalDeviceFeatures2>().features.setShaderSampledImageArrayDynamicIndexing(
                              VK_TRUE); // Example: If needed by shaders
                          f.get<vk::PhysicalDeviceVulkan13Features>().setDynamicRendering(true);
                          // Add other features if required by your shaders or techniques
                      })
                      .build(instance);
    auto graphicsQueueInfo = device->getQueue("graphics"); // Renamed for clarity
    auto graphicsQueue = graphicsQueueInfo.get()[0];       // Get the first graphics queue handle

    auto swapchain = letc::SwapchainBuilder{}.build(window, instance, device);
    auto format = swapchain->getFormat().format;
    auto extent = swapchain->getExtent();

    auto allocator = std::make_shared<letc::Allocator>(instance, device);
    auto descPool = std::make_shared<letc::DescriptorSetPool>(device);

    auto shaderManager = std::move(letc::ShaderManager{device}
                                       .add(resourcePath / "shaders" / "pbr" / "vert.spirv", "main")
                                       .add(resourcePath / "shaders" / "pbr" / "frag.spirv", "main"));

    auto pbrVert = shaderManager.getShader(resourcePath / "shaders" / "pbr" / "vert.spirv", "main");
    auto pbrFrag = shaderManager.getShader(resourcePath / "shaders" / "pbr" / "frag.spirv", "main");

    auto pbrPipeline = letc::GraphicsPipelineBuilder{}
                           .addShader(pbrVert)
                           .addShader(pbrFrag)
                           // Vertex buffer bindings match model.hh
                           .addVertexBinding(0, sizeof(glm::vec3), vk::VertexInputRate::eVertex) // Position
                           .addVertexAttribute(0, 0, vk::Format::eR32G32B32Sfloat, 0)
                           .addVertexBinding(1, sizeof(glm::vec3), vk::VertexInputRate::eVertex) // Normal
                           .addVertexAttribute(1, 1, vk::Format::eR32G32B32Sfloat, 0)
                           // .addVertexBinding(2, sizeof(glm::vec2), vk::VertexInputRate::eVertex) // TexCoord (if
                           // needed) .addVertexAttribute(2, 2, vk::Format::eR32G32Sfloat, 0)
                           .setRendering([format](vk::PipelineRenderingCreateInfo &prci) {
                               prci.setColorAttachmentFormats(format);
                               prci.setDepthAttachmentFormat(vk::Format::eD32Sfloat); // Set depth format
                           })
                           .setDepthStencil([](vk::PipelineDepthStencilStateCreateInfo &dsci) { // Enable depth testing
                               dsci.setDepthTestEnable(VK_TRUE);
                               dsci.setDepthWriteEnable(VK_TRUE);
                               dsci.setDepthCompareOp(vk::CompareOp::eLess);
                           })
                           .setRasterization([](vk::PipelineRasterizationStateCreateInfo &
                                                    rsci) { // Set rasterization state (optional, defaults are often ok)
                               rsci.setCullMode(vk::CullModeFlagBits::eBack);
                               rsci.setFrontFace(vk::FrontFace::eCounterClockwise); // Match glTF winding order
                           })
                           .build(device);

    auto commandPool =
        device->getLogical().createCommandPoolUnique(vk::CommandPoolCreateInfo{}
                                                         .setQueueFamilyIndex(graphicsQueueInfo.getFamily())
                                                         .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer));

    // --- Per-Frame Resources ---
    const uint32_t MAX_FRAMES_IN_FLIGHT = 2; // Or 3, depending on preference/needs
    std::vector<vk::UniqueCommandBuffer> commandBuffers;
    std::vector<vk::UniqueSemaphore> imageAvailableSemaphores;
    std::vector<vk::UniqueSemaphore> renderFinishedSemaphores;
    std::vector<vk::UniqueFence> inFlightFences;
    std::vector<std::shared_ptr<letc::Image>> depthImages;
    std::vector<std::shared_ptr<letc::ImageView>> depthImageViews;

    commandBuffers =
        device->getLogical().allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{}
                                                              .setCommandPool(*commandPool)
                                                              .setLevel(vk::CommandBufferLevel::ePrimary)
                                                              .setCommandBufferCount(MAX_FRAMES_IN_FLIGHT));

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        imageAvailableSemaphores.push_back(device->getLogical().createSemaphoreUnique({}));
        renderFinishedSemaphores.push_back(device->getLogical().createSemaphoreUnique({}));
        inFlightFences.push_back(
            device->getLogical().createFenceUnique({vk::FenceCreateFlagBits::eSignaled})); // Start signaled

        depthImages.push_back(letc::laconic::depthImage(allocator, extent.width, extent.height));
        depthImageViews.push_back(letc::laconic::depthImageView(device, depthImages[i]));
    }

    // --- Uniform Buffers and Descriptor Sets ---
    auto frameBuffer =
        letc::ObjectBuffer<FrameData>(allocator, vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
    frameBuffer->frame = 0; // Start frame count at 0
    frameBuffer->time = 0.0f;
    // No initial sync needed, will sync in loop

    std::println("Width: {}, Height: {}, Aspect: {}", extent.width, extent.height,
                 (float)extent.width / (float)extent.height);
    auto camera = letc::FirstPersonCamera(allocator, (float)extent.width / (float)extent.height, glm::radians(60.0f),
                                          0.1f, 1000.0f, {0.0f, 1.0f, 5.0f}); // Start pos
    // No initial sync needed, will sync in loop

    // Lights buffer (ensure it's created if needed by shader)
    auto lights =
        letc::VectorBuffer<LightData>(allocator, 4, vk::BufferUsageFlagBits::eStorageBuffer, // Size 4 for example
                                      VMA_MEMORY_USAGE_CPU_TO_GPU);
    // ... (populate light data as before) ...
    lights->at(0) = {{-5.0f, 5.0f, -5.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 10.0f}}; // Example light
    lights->at(1) = {{5.0f, 5.0f, -5.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 10.0f}};
    lights->at(2) = {{-5.0f, 5.0f, 5.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 10.0f}};
    lights->at(3) = {{5.0f, 5.0f, 5.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 10.0f}};
    // No initial sync needed

    // Build the descriptor set for frame data (Set 0)
    // Ensure the layout matches the shaders (UFrameData, UCamera, LightsBuffer etc.)
    auto frameSetLayout =
        std::make_shared<letc::DescriptorSetLayout>(pbrPipeline->getSetLayouts().at(0)); // Get layout for set 0
    std::vector<std::shared_ptr<letc::DescriptorSet>> frameSets;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        auto fs = frameSetLayout->build(descPool);
        fs->attachBuffer("UFrameData", frameBuffer.get(), frameBuffer.containedSize());
        fs->attachBuffer("UCamera", camera.getBuffer(), camera.containedSize());
        // Attach other global buffers like lights if they are in set 0
        fs->attachBuffer("BLights", lights.get(),
                         lights.containedSize()); // Check shader for correct name and set/binding
        frameSets.push_back(fs);
    }

    // Load Models (Set 1 per model)
    auto modelLayout =
        std::make_shared<letc::DescriptorSetLayout>(pbrPipeline->getSetLayouts().at(1)); // Get layout for set 1
    auto models = loadModels(resourcePath / "models" / "sponza" / "Sponza.gltf", modelLayout, descPool, allocator);
    // auto models = loadModels(resourcePath / "models" / "Box.glb", modelLayout, descPool, allocator);
    if (!models || models->empty())
    {
        std::cerr << "Failed to load models or no models found." << std::endl;
        return 1; // Exit if models didn't load
    }
    std::println("Loaded {} models.", models->size()); // Print number of models loaded

    // --- Main Loop ---
    uint32_t currentFrame = 0;
    auto startTime = std::chrono::high_resolution_clock::now();

    double lastMouseX, lastMouseY = 0.0f;
    std::tie(lastMouseX, lastMouseY) = window->get().getCursorPos();

    // auto cube = loadModels(resourcePath / "models" / "BoxTextured.glb", modelLayout, descPool, allocator);

    while (window->get().shouldClose() == false)
    {
        vkfw::pollEvents();

        auto [mx, my] = window->get().getCursorPos();

        // --- Panning (Rotation) ---
        if (window->get().getMouseButton(vkfw::MouseButton::eLeft))
        {
            // Calculate delta since last frame
            float deltaX = static_cast<float>(mx - lastMouseX);
            // Invert Y delta: window coords often increase downwards, camera pitch increases upwards
            float deltaY = static_cast<float>(lastMouseY - my);
            camera.pan(deltaX, deltaY);
        }

        // Update last mouse position *after* calculating delta
        lastMouseX = mx;
        lastMouseY = my;

        // --- Movement ---
        glm::vec3 movementInput{0.0f}; // Use the dedicated input vector

        if (window->get().getKey(vkfw::Key::eW))
        {
            movementInput.x += 1.0f; // Forward -> +X input
        }
        if (window->get().getKey(vkfw::Key::eS))
        {
            movementInput.x -= 1.0f; // Backward -> -X input
        }
        if (window->get().getKey(vkfw::Key::eA))
        {
            movementInput.y -= 1.0f; // Left -> -Y input
        }
        if (window->get().getKey(vkfw::Key::eD))
        {
            movementInput.y += 1.0f; // Right -> +Y input
        }
        if (window->get().getKey(vkfw::Key::eSpace)) // Example key
        {
            movementInput.z += 1.0f; // Up -> +Z input
        }
        if (window->get().getKey(vkfw::Key::eLeftShift)) // Example key
        {
            movementInput.z -= 1.0f; // Down -> -Z input
        }
        if (glm::length(movementInput) > 0.0f) // Check length before normalizing
        {
            // Optional: Normalize if you want consistent speed in all directions
            // movementInput = glm::normalize(movementInput);

            // Call move with the input vector and deltaTime
            camera.move(movementInput);
        }
        camera.sync();

        // --- Wait for previous frame ---
        // Waits for the fence associated with the *current* frame index to be signaled,
        // ensuring that the command buffer for this frame index is finished executing.
        vk::Result waitResult =
            device->getLogical().waitForFences(1, &(*inFlightFences[currentFrame]), VK_TRUE, UINT64_MAX);
        if (waitResult != vk::Result::eSuccess)
        {
            std::cerr << "Failed to wait for fence!" << std::endl;
            // Handle error appropriately, maybe break or throw
        }

        // --- Acquire Swapchain Image ---
        uint32_t imageIndex;
        vk::Result acquireResult = device->getLogical().acquireNextImageKHR(
            swapchain->get(), UINT64_MAX, *imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (acquireResult == vk::Result::eErrorOutOfDateKHR)
        {
            // Handle swapchain recreation (not implemented here for brevity)
            // Usually involves rebuilding swapchain, framebuffers, potentially pipelines
            std::cerr << "Swapchain out of date/suboptimal, needs recreation." << std::endl;
            // For now, just continue or break
            continue;
        }
        else if (acquireResult != vk::Result::eSuccess && acquireResult != vk::Result::eSuboptimalKHR)
        {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        // --- Reset Fence (only after successful acquire and wait) ---
        // Reset the fence *before* submitting the new command buffer that will signal it.
        (void)device->getLogical().resetFences(1, &(*inFlightFences[currentFrame]));

        // --- Update Uniforms ---
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        frameBuffer->time = time;
        frameBuffer->frame = (frameBuffer->frame + 1); // Increment frame counter
        frameBuffer.sync();

        camera.sync();
        lights.sync();

        const vk::CommandBuffer &cmd = *commandBuffers[currentFrame];
        cmd.reset();
        cmd.begin(vk::CommandBufferBeginInfo{});

        // 1. Transition Depth Image Layout
        letc::laconic::transitionImageLayout(
            cmd, depthImages[currentFrame], vk::ImageAspectFlagBits::eDepth, vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthAttachmentOptimal, {}, vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
            vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests);

        // 2. Transition Swapchain Image Layout
        letc::laconic::transitionImageLayout(
            cmd, swapchain->getImages()[imageIndex], vk::ImageAspectFlagBits::eColor, vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal, {}, vk::AccessFlagBits::eColorAttachmentWrite,
            vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput);

        // 3. Begin Dynamic Rendering
        vk::RenderingAttachmentInfo colorAttachment{};
        colorAttachment.setImageView(swapchain->getImageViews()[imageIndex]);
        colorAttachment.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);
        colorAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
        colorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
        colorAttachment.clearValue.color = vk::ClearColorValue{std::array<float, 4>{0.1f, 0.1f, 0.1f, 1.0f}};

        vk::RenderingAttachmentInfo depthAttachment{};
        depthAttachment.setImageView(depthImageViews[currentFrame]->get());
        depthAttachment.setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal);
        depthAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
        depthAttachment.setStoreOp(vk::AttachmentStoreOp::eStore); // Store if needed later, e.g., post-processing
        depthAttachment.clearValue.depthStencil = vk::ClearDepthStencilValue{1.0f, 0};

        vk::RenderingInfo renderingInfo{};
        renderingInfo.setRenderArea({{0, 0}, extent});
        renderingInfo.setLayerCount(1);
        renderingInfo.setColorAttachments(colorAttachment);  // Set color attachment(s)
        renderingInfo.setPDepthAttachment(&depthAttachment); // Set depth attachment
        // renderingInfo.setPStencilAttachment(&depthAttachment); // Set stencil if used

        cmd.beginRendering(renderingInfo);

        // 4. Set Viewport and Scissor (dynamic states)
        vk::Viewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        cmd.setViewport(0, 1, &viewport);

        vk::Rect2D scissor{};
        scissor.offset = vk::Offset2D{0, 0};
        scissor.extent = extent;
        cmd.setScissor(0, 1, &scissor);

        // 5. Bind Pipeline and Frame Descriptor Set
        pbrPipeline->bind(cmd);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pbrPipeline->getLayout(), 0, 1,
                               &frameSets[currentFrame]->get(), 0, nullptr);

        for (auto &model : *models)
        {
            model.sync();
            model.draw(cmd, pbrPipeline->getLayout());
        }

        // cube->at(0).draw(cmd, pbrPipeline->getLayout());

        // 7. End Dynamic Rendering
        cmd.endRendering();

        // 8. Transition Swapchain Image for Present
        letc::laconic::transitionImageLayout(
            cmd, swapchain->getImages()[imageIndex], vk::ImageAspectFlagBits::eColor,
            vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits::eColorAttachmentWrite, {}, // No read access needed typically for present
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eBottomOfPipe); // Wait till bottom for presentation

        // --- End Command Buffer ---
        cmd.end();

        // --- Submit Command Buffer ---
        vk::Semaphore waitSemaphores[] = {*imageAvailableSemaphores[currentFrame]};
        vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
        vk::Semaphore signalSemaphores[] = {*renderFinishedSemaphores[currentFrame]};

        vk::SubmitInfo submitInfo{};
        submitInfo.setWaitSemaphores(waitSemaphores);
        submitInfo.setWaitDstStageMask(waitStages);
        submitInfo.setCommandBuffers(*commandBuffers[currentFrame]);
        submitInfo.setSignalSemaphores(signalSemaphores);

        // Submit to the graphics queue, signal the fence for this frame
        (void)graphicsQueue.submit(1, &submitInfo, *inFlightFences[currentFrame]);

        // --- Present Swapchain Image ---
        vk::PresentInfoKHR presentInfo{};
        presentInfo.setWaitSemaphores(signalSemaphores); // Wait for rendering to finish
        presentInfo.setSwapchains(swapchain->get());
        presentInfo.setImageIndices(imageIndex);

        vk::Result presentResult = graphicsQueue.presentKHR(&presentInfo);

        if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR)
        {
            // Handle swapchain recreation (as above)
            std::cerr << "Swapchain out of date/suboptimal during present, needs recreation." << std::endl;
        }
        else if (presentResult != vk::Result::eSuccess)
        {
            throw std::runtime_error("failed to present swap chain image!");
        }

        // --- Advance Frame Index ---
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    // --- Cleanup ---
    device->getLogical().waitIdle(); // Wait for all GPU operations to complete before destroying resources

    // Unique handles (command buffers, semaphores, fences, command pool) clean up automatically
    // Shared pointers (window, instance, device, allocator, descPool, pipeline, models, frameSets etc.) clean up
    // automatically

    return 0;
}
