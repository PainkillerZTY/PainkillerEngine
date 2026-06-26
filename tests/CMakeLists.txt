enable_testing()
add_executable(nebula_tests
    test_math.cpp
)
target_link_libraries(nebula_tests PRIVATE nebula_engine)
add_test(NAME MathTests COMMAND nebula_tests)
