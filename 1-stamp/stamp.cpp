#include <SDL.h>
#include <stdio.h>
#include <vector>

#ifndef BLOB_DIR
#define BLOB_DIR "./"
#endif

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

SDL_Window *gWindow = NULL;
SDL_Surface *gScreenSurface = NULL;
SDL_Surface *gHelloWorld = NULL;

bool init()
{
	bool success = true;
	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		// Create window
		gWindow = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (gWindow == NULL)
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

bool loadMedia()
{
	// Loading success flag
	bool success = true;
	gHelloWorld = SDL_LoadBMP(BLOB_DIR "bitmaps/hello_world.bmp"); // relative to build dir
	if (gHelloWorld == NULL)
	{
		printf("Failed to load media!\n");
		success = false;
	}
	return success;
}

void close()
{
	// Free loaded image
	SDL_FreeSurface(gHelloWorld);

	// Destroy window
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;
	// Quit SDL subsystems
	SDL_Quit();
}

struct StampManager
{
	bool delete_mode = false;
	SDL_Surface *stamp_surface = NULL;
	SDL_Surface *d_stamp_surface = NULL;
	std::vector<SDL_Point> stamps_poss;
	int w;
	int h;

	StampManager(const char *stamp_path, int w = 50, int h = 50) : w(w), h(h)
	{
		stamp_surface = SDL_LoadBMP(stamp_path);
		if (stamp_surface == NULL)
		{
			printf("Failed to load stamp media!\n");
			return;
		}
		d_stamp_surface = SDL_ConvertSurface(stamp_surface, stamp_surface->format, 0);
		SDL_SetSurfaceColorMod(d_stamp_surface, 255, 1, 1);
	}

	~StampManager()
	{
		SDL_FreeSurface(stamp_surface);
		SDL_FreeSurface(d_stamp_surface);
	}

	void add_stamp(int x, int y)
	{
		stamps_poss.push_back(SDL_Point{x, y});
	}

	void pop_stamp(int x, int y)
	{
		int n = stamps_poss.size();
		int r2 = (w * w + h * h) / 4;
		int i = n - 1;
		for (; i >= 0; i--)
		{
			auto &pos = stamps_poss[i];
			const int dx = pos.x - x;
			const int dy = pos.y - y;
			if (dx * dx + dy * dy <= r2)
				break;
		}

		if (i < 0)
			return;
		for (; i < n - 1; i++)
		{
			stamps_poss[i] = stamps_poss[i + 1];
		}
		stamps_poss.pop_back();
	}

	void click(int x, int y)
	{
		if (delete_mode)
		{
			pop_stamp(x, y);
		}
		else
		{
			add_stamp(x, y);
		}
	}

	void draw(SDL_Surface *surface, int mouse_x, int mouse_y)
	{
		SDL_Rect rect;
		for (auto &p : stamps_poss)
		{
			rect.x = p.x - w / 2;
			rect.y = p.y - h / 2;
			rect.w = w;
			rect.h = h;
			SDL_BlitScaled(stamp_surface, NULL, surface, &rect);
		}
		rect.x = mouse_x - w / 2;
		rect.y = mouse_y - h / 2;
		rect.w = w;
		rect.h = h;
		if (delete_mode)
		{
			SDL_BlitScaled(d_stamp_surface, NULL, surface, &rect);
		}
		else
		{
			SDL_BlitScaled(stamp_surface, NULL, surface, &rect);
		}
	}
};

int main(int argc, char *args[])
{
	if (!init())
	{
		printf("Failed to initialize!\n");
		return -1;
	}

	if (!loadMedia())
	{
		printf("Failed to load media!\n");
		return -1;
	}

	StampManager stamps(BLOB_DIR "bitmaps/mouse_shadow.bmp");
	SDL_BlitSurface(gHelloWorld, NULL, gScreenSurface, NULL);
	SDL_UpdateWindowSurface(gWindow);

	bool draw = false;
	SDL_Event e;
	SDL_Point mouse_pos;
	bool quit = false;
	while (quit == false)
	{
		while (SDL_PollEvent(&e))
		{
			draw = false;
			switch (e.type)
			{
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_MOUSEMOTION:
				mouse_pos.x = e.motion.x;
				mouse_pos.y = e.motion.y;
				draw = true;
				break;
			case SDL_MOUSEBUTTONUP:
				stamps.click(mouse_pos.x, mouse_pos.y);
				draw = true;
			case SDL_KEYDOWN:
				if (e.key.keysym.sym == SDLK_z && !stamps.delete_mode)
				{
					stamps.delete_mode = true;
					draw = true;
				}
				break;
			case SDL_KEYUP:
				if (e.key.keysym.sym == SDLK_z && stamps.delete_mode)
				{
					stamps.delete_mode = false;
					draw = true;
				}
				break;
			}
			if (draw)
			{
				SDL_BlitSurface(gHelloWorld, NULL, gScreenSurface, NULL);
				stamps.draw(gScreenSurface, mouse_pos.x, mouse_pos.y);
				SDL_UpdateWindowSurface(gWindow);
			}
		}
		SDL_Delay(10);
	}

	close();

	return 0;
}
