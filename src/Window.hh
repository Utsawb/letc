#pragma once
#ifndef LETC_WINDOW_HH
#define LETC_WINDOW_HH

#include "pch.hh"

namespace letc
{
    struct WindowBuilder
    {
        std::size_t width;
        std::size_t height;
        std::string title;
        vkfw::WindowHints hints;
        vkfw::Monitor monitor;
        vkfw::Window share;

        WindowBuilder()
        {
            width = 1024;
            height = 1024;
            title = "Debug";
            hints = {};
            monitor = nullptr;
            share = nullptr;
        }

        WindowBuilder &setWidth(const std::size_t &width)
        {
            this->width = width;
            return *this;
        }

        WindowBuilder &setHeight(const std::size_t &height)
        {
            this->height = height;
            return *this;
        }

        WindowBuilder &setTitle(const std::string &title)
        {
            this->title = title;
            return *this;
        }

        WindowBuilder &setHints(const vkfw::WindowHints &hints)
        {
            this->hints = hints;
            return *this;
        }

        WindowBuilder &setMonitor(const vkfw::Monitor &monitor)
        {
            this->monitor = monitor;
            return *this;
        }

        WindowBuilder &setShare(const vkfw::Window &share)
        {
            this->share = share;
            return *this;
        }
    };
}; // namespace letc

// this is cursed don't do this, I just really
// want the interface to fit so I can stop refactoring
namespace vkfw
{
     vkfw::UniqueWindow createWindowUnique(const letc::WindowBuilder &builder)
     {
         return vkfw::createWindowUnique(builder.width, builder.height, builder.title.c_str(), builder.hints, builder.monitor, builder.share);
     }
}

#endif // LETC_WINDOW_HH
