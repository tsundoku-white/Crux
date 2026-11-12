cmake_minimum_required(VERSION 3.26)
project(Crux VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependency options
set(GLFW_BUILD_DOCS         OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS        OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL            OFF CACHE BOOL "" FORCE)
set(VMA_BUILD_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(VMA_BUILD_SAMPLES       OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS         OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL       OFF CACHE BOOL "" FORCE)

include(FetchContent)

# git fetch content 
FetchContent_Declare(
  VulkanHeaders
  SYSTEM
  GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
  GIT_TAG        v1.4.367
  GIT_SHALLOW    TRUE
)

FetchContent_Declare(
  GLFW
  SYSTEM
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.5.1
  GIT_SHALLOW    TRUE
)

FetchContent_Declare(
  GLM
  SYSTEM
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        1.0.3
  GIT_SHALLOW    TRUE
)

FetchContent_Declare(
  VKB
  SYSTEM
  GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap.git
  GIT_TAG        v1.4.367
  GIT_SHALLOW    TRUE
)

FetchContent_Declare(
  VMA
  SYSTEM
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
  GIT_TAG        v3.5.0
  GIT_SHALLOW    TRUE
)

FetchContent_Declare(
  CGLTF
  SYSTEM
  GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
  GIT_TAG        v1.15
  GIT_SHALLOW    TRUE
  SOURCE_SUBDIR  nonexistent
)

FetchContent_Declare(
  STB
  SYSTEM
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG        4e9b368bfc071e97206ea5c6adba68dd0cbf8897
  SOURCE_SUBDIR  nonexistent
)

FetchContent_Declare(
  MINIAUDIO
  SYSTEM
  GIT_REPOSITORY https://github.com/mackron/miniaudio.git
  GIT_TAG        0.11.26
  GIT_SHALLOW    TRUE
  SOURCE_SUBDIR  nonexistent
)

FetchContent_MakeAvailable(
  VulkanHeaders
  GLFW
  GLM
  VKB
  VMA
  CGLTF
  STB
  MINIAUDIO
)

find_package(Vulkan REQUIRED)
find_package(Threads REQUIRED)

# Executable
add_executable(Crux
  src/main.cc
  src/engine.cc

  src/platform/window.cc
  src/platform/input.cc

  src/vulkan/context.cc
  src/vulkan/pipeline.cc
  src/vulkan/command.cc
  src/vulkan/descriptor.cc
  src/vulkan/buffer.cc
  src/vulkan/render.cc
  src/vulkan/image.cc

  src/resource/resource_manager.cc

  ext/vma_impl.cc
  ext/cgltf_impl.cc
  ext/stb_impl.cc
  ext/miniaudio_impl.cc
)

target_include_directories(Crux SYSTEM PRIVATE
  ${cgltf_SOURCE_DIR}
  ${stb_SOURCE_DIR}
  ${miniaudio_SOURCE_DIR}
)

# Lets you write #include "src/..." from anywhere
target_include_directories(Crux PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Crux PRIVATE
  Vulkan::Headers
  Vulkan::Vulkan
  glfw
  glm::glm
  vk-bootstrap::vk-bootstrap
  GPUOpen::VulkanMemoryAllocator
  Threads::Threads
  ${CMAKE_DL_LIBS}
)

target_compile_definitions(Crux PRIVATE
  GLFW_INCLUDE_NONE             # don't pull in GL/gl.h (Vulkan-only engine)
  GLFW_INCLUDE_VULKAN           # still expose glfwCreateWindowSurface etc.
  GLM_FORCE_DEPTH_ZERO_TO_ONE   # Vulkan clip-space depth is 0..1, not -1..1
)

target_precompile_headers(Crux PRIVATE pch.hh)

target_compile_options(Crux PRIVATE
  -fdiagnostics-show-template-tree
  -fno-elide-type
  -fdiagnostics-color=always
  -Wall -Wextra
  $<$<CXX_COMPILER_ID:GNU>:-fmax-errors=5>
  $<$<CXX_COMPILER_ID:Clang>:-ferror-limit=5>
)

set_source_files_properties(ext/vma_impl.cc PROPERTIES
  COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:Clang>:-Wno-nullability-completeness>"
)

# Shaders (Slang -> SPIR-V)
set(SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders")
set(SHADER_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets/shaders")
file(MAKE_DIRECTORY ${SHADER_OUT_DIR})

file(GLOB SLANG_SOURCES CONFIGURE_DEPENDS "${SHADER_SRC_DIR}/*.slang")

set(SPV_OUTPUTS "")
foreach(SRC ${SLANG_SOURCES})
  get_filename_component(NAME ${SRC} NAME_WE)
  set(OUT "${SHADER_OUT_DIR}/${NAME}.spv")

  add_custom_command(
    OUTPUT  ${OUT}
    COMMAND slangc ${SRC} -target spirv -o ${OUT}
    DEPENDS ${SRC}              # only rebuild the shader that changed
    COMMENT "Compiling ${NAME}.slang -> ${NAME}.spv"
    VERBATIM
  )
  list(APPEND SPV_OUTPUTS ${OUT})
endforeach()

add_custom_target(CruxShaders DEPENDS ${SPV_OUTPUTS})
add_dependencies(Crux CruxShaders)

target_compile_definitions(Crux PRIVATE
  SHADER_DIR="${SHADER_OUT_DIR}/"
  MODEL_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/models/"
)
