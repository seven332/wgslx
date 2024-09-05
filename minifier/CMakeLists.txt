add_library(minifier src/minifier.cpp)
target_link_libraries(minifier PRIVATE tint_api)
