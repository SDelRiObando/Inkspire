add_executable(test_dud)
target_sources(test_dud PRIVATE test_dud.cpp)
add_test(NAME test_dud COMMAND "$<TARGET_FILE:test_dud>")
