cmake_minimum_required(VERSION 3.18)
project(voxelspire LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(fizmo
    GIT_REPOSITORY https://github.com/ThatNewCoderJSCpp/Fizmo.git
    GIT_TAG        v0.2.3
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  Fizmo)
FetchContent_MakeAvailable(fizmo)

file(GLOB_RECURSE VOXELSPIRE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp")

add_executable(main main.cpp ${VOXELSPIRE_SOURCES})
target_compile_features(main PRIVATE cxx_std_17)
target_include_directories(main PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Include")
target_link_libraries(main PRIVATE fizmo::fizmo)
target_precompile_headers(main PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Include/fizmo.hpp")
if(MINGW)
    target_compile_options(main PRIVATE -Wa,-mbig-obj)
elseif(MSVC)
    target_compile_options(main PRIVATE /bigobj /utf-8)
endif()
