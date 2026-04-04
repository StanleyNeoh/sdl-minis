add_executable(raytracer main.cpp hittable.cpp ray.cpp)

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
	target_link_libraries(particlebox PRIVATE OpenMP::OpenMP_CXX)
	target_compile_definitions(particlebox PRIVATE PARTICLEBOX_USE_OPENMP=1)
endif()
