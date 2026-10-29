#include "input.hh"

void Input::init(Window &window)
{
  m_handle = window.getHandle();
}

void Input::visibleCursor(bool value)
{
  m_isMouseVisible = value;
}

void Input::lockCursor(bool value)
{
  m_isMouseCaptured = value;
}

bool Input::isKeyHeld(Key key)
{
  return glfwGetKey(m_handle, static_cast<int>(key) == GLFW_PRESS);
}

void Input::update()
{
  int mode;
  if (!m_isMouseVisible)          mode = GLFW_CURSOR_HIDDEN;
  else if (m_isMouseCaptured && !m_isMouseVisible)     mode = GLFW_CURSOR_DISABLED;
  else if (m_isMouseCaptured)     mode = GLFW_CURSOR_CAPTURED;
  else                            mode = GLFW_CURSOR_NORMAL;

  if (mode != m_currentCursorMode) {
    glfwSetInputMode(m_handle, GLFW_CURSOR, mode);
    m_currentCursorMode = mode;
  }

}

