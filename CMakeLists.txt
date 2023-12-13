project(Extism-Miniaudio)
cmake_minimum_required(VERSION 3.19)

option(EXTISM_CPP_BUILD_IN_TREE "Set to ON to build with submodule deps" ON)
if(EXTISM_CPP_BUILD_IN_TREE)
  message("EXTISM_CPP_BUILD_IN_TREE (Extism-Miniaudio): using deps from parent directory")
  add_subdirectory(third_party/cpp-sdk)
else()
  find_package(extism-cpp)
  if(extism-cpp_FOUND)
    message("Found extism-cpp")
  else()
    message("Did not find extism-cpp, using FetchContent")
    include(FetchContent)
    FetchContent_Declare(
      extism-cpp
      GIT_REPOSITORY https://github.com/extism/cpp-sdk.git
      GIT_TAG main
    )
    FetchContent_MakeAvailable(extism-cpp)
  endif()
endif()

# build the plugin and convert it to a C header
add_subdirectory(wasm-src)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/miniaudio_decoder_extism_wasm.h
  COMMAND xxd -i plugin.wasm ${CMAKE_CURRENT_BINARY_DIR}/miniaudio_decoder_extism_wasm.h
  DEPENDS plugin
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/wasm-src
)

# Declare the library, only headers
add_library(extism-miniaudio INTERFACE src/miniaudio_decoder_extism.h ${CMAKE_CURRENT_BINARY_DIR}/miniaudio_decoder_extism_wasm.h)
target_include_directories(extism-miniaudio INTERFACE third_party/miniaudio src ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(extism-miniaudio INTERFACE extism-cpp)

# build a downloader plugin, convert it to a C header, and turn it into a library
add_subdirectory(player/wasm-src)
# commented until we can load a plugin from bytes and a manifest at the same time
# add_custom_command(
#   OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/player/downloader.h
#   COMMAND xxd -i downloader.wasm ${CMAKE_CURRENT_BINARY_DIR}/player/downloader.h
#   DEPENDS downloader
#   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/player/wasm-src
# )
# add_library(extism-downloader INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/player/downloader.h)
# target_include_directories(extism-downloader INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/player)

# Use the plugins
add_executable(extism-miniaudio-player player/main.cpp)
target_include_directories(extism-miniaudio-player PRIVATE third_party/miniaudio)
target_link_libraries(extism-miniaudio-player PRIVATE extism-cpp extism-miniaudio)
