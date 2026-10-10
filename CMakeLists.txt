cmake_minimum_required(VERSION 3.18)
project(voxelspire VERSION 0.1.0 LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(fizmo
    GIT_REPOSITORY https://github.com/ThatNewCoderJSCpp/Fizmo.git
    GIT_TAG        v0.2.7
    GIT_SHALLOW    TRUE)
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

option(VOXELSPIRE_PACKAGE "Build Voxelspire as a standalone game for players" OFF)
if(VOXELSPIRE_PACKAGE)
    target_compile_definitions(main PRIVATE VOXELSPIRE_PORTABLE)
    if(WIN32)
        set_target_properties(main PROPERTIES WIN32_EXECUTABLE ON)
        if(MINGW)
            target_link_options(main PRIVATE -static)
        endif()
        set(VOXELSPIRE_PLATFORM windows)
        set(CPACK_GENERATOR ZIP)
    else()
        target_link_options(main PRIVATE -static-libstdc++ -static-libgcc)
        set(VOXELSPIRE_PLATFORM linux-x86_64)
        set(CPACK_GENERATOR TGZ)
    endif()

    install(PROGRAMS $<TARGET_FILE:main> DESTINATION . RENAME Voxelspire${CMAKE_EXECUTABLE_SUFFIX})

    set(CPACK_PACKAGE_NAME Voxelspire)
    set(CPACK_PACKAGE_VENDOR ThatNewCoderJSCpp)
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_PACKAGE_FILE_NAME Voxelspire-${PROJECT_VERSION}-${VOXELSPIRE_PLATFORM})
    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
    set(CPACK_STRIP_FILES ON)
    include(CPack)
endif()
