add_executable(painkiller_demo
    main.cpp
    DemoApp.h DemoApp.cpp
)
target_link_libraries(painkiller_demo PRIVATE painkiller_engine)
if(WIN32)
    set_target_properties(painkiller_demo PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
