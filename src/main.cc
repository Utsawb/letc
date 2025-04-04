// Created by: Utsawb Lamichhane
#include "pch.hh"

#include "Allocator.hh"
#include "Buffer.hh"
#include "Camera.hh"
#include "Descriptor.hh"
#include "Device.hh"
#include "Material.hh"
#include "Model.hh"
#include "Pipeline.hh"
#include "Swapchain.hh"
#include "Window.hh"

std::filesystem::path resourcePath = "../../resources/";

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
    std::unique_ptr<letc::Instance> instance;
    vkfw::UniqueWindow window;
    vk::UniqueSurfaceKHR surface;

    std::unique_ptr<letc::Device> device;
    std::unique_ptr<letc::Allocator> allocator;
    vk::Queue queue;
    std::unique_ptr<letc::Swapchain> swapchain;

    vk::UniqueCommandPool commandPool;
    vk::UniqueCommandBuffer commandBuffer;
    uint32_t m_currentImageIndex = 0;
    vk::UniqueFence imageFence;
    vk::UniqueFence commandFence;

    GlobalUniforms globalUniforms;
    std::unique_ptr<letc::Buffer> globalUniformsBuffer;

    std::vector<Light> lights;
    std::unique_ptr<letc::Buffer> lightsBuffer;

    std::unique_ptr<letc::Camera> camera;

    std::vector<letc::Model> models;
    std::vector<letc::Model::UniformBuffer> modelUniforms{};
    std::unique_ptr<letc::Buffer> modelUniformsBuffer;

    std::unique_ptr<letc::DescriptorLayout> pbrLayout;
    std::unique_ptr<letc::Material> pbrMaterial;
    std::unique_ptr<letc::GraphicsPipeline> pbrPipeline;

    std::unique_ptr<letc::ImageBuffer<float>> depthBuffer;
    vk::UniqueImageView depthImageView;

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
        instance = std::make_unique<letc::Instance>(letc::InstanceBuilder{}.setDebug(true));
        VULKAN_HPP_DEFAULT_DISPATCHER.init(instance->instance);

        // window surface creation
        surface = vkfw::createWindowSurfaceUnique(*instance, *window);
        assertThrow(surface, "failed to create surface");
        device = std::make_unique<letc::Device>(*instance);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(device->device);

        // allocator initialization
        allocator = std::make_unique<letc::Allocator>(*instance, *device);

        // swapchain + queue initialization
        swapchain = std::make_unique<letc::Swapchain>(*window, *surface, *device, *device);
        queue = device->device.getQueue(device->graphicsQueueFamilyIndex, 0);

        // command buffer initialization
        commandPool =
            device->device.createCommandPoolUnique(vk::CommandPoolCreateInfo{}
                                                       .setQueueFamilyIndex(device->graphicsQueueFamilyIndex)
                                                       .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer));
        commandBuffer = std::move(device->device
                                      .allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{}
                                                                        .setCommandBufferCount(1)
                                                                        .setCommandPool(commandPool.get())
                                                                        .setLevel(vk::CommandBufferLevel::ePrimary))
                                      .at(0));

        // fence initialization
        imageFence = device->device.createFenceUnique(vk::FenceCreateInfo{});
        commandFence = device->device.createFenceUnique(vk::FenceCreateInfo{});

        // data initialization
        globalUniforms = {0.0f, 0.0f};
        globalUniformsBuffer = std::make_unique<letc::Buffer>(
            *allocator, sizeof(GlobalUniforms), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

        lights.push_back({{0.0f, 0.0f, 2.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}});
        lights.push_back({{2.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}});
        lights.push_back({{0.0f, 2.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}});
        lights.push_back({{0.0f, 0.0f, -2.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}});
        lightsBuffer =
            std::make_unique<letc::Buffer>(*allocator, sizeof(Light) * lights.size(),
                                           vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

        camera = std::make_unique<letc::Camera>(*allocator, glm::vec4{0.0f, 0.0f, 2.0f, 1.0f},
                                                glm::vec4{0.0f, 0.0f, 0.0f, 1.0f}, glm::vec4{0.0f, 1.0f, 0.0f, 1.0f},
                                                60.0f, (float)window->getWidth() / (float)window->getHeight());

        models.emplace_back(*allocator, resourcePath / "Avocado.glb");
        models.emplace_back(*allocator, resourcePath / "platform.glb");
        std::for_each(models.begin(), models.end(), [](letc::Model &m) { m.cpyAttributes(); });

        std::for_each(models.begin(), models.end(),
                      [this](const letc::Model &m) { modelUniforms.push_back(m.uniform); });
        modelUniformsBuffer =
            std::make_unique<letc::Buffer>(*allocator, sizeof(letc::Model::UniformBuffer) * models.size(),
                                           vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

        // descriptor layout and material initialization
        pbrLayout = std::make_unique<letc::DescriptorLayout>(*device);
        pbrLayout->addBinding(0, 0, vk::DescriptorType::eUniformBuffer,
                              vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 1);
        pbrLayout->addBinding(0, 1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment, 1);
        pbrLayout->addBinding(0, 2, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex, 1);
        pbrLayout->addBinding(1, 0, vk::DescriptorType::eUniformBufferDynamic,
                              vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 1);
        pbrLayout->generateLayouts();

        pbrMaterial = std::make_unique<letc::Material>(*device, *allocator, *pbrLayout);
        pbrMaterial->updateDescriptorBufferInfo(0, 0, globalUniformsBuffer->buffer, 0, sizeof(GlobalUniforms));
        pbrMaterial->updateDescriptorBufferInfo(0, 1, lightsBuffer->buffer, 0, sizeof(Light) * lights.size());
        pbrMaterial->updateDescriptorBufferInfo(0, 2, *camera->buffer, 0, sizeof(letc::Camera::Uniform));
        pbrMaterial->updateDescriptorBufferInfo(1, 0, modelUniformsBuffer->buffer, 0,
                                                sizeof(letc::Model::UniformBuffer));
        pbrMaterial->updateDescriptorSets();
        pbrMaterial->updateDynamicOffset(1, 0);

        // pipeline initialization
        letc::GraphicsPipelineBuilder gpb;
        gpb.addShaderStage(readFile(resourcePath / "pbr.vert.spv"), vk::ShaderStageFlagBits::eVertex);
        gpb.addShaderStage(readFile(resourcePath / "pbr.frag.spv"), vk::ShaderStageFlagBits::eFragment);
        gpb.addVertexInputBinding(0, sizeof(glm::vec4), vk::VertexInputRate::eVertex); // Position
        gpb.addVertexInputAttribute(0, 0, vk::Format::eR32G32B32A32Sfloat, 0);
        gpb.addVertexInputBinding(1, sizeof(glm::vec4), vk::VertexInputRate::eVertex); // Normal
        gpb.addVertexInputAttribute(1, 1, vk::Format::eR32G32B32A32Sfloat, 0);
        gpb.addVertexInputBinding(2, sizeof(glm::vec4), vk::VertexInputRate::eVertex); // Tangent
        gpb.addVertexInputAttribute(2, 2, vk::Format::eR32G32B32A32Sfloat, 0);
        gpb.addVertexInputBinding(3, sizeof(glm::vec2), vk::VertexInputRate::eVertex); // UV
        gpb.addVertexInputAttribute(3, 3, vk::Format::eR32G32Sfloat, 0);
        gpb.setLayout(pbrLayout.get());
        gpb.renderingInfo.setColorAttachmentCount(1);
        gpb.renderingInfo.setPColorAttachmentFormats(&swapchain->format.format);
        gpb.setRasterization(gpb.rasterizationInfo.setCullMode(vk::CullModeFlagBits::eNone));
        pbrPipeline = std::make_unique<letc::GraphicsPipeline>(*device, *swapchain, gpb);

        // depth buffer initialization
        depthBuffer = std::make_unique<letc::ImageBuffer<float>>(
            allocator->allocator, static_cast<uint32_t>(window->getWidth()), static_cast<uint32_t>(window->getHeight()),
            vk::Format::eD32Sfloat, std::vector<float>(window->getWidth() * window->getHeight(), 0.0f),
            vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::ImageTiling::eOptimal);

        depthImageView = device->device.createImageViewUnique(
            vk::ImageViewCreateInfo{}
                .setImage(depthBuffer->m_gpuImage)
                .setViewType(vk::ImageViewType::e2D)
                .setFormat(vk::Format::eD32Sfloat)
                .setSubresourceRange(vk::ImageSubresourceRange{}
                                         .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                         .setBaseMipLevel(0)
                                         .setLevelCount(1)
                                         .setBaseArrayLayer(0)
                                         .setLayerCount(1)));

        std::tie(lastMouseX, lastMouseY) = window->getCursorPos();
        window->callbacks()->on_scroll = [this](vkfw::Window const &, double x, double y) {
            camera->zoom(static_cast<float>(y));
        };
    }

    void beginFrame()
    {
        vkfw::pollEvents();

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

        globalUniformsBuffer->cpy(&globalUniforms, sizeof(GlobalUniforms));
        lightsBuffer->cpy(lights.data(), sizeof(Light) * lights.size());
        camera->cpy();

        for (size_t i = 0; i < models.size(); ++i)
        {
            modelUniforms[i].model = models[i].uniform.model;
        }
        modelUniformsBuffer->cpy(modelUniforms.data(), sizeof(letc::Model::UniformBuffer) * models.size());

        pbrMaterial->updateDescriptorSets();

        auto [result, imageIndex] =
            device->device.acquireNextImageKHR(*swapchain, 5000000000, nullptr, imageFence.get());
        assertThrow(result == vk::Result::eSuccess, "failed to acquire next image: " + vk::to_string(result));
        m_currentImageIndex = imageIndex;

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

        vk::ImageMemoryBarrier colorBarrier{};
        colorBarrier.setSrcAccessMask(vk::AccessFlags{});
        colorBarrier.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
        colorBarrier.setOldLayout(vk::ImageLayout::eUndefined);
        colorBarrier.setNewLayout(vk::ImageLayout::eColorAttachmentOptimal);
        colorBarrier.setSrcQueueFamilyIndex(vk::QueueFamilyIgnored);
        colorBarrier.setDstQueueFamilyIndex(vk::QueueFamilyIgnored);
        colorBarrier.setImage(swapchain->images.at(m_currentImageIndex));
        colorBarrier.setSubresourceRange(vk::ImageSubresourceRange{}
                                             .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                             .setBaseMipLevel(0)
                                             .setLevelCount(1)
                                             .setBaseArrayLayer(0)
                                             .setLayerCount(1));

        vk::ImageMemoryBarrier depthBarrier{};
        depthBarrier.setSrcAccessMask(vk::AccessFlags{});
        depthBarrier.setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead |
                                      vk::AccessFlagBits::eDepthStencilAttachmentWrite);
        depthBarrier.setOldLayout(vk::ImageLayout::eUndefined);
        depthBarrier.setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
        depthBarrier.setSrcQueueFamilyIndex(vk::QueueFamilyIgnored);
        depthBarrier.setDstQueueFamilyIndex(vk::QueueFamilyIgnored);
        depthBarrier.setImage(depthBuffer->m_gpuImage);
        depthBarrier.setSubresourceRange(vk::ImageSubresourceRange{}
                                             .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                             .setBaseMipLevel(0)
                                             .setLevelCount(1)
                                             .setBaseArrayLayer(0)
                                             .setLayerCount(1));

        commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                                       vk::PipelineStageFlagBits::eEarlyFragmentTests, {}, 0, nullptr, 0, nullptr, 1,
                                       &depthBarrier);
        commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                                       vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, 0, nullptr, 0, nullptr, 1,
                                       &colorBarrier);

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

        pbrPipeline->bind(commandBuffer.get());
        pbrMaterial->bind(commandBuffer.get(), *pbrPipeline);

        for (uint32_t i = 0; i < models.size(); ++i)
        {
            uint32_t dynamicOffset = i * sizeof(letc::Model::UniformBuffer);
            pbrMaterial->updateDynamicOffset(1, dynamicOffset);
            pbrMaterial->bind(*commandBuffer, *pbrPipeline, 1);

            models[i].draw(*commandBuffer);
        }

        commandBuffer->endRendering();

        commandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                       vk::PipelineStageFlagBits::eBottomOfPipe, {}, 0, nullptr, 0, nullptr, 1,
                                       &vk::ImageMemoryBarrier{}
                                            .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
                                            .setDstAccessMask(vk::AccessFlagBits::eMemoryRead)
                                            .setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
                                            .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
                                            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                                            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                                            .setImage(swapchain->images.at(m_currentImageIndex))
                                            .setSubresourceRange(vk::ImageSubresourceRange{}
                                                                     .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                                     .setBaseMipLevel(0)
                                                                     .setLevelCount(1)
                                                                     .setBaseArrayLayer(0)
                                                                     .setLayerCount(1)));

        commandBuffer->end();

        assertThrow(device->device.resetFences(1, &commandFence.get()) == vk::Result::eSuccess,
                    "failed to reset command fence");

        queue.submit(vk::SubmitInfo{}.setCommandBufferCount(1).setPCommandBuffers(&commandBuffer.get()),
                     commandFence.get());

        assertThrow(device->device.waitForFences(1, &commandFence.get(), VK_TRUE, 5000000000) == vk::Result::eSuccess,
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

int main()
{
    App app{};
    while (!app.window->shouldClose())
    {
        app.beginFrame();
    }
    return 0;
}
