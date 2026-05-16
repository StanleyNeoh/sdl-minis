add_executable(tcp tcp.cpp)
add_executable(client client.cpp)

if(WIN32)
	target_link_libraries(tcp PRIVATE ws2_32)
	target_link_libraries(client PRIVATE ws2_32)
endif()
