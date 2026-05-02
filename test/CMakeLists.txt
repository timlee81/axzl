add_executable(testAxzlCore testMutex.cpp)
target_link_libraries(testAxzlCore PRIVATE axzl_core)

enable_testing()
add_test(NAME AxzlCoreTest COMMAND testAxzlCore)
