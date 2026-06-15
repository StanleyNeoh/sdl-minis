add_executable(p2p-pong main.cpp)

target_include_directories(p2p-pong PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/lib
)

sdl_minis_link_common(p2p-pong)