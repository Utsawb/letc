#include <letc/letc.hh>

#include "model.hh"

std::filesystem::path resourcePath = "../resources";

struct FrameData
{
    float time;
    float frame;
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

    auto window = letc::WindowBuilder{}.build();
    auto instance = letc::InstanceBuilder{}.addExtension(vk::KHRSurfaceExtensionName).build();
    auto device = letc::DeviceBuilder{}
                      .addExtension(vk::KHRSwapchainExtensionName)
                      .requestQueues("graphics", vk::QueueFlagBits::eGraphics)
                      .requestQueues("compute", vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer, 2)
                      .setDeviceFeatures([](letc::DeviceBuilder::FeatureChain &f) {
                          f.get<vk::PhysicalDeviceVulkan13Features>().setDynamicRendering(true);
                      })
                      .build(instance);
    auto graphicsQueue = device->getQueue("graphics");
    auto computeQueues = device->getQueue("compute");
    auto swapchain = letc::SwapchainBuilder{}.build(window, instance, device);
    auto format = swapchain->getFormat().format;

    auto allocator = std::make_shared<letc::Allocator>(instance, device);
    auto descPool = std::make_shared<letc::DescriptorSetPool>(device);

    auto shaderManager = std::move(letc::ShaderManager{device}
                                       .add(resourcePath / "shaders" / "pbr" / "vert.spirv", "main")
                                       .add(resourcePath / "shaders" / "pbr" / "frag.spirv", "main"));

    auto pbrVert = shaderManager.getShader(resourcePath / "shaders" / "pbr" / "vert.spirv", "main");
    auto pbrFrag = shaderManager.getShader(resourcePath / "shaders" / "pbr" / "frag.spirv", "main");

    auto pbrPipeline =
        letc::GraphicsPipelineBuilder{}
            .addShader(pbrVert)
            .addShader(pbrFrag)
            .addVertexBinding(0, sizeof(glm::vec4), vk::VertexInputRate::eVertex)
            .addVertexAttribute(0, 0, vk::Format::eR32G32B32A32Sfloat, 0)
            .addVertexBinding(1, sizeof(glm::vec4), vk::VertexInputRate::eVertex)
            .addVertexAttribute(1, 1, vk::Format::eR32G32B32A32Sfloat, 0)
            .addVertexBinding(2, sizeof(glm::vec2), vk::VertexInputRate::eVertex)
            .addVertexAttribute(2, 2, vk::Format::eR32G32Sfloat, 0)
            .setRendering([format](vk::PipelineRenderingCreateInfo &prci) { prci.setColorAttachmentFormats(format); })
            .build(device);

    auto commandPool = device->getLogical().createCommandPoolUnique(
        vk::CommandPoolCreateInfo{}.setQueueFamilyIndex(graphicsQueue.getFamily()));

    auto commandBuffers =
        device->getLogical().allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo{}
                                                              .setCommandPool(*commandPool)
                                                              .setLevel(vk::CommandBufferLevel::ePrimary)
                                                              .setCommandBufferCount(2));

    std::array<std::shared_ptr<letc::Image>, 2> depthImages = {
        letc::laconic::depthImage(allocator, window->get().getWidth(), window->get().getHeight()),
        letc::laconic::depthImage(allocator, window->get().getWidth(), window->get().getHeight())};

    std::array<std::shared_ptr<letc::ImageView>, 2> depthImageViews = {
        letc::laconic::depthImageView(device, depthImages[0]), letc::laconic::depthImageView(device, depthImages[1])};

    auto frameBuffer =
        letc::ObjectBuffer<FrameData>(allocator, vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
    frameBuffer->frame = 0.0f;
    frameBuffer->time = 0.0f;
    frameBuffer.sync();

    auto camera =
        letc::FirstPersonCamera(allocator, (float)window->get().getWidth() / (float)window->get().getHeight());

    auto lights = letc::VectorBuffer<LightData>(allocator, 4, vk::BufferUsageFlagBits::eStorageBuffer,
                                                VMA_MEMORY_USAGE_CPU_TO_GPU);
    float s = 4.0f;
    float h = 2.0f;
    lights->at(0) = {{-s, h, -s, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}};
    lights->at(1) = {{-s, h, s, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}};
    lights->at(2) = {{s, h, s, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}};
    lights->at(3) = {{s, h, -s, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}};

    auto frameSet = pbrPipeline->getSetLayouts()[0].build(descPool);
    frameSet->attachBuffer("UFrameData", frameBuffer.get(), frameBuffer.containedSize());
    frameSet->attachBuffer("UCamera", camera.getBuffer(), camera.containedSize());

    auto modelLayout = std::make_shared<letc::DescriptorSetLayout>(pbrPipeline->getSetLayouts()[1]);
    auto models = loadModels(resourcePath / "models" / "sponza" / "Sponza.gltf", modelLayout, descPool, allocator);

    // while (window->get().shouldClose() == false)
    // {
    //     vkfw::pollEvents();
    // }

    return 0;
}
