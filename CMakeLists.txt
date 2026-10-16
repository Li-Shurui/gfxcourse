cmake_minimum_required(VERSION 3.20)

# 一门按课次推进的 C++23 软件渲染器 / 光线追踪课程。
# 每一课都是独立可执行程序，并且只能看到截至该课的头文件。
project(gfxcourse
  VERSION 1.0.0
  DESCRIPTION "Step-by-step C++23 software rasterizer and path tracer"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Debug or Release" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo)
endif()

file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" GFX_ROOT_FWD)

set(GFX_INCLUDES "")
set(GFX_SOURCES "")

enable_testing()

# number  : 两位课号，也是可执行文件名 lessonXX
# folder  : lessons/ 下的目录
# ARGN    : 传给该课测试的命令行参数（例如 --preview）
function(gfx_add_lesson number folder)
  list(APPEND GFX_INCLUDES "${CMAKE_SOURCE_DIR}/lessons/${folder}")
  file(GLOB extra CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/lessons/${folder}/*.cpp")
  list(FILTER extra EXCLUDE REGEX "main\\.cpp$")
  list(APPEND GFX_SOURCES ${extra})

  set(target "lesson${number}")
  add_executable(${target}
    "${CMAKE_SOURCE_DIR}/lessons/${folder}/main.cpp"
    ${GFX_SOURCES})
  target_include_directories(${target} PRIVATE ${GFX_INCLUDES})
  target_compile_features(${target} PRIVATE cxx_std_23)
  target_compile_definitions(${target} PRIVATE "GFX_ROOT=\"${GFX_ROOT_FWD}\"")

  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  add_test(NAME ${target} COMMAND ${target} ${ARGN})
  set_tests_properties(${target} PROPERTIES TIMEOUT 180)

  set(GFX_INCLUDES "${GFX_INCLUDES}" PARENT_SCOPE)
  set(GFX_SOURCES "${GFX_SOURCES}" PARENT_SCOPE)
endfunction()

gfx_add_lesson(00 00_image)
gfx_add_lesson(01 01_vectors)
gfx_add_lesson(02 02_lines)
gfx_add_lesson(03 03_triangles)
gfx_add_lesson(04 04_matrices)
gfx_add_lesson(05 05_pipeline)
gfx_add_lesson(06 06_depth)
gfx_add_lesson(07 07_shading)
gfx_add_lesson(08 08_texture)
gfx_add_lesson(09 09_mesh)
gfx_add_lesson(10 10_raster_scene)
gfx_add_lesson(11 11_rays)
gfx_add_lesson(12 12_materials)
gfx_add_lesson(13 13_mesh_ray)
gfx_add_lesson(14 14_bvh --preview)
gfx_add_lesson(15 15_path --preview)
gfx_add_lesson(16 16_capstone --preview)

add_executable(make_assets tools/make_assets.cpp)
target_compile_features(make_assets PRIVATE cxx_std_23)
if(MSVC)
  target_compile_options(make_assets PRIVATE /W4 /permissive- /utf-8)
else()
  target_compile_options(make_assets PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_compile_definitions(make_assets PRIVATE "GFX_ROOT=\"${GFX_ROOT_FWD}\"")
