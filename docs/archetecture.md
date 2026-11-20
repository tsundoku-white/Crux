# How It's Built

## Goal

This is an engine for me to make games, or anything else that needs some light graphics.

It also gives me a way to get better at C++, and I'm getting into Rust and mixing languages in one project. Since I'm undertaking the CCNA, writing the networking side will do me good.

I'll be focusing on 2D, because the overall dev time should go into the game, not a tech demo, and into the art.

The networking will be written in Rust because it helps with safety in the general sense of the word.

## The Engine

The engine is put in one main class, which is the interface of the engine. Everything the user should ever need to access should be in the `Engine` class.

```cpp
Engine engine;

engine.input   // keyboard and mouse
engine.registery   // entities and components
engine.render()    // draw a frame
engine.running()   // main loop condition
engine.close()     // request shutdown
engine.deltaTime   // seconds since the last frame
```

## Languages

The engine will be mainly in **C++23–26**. This is a learning experience for me to get more familiar with C++ as a whole.

**Rust** I'm planning to use for the networking side, which will be purely P2P (peer-to-peer).

**Lua** is used for UI. This will be limited to 60 updates per second (~16.67 ms per update) because there is no need to update more than 60 times a second.

## How the ECS Works

The ECS architecture is derived from a sparse and dense array (a sparse set) that has dynamic recycling of ID slots.

This is an example of how to create an empty entity:

```cpp
Entity Model = engine.registery.create();
```

To add components, use the `.add` keyword to add pre-built components. Components must be a struct. Calls can be chained:

```cpp
Entity freeCam = engine.registery.create()
                     .add(Camera {.fov = 110.f})
                     .add(Transform {.location = {0, 0, 7}});
```

To get a component from an entity, call `get` on it. You can declare the variable with the `auto` keyword, but use the exact type you want from the entity for readability.

```cpp
Transform &cam = freeCam.get<Transform>();
```

## Example: Free Camera

This is a trimmed version of the engine's `main.cc`. It shows the whole loop: setup, input, camera movement and rendering.

```cpp
#include "engine.hh"

int main()
{
  Engine engine;
  engine.input.visibleCursor(false);
  engine.input.lockCursor(true);

  Entity freeCam = engine.registery.create()
                       .add(Camera {.fov = 110.f})
                       .add(Transform {.location = {0, 0, 7}});

  Entity Model = engine.registery.create().add(Mesh {}).add(Transform {});

  constexpr float moveSpeed   = 5.f;
  constexpr float sensitivity = 0.0025f;
  constexpr float maxPitch    = glm::radians(89.f);

  float yaw   = 0.f;
  float pitch = 0.f;

  while (engine.running())
  {
    if (engine.input.isKeyHeld(Key::ESCAPE))
      engine.close();

    Transform &cam = freeCam.get<Transform>();

    // Movement: I/K forward/back, J/L left/right, SPACE/SLASH up/down
    glm::vec3 dir(0.f);
    if (engine.input.isKeyHeld(Key::I)) dir += cam.forward;
    if (engine.input.isKeyHeld(Key::K)) dir -= cam.forward;
    if (engine.input.isKeyHeld(Key::L)) dir += cam.right;
    if (engine.input.isKeyHeld(Key::J)) dir -= cam.right;
    if (engine.input.isKeyHeld(Key::SPACE)) dir += cam.up;
    if (engine.input.isKeyHeld(Key::SLASH)) dir -= cam.up;

    if (glm::dot(dir, dir) > 0.f)
      cam.location += glm::normalize(dir) * moveSpeed * engine.deltaTime;

    // Mouse look
    glm::vec2 mouse_dt = engine.input.getMouseDelta();
    yaw -= mouse_dt.x * sensitivity;
    pitch = glm::clamp(pitch + mouse_dt.y * sensitivity, -maxPitch, maxPitch);
    cam.rotation =
        glm::angleAxis(yaw, glm::vec3(0, 1, 0)) * glm::angleAxis(pitch, glm::vec3(1, 0, 0));

    engine.render();
  }
}
```

## How Rendering Works

The rendering side is written in C++ and Vulkan 1.4. Naturally it's fast, but as of now I have not yet added multithreading. It will be implemented by v0.5.

By default the engine uses 3 frames in flight, because GPUs are fast enough to handle 3 frames.

The renderer is designed to be a forward renderer in the future.

## Resource Manager

The Resource Manager is there to cache all models that are static in the world and to ensure that no unneeded resources are being allocated.

Responsibilities are as follows:

- Managing components (changing soon...)
- Camera projection
- Creating models and textures
- Managing draw calls

## Input

Input is only for keyboard and mouse. I have no need to add controller support at this point in the dev cycle.

The keyboard uses a `Key` enum. This list can be extended by the dev. The values match the GLFW key codes, and the enum currently covers:

- Letters `A`–`Z` and number-row digits `NUM0`–`NUM9`
- Function keys `F1`–`F12`
- Navigation and editing keys (arrows, `HOME`, `END`, `PAGEUP`, `PAGEDOWN`, `INSERT`, `DELETE`, `BACKSPACE`, `ENTER`, `TAB`, `ESCAPE`)
- Lock and system keys (`CAPSLOCK`, `SCROLLLOCK`, `NUMLOCK`, `PRINTSCREEN`, `PAUSE`)
- Modifiers (left and right `SHIFT`, `CONTROL`, `ALT`, `SUPER`)
- Punctuation (`SPACE`, `APOSTROPHE`, `COMMA`, `MINUS`, `PERIOD`, `SLASH`, `SEMICOLON`, `EQUAL`, brackets, `BACKSLASH`, `GRAVEACCENT`)

The `Input` class is used through `engine.input`:

| Function                  | Purpose                                      |
|---------------------------|----------------------------------------------|
| `visibleCursor(bool)`     | Show or hide the cursor                      |
| `lockCursor(bool)`        | Capture the mouse to the window              |
| `isKeyPressed(Key)`       | Key went down this frame                     |
| `isKeyHeld(Key)`          | Key is currently down                        |
| `getMouseLocation()`      | Current mouse position                       |
| `getMouseDelta()`         | Mouse movement since the last update         |

## Audio

> **Work in progress.** The interface below is likely to change.

Audio is handled by its own class. It can be used on its own for UI or global audio, and it can be placed in the world.

The current interface is:

```cpp
class Audio
{
public:
  void load(std::string &path);
  void play(bool once);
  void stop();
};
```

I have not yet added 3D audio, but my goal for the engine is to be 2D first and foremost.

## Targeted Specs

| ITEM       | NAME               |
|------------|--------------------|
| OS         | WINDOWS 11 & LINUX |
| CPU        | INTEL 8 CORE       |
| GPU        | NVIDIA 3060        |
| VRAM       | 8GB                |
| STORAGE    | 100GB              |
| RESOLUTION | 1920x1080          |

These were taken from Steam and modified to my personal taste.
