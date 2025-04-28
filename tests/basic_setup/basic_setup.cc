#include "letc/letc.hh"

std::filesystem::path resourcePath = "../resources";

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
    auto graphicsQueue = device->getQueue("graphics").get().at(0);
    auto computeQueues = device->getQueue("compute");
    auto swapchain = letc::SwapchainBuilder{}.build(window, instance, device);
    auto format = swapchain->getFormat().format;
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
            .addVertexBinding(2, sizeof(glm::vec4), vk::VertexInputRate::eVertex)
            .addVertexAttribute(2, 2, vk::Format::eR32G32B32A32Sfloat, 0)
            .addVertexBinding(3, sizeof(glm::vec2), vk::VertexInputRate::eVertex)
            .addVertexAttribute(3, 3, vk::Format::eR32G32Sfloat, 0)
            .setRendering([format](vk::PipelineRenderingCreateInfo &prci) { prci.setColorAttachmentFormats(format); })
            .build(device);

    return 0;
}
