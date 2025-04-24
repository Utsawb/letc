#include "window.hh"
#include <vulkan/vulkan_handles.hpp>

namespace letc
{
    WindowBuilder::WindowBuilder()
    {
        m_width = 1024;
        m_height = 1024;
        m_title = "dev";
    }

    auto WindowBuilder::setWidth(const std::size_t &width) -> WindowBuilder &
    {
        m_width = width;
        return *this;
    }
    auto WindowBuilder::setHeight(const std::size_t &height) -> WindowBuilder &
    {
        m_height = height;
        return *this;
    }
    auto WindowBuilder::setTitle(const std::string &title) -> WindowBuilder &
    {
        m_title = title;
        return *this;
    }

    auto WindowBuilder::build() -> std::shared_ptr<Window>
    {
        vkfw::init();

        auto window = std::make_shared<Window>();
        window->m_windowBuilder = *this;
        window->m_handle = vkfw::createWindow(m_width, m_height, m_title.c_str());

        return window;
    }

    Window::~Window()
    {
        m_handle.destroy();
    }

    auto Window::get() -> vkfw::Window
    {
        return m_handle;
    }

    auto Window::createSurface(std::weak_ptr<Instance> instance) -> vk::SurfaceKHR
    {
        return vkfw::createWindowSurface(instance.lock()->get(), m_handle);
    }

} // namespace letc
