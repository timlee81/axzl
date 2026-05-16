add_executable(testAxzlCore
    testMutex.cpp
)

target_link_libraries(testAxzlCore PRIVATE
    axzl_core
)

add_test(NAME AxzlCoreTest COMMAND
    testAxzlCore
)
