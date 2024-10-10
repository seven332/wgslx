add_library(writer src/writer.cpp src/mini_printer.cpp src/operator_group.cpp)
target_compile_options(writer PRIVATE ${WGSLX_COMPILE_OPTIONS})
target_include_directories(writer PUBLIC include PRIVATE src)
target_link_libraries(writer PUBLIC tint_api PRIVATE range-v3)

add_executable(writer_test src/writer_test.cpp)
target_link_libraries(writer_test PRIVATE writer gmock_main range-v3)
