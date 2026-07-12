enable_testing()
add_executable(painkiller_tests
    test_math.cpp
)
target_link_libraries(painkiller_tests PRIVATE painkiller_engine)
add_test(NAME MathTests COMMAND painkiller_tests)
