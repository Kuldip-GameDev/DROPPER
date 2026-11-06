cmake_minimum_required(VERSION 3.20)
project(logicforge_core VERSION 0.1.0 LANGUAGES CXX)

# When the core is built on its own (desktop), build tests and tools too.
# When it is pulled in by the Android build, only the library is built.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(FORGE_TOP_LEVEL ON)
else()
    set(FORGE_TOP_LEVEL OFF)
endif()

option(FORGE_BUILD_TESTS "Build unit tests" ${FORGE_TOP_LEVEL})
option(FORGE_BUILD_TOOLS "Build forge_cli" ${FORGE_TOP_LEVEL})
option(FORGE_EXERCISES "Build the learner exercises (red until solved)" OFF)
option(FORGE_SANITIZE "Enable AddressSanitizer + UndefinedBehaviorSanitizer" OFF)
option(FORGE_WERROR "Treat warnings as errors" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE FORGE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_library(forge_core STATIC ${FORGE_SOURCES})
target_compile_features(forge_core PUBLIC cxx_std_20)
target_include_directories(forge_core
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
target_compile_definitions(forge_core PRIVATE FORGE_VERSION="${PROJECT_VERSION}")

# Warnings are the compiler doing free code review.
if(MSVC)
    target_compile_options(forge_core PRIVATE /W4 $<$<BOOL:${FORGE_WERROR}>:/WX>)
else()
    target_compile_options(forge_core PRIVATE -Wall -Wextra -Wpedantic -Wshadow
        -Wnon-virtual-dtor -Woverloaded-virtual $<$<BOOL:${FORGE_WERROR}>:-Werror>)
endif()

if(FORGE_SANITIZE AND NOT MSVC)
    set(FORGE_SAN_FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
    target_compile_options(forge_core PUBLIC ${FORGE_SAN_FLAGS})
    target_link_options(forge_core PUBLIC ${FORGE_SAN_FLAGS})
endif()

if(FORGE_BUILD_TOOLS)
    add_executable(forge_cli tools/forge_cli.cpp)
    target_link_libraries(forge_cli PRIVATE forge_core)
endif()

if(FORGE_BUILD_TESTS OR FORGE_EXERCISES)
    enable_testing()
    add_library(forge_doctest_main STATIC tests/doctest_main.cpp)
    target_include_directories(forge_doctest_main SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
    target_compile_features(forge_doctest_main PUBLIC cxx_std_20)
endif()

if(FORGE_BUILD_TESTS)
    file(GLOB FORGE_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.cpp)
    add_executable(forge_tests ${FORGE_TEST_SOURCES})
    target_link_libraries(forge_tests PRIVATE forge_core forge_doctest_main)
    add_test(NAME forge_tests COMMAND forge_tests)
endif()

if(FORGE_EXERCISES)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../exercises ${CMAKE_BINARY_DIR}/exercises)
endif()
