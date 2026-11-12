#include "input.hh"

void Input::init(Window &window)
{
  m_handle = window.getHandle();

  glfwGetCursorPos(m_handle, &xpos, &ypos);
  lastXpos  = xpos;
  lastYpos  = ypos;
  deltaXpos = 0.0;
  deltaYpos = 0.0;
}

void Input::visibleCursor(bool value) { m_isMouseVisible = value; }

void Input::lockCursor(bool value) { m_isMouseCaptured = value; }

bool Input::isKeyHeld(Key key) { return glfwGetKey(m_handle, static_cast<int>(key)) == GLFW_PRESS; }

glm::vec2 Input::getMouseLocation() { return glm::vec2(xpos, -ypos); }

glm::vec2 Input::getMouseDelta() { return glm::vec2(deltaXpos, -deltaYpos); }

void Input::update()
{
  int mode;
  if (m_isMouseCaptured && !m_isMouseVisible)
    mode = GLFW_CURSOR_DISABLED;
  else if (!m_isMouseVisible)
    mode = GLFW_CURSOR_HIDDEN;
  else if (m_isMouseCaptured)
    mode = GLFW_CURSOR_CAPTURED;
  else
    mode = GLFW_CURSOR_NORMAL;

  if (mode != m_currentCursorMode)
  {
    glfwSetInputMode(m_handle, GLFW_CURSOR, mode);
    m_currentCursorMode = mode;

    if (glfwRawMouseMotionSupported())
      glfwSetInputMode(m_handle, GLFW_RAW_MOUSE_MOTION,
                       mode == GLFW_CURSOR_DISABLED ? GLFW_TRUE : GLFW_FALSE);

    glfwGetCursorPos(m_handle, &xpos, &ypos);
    lastXpos = xpos;
    lastYpos = ypos;
  }

  glfwGetCursorPos(m_handle, &xpos, &ypos);

  deltaXpos = xpos - lastXpos;
  deltaYpos = ypos - lastYpos;

  lastXpos = xpos;
  lastYpos = ypos;
}
