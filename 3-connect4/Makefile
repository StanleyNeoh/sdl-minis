
CURRENT_DIR := $(shell pwd)


#OBJS specifies which files to compile as part of the project
OBJS = main.cpp

#OBJ_NAME specifies the name of our exectuable
OBJ_NAME = main

#CC specifies which compiler we're using
CC = g++ -std=c++17 -g

#INCLUDE_PATHS specifies the additional include paths we'll need
INCLUDE_PATHS = -I"$(CURRENT_DIR)/../vendored/SDL2/include/SDL2"

#LIBRARY_PATHS specifies the additional library paths we'll need
LIBRARY_PATHS = -L"$(CURRENT_DIR)/../vendored/SDL2/lib"

#COMPILER_FLAGS specifies the additional compilation options we're using
# -w suppresses all warnings
# -Wl,-subsystem,windows gets rid of the console window
COMPILER_FLAGS = -std=c++17

#LINKER_FLAGS specifies the libraries we're linking against
LINKER_FLAGS = -lmingw32 -lSDL2main -lSDL2


#This is the target that compiles our executable
all: $(OBJ_NAME)

$(OBJ_NAME): $(OBJS)
	$(CC) $(OBJS) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)
