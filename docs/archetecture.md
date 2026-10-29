## How It's Built

**Languages**
- Rust for data manipulation, e.g. ECS and game logic.
- C++ for rendering and heavy code, e.g. Vulkan backend, input and shader data.
- Lua for UI management and creation (limited to 60 FPS, or 16.67 ms).

**Language Boundaries**
C++ is the intermediary: it provides the glue (bindings) and the core API that both Lua and Rust use.
Rust and Lua never interact with each other directly, which avoids unnecessary problems.
Rust is also the handler of CPU threads.

**Render**
Rendering uses triple buffering. Threading is not handled on the C++ side, so the API user (Rust) must call the threading functions C++ exposes.

**Reasoning**
C++ is for speed and performance, Rust for type safety, memory management and thread management, Lua for ease of use.
