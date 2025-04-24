#include "letc/letc.hh"

auto main(int argc, char *argv[]) -> int
{
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

    return 0;
}
