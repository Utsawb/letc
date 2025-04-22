#include "Allocator.hh"
#include "Buffer.hh"
#include "Camera.hh"
#include "Curves.hh"
#include "Descriptor.hh"
#include "Device.hh"
#include "FrenetFrames.hh"
#include "Image.hh"
#include "Material.hh"
#include "Model.hh"
#include "Pipeline.hh"
#include "Points.hh"
#include "Swapchain.hh"
#include "Window.hh"

#include "xr.hh"

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
    std::unique_ptr<letc::Instance> instance;
    vkfw::UniqueWindow window;
    // vk::UniqueSurfaceKHR surface;

    std::unique_ptr<letc::Device> device;
    std::unique_ptr<letc::Allocator> allocator;
    vk::Queue queue;

    std::unique_ptr<letc::DescriptorManager> descriptorManager;

    vk::UniqueCommandPool commandPool;
    std::vector<vk::UniqueCommandBuffer> commandBuffers;
    uint32_t m_currentImageIndex = 0;
    vk::UniqueFence imageFence;
    std::vector<vk::UniqueFence> commandFences;

    GlobalUniforms globalUniforms;
    std::unique_ptr<letc::Buffer> globalUniformsBuffer;

    std::vector<Light> lights;
    std::unique_ptr<letc::Buffer> lightsBuffer;

    std::vector<letc::Model> models;
    std::vector<letc::Model::UniformBuffer> modelUniforms{};
    std::unique_ptr<letc::Buffer> modelUniformsBuffer;

    std::unique_ptr<letc::ModelRenderer> modelRenderer;

    std::unique_ptr<letc::Points> points;
    std::unique_ptr<letc::PointsRenderer> pointsRenderer;

    std::unique_ptr<letc::FrenetFramesRenderer> frenetRenderer;
    std::unique_ptr<letc::FrenetFrame> frenetFrames;

    XrContext xrCtx;

    double lastMouseX, lastMouseY;
    double timeScaling = 1.0f;

    std::unique_ptr<letc::Curves> curves;

    size_t currentFrame = 0;
    App()
    {
        // basic initialization
        VULKAN_HPP_DEFAULT_DISPATCHER.init();
        vkfw::init();
        vkfw::initHint<vkfw::InitializationHint::ePlatform>(vkfw::Platform::eX11);

        // window and vulkan initialization
        vkfw::WindowHints windowHints{};
        window = vkfw::createWindowUnique(letc::WindowBuilder{});
        instance =
            std::make_unique<letc::Instance>(letc::InstanceBuilder{}
                                                 .setDebug(true)
                                                 .addInstanceExtension("VK_KHR_external_memory_capabilities")
                                                 .addInstanceExtension("VK_KHR_get_physical_device_properties2"));
        VULKAN_HPP_DEFAULT_DISPATCHER.init(instance->instance);

        // // window surface creation
        // surface = vkfw::createWindowSurfaceUnique(*instance, *window);
        // assertThrow(surface, "failed to create surface");

        letc::DeviceBuilder deviceBuilder;
        deviceBuilder.requestDevice(xrCtx.getGraphicsDevice(*instance));
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
        descriptorManager->createLayout("pbrGlobalLayout");

        descriptorManager->addLayoutBinding("pbrModelLayout", 0, vk::DescriptorType::eStorageBuffer,
                                            vk::ShaderStageFlagBits::eAllGraphics);
        descriptorManager->createLayout("pbrModelLayout");

        descriptorManager->addLayoutBinding("soloCameraLayout", 0, vk::DescriptorType::eUniformBuffer,
                                            vk::ShaderStageFlagBits::eAllGraphics);
        descriptorManager->createLayout("soloCameraLayout");

        descriptorManager->createSet("pbrGlobalLayout", "pbrGlobalSet");
        descriptorManager->createSet("pbrModelLayout", "pbrModelSet");

        descriptorManager->createSet("soloCameraLayout", "soloCamera0");
        descriptorManager->createSet("soloCameraLayout", "soloCamera1");

        // command buffer initialization
        commandPool =
            device->device.createCommandPoolUnique(vk::CommandPoolCreateInfo{}
                                                       .setQueueFamilyIndex(device->queues["graphics"].queueFamilyIndex)
                                                       .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer));
        commandBuffers =
            std::move(device->device.allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{}
                                                                      .setCommandBufferCount(2)
                                                                      .setCommandPool(commandPool.get())
                                                                      .setLevel(vk::CommandBufferLevel::ePrimary)));

        // fence initialization
        imageFence = device->device.createFenceUnique(vk::FenceCreateInfo{});
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

        models.emplace_back(*allocator, resourcePath / "Avocado.glb"); // Left Hand Model
        models.emplace_back(*allocator, resourcePath / "Avocado.glb"); // Right Hand Model
        models.emplace_back(*allocator, resourcePath / "pointy.glb");
        models.emplace_back(*allocator, resourcePath / "fish.glb");

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
        modelRenderer =
            std::make_unique<letc::ModelRenderer>(*allocator, *device, readFile(resourcePath / "pbr/pbr.vert.spirv"),
                                                  readFile(resourcePath / "pbr/pbr.frag.spirv"));

        points = std::make_unique<letc::Points>(*allocator);
        pointsRenderer = std::make_unique<letc::PointsRenderer>(*allocator, *device,
                                                                readFile(resourcePath / "points/points.vert.spirv"),
                                                                readFile(resourcePath / "points/points.frag.spirv"));

        frenetFrames = std::make_unique<letc::FrenetFrame>(*allocator);
        frenetRenderer = std::make_unique<letc::FrenetFramesRenderer>(
            *allocator, *device, readFile(resourcePath / "frenetFrame/frenetFrame.vert.spirv"),
            readFile(resourcePath / "frenetFrame/frenetFrame.frag.spirv"));

        std::tie(lastMouseX, lastMouseY) = window->getCursorPos();

        xrCtx.sessionCreate(*allocator, instance->instance, device->physicalDevice, device->device,
                            device->queues["graphics"].queueFamilyIndex);
    }

    const std::chrono::duration<long long, std::milli> BUTTON_DEBOUNCE_TIME = std::chrono::milliseconds(150);
    std::chrono::time_point<std::chrono::steady_clock> lastButtonPressTime[4];
    std::chrono::time_point<std::chrono::steady_clock> lastTriggerPressTime[2];
    bool shouldAnimate = false;
    bool shouldShowFrames = true;
    void updateLogic()
    {
        vkfw::pollEvents();

        if (window->getKey(vkfw::Key::eQ))
        {
            window->setShouldClose(true);
        }

        globalUniforms.time = static_cast<float>(vkfw::getTime());
        globalUniforms.frame = static_cast<float>(currentFrame++);

        // rotate the lights around the y axis
        for (size_t i = 0; i < lights.size(); ++i)
        {
            float angle = globalUniforms.time + static_cast<float>(i) * glm::pi<float>() / 2.0f;
            lights[i].position.x = 4.0f * cos(angle); // Using 4.0f like non-xr, adjust if needed
            lights[i].position.z = 4.0f * sin(angle); // Using 4.0f like non-xr, adjust if needed
        }

        auto eventStatus = xrCtx.updateLogic();
        models.at(0).uniform.model = eventStatus.leftHand;
        models.at(1).uniform.model = eventStatus.rightHand;

        auto now = std::chrono::steady_clock::now();
        if (eventStatus.leftTriggerPressed && (now - lastTriggerPressTime[0] >= BUTTON_DEBOUNCE_TIME))
        {
            lastTriggerPressTime[0] = now;
        }
        if (eventStatus.rightTriggerPressed && (now - lastTriggerPressTime[1] >= BUTTON_DEBOUNCE_TIME))
        {
            frenetFrames->frames.push_back(eventStatus.rightHand);
            curves = std::make_unique<letc::Curves>(frenetFrames->frames);
            points->points.clear();
            const float &length = curves->totalLength;
            for (std::size_t i = 0; i < points->capacity; ++i)
            {
                points->points.push_back(curves->getInterp((float)i / (float)points->capacity * length)[3]);
            }

            lastTriggerPressTime[1] = now;
        }
        if (eventStatus.a && (now - lastButtonPressTime[0] >= BUTTON_DEBOUNCE_TIME))
        {
            lastButtonPressTime[0] = now;
        }
        if (eventStatus.b && (now - lastButtonPressTime[1] >= BUTTON_DEBOUNCE_TIME))
        {
            shouldShowFrames = !shouldShowFrames;
            lastButtonPressTime[1] = now;
        }
        if (eventStatus.x && (now - lastButtonPressTime[2] >= BUTTON_DEBOUNCE_TIME))
        {
            frenetFrames->frames.clear();
            points->points.clear();
            lastButtonPressTime[2] = now;
        }
        if (eventStatus.y && (now - lastButtonPressTime[3] >= BUTTON_DEBOUNCE_TIME))
        {
            shouldAnimate = !shouldAnimate;
            lastButtonPressTime[3] = now;
        }

        if (eventStatus.leftJoystick != glm::vec2{0.0f, 0.0f})
        {
            timeScaling = (eventStatus.leftJoystick.y + 1.0f) / 2.0f;
        }
        if (eventStatus.rightJoystick != glm::vec2{0.0f, 0.0f})
        {
        }

        if (shouldAnimate)
        {
            if (frenetFrames->frames.size() > 1)
            {
                float t = fmod(vkfw::getTime() * timeScaling, frenetFrames->frames.size());

                if (curves)
                {
                    models.at(3).uniform.model = curves->getInterp(t);
                }
            }
            else
            {

                models.at(3).uniform.model = glm::mat4(0.0f);
            }
        }
        else
        {
            models.at(3).uniform.model = glm::mat4(0.0f);
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
        points->cpy();
        frenetFrames->cpy();
    }

    void xrFrame()
    {
        if (xrCtx.startFrame() == false)
        {
            return;
        }

        auto eyes = xrCtx.getViews();

        auto renderingStates = xrCtx.startRendering();

        for (uint32_t state = 0; state < renderingStates.size(); ++state)
        {
            auto &rs = renderingStates[state];

            descriptorManager->attachBuffer(std::format("soloCamera{}", state), 0, rs.camera->buffer->buffer,
                                            sizeof(letc::Camera::Uniform));

            vk::UniqueCommandBuffer &commandBuffer = commandBuffers.at(state);
            commandBuffer->reset(vk::CommandBufferResetFlagBits::eReleaseResources);
            commandBuffer->begin(vk::CommandBufferBeginInfo{}.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

            vk::Rect2D scissor({0, 0}, {rs.width, rs.height});
            commandBuffer->setScissor(0, 1, &scissor);
            vk::Viewport viewport(0.0f, 0.0f, static_cast<float>(rs.width), static_cast<float>(rs.height), 0.0f, 1.0f);
            commandBuffer->setViewport(0, 1, &viewport);

            // color attachment layout transition
            letc::laconic::transitionImageLayout(
                *commandBuffer, rs.swapchainImage, vk::ImageAspectFlagBits::eColor, vk::ImageLayout::eUndefined,
                vk::ImageLayout::eColorAttachmentOptimal, vk::AccessFlags{}, vk::AccessFlagBits::eColorAttachmentWrite,
                vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput);
            // depth attachment layout transition (XR depth image)
            letc::laconic::transitionImageLayout(
                *commandBuffer, rs.depthImage, vk::ImageAspectFlagBits::eDepth,
                vk::ImageLayout::eUndefined, // Use xrDepthImage
                vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::AccessFlags{},
                vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eEarlyFragmentTests);

            vk::RenderingInfo renderingInfo{};
            renderingInfo.setRenderArea({{0, 0}, {rs.width, rs.height}});
            renderingInfo.setLayerCount(1);

            vk::RenderingAttachmentInfo colorAttachment{};
            colorAttachment.setImageView(rs.swapchainImageView);
            colorAttachment.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);
            colorAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
            colorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
            colorAttachment.setClearValue(
                vk::ClearValue{}.setColor(vk::ClearColorValue{}.setFloat32({0.1176f, 0.1176f, 0.1804f, 1.0f})));

            vk::RenderingAttachmentInfo depthAttachment{};
            depthAttachment.setImageView(rs.depthImageView);
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

            auto sets =
                descriptorManager->organizeSets("pbrGlobalSet", "pbrModelSet", std::format("soloCamera{}", state));
            commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, modelRenderer->pipeline->layout, 0,
                                              sets.size(), sets.data(), 0, nullptr);

            for (uint32_t i = 0; i < models.size(); ++i)
            {
                commandBuffer->pushConstants(modelRenderer->pipeline->layout, vk::ShaderStageFlagBits::eAllGraphics, 0,
                                             sizeof(uint32_t), &i);
                models[i].draw(*commandBuffer);
            }

            if (shouldShowFrames)
            {
                pointsRenderer->pipeline->bind(commandBuffer.get());
                sets = descriptorManager->organizeSets(std::format("soloCamera{}", state));
                commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pointsRenderer->pipeline->layout, 0,
                                                  sets.size(), sets.data(), 0, nullptr);
                points->draw(*commandBuffer);

                frenetRenderer->pipeline->bind(commandBuffer.get());
                frenetFrames->draw(*commandBuffer);
            }

            commandBuffer->endRendering();

            // color attachment layout transition
            letc::laconic::transitionImageLayout(
                *commandBuffer, rs.swapchainImage, vk::ImageAspectFlagBits::eColor,
                vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
                vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eMemoryRead,
                vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe);

            commandBuffer->end();
        }

        assertThrow(device->device.resetFences(1, &commandFences.at(0).get()) == vk::Result::eSuccess,
                    "failed to reset command fence");

        std::array<vk::CommandBuffer, 2> cmdBuffPack = {commandBuffers[0].get(), commandBuffers[1].get()};
        queue.submit(vk::SubmitInfo{}.setCommandBuffers(cmdBuffPack), commandFences.at(0).get());
        assertThrow(device->device.waitForFences(1, &commandFences.at(0).get(), VK_TRUE, 5000000000) ==
                        vk::Result::eSuccess,
                    "failed to wait for command fence");

        xrCtx.endRendering();
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
