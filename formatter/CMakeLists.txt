add_library(formatter src/formatter.cpp src/mini_printer.cpp)
target_compile_options(formatter PRIVATE "-fno-exceptions" "-fno-rtti")
target_include_directories(formatter PUBLIC include PRIVATE src)
target_link_libraries(formatter PUBLIC tint_api PRIVATE range-v3)

add_executable(formatter_test src/formatter_test.cpp)
target_link_libraries(formatter_test PRIVATE formatter gmock_main range-v3)
