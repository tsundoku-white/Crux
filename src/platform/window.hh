#pragma once

#include "src/core/pch.hh"
#include <cstdint>

// rename window -> platform
// and handle -> window

class Window
{
  private:
    GLFWwindow *m_handle  = nullptr;
    uint32_t    m_width   = 900;
    uint32_t    m_height  = 900;
    std::string m_title   = "Crux";

    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);

  public:
    // -- functions ----------
    void init();
    void destroy();
    bool shouldClose();
    void setShouldClose(bool value);
    void pollEvents();

    // -- public exposed varables ----------
    bool frameResized = false;

    // -- getters ----------
    GLFWwindow *getHandle() const { return m_handle; }
};
