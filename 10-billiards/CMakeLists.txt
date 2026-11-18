file(GLOB_RECURSE SOURCES 
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" 
	"${CMAKE_CURRENT_SOURCE_DIR}/apps/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/apps/connect4/*.cpp"
)

add_executable(billiards ${SOURCES})
sdl_minis_link_common(billiards)

target_compile_options(billiards PRIVATE -g)