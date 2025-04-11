#pragma once

#include "pch.hh"

#include "Buffer.hh"

/*
    Lets think this through, what type of renderables can I have?
        - Simple models, with non indexed rendering, ie just vertices in a list
        - Indexed models, with index buffers and vertex buffers, saves on vertices
        - Instanced Indexed models,
*/

namespace letc
{
    class IRenderable
    {
      public:
        virtual void bindBuffers(const vk::CommandBuffer &commandBuffer) = 0;

        virtual void draw(const vk::CommandBuffer &commandBuffer) = 0;

        virtual ~IRenderable() = default;
    };
} // namespace letc
