//Using SDL and standard IO
#include <SDL.h>
#include <stdio.h>

//Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

SDL_Window* gWindow = NULL;
SDL_Surface* gScreenSurface = NULL;
SDL_Surface* gHelloWorld = NULL;

bool init() {
	bool success = true;
	//Initialize SDL
	if(SDL_Init( SDL_INIT_VIDEO ) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		//Create window
		gWindow = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if( gWindow == NULL )
		{
			printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
			success = false;
		}
		else
		{
			gScreenSurface = SDL_GetWindowSurface(gWindow);
		}
	}
	return success;
}

bool loadMedia() {
	//Loading success flag
	bool success = true;
	gHelloWorld = SDL_LoadBMP("blob/bitmaps/hello_world.bmp");
	if (gHelloWorld == NULL) {
		printf("Failed to load media!\n");
		success = false;
	}
	return success;
}

void close()  {
	// Free loaded image
	SDL_FreeSurface( gHelloWorld );

	//Destroy window
	SDL_DestroyWindow( gWindow );
	gWindow = NULL;
	//Quit SDL subsystems
	SDL_Quit();
}

int main(int argc, char* args[])
{
	if (!init()) {
		printf("Failed to initialize!\n");
		return -1;
	}


	if (!loadMedia()) {
		printf("Failed to load media!\n");
		return -1;
	}
	printf("Media loaded successfully!\n");

	SDL_BlitSurface( gHelloWorld, NULL, gScreenSurface, NULL );
	SDL_UpdateWindowSurface( gWindow );

	
	//Hack to get window to stay up
	SDL_Event e;
	bool quit = false;
	while( quit == false ){
		while( SDL_PollEvent( &e ) ){
			if( e.type == SDL_QUIT ) quit = true; 
		} 
	}

	close();

	return 0;
}
