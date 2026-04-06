add_executable(raytracer main.cpp hittable.cpp ray.cpp)

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
	target_link_libraries(raytracer PRIVATE OpenMP::OpenMP_CXX)
endif()
