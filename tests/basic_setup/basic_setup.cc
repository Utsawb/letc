#include <letc/letc.hh>

#include "model.hh"

std::filesystem::path resourcePath = "../resources";

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

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
    auto window = letc::WindowBuilder{}.setWidth(1280).setHeight(720).setTitle("fake dof").build();
    auto instance = letc::InstanceBuilder{}
                        .addExtension(vk::KHRSurfaceExtensionName)
                        .addExtension(vk::KHRGetPhysicalDeviceProperties2ExtensionName)
                        .build();
    auto device =
        letc::DeviceBuilder{}
            .addExtension(vk::KHRSwapchainExtensionName)
            .addExtension(vk::KHRDynamicRenderingExtensionName)
            .addExtension(vk::KHRSeparateDepthStencilLayoutsExtensionName)
            .addExtension(vk::KHRCreateRenderpass2ExtensionName)
            .addExtension(vk::KHRDepthStencilResolveExtensionName)
            .addExtension(vk::KHRMultiviewExtensionName)
            .addExtension(vk::KHRMaintenance2ExtensionName)
            .requestQueues("graphics", vk::QueueFlagBits::eGraphics)
            .setDeviceFeatures([](letc::DeviceBuilder::FeatureChain &f) {
                f.get<vk::PhysicalDeviceFeatures2>().features.setShaderSampledImageArrayDynamicIndexing(VK_TRUE);
                f.get<vk::PhysicalDeviceVulkan13Features>().setDynamicRendering(true);
            })
            .build(instance);
    auto graphicsQueueInfo = device->getQueue("graphics");
    auto graphicsQueue = graphicsQueueInfo.get()[0];

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

    format = vk::Format::eR8G8B8A8Unorm;
    auto depthFormat = vk::Format::eD32Sfloat;
    auto pbrPipeline = letc::GraphicsPipelineBuilder{}
                           .addShader(pbrVert)
                           .addShader(pbrFrag)
                           .addVertexBinding(0, sizeof(glm::vec3), vk::VertexInputRate::eVertex)
                           .addVertexAttribute(0, 0, vk::Format::eR32G32B32Sfloat, 0)
                           .addVertexBinding(1, sizeof(glm::vec3), vk::VertexInputRate::eVertex)
                           .addVertexAttribute(1, 1, vk::Format::eR32G32B32Sfloat, 0)
                           .setRendering([format, depthFormat](vk::PipelineRenderingCreateInfo &prci) {
                               prci.setColorAttachmentFormats(format).setDepthAttachmentFormat(depthFormat);
                           })
                           .build(device);

    shaderManager.add(resourcePath / "shaders" / "dof" / "compute.spirv", "main");
    auto postProcessCompute = shaderManager.getShader(resourcePath / "shaders" / "dof" / "compute.spirv", "main");
    auto computeSetLayoutInfo = postProcessCompute.getLayouts().at(0);
    vk::UniqueDescriptorSetLayout computeSetLayout = computeSetLayoutInfo.build(device);

    vk::PipelineLayoutCreateInfo computePipelineLayoutInfo{};
    computePipelineLayoutInfo.setSetLayouts(*computeSetLayout);
    vk::UniquePipelineLayout computePipelineLayout =
        device->getLogical().createPipelineLayoutUnique(computePipelineLayoutInfo);

    vk::ComputePipelineCreateInfo computePipelineInfo{};
    computePipelineInfo.setLayout(*computePipelineLayout);
    computePipelineInfo.setStage(vk::PipelineShaderStageCreateInfo{}
                                     .setStage(vk::ShaderStageFlagBits::eCompute)
                                     .setModule(postProcessCompute.get())
                                     .setPName("main"));

    auto computeResult = device->getLogical().createComputePipelineUnique(VK_NULL_HANDLE, computePipelineInfo);
    ATHROW(computeResult.result == vk::Result::eSuccess, "Failed to create compute pipeline");
    vk::UniquePipeline computePipeline = std::move(computeResult.value);

    std::vector<vk::UniqueDescriptorSet> computeDescriptorSets;
    auto computeDescLayoutForAlloc = std::make_shared<letc::DescriptorSetLayout>(computeSetLayoutInfo);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vk::DescriptorSetAllocateInfo allocInfo{};
        allocInfo.setDescriptorPool(descPool->get());
        allocInfo.setSetLayouts(*computeSetLayout);
        computeDescriptorSets.push_back(std::move(device->getLogical().allocateDescriptorSetsUnique(allocInfo)[0]));
    }

    auto commandPool =
        device->getLogical().createCommandPoolUnique(vk::CommandPoolCreateInfo{}
                                                         .setQueueFamilyIndex(graphicsQueueInfo.getFamily())
                                                         .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer));

    vk::UniqueSampler depthSampler =
        device->getLogical().createSamplerUnique(vk::SamplerCreateInfo{}
                                                     .setMagFilter(vk::Filter::eNearest)
                                                     .setMinFilter(vk::Filter::eNearest)
                                                     .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
                                                     .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
                                                     .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
                                                     .setMipmapMode(vk::SamplerMipmapMode::eNearest));

    // --- Per-Frame Resources ---
    std::vector<vk::UniqueCommandBuffer> commandBuffers;
    std::vector<vk::UniqueSemaphore> imageAvailableSemaphores;
    std::vector<vk::UniqueSemaphore> renderFinishedSemaphores;
    std::vector<vk::UniqueFence> inFlightFences;
    std::vector<std::shared_ptr<letc::Image>> depthImages;
    std::vector<std::shared_ptr<letc::ImageView>> depthImageViews;
    std::vector<std::shared_ptr<letc::Image>> offscreenColorImages;
    std::vector<std::shared_ptr<letc::ImageView>> offscreenColorImageViews;

    commandBuffers =
        device->getLogical().allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{}
                                                              .setCommandPool(*commandPool)
                                                              .setLevel(vk::CommandBufferLevel::ePrimary)
                                                              .setCommandBufferCount(MAX_FRAMES_IN_FLIGHT));

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        imageAvailableSemaphores.push_back(device->getLogical().createSemaphoreUnique({}));
        renderFinishedSemaphores.push_back(device->getLogical().createSemaphoreUnique({}));
        inFlightFences.push_back(device->getLogical().createFenceUnique({vk::FenceCreateFlagBits::eSignaled}));
        depthImages.push_back(letc::laconic::depthImage(allocator, extent.width, extent.height));
        depthImageViews.push_back(letc::laconic::depthImageView(device, depthImages[i]));

        // --> MODIFIED: Create Offscreen Color Image & View
        auto colorImageCreateInfo =
            vk::ImageCreateInfo{}
                .setImageType(vk::ImageType::e2D)
                // --> CHANGED: Use UNORM format supporting storage
                .setFormat(vk::Format::eR8G8B8A8Unorm)
                .setExtent({extent.width, extent.height, 1})
                .setMipLevels(1)
                .setArrayLayers(1)
                .setSamples(vk::SampleCountFlagBits::e1)
                .setTiling(vk::ImageTiling::eOptimal)
                // --> ADDED: TransferSrc for Blit
                .setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage |
                          vk::ImageUsageFlagBits::eSampled |
                          vk::ImageUsageFlagBits::eTransferSrc) // Added Sampled (optional) + TransferSrc
                .setInitialLayout(vk::ImageLayout::eUndefined);

        offscreenColorImages.push_back(
            std::make_shared<letc::Image>(allocator, colorImageCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY));

        auto colorImageViewCreateInfo = vk::ImageViewCreateInfo{}
                                            .setImage(offscreenColorImages[i]->get())
                                            .setViewType(vk::ImageViewType::e2D)
                                            // --> CHANGED: Match image format
                                            .setFormat(vk::Format::eR8G8B8A8Unorm)
                                            .setSubresourceRange(vk::ImageSubresourceRange{}
                                                                     .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                                     .setLevelCount(1)
                                                                     .setLayerCount(1));
        offscreenColorImageViews.push_back(std::make_shared<letc::ImageView>(device, colorImageViewCreateInfo));
    }

    auto frameBuffer =
        letc::ObjectBuffer<FrameData>(allocator, vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
    frameBuffer->frame = 0;
    frameBuffer->time = 0.0f;

    std::println("Width: {}, Height: {}, Aspect: {}", extent.width, extent.height,
                 (float)extent.width / (float)extent.height);
    auto camera = letc::FirstPersonCamera(allocator, (float)extent.width / (float)extent.height, glm::radians(60.0f),
                                          0.1f, 100000.0f, {0.0f, 1.0f, 5.0f});

    auto lights = letc::VectorBuffer<LightData>(allocator, 4, vk::BufferUsageFlagBits::eStorageBuffer,
                                                VMA_MEMORY_USAGE_CPU_TO_GPU);
    lights->at(0) = {{-5.0f, 5.0f, -5.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 10.0f}};
    lights->at(1) = {{5.0f, 5.0f, -5.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 10.0f}};
    lights->at(2) = {{-5.0f, 5.0f, 5.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 10.0f}};
    lights->at(3) = {{5.0f, 5.0f, 5.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 10.0f}};

    auto frameSetLayout = std::make_shared<letc::DescriptorSetLayout>(pbrPipeline->getSetLayouts().at(0));
    std::vector<std::shared_ptr<letc::DescriptorSet>> frameSets;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        auto fs = frameSetLayout->build(descPool);
        fs->attachBuffer("UFrameData", frameBuffer.get(), frameBuffer.containedSize());
        fs->attachBuffer("UCamera", camera.getBuffer(), camera.containedSize());
        fs->attachBuffer("BLights", lights.get(), lights.containedSize());
        frameSets.push_back(fs);
    }

    auto modelLayout = std::make_shared<letc::DescriptorSetLayout>(pbrPipeline->getSetLayouts().at(1));
    auto models = loadModels(resourcePath / "models" / "sponza" / "Sponza.gltf", modelLayout, descPool, allocator);
    uint32_t currentFrame = 0;
    auto startTime = std::chrono::high_resolution_clock::now();

    double lastMouseX, lastMouseY = 0.0f;
    std::tie(lastMouseX, lastMouseY) = window->get().getCursorPos();

    while (window->get().shouldClose() == false)
    {
        vkfw::pollEvents();

        auto [mx, my] = window->get().getCursorPos();

        if (window->get().getMouseButton(vkfw::MouseButton::eLeft))
        {
            window->get().set<vkfw::InputMode::eCursor>(vkfw::CursorMode::eDisabled);
            float deltaX = static_cast<float>(mx - lastMouseX);
            float deltaY = static_cast<float>(lastMouseY - my);
            camera.pan(deltaX, deltaY);
        }
        else
        {
            window->get().set<vkfw::InputMode::eCursor>(vkfw::CursorMode::eNormal);
        }

        lastMouseX = mx;
        lastMouseY = my;

        glm::vec3 movementInput{0.0f};
        if (window->get().getKey(vkfw::Key::eW))
        {
            movementInput.x += 1.0f;
        }
        if (window->get().getKey(vkfw::Key::eS))
        {
            movementInput.x -= 1.0f;
        }
        if (window->get().getKey(vkfw::Key::eA))
        {
            movementInput.y -= 1.0f;
        }
        if (window->get().getKey(vkfw::Key::eD))
        {
            movementInput.y += 1.0f;
        }
        if (window->get().getKey(vkfw::Key::eSpace))
        {
            movementInput.z += 1.0f;
        }
        if (window->get().getKey(vkfw::Key::eLeftShift))
        {
            movementInput.z -= 1.0f;
        }
        if (glm::length(movementInput) > 0.0f)
        {
            camera.move(movementInput);
        }
        camera.sync();

        vk::Result waitResult =
            device->getLogical().waitForFences(1, &(*inFlightFences[currentFrame]), VK_TRUE, UINT64_MAX);
        if (waitResult != vk::Result::eSuccess)
        {
            std::cerr << "Failed to wait for fence!" << std::endl;
        }

        uint32_t imageIndex;
        vk::Result acquireResult = device->getLogical().acquireNextImageKHR(
            swapchain->get(), UINT64_MAX, *imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (acquireResult == vk::Result::eErrorOutOfDateKHR)
        {
            std::cerr << "Swapchain out of date/suboptimal, needs recreation." << std::endl;
            continue;
        }
        else if (acquireResult != vk::Result::eSuccess && acquireResult != vk::Result::eSuboptimalKHR)
        {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        (void)device->getLogical().resetFences(1, &(*inFlightFences[currentFrame]));

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        frameBuffer->time = time;
        frameBuffer->frame = (frameBuffer->frame + 1);
        frameBuffer.sync();

        camera.sync();
        lights.sync();

        const vk::CommandBuffer &cmd = *commandBuffers[currentFrame];
        cmd.reset();
        cmd.begin(vk::CommandBufferBeginInfo{});

        letc::laconic::transitionImageLayout(
            cmd, depthImages[currentFrame], vk::ImageAspectFlagBits::eDepth, vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthAttachmentOptimal, {}, vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests);

        letc::laconic::transitionImageLayout(
            cmd, offscreenColorImages[currentFrame], vk::ImageAspectFlagBits::eColor, vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal, {}, vk::AccessFlagBits::eColorAttachmentWrite,
            vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput);

        vk::RenderingAttachmentInfo colorAttachment{};
        colorAttachment.setImageView(offscreenColorImageViews[currentFrame]->get());
        colorAttachment.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);
        colorAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
        colorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
        colorAttachment.clearValue.color = vk::ClearColorValue{std::array<float, 4>{0.1f, 0.1f, 0.1f, 1.0f}};

        vk::RenderingAttachmentInfo depthAttachment{};
        depthAttachment.setImageView(depthImageViews[currentFrame]->get());
        depthAttachment.setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal);
        depthAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
        depthAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
        depthAttachment.clearValue.depthStencil = vk::ClearDepthStencilValue{1.0f, 0};

        vk::RenderingInfo renderingInfo{};
        renderingInfo.setRenderArea({{0, 0}, extent});
        renderingInfo.setLayerCount(1);
        renderingInfo.setColorAttachments(colorAttachment);
        renderingInfo.setPDepthAttachment(&depthAttachment);

        cmd.beginRendering(renderingInfo);

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

        pbrPipeline->bind(cmd);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pbrPipeline->getLayout(), 0, 1,
                               &frameSets[currentFrame]->get(), 0, nullptr);

        for (auto &model : *models)
        {
            model.sync();
            model.draw(cmd, pbrPipeline->getLayout());
        }

        cmd.endRendering();

        letc::laconic::transitionImageLayout(cmd, offscreenColorImages[currentFrame], vk::ImageAspectFlagBits::eColor,
                                             vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eGeneral,
                                             vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead,
                                             vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                             vk::PipelineStageFlagBits::eComputeShader);

        letc::laconic::transitionImageLayout(
            cmd, depthImages[currentFrame],
            vk::ImageAspectFlagBits::eDepth, // CORRECT for D32_SFLOAT
            vk::ImageLayout::eDepthAttachmentOptimal,
            // --> CHANGED: Optimal layout for sampling
            vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            // --> CHANGED: Read access for shader sampling
            vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eLateFragmentTests,
            vk::PipelineStageFlagBits::eComputeShader);

        letc::laconic::transitionImageLayout(cmd, swapchain->getImages()[imageIndex], vk::ImageAspectFlagBits::eColor,
                                             vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, {},
                                             vk::AccessFlagBits::eShaderWrite, vk::PipelineStageFlagBits::eTopOfPipe,
                                             vk::PipelineStageFlagBits::eComputeShader);

        std::vector<vk::WriteDescriptorSet> computeWrites;

        // Input Color (Binding 0) - Storage Image (or Sampled)
        vk::DescriptorImageInfo colorInputInfo{};
        colorInputInfo.setImageView(offscreenColorImageViews[currentFrame]->get());
        colorInputInfo.setImageLayout(vk::ImageLayout::eGeneral); // Or ShaderReadOnlyOptimal if read-only
        computeWrites.push_back(vk::WriteDescriptorSet{}
                                    .setDstSet(*computeDescriptorSets[currentFrame])
                                    .setDstBinding(0) // Input color binding
                                    .setDescriptorCount(1)
                                    .setDescriptorType(vk::DescriptorType::eStorageImage) // Use Storage for read/write
                                    .setImageInfo(colorInputInfo));

        // Input Depth (Binding 1) - --> CHANGED to Combined Image Sampler
        vk::DescriptorImageInfo depthInputInfo{};
        depthInputInfo.setImageView(depthImageViews[currentFrame]->get());
        depthInputInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal); // Use read-only layout for sampling
        depthInputInfo.setSampler(*depthSampler);                               // --> Set the sampler
        computeWrites.push_back(vk::WriteDescriptorSet{}
                                    .setDstSet(*computeDescriptorSets[currentFrame])
                                    .setDstBinding(1) // Input depth binding
                                    .setDescriptorCount(1)
                                    // --> CHANGED descriptor type
                                    .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                                    .setImageInfo(depthInputInfo));

        // Output Color (Binding 2) - Storage Image (writing back to offscreen)
        vk::DescriptorImageInfo computeOutputInfo{};
        computeOutputInfo.setImageView(offscreenColorImageViews[currentFrame]->get());
        computeOutputInfo.setImageLayout(vk::ImageLayout::eGeneral);
        computeWrites.push_back(vk::WriteDescriptorSet{}
                                    .setDstSet(*computeDescriptorSets[currentFrame])
                                    .setDstBinding(2) // Output color binding
                                    .setDescriptorCount(1)
                                    .setDescriptorType(vk::DescriptorType::eStorageImage)
                                    .setImageInfo(computeOutputInfo));

        device->getLogical().updateDescriptorSets(computeWrites, nullptr);

        cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *computePipeline);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *computePipelineLayout, 0, 1,
                               &(*computeDescriptorSets[currentFrame]), 0, nullptr);

        uint32_t groupCountX = (extent.width / 32);
        uint32_t groupCountY = (extent.height / 32);
        cmd.dispatch(groupCountX, groupCountY, 1);

        // --- Barrier Before Blit ---

        // Transition Offscreen image (Compute Output) to Transfer Source
        letc::laconic::transitionImageLayout(cmd, offscreenColorImages[currentFrame], vk::ImageAspectFlagBits::eColor,
                                             vk::ImageLayout::eGeneral,            // From compute write
                                             vk::ImageLayout::eTransferSrcOptimal, // To blit source
                                             vk::AccessFlagBits::eShaderWrite,     // Wait for compute write
                                             vk::AccessFlagBits::eTransferRead,    // Read access for blit
                                             vk::PipelineStageFlagBits::eComputeShader,
                                             vk::PipelineStageFlagBits::eTransfer);

        // Transition Swapchain image to Transfer Destination
        letc::laconic::transitionImageLayout(cmd, swapchain->getImages()[imageIndex], vk::ImageAspectFlagBits::eColor,
                                             vk::ImageLayout::eUndefined,           // Current layout (after acquire)
                                             vk::ImageLayout::eTransferDstOptimal,  // To blit destination
                                             {},                                    // No prior access needed
                                             vk::AccessFlagBits::eTransferWrite,    // Write access for blit
                                             vk::PipelineStageFlagBits::eTopOfPipe, // Can start early
                                             vk::PipelineStageFlagBits::eTransfer);

        // --- Blit Image Command ---
        vk::ImageBlit blitRegion{};
        blitRegion.srcSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor);
        blitRegion.srcSubresource.setLayerCount(1);
        blitRegion.srcOffsets[1] = vk::Offset3D{static_cast<int32_t>(extent.width), static_cast<int32_t>(extent.height),
                                                1}; // Source region extent
        blitRegion.dstSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor);
        blitRegion.dstSubresource.setLayerCount(1);
        blitRegion.dstOffsets[1] = vk::Offset3D{static_cast<int32_t>(extent.width), static_cast<int32_t>(extent.height),
                                                1}; // Destination region extent

        cmd.blitImage(offscreenColorImages[currentFrame]->get(), vk::ImageLayout::eTransferSrcOptimal,
                      swapchain->getImages()[imageIndex], vk::ImageLayout::eTransferDstOptimal, 1, &blitRegion,
                      vk::Filter::eNearest // Or vk::Filter::eLinear if supported and desired
        );

        // --- Barrier Before Present ---

        // Transition Swapchain Image from Transfer Dst to PresentSrc
        letc::laconic::transitionImageLayout(cmd, swapchain->getImages()[imageIndex], vk::ImageAspectFlagBits::eColor,
                                             vk::ImageLayout::eTransferDstOptimal, // From blit write
                                             vk::ImageLayout::ePresentSrcKHR,      // To present
                                             vk::AccessFlagBits::eTransferWrite,   // Wait for blit write
                                             {},                                   // No subsequent access needed
                                             vk::PipelineStageFlagBits::eTransfer,
                                             vk::PipelineStageFlagBits::eBottomOfPipe);

        // --- End Command Buffer and Submit (Existing) ---
        cmd.end();
        // ... submit and present ...

        vk::Semaphore waitSemaphores[] = {*imageAvailableSemaphores[currentFrame]};
        vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
        vk::Semaphore signalSemaphores[] = {*renderFinishedSemaphores[currentFrame]};

        vk::SubmitInfo submitInfo{};
        submitInfo.setWaitSemaphores(waitSemaphores);
        submitInfo.setWaitDstStageMask(waitStages);
        submitInfo.setCommandBuffers(*commandBuffers[currentFrame]);
        submitInfo.setSignalSemaphores(signalSemaphores);

        (void)graphicsQueue.submit(1, &submitInfo, *inFlightFences[currentFrame]);

        vk::PresentInfoKHR presentInfo{};
        presentInfo.setWaitSemaphores(signalSemaphores);
        presentInfo.setSwapchains(swapchain->get());
        presentInfo.setImageIndices(imageIndex);

        vk::Result presentResult = graphicsQueue.presentKHR(&presentInfo);

        if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR)
        {
            std::cerr << "Swapchain out of date/suboptimal during present, needs recreation." << std::endl;
        }
        else if (presentResult != vk::Result::eSuccess)
        {
            throw std::runtime_error("failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    device->getLogical().waitIdle();
    return 0;
}
