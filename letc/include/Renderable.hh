#pragma once

#include "pch.hh"

#include "Buffer.hh"

/*
    A renderable is anything that interacts with any type of vulkan
        draw call.
    It's easy to manage the vertex inputs, since it's inherent 
        to a renderable, however how do I deal with the uniforms
        that a renderable might need
    It is easy to just say its not the job of the renderable to 
        manage the "Material" that is applied on it, since thats
        the job of the pipeline/shader to do the shading.
        However, not all renderables have all the vertex inputs needed
        to be used with a certain pipeline/shaders. For example, a 
        renderable might not have tangent attributes, since not every
        renderable needs to use it. It might not have UV's cause it doesn't
        have textures to use.
    Where does the boundry lie between how to couple/decouple data for a renderable,
        material, resource binding, and shaders/pipelines
*/

namespace letc
{
    struct IRenderable
    {
        virtual void draw(const vk::CommandBuffer &commandBuffer) = 0;

        virtual ~IRenderable() = default;
    };
} // namespace letc
