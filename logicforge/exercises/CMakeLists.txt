# Learner exercises ("koans"). Each ex_*.cpp is its own test executable.
# They FAIL until you implement the TODOs. That is the point.
#
#   cmake -S core -B build -DFORGE_EXERCISES=ON && cmake --build build
#   ctest --test-dir build -R ex_ --output-on-failure
file(GLOB FORGE_EXERCISE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/ex_*.cpp)
foreach(src ${FORGE_EXERCISE_SOURCES})
    get_filename_component(name ${src} NAME_WE)
    add_executable(${name} ${src})
    target_link_libraries(${name} PRIVATE forge_doctest_main)
    target_compile_features(${name} PRIVATE cxx_std_20)
    if(FORGE_SANITIZE AND NOT MSVC)
        # Sanitizers turn silent pointer bugs into loud, explained crashes.
        target_compile_options(${name} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${name} PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME ${name} COMMAND ${name})
endforeach()
