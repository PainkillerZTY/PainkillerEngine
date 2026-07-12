 add_library(painkiller_core OBJECT
     Types.h
     Logger.h Logger.cpp
 )
 target_include_directories(painkiller_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
 target_link_libraries(painkiller_core PUBLIC glm)
