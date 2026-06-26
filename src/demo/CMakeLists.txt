add_executable(nebula_demo
    main.cpp
    DemoApp.h DemoApp.cpp
)
target_link_libraries(nebula_demo PRIVATE nebula_engine)
if(WIN32)
    set_target_properties(nebula_demo PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
