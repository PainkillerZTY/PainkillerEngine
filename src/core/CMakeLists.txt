 add_library(nebula_core OBJECT
     Types.h
     Logger.h Logger.cpp
 )
 target_include_directories(nebula_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
 target_link_libraries(nebula_core PUBLIC glm)
