#include "engine.hh"
#include "src/platform/input.hh"

int main()
{
  Engine engine;
  engine.init();
  while (engine.running())
  {
    if (engine.input.isKeyHeld(Key::Escape))
    {
      engine.close();
    }
    engine.render();
  }
  engine.destroy();
}
