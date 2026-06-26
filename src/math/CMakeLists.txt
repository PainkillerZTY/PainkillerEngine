add_library(nebula_math OBJECT
    Vector.h
    Matrix.h
    Quaternion.h
    Transform.h
    Math.h
)
target_include_directories(nebula_math PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(nebula_math PUBLIC glm)
