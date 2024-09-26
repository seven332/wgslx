add_library(parser src/parser.cpp)
target_compile_options(
    parser
    PRIVATE -fno-exceptions -fno-rtti -Wall -Wextra -Werror
)
target_include_directories(parser PUBLIC include PRIVATE src)
target_link_libraries(parser PUBLIC tint_api PRIVATE range-v3)

add_executable(parser_test src/parser_test.cpp)
target_link_libraries(parser_test PRIVATE parser gmock_main)
