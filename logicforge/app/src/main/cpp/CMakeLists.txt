cmake_minimum_required(VERSION 3.20)
project(logicforge_jni LANGUAGES CXX)

# Pull in the portable core. Tests and tools are switched off automatically
# because the core is not the top-level project here.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../../core ${CMAKE_BINARY_DIR}/core)

add_library(logicforge SHARED jni_bridge.cpp)
target_link_libraries(logicforge PRIVATE forge_core log)
# Only export the JNI entry points; keeps the .so small.
set_target_properties(logicforge PROPERTIES CXX_VISIBILITY_PRESET hidden)
# Strip unused code and support 16 KB memory pages (required on newer Android devices).
target_link_options(logicforge PRIVATE -Wl,--gc-sections -Wl,-z,max-page-size=16384)
