add_library(
    minifier
    src/minifier.cpp
    src/rename_identifiers.cpp
    src/remove_useless_functions.cpp
    src/traverser.cpp
)
target_compile_options(minifier PRIVATE "-fno-exceptions" "-fno-rtti")
target_include_directories(minifier PUBLIC include PRIVATE src)
target_link_libraries(minifier PRIVATE tint_api range-v3)

add_executable(minifier_test src/minifier_test.cpp)
target_link_libraries(minifier_test PRIVATE minifier gmock_main)
