add_executable(tcp tcp.cpp)
add_executable(client client.cpp)

if(WIN32)
	target_link_libraries(tcp PRIVATE ws2_32)
	target_link_libraries(client PRIVATE ws2_32)
endif()

sdl_minis_link_common(tcp)
sdl_minis_link_common(client)
