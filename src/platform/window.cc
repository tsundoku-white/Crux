#include "window.hh"
#include "GLFW/glfw3.h"

void Window::framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
  Window *win = static_cast<Window *>(glfwGetWindowUserPointer(window));
  if (win)
  {
    win->m_width      = width;
    win->m_height     = height;
    win->frameResized = true;
  }
}

void Window::init()
{
  // init glfw.
  if (!glfwInit())
  {
    throw std::runtime_error("failed to init glfw\n");
  }

  // glfw dont load opengl.
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  // creating a window.
  m_handle = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);

  // check if window is create.
  if (!m_handle)
  {
    throw std::runtime_error("failed to create window context");
  }

  // add class in glfw frame callback.
  glfwSetWindowUserPointer(m_handle, this);
  glfwSetFramebufferSizeCallback(m_handle, framebuffer_size_callback);

  std::print("pass: window\n");
}

void Window::destroy()
{
  // check if m_handle is valid before destroying window.
  if (m_handle)
    glfwDestroyWindow(m_handle);
  // clear resources
  glfwTerminate();
}

// glfw pass throught functions
bool Window::shouldClose() { return glfwWindowShouldClose(m_handle); }
void Window::setShouldClose(bool value) { glfwSetWindowShouldClose(m_handle, value); }
void Window::pollEvents() { glfwPollEvents(); }
