#pragma once

#include "letc/pch.hh"

namespace letc
{
    class Window;

    class WindowBuilder
    {
      public:
        WindowBuilder();

        auto setWidth(const std::size_t &width) -> WindowBuilder &;
        auto setHeight(const std::size_t &height) -> WindowBuilder &;
        auto setTitle(const std::string &title) -> WindowBuilder &;

        auto build() -> std::shared_ptr<Window>;

      private:
        std::size_t m_width;
        std::size_t m_height;
        std::string m_title;
    };

    class Window
    {
      public:
        ~Window();
        auto get() -> vkfw::Window;

      private:
        friend WindowBuilder;
        WindowBuilder m_windowBuilder;

        vkfw::Window m_handle;
    };
} // namespace letc
