cmake_minimum_required(VERSION 3.26)
project(Crux VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(GLFW_BUILD_DOCS         OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS        OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL            OFF CACHE BOOL "" FORCE)
set(VMA_BUILD_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(VMA_BUILD_SAMPLES       OFF CACHE BOOL "" FORCE)

find_package(Vulkan REQUIRED)

include(FetchContent)

# SYSTEM: treat downloaded libs as system headers so their warnings are hidden
FetchContent_Declare(
  GLFW
  SYSTEM
  SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/glfw
  DOWNLOAD_COMMAND
    ${CMAKE_COMMAND} -E rm -rf ${FETCHCONTENT_BASE_DIR}/glfw
    COMMAND git clone --filter=blob:none --no-checkout --depth 1
            --branch 3.5.1 https://github.com/glfw/glfw.git
            ${FETCHCONTENT_BASE_DIR}/glfw
    COMMAND git -C ${FETCHCONTENT_BASE_DIR}/glfw sparse-checkout init --cone
    COMMAND git -C ${FETCHCONTENT_BASE_DIR}/glfw sparse-checkout set
            include src CMake deps
    COMMAND git -C ${FETCHCONTENT_BASE_DIR}/glfw checkout 3.5.1
)

FetchContent_Declare(
  VKB
  SYSTEM
  GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
  GIT_TAG        main
)

FetchContent_Declare(
  VMA
  SYSTEM
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
  GIT_TAG        master
)

FetchContent_Declare(
  CGLTF
  SYSTEM
  GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
  GIT_TAG        master
  SOURCE_SUBDIR  nonexistent
)

FetchContent_MakeAvailable(GLFW VKB VMA CGLTF)

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

  ext/vma_impl.cc
  ext/cgltf_impl.cc
)

target_include_directories(Crux SYSTEM PRIVATE ${cgltf_SOURCE_DIR})

target_include_directories(Crux PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Crux PRIVATE
  Vulkan::Vulkan
  glfw vk-bootstrap::vk-bootstrap
  GPUOpen::VulkanMemoryAllocator
  ${CMAKE_DL_LIBS}
)

# Readable diagnostics (works on both GCC and Clang)
target_compile_options(Crux PRIVATE
  -fdiagnostics-show-template-tree
  -fno-elide-type
  -fdiagnostics-color=always
  -Wall -Wextra
  $<$<CXX_COMPILER_ID:GNU>:-fmax-errors=5>
  $<$<CXX_COMPILER_ID:Clang>:-ferror-limit=5>
)

# Fallback: silence VMA's nullability warnings in the file that compiles it
set_source_files_properties(ext/vma_impl.cc PROPERTIES
  COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:Clang>:-Wno-nullability-completeness>"
)

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
    DEPENDS ${SLANG_SOURCES}
    COMMENT "Compiling ${NAME}.slang -> ${NAME}.spv"
    VERBATIM
  )
  list(APPEND SPV_OUTPUTS ${OUT})
endforeach()

add_custom_target(CruxShaders DEPENDS ${SPV_OUTPUTS})
add_dependencies(Crux CruxShaders)

target_compile_definitions(Crux PRIVATE
  SHADER_DIR="${SHADER_OUT_DIR}/"
)
