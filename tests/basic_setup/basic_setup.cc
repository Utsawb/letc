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
    auto graphicsQueue = device->getQueue("graphics");
    auto computeQueues = device->getQueue("compute");
    auto swapchain = letc::SwapchainBuilder{}.build(window, instance, device);

    // ok what is the differentiation between a resource and an asset
    // obv an asset is something you read in from disk
    // uhh but like what abt resource
    // the code for a shader would be an asset, but the shader itself
    // would be a resource?

    return 0;
}
