add_library(
    minifier
    src/minifier.cpp
    src/rename_identifiers.cpp
    src/remove_useless.cpp
    src/traverser.cpp
)
target_compile_options(
    minifier
    PRIVATE -fno-rtti -Wall -Wextra -Wno-missing-field-initializers -Werror
)
target_include_directories(minifier PUBLIC include PRIVATE src)
target_link_libraries(minifier PUBLIC tint_api PRIVATE range-v3)

add_executable(minifier_test src/minifier_test.cpp)
target_link_libraries(minifier_test PRIVATE minifier gmock_main)
