add_executable(raytracer main.cpp)

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
	target_link_libraries(raytracer PRIVATE OpenMP::OpenMP_CXX)
endif()

sdl_minis_link_common(raytracer)
