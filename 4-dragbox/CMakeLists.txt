file(GLOB DRAGBOX_SOURCES "lib/**/*.cpp")
add_executable(dragbox main.cpp ${DRAGBOX_SOURCES})
sdl_minis_link_common(dragbox)
