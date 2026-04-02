# Clear inherited global link_libraries (SDL2, imgui, etc.) so they don't leak into gtest targets
set_directory_properties(PROPERTIES LINK_LIBRARIES "")

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(test_particlebox
    test_vec.cpp
    test_particle.cpp
)
target_link_libraries(test_particlebox PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(test_particlebox)
