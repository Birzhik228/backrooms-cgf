cmake_minimum_required(VERSION 3.21)
project(BackroomsCGF VERSION 0.12.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
include(CTest)
option(BACKROOMS_GRAPHICS_TESTS "Build tests that need a working OpenGL 3.3 context" OFF)
option(BACKROOMS_STATIC_MINGW "Link the Windows GLFW and compiler runtimes statically" ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(Threads REQUIRED)

# Compiler settings and reusable libraries are shared by the game and tests.
add_library(backrooms_options INTERFACE)
target_include_directories(backrooms_options INTERFACE "${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/include")
if(MSVC)
  target_compile_options(backrooms_options INTERFACE /W4)
else()
  target_compile_options(backrooms_options INTERFACE -Wall -Wextra "$<$<CONFIG:Release>:-O2>")
endif()
if(MINGW AND BACKROOMS_STATIC_MINGW)
  target_link_options(backrooms_options INTERFACE -static)
  # The MSYS2 glfw CMake target points at its DLL import library. Select the
  # actual static archive explicitly; -static alone does not override that path.
  get_filename_component(BACKROOMS_COMPILER_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
  get_filename_component(BACKROOMS_TOOLCHAIN_ROOT "${BACKROOMS_COMPILER_BIN}" DIRECTORY)
  find_library(BACKROOMS_GLFW_ARCHIVE NAMES libglfw3.a
    PATHS "${BACKROOMS_TOOLCHAIN_ROOT}/lib" NO_DEFAULT_PATH REQUIRED)
  add_library(backrooms_glfw STATIC IMPORTED)
  set_target_properties(backrooms_glfw PROPERTIES IMPORTED_LOCATION "${BACKROOMS_GLFW_ARCHIVE}")
  get_target_property(BACKROOMS_GLFW_INCLUDES glfw INTERFACE_INCLUDE_DIRECTORIES)
  target_include_directories(backrooms_glfw INTERFACE ${BACKROOMS_GLFW_INCLUDES})
  target_link_libraries(backrooms_glfw INTERFACE gdi32)
else()
  add_library(backrooms_glfw INTERFACE)
  target_link_libraries(backrooms_glfw INTERFACE glfw)
endif()

function(backrooms_library target)
  add_library(${target} STATIC ${ARGN})
  target_link_libraries(${target} PUBLIC backrooms_options)
endfunction()

backrooms_library(backrooms_world world.cpp)
backrooms_library(backrooms_threat common/threat.cpp)
target_link_libraries(backrooms_threat PUBLIC backrooms_world)
backrooms_library(backrooms_model common/threat_model.cpp)
backrooms_library(backrooms_audio common/audio.cpp)
target_link_libraries(backrooms_audio PUBLIC Threads::Threads)
backrooms_library(backrooms_music common/music.cpp)
if(WIN32)
  target_link_libraries(backrooms_audio PUBLIC winmm)
  target_link_libraries(backrooms_music PUBLIC winmm)
endif()
backrooms_library(backrooms_gl_loader common/glad.c)
target_link_libraries(backrooms_gl_loader PUBLIC ${CMAKE_DL_LIBS})
backrooms_library(backrooms_shaders common/Shader.cpp)
target_link_libraries(backrooms_shaders PUBLIC backrooms_gl_loader)
backrooms_library(backrooms_render common/ui.cpp common/textures.cpp)
target_link_libraries(backrooms_render PUBLIC backrooms_shaders backrooms_glfw OpenGL::GL)
backrooms_library(backrooms_app_support app/render_resources.cpp app/diagnostics.cpp)
target_link_libraries(backrooms_app_support PUBLIC backrooms_render Threads::Threads)

file(GLOB_RECURSE BACKROOMS_APP_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/app/*.cpp")
list(REMOVE_ITEM BACKROOMS_APP_SOURCES
  "${PROJECT_SOURCE_DIR}/app/render_resources.cpp" "${PROJECT_SOURCE_DIR}/app/diagnostics.cpp")
add_executable(Backrooms backrooms.cpp ${BACKROOMS_APP_SOURCES})
target_link_libraries(Backrooms PRIVATE backrooms_world backrooms_threat backrooms_model
  backrooms_audio backrooms_music backrooms_app_support)
if(WIN32)
  target_link_libraries(Backrooms PRIVATE psapi)
  set_target_properties(Backrooms PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# This target also runs for an incremental build: edited shaders/config/assets
# must reach the runnable copy even when no C++ file needed recompiling.
set(BACKROOMS_RUNTIME_FILES
  vshader_backrooms.glsl fshader_backrooms.glsl vshader_lens.glsl fshader_lens.glsl
  vshader_shadow.glsl fshader_shadow.glsl fshader_ao.glsl jumpscare.cfg balance.cfg)
add_custom_target(backrooms_runtime
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Backrooms>"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/assets/audio" "$<TARGET_FILE_DIR:Backrooms>/assets/audio"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/assets/models" "$<TARGET_FILE_DIR:Backrooms>/assets/models"
  VERBATIM)
foreach(runtime_file IN LISTS BACKROOMS_RUNTIME_FILES)
  add_custom_command(TARGET backrooms_runtime POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PROJECT_SOURCE_DIR}/${runtime_file}" "$<TARGET_FILE_DIR:Backrooms>/${runtime_file}"
    VERBATIM)
endforeach()
add_dependencies(Backrooms backrooms_runtime)
# Player launchers keep the familiar executable beside the source assets.
add_custom_target(backrooms_deploy
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:Backrooms>" "${PROJECT_SOURCE_DIR}/$<TARGET_FILE_NAME:Backrooms>"
  DEPENDS Backrooms VERBATIM)

if(BUILD_TESTING)
  add_custom_target(native_tests)
  function(backrooms_test target test_name)
    cmake_parse_arguments(PARSE_ARGV 2 TEST "" "LABEL" "LIBRARIES;SOURCES")
    if(NOT TEST_LABEL)
      set(TEST_LABEL native)
    endif()
    add_executable(${target} "tests/${target}.cpp" ${TEST_SOURCES})
    target_link_libraries(${target} PRIVATE backrooms_options ${TEST_LIBRARIES})
    if(MSVC)
      target_compile_options(${target} PRIVATE /UNDEBUG)
    else()
      target_compile_options(${target} PRIVATE -UNDEBUG)
    endif()
    add_test(NAME ${test_name} COMMAND ${target})
    set_tests_properties(${test_name} PROPERTIES
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}" LABELS "${TEST_LABEL}" TIMEOUT 900)
    if(TEST_LABEL MATCHES "native")
      add_dependencies(native_tests ${target})
    else()
      add_dependencies(graphics_tests_build ${target})
    endif()
  endfunction()

  backrooms_test(flow_tests application_flow)
  backrooms_test(options_tests diagnostic_options SOURCES app/options.cpp)
  backrooms_test(balance_tests balance_configuration LIBRARIES backrooms_threat)
  add_test(NAME balance_invalid_startup COMMAND balance_tests --invalid-startup)
  add_test(NAME balance_missing_startup COMMAND balance_tests --missing-startup)
  set_tests_properties(balance_invalid_startup balance_missing_startup PROPERTIES
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}" LABELS native TIMEOUT 60)
  backrooms_test(performance_tests performance_statistics)
  backrooms_test(gl_resource_tests graphics_resource_ownership LIBRARIES backrooms_gl_loader)
  backrooms_test(level0_landmark_tests level0_landmark_invariants LIBRARIES backrooms_world)
  backrooms_test(presence_tests presence_invariants LIBRARIES backrooms_world)
  backrooms_test(atmosphere_tests atmosphere_invariants LIBRARIES backrooms_world)
  backrooms_test(parking_tests parking_invariants LIBRARIES backrooms_world)
  backrooms_test(parking_detail_tests parking_detail_invariants LIBRARIES backrooms_world)
  backrooms_test(partygoer_tests partygoer_invariants LIBRARIES backrooms_world)
  backrooms_test(fun_tests fun_invariants LIBRARIES backrooms_world)
  backrooms_test(fun_detail_tests fun_detail_invariants LIBRARIES backrooms_world)
  backrooms_test(fun_audio_tests fun_audio_invariants LIBRARIES backrooms_world backrooms_audio)
  backrooms_test(parking_audio_tests parking_audio_invariants LIBRARIES backrooms_world backrooms_audio)
  backrooms_test(progression_tests progression_invariants LIBRARIES backrooms_threat)
  backrooms_test(world_tests world_invariants LIBRARIES backrooms_world)
  backrooms_test(streaming_tests streaming_invariants LIBRARIES backrooms_world)
  backrooms_test(movement_tests movement_invariants)
  backrooms_test(threat_tests threat_invariants LIBRARIES backrooms_threat)
  backrooms_test(threat_model_tests threat_model_invariants LIBRARIES backrooms_model)
  backrooms_test(encounter_tests encounter_invariants)
  backrooms_test(audio_tests audio_invariants LIBRARIES backrooms_audio)
  backrooms_test(ending_tests ending_timeline)
  backrooms_test(ending_audio_tests ending_audio LIBRARIES backrooms_audio)
  if(WIN32)
    # This suite deliberately uses the real Windows audio device. Keep it in
    # local native checks; headless hosted CI can explicitly exclude its label.
    backrooms_test(music_tests music_lifecycle LABEL "native;audio-device" LIBRARIES backrooms_music)
  endif()

  if(BACKROOMS_GRAPHICS_TESTS)
    add_custom_target(graphics_tests_build)
    backrooms_test(graphics_tests graphics_occlusion LABEL graphics LIBRARIES backrooms_shaders backrooms_glfw OpenGL::GL)
    backrooms_test(lens_noise_tests lens_noise LABEL graphics LIBRARIES backrooms_shaders backrooms_glfw OpenGL::GL)
    backrooms_test(ending_lighting_tests ending_sunlight LABEL graphics LIBRARIES backrooms_shaders backrooms_glfw OpenGL::GL)
    backrooms_test(gl_lifetime_tests graphics_resource_lifetime LABEL graphics LIBRARIES backrooms_app_support)
  endif()
endif()
