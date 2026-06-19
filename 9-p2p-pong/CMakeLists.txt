file(GLOB_RECURSE SOURCES 
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" 
	"${CMAKE_CURRENT_SOURCE_DIR}/lib/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/p2p/*.cpp"
)

add_executable(p2p-pong ${SOURCES})

target_include_directories(p2p-pong PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/lib
)

sdl_minis_link_common(p2p-pong)