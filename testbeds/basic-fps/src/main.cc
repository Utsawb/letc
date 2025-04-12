#include "Allocator.hh"
#include "Buffer.hh"
#include "Camera.hh"
#include "Device.hh"
#include "Material.hh"
#include "Model.hh"
#include "Pipeline.hh"
#include "Points.hh"
#include "Swapchain.hh"
#include "Window.hh"

std::filesystem::path resourcePath = "../../../resources";

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

    std::vector<letc::Model> models;
    std::vector<letc::Model::UniformBuffer> modelUniforms{};
    std::unique_ptr<letc::Buffer> modelUniformsBuffer;
    std::unique_ptr<letc::ModelRenderer> modelRenderer;

    std::unique_ptr<letc::Points> points;
    std::unique_ptr<letc::PointsRenderer> pointsRenderer;

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
        deviceBuilder.requestQueue("graphics", vk::QueueFlagBits::eGraphics);
        device = std::make_unique<letc::Device>(*instance, deviceBuilder);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(device->device);

        // allocator initialization
        allocator = std::make_unique<letc::Allocator>(*instance, *device);

        // swapchain + queue initialization
        swapchain = std::make_unique<letc::Swapchain>(*window, *surface, *device);
        queue = device->queues["graphics"].queue;

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

        camera = std::make_unique<letc::Camera>(*allocator, glm::vec4{0.0f, 0.0f, 2.0f, 1.0f},
                                                glm::vec4{0.0f, 0.0f, 0.0f, 1.0f}, glm::vec4{0.0f, 1.0f, 0.0f, 1.0f},
                                                60.0f, (float)window->getWidth() / (float)window->getHeight());

        models.emplace_back(*allocator, resourcePath / "Avocado.glb");
        models.emplace_back(*allocator, resourcePath / "pointy.glb");

        float modelScalingFactor = 10.0f;
        models.at(0).uniform.model *= glm::scale(models.at(0).uniform.model, glm::vec3(modelScalingFactor));
        models.at(1).uniform.model *= glm::scale(models.at(1).uniform.model, glm::vec3(modelScalingFactor));

        std::for_each(models.begin(), models.end(), [](letc::Model &m) { m.cpyAttributes(); });

        std::for_each(models.begin(), models.end(),
                      [this](const letc::Model &m) { modelUniforms.push_back(m.uniform); });
        modelUniformsBuffer =
            std::make_unique<letc::Buffer>(*allocator, sizeof(letc::Model::UniformBuffer) * models.size(),
                                           vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

        modelRenderer = std::make_unique<letc::ModelRenderer>(*allocator, *device, *swapchain,
                                                              readFile(resourcePath / "pbr.vert.spv"),
                                                              readFile(resourcePath / "pbr.frag.spv"));

        modelRenderer->material->updateDescriptorBufferInfo(0, 0, globalUniformsBuffer->buffer, 0,
                                                            sizeof(GlobalUniforms));
        modelRenderer->material->updateDescriptorBufferInfo(0, 1, lightsBuffer->buffer, 0,
                                                            sizeof(Light) * lights.size());
        modelRenderer->material->updateDescriptorBufferInfo(0, 2, *camera->buffer, 0, sizeof(letc::Camera::Uniform));
        modelRenderer->material->updateDescriptorBufferInfo(1, 0, modelUniformsBuffer->buffer, 0,
                                                            sizeof(letc::Model::UniformBuffer) * modelUniforms.size());
        modelRenderer->material->updateDescriptorSets();

        points = std::make_unique<letc::Points>(*allocator);
        pointsRenderer = std::make_unique<letc::PointsRenderer>(*allocator, *device, *swapchain,
                                                                readFile(resourcePath / "points.vert.spv"),
                                                                readFile(resourcePath / "points.frag.spv"));

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

    void updateLogic()
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

        // rotate the lights around the y axis
        for (size_t i = 0; i < lights.size(); ++i)
        {
            float angle = globalUniforms.time + static_cast<float>(i) * glm::pi<float>() / 2.0f;
            lights[i].position.x = 2.0f * cos(angle);
            lights[i].position.z = 2.0f * sin(angle);
        }

        for (size_t i = 0; i < models.size(); ++i)
        {
            modelUniforms[i].model = models[i].uniform.model;
            modelUniforms[i].modelInvTranspose = glm::transpose(glm::inverse(models[i].uniform.model));
        }
    }

    void syncData()
    {
        globalUniformsBuffer->cpy(&globalUniforms, sizeof(GlobalUniforms));
        lightsBuffer->cpy(lights.data(), sizeof(Light) * lights.size());
        camera->cpy();
        modelUniformsBuffer->cpy(modelUniforms.data(), sizeof(letc::Model::UniformBuffer) * models.size());
        modelRenderer->material->updateDescriptorSets();

        points->cpy();
    }

    void flatFrame()
    {
        auto [result, imageIndex] =
            device->device.acquireNextImageKHR(*swapchain, 5000000000, nullptr, imageFence.get());
        assertThrow(result == vk::Result::eSuccess, "failed to acquire next image: " + vk::to_string(result));
        m_currentImageIndex = imageIndex;

        modelRenderer->material->updateDescriptorBufferInfo(0, 2, *camera->buffer, 0, sizeof(letc::Camera::Uniform));
        modelRenderer->material->updateDescriptorSet(0);

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

        modelRenderer->pipeline->bind(commandBuffer.get());
        modelRenderer->material->bind(commandBuffer.get(), *modelRenderer->pipeline);

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
        app.flatFrame();
    }
    return 0;
}
