# Clear inherited global link_libraries (SDL2, imgui, etc.) so they don't leak into test targets
set_directory_properties(PROPERTIES LINK_LIBRARIES "")

add_executable(test_particlebox
    test_vec.cpp
    test_particle.cpp
    test_particlebox.cpp
)
target_link_libraries(test_particlebox PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(test_particlebox)
