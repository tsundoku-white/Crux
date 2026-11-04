#include "engine.hh"

int main()
{
  Engine engine;
  engine.init();
  engine.input.visibleCursor(false);
  engine.input.lockCursor(true);

  Entity freeCam = engine.registery.create()
                       .add(Camera {.fov = 110.f})
                       .add(Transform {.location = {0, 0, 7}});

  Entity Model = engine.registery.create()
                     .add(Mesh {})
                     .add(Transform {});

  constexpr float moveSpeed   = 5.f;      // units per second
  constexpr float sensitivity = 0.0025f;  
  constexpr float maxPitch    = glm::radians(89.f);

  float yaw   = 0.f;
  float pitch = 0.f;

  float i = 0;
  while (engine.running())
  {
    if (engine.input.isKeyHeld(Key::ESCAPE))
    {
      engine.close();
    }

    Transform &cam = freeCam.get<Transform>();

    glm::vec3 dir(0.f);
    if (engine.input.isKeyHeld(Key::I))
      dir += cam.forward;
    if (engine.input.isKeyHeld(Key::K))
      dir -= cam.forward;
    if (engine.input.isKeyHeld(Key::L))
      dir += cam.right;
    if (engine.input.isKeyHeld(Key::J))
      dir -= cam.right;

    if (engine.input.isKeyHeld(Key::SPACE))
      dir += cam.up;
    if (engine.input.isKeyHeld(Key::SLASH))
      dir -= cam.up;

    if (glm::dot(dir, dir) > 0.f)
      cam.location += glm::normalize(dir) * moveSpeed * engine.deltaTime;

    // Look: yaw about WORLD up, pitch about the camera's LOCAL right.
    glm::vec2 mouse_dt = engine.input.getMouseDelta();
    yaw   -= mouse_dt.x * sensitivity;
    pitch  = glm::clamp(pitch + mouse_dt.y * sensitivity, -maxPitch, maxPitch);
    cam.rotation = glm::angleAxis(yaw, glm::vec3(0, 1, 0)) *
                   glm::angleAxis(pitch, glm::vec3(1, 0, 0));

    engine.render();
    i += engine.deltaTime;
    if (i >= 2)
    {
      std::print("FPS: {}\n", 1.f / engine.deltaTime);
      i = 0;
    }
  }
  engine.destroy();
}
