add_executable(cmd src/main.cpp)
target_compile_options(cmd PRIVATE ${WGSLX_COMPILE_OPTIONS})
target_link_libraries(cmd PRIVATE minifier writer tint_utils_cli nlohmann_json)
set_target_properties(cmd PROPERTIES OUTPUT_NAME "wgslx")

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    target_link_options(
        cmd
        PRIVATE
            -sENVIRONMENT=node
            -sALLOW_MEMORY_GROWTH=1
            -sINITIAL_MEMORY=32MB
            -sDISABLE_EXCEPTION_CATCHING=1
            -sEXIT_RUNTIME=0
            -sSTACK_SIZE=1MB
            -lnodefs.js
            -lnoderawfs.js
    )
endif()
