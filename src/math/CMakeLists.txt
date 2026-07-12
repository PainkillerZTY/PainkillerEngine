add_library(painkiller_math OBJECT
    Vector.h
    Matrix.h
    Quaternion.h
    Transform.h
    Math.h
    Math.cpp
)
target_include_directories(painkiller_math PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(painkiller_math PUBLIC painkiller_core glm)
