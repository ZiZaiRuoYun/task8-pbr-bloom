cmake_minimum_required(VERSION 3.10)
project(Task2)
set(CMAKE_CXX_STANDARD 17)

find_package(OpenGL REQUIRED)

find_package(glfw3 CONFIG QUIET)
if (NOT glfw3_FOUND)
  find_package(glfw3 REQUIRED)
endif()

find_package(glad CONFIG QUIET)
if (NOT glad_FOUND)
  find_package(glad REQUIRED)
endif()

find_package(glm CONFIG QUIET)
if (NOT glm_FOUND)
  find_package(glm REQUIRED)
endif()

find_package(assimp CONFIG QUIET)
if (NOT assimp_FOUND)
  find_package(assimp REQUIRED)
endif()


add_executable(Task2
  core/Application.cpp
  core/Window.cpp
  core/Renderer.cpp
  core/Shader.cpp
  core/Mesh.cpp
  core/main.cpp
  core/GBuffer.h
  core/GBuffer.cpp

  systems/RenderSystem.cpp
  systems/CameraSystem.cpp

  assets/AsyncLoader.cpp
  assets/AssetManager.cpp
  assets/Model.cpp
  "post/effects/ToneMapping.h" "post/effects/ToneMapping.cpp" "post/PostProcessor.h" "post/PostProcessor.cpp" "post/effects/Bloom.h" "post/effects/Bloom.cpp" "profiling/Profiler.cpp" "profiling/Profiler.h")

target_include_directories(Task2 PRIVATE
  ${CMAKE_SOURCE_DIR}/core
  ${CMAKE_SOURCE_DIR}/ecs
  ${CMAKE_SOURCE_DIR}/components
  ${CMAKE_SOURCE_DIR}/systems
  ${CMAKE_SOURCE_DIR}
)

target_link_libraries(Task2 PRIVATE
  glad::glad
  glfw
  OpenGL::GL
  glm::glm
  assimp::assimp
)
