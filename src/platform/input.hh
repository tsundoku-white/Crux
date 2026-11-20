#pragma once

#include "src/core/pch.hh"
#include "src/platform/window.hh"

enum class Key
{
  A = 65,
  B = 66,
  C = 67,
  D = 68,
  E = 69,
  F = 70,
  G = 71,
  H = 72,
  I = 73,
  J = 74,
  K = 75,
  L = 76,
  M = 77,
  N = 78,
  O = 79,
  P = 80,
  Q = 81,
  R = 82,
  S = 83,
  T = 84,
  U = 85,
  V = 86,
  W = 87,
  X = 88,
  Y = 89,
  Z = 90,

  NUM0 = 48,
  NUM1 = 49,
  NUM2 = 50,
  NUM3 = 51,
  NUM4 = 52,
  NUM5 = 53,
  NUM6 = 54,
  NUM7 = 55,
  NUM8 = 56,
  NUM9 = 57,

  ESCAPE      = 256,
  ENTER       = 257,
  TAB         = 258,
  BACKSPACE   = 259,
  INSERT      = 260,
  DELETE      = 261,
  RIGHT       = 262,
  LEFT        = 263,
  DOWN        = 264,
  UP          = 265,
  PAGEUP      = 266,
  PAGEDOWN    = 267,
  HOME        = 268,
  END         = 269,
  CAPSLOCK    = 280,
  SCROLLLOCK  = 281,
  NUMLOCK     = 282,
  PRINTSCREEN = 283,
  PAUSE       = 284,

  F1  = 290,
  F2  = 291,
  F3  = 292,
  F4  = 293,
  F5  = 294,
  F6  = 295,
  F7  = 296,
  F8  = 297,
  F9  = 298,
  F10 = 299,
  F11 = 300,
  F12 = 301,

  LEFTSHIFT    = 340,
  LEFTCONTROL  = 341,
  LEFTALT      = 342,
  LEFTSUPER    = 343,
  RIGHTSHIFT   = 344,
  RIGHTCONTROL = 345,
  RIGHTALT     = 346,
  RIGHTSUPER   = 347,
  SPACE        = 32,
  APOSTROPHE   = 39,
  COMMA        = 44,
  MINUS        = 45,
  PERIOD       = 46,
  SLASH        = 47,
  SEMICOLON    = 59,
  EQUAL        = 61,
  LEFTBRACKET  = 91,
  BACKSLASH    = 92,
  RIGHTBRACKET = 93,
  GRAVEACCENT  = 96,
};

class Input
{
private:
  GLFWwindow *m_handle = nullptr;

  int  m_currentCursorMode = GLFW_CURSOR_NORMAL;
  bool m_isMouseCaptured   = false;
  bool m_isMouseVisible    = true;

  double xpos, ypos;
  double lastXpos, lastYpos;
  double deltaXpos, deltaYpos;

public:
  // -- functions ---------
  void init(Window &window);

  void visibleCursor(bool value);
  void lockCursor(bool value);
  bool isKeyPressed(Key key);
  bool isKeyHeld(Key key);

  glm::vec2 getMouseLocation();
  glm::vec2 getMouseDelta();
  void      update();
};
