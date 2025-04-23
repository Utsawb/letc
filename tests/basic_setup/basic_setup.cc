#include "letc/core/instance.hh"
#include "letc/core/window.hh"

auto main(int argc, char *argv[]) -> int
{
    auto window = letc::WindowBuilder{}.build();
    auto instance = letc::InstanceBuilder{}.addExtension(vk::KHRSurfaceExtensionName).build();

    return 0;
}
