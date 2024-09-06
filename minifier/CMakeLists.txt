add_library(minifier src/minifier.cpp)
target_include_directories(minifier PUBLIC include PRIVATE src)
target_link_libraries(minifier PRIVATE tint_api range-v3)

add_executable(minifier_test src/minifier_test.cpp)
target_link_libraries(minifier_test PRIVATE minifier gtest_main)
