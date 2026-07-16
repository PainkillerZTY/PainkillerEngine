# GLM (Header-only math library)
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/glm)

# stb_image
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/stb)

# GLFW
set(GLFW_BUILD_DIR ${CMAKE_BINARY_DIR}/../build_deps/glfw_build)
if(EXISTS ${GLFW_BUILD_DIR})
    add_library(glfw STATIC IMPORTED)
    set_target_properties(glfw PROPERTIES
        IMPORTED_LOCATION ${GLFW_BUILD_DIR}/src/libglfw3.a
        INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include
    )
    if(WIN32)
        set_property(TARGET glfw APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES "opengl32;gdi32;user32"
        )
    endif()
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(GLFW IMPORTED_TARGET glfw3)
        if(GLFW_FOUND)
            add_library(glfw ALIAS PkgConfig::GLFW)
        endif()
    endif()
endif()
