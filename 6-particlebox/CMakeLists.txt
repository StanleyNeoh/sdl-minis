add_executable(particlebox main.cpp utils.cpp)

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
	target_link_libraries(particlebox PRIVATE OpenMP::OpenMP_CXX)
endif()
