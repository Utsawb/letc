#pragma once

#include <vector>

#include "openxr.hpp"

namespace letc
{
    /*
     *  For now this will be a very, very, hard coded interface, just enough
     *  to get me through my class project. I would love to support it more,
     *  but my interests lie more on the Vulkan side. Maybe once I am content
     *  with the Vulkan interface I can come back and give this some love.
     */
    struct XrContext
    {
        xr::DispatchLoaderDynamic disp;
        xr::UniqueDynamicInstance instance;
        xr::SystemId systemId;

        xr::UniqueDynamicSession session;
        std::vector<xr::UniqueDynamicSwapchain> swapchains;
        
        xr::Time predictedDisplayTime;

        xr::UniqueDynamicSpace leftHandSpace;
        xr::UniqueDynamicSpace rightHandSpace;

        xr::Path leftHandPath;
        xr::Path rightHandPath;
    };

}; // namespace letc
