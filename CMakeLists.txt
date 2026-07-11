cmake_minimum_required(VERSION 3.20)
project(NebulaEngine VERSION 0.1.0 LANGUAGES CXX C)

# ?? C++ Standard ??
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ?? Build Options ??
option(NEBULA_BUILD_TESTS  "Build unit tests"        OFF)
option(NEBULA_BUILD_DEMO   "Build demo application"  OFF)
option(NEBULA_BUILD_GAME   "Build voxel game"        ON)
option(NEBULA_USE_OPENGL   "Use OpenGL render backend" ON)

# GLM compatibility with older compilers
add_definitions(-DGLM_FORCE_NO_CTOR_INIT)
add_definitions(-DGLM_FORCE_NO_CONSTEXPR)

# ?? Output Directories ??
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ?? Global Compiler Settings ??
if(MSVC)
    add_compile_options(/W4 /utf-8)
elseif(MINGW OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-missing-field-initializers)
endif()

# ?? Third-party Dependencies ??
add_subdirectory(third_party)

# ?? Engine Library ??
add_subdirectory(src)

# ?? Tests ??
if(NEBULA_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# ?? Demo ??
if(NEBULA_BUILD_DEMO)
    add_subdirectory(src/demo)
endif()

# ?? Game ??
if(NEBULA_BUILD_GAME)
    add_subdirectory(src/game)
endif()
