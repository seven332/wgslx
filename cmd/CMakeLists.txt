add_executable(cmd src/main.cpp)
target_compile_options(cmd PRIVATE "-fno-exceptions" "-fno-rtti")
target_link_libraries(cmd PRIVATE minifier writer tint_utils_cli)
set_target_properties(cmd PROPERTIES OUTPUT_NAME "wgslx")
