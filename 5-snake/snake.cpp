#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <stdio.h>
#include <SDL.h>
#include <deque>
#include <iostream>
#include <vector>
#include <array>
#include <random>


int get_rand_int(int l, int r) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> uniform_dist(l, r);
    return uniform_dist(gen);
}

void NewStaticCenterFrame() {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x * 0.5f, viewport->Size.y * 0.5f));
}

enum class GameState {
    menu = 0,
    game_paused,
    game_running,
    game_over,
};

template <int N, typename T = int>
struct Vec {
    std::array<T, N> vec;

    Vec(T val = 0) {
        for (int i = 0; i < N; i++) vec[i] = val;
    }

    Vec(std::initializer_list<int> vals) {
        int i = 0;
        for (int x: vals) vec[i++] = x;
    };

    T& operator[](std::size_t idx) {
        return vec[idx];
    }

    const T& operator[](std::size_t idx) const {
        return vec[idx];
    }

    Vec<N, T> operator-() {
        Vec<N, T> ret;
        for (int i = 0; i < N; i++) ret[i] = -vec[i];
        return ret;
    }

    Vec<N, T>& operator+=(const Vec<N, T> other) {
        for (int i = 0; i < N; i++) vec[i] += other[i];
        return *this;
    }

    Vec<N, T>& operator-=(const Vec<N, T> other) {
        for (int i = 0; i < N; i++) vec[i] -= other[i];
        return *this;
    }
};

template <int N, typename T>
Vec<N, T> operator+(const Vec<N, T>& a, const Vec<N, T>& b) {
    Vec<N, T> ret;
    for (int i = 0; i < N; i++) ret[i] = a[i] + b[i];
    return ret;
};

template <int N, typename T>
Vec<N, T> operator-(const Vec<N, T>& a, const Vec<N, T>& b) {
    Vec<N, T> ret;
    for (int i = 0; i < N; i++) ret[i] = a[i] - b[i];
    return ret;
};


using Vec2 = Vec<2, int>;

struct SnakeGame {
    enum Cell {
        EMPTY = 0,
        SNAKE = 1,
        APPLE = 2
    };

    int N = 0; 
    int M = 0;
    int L = 0;
    int cell_size = 5;
    int win_w = 0, win_h = 0;
    std::vector<Cell> grid;
    std::deque<Vec2> body;
    Vec2 dir{1, 0};
    SDL_Texture* tex = NULL;

    ~SnakeGame() {
        if (tex) {
            SDL_DestroyTexture(tex);
            tex = NULL;
        }
    }

    void set_settings(int N, int M, int L, int cell_size) {
        this->N = N;
        this->M = M;
        this->L = std::min(L, N / 2);
        this->cell_size = cell_size;
        this->win_h = N * cell_size;
        this->win_w = M * cell_size;
        grid.assign(N * M, EMPTY);
        body.clear();
        Vec2 curr{N / 2, M / 2};
        for (int i = 0; i < L; i++) {
            body.push_back(curr);
            grid_set(curr, SNAKE);
            curr -= dir;
        }
    }

    bool init_tex(SDL_Renderer* renderer) {
        if (tex != NULL) return true;
        tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, win_w, win_h);
        return tex != NULL;
    }

    SDL_Texture* render(SDL_Renderer* renderer) {
        SDL_SetRenderTarget(renderer, tex);
        SDL_SetRenderDrawColor(renderer, 0, 128, 0, 255); // Transparent if blend mode allows
        SDL_RenderClear(renderer);
        for (int r = 0; r < N; r++) {
            for (int c = 0; c < M; c++) {
                int i = M * r + c;
                if (grid[i] == EMPTY) continue;
                if (grid[i] == SNAKE) {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Transparent if blend mode allows
                } else {
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Transparent if blend mode allows
                }
                SDL_Rect rect{r * cell_size, c * cell_size, cell_size, cell_size};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
        SDL_SetRenderTarget(renderer, nullptr);
        return tex;
    }

    bool in_bound(Vec2 pos) {
        return (pos[0] < N && pos[0] >= 0 && pos[1] < M && pos[1] >= 0);
    }

    Cell* grid_get(Vec2 pos) {
        if (!in_bound(pos)) return nullptr;
        int i = M * pos[0] + pos[1];
        return &grid[i];
    }

    bool grid_set(Vec2 pos, Cell val) {
        Cell* ptr = grid_get(pos);
        if (ptr == nullptr) return false;
        *ptr = val;
        return true;
    }

    bool grid_assert(Vec2 pos, Cell val) {
        Cell* ptr = grid_get(pos);
        if (ptr == nullptr) return false;
        return *ptr == val;
    }

    bool step() {
        Vec2 next_pos = body.front() + dir;
        if (!in_bound(next_pos)) return false;
        
        if (grid_assert(next_pos, APPLE)) {
            spawn_apple();
        } else if (grid_assert(next_pos, EMPTY)) {
            grid_set(body.back(), EMPTY);
            body.pop_back();
        } else {
            return false;
        }
        grid_set(next_pos, SNAKE);
        body.push_front(next_pos);
        return true;
    }

    bool move_up() {
        if (dir[1] != 0) return false;
        dir[0] = 0;
        dir[1] = -1;
        return true;
    }

    bool move_down() {
        if (dir[1] != 0) return false;
        dir[0] = 0;
        dir[1] = 1;
        return true;
    }

    bool move_left() {
        if (dir[0] != 0) return false;
        dir[0] = -1;
        dir[1] = 0;
        return true;
    }

    bool move_right() {
        if (dir[0] != 0) return false;
        dir[0] = 1;
        dir[1] = 0;
        return true;
    }


    void spawn_apple() {
        int r = get_rand_int(1, N-2);
        int c = get_rand_int(1, M-2);
        int i = M * r + c;
        while (grid[i] != EMPTY) {
            r = get_rand_int(1, N - 2);
            c = get_rand_int(1, M - 2);
            i = M * r + c;
        }
        grid[i] = APPLE;
    }
};

int main() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return 1;
    }
    float main_scale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    int win_w = 1280 * main_scale;
    int win_h = 800 * main_scale;
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL2+SDL_Renderer example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, win_w, win_h, window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr)
    {
        SDL_Log("Error creating SDL_Renderer!");
        return 1;
    }


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    SnakeGame snake;
    GameState game_state = GameState::menu;
    int num_of_apples = 1;
    int starting_l = 3;
    int cell_size = 5;
    int grid_size[2] = {32, 32};
    SDL_Rect game_rect{win_w/8, win_h/8, 6*win_w/8, 6*win_h/8};

    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            switch (event.type) {
                case SDL_QUIT:
                    done = true;
                    break;
                case SDL_WINDOWEVENT:
                    done |= (
                        event.window.event == SDL_WINDOWEVENT_CLOSE 
                        && event.window.windowID == SDL_GetWindowID(window)
                    );
                    break;
                case SDL_KEYDOWN:
                    switch(event.key.keysym.sym) {
                        case SDLK_w:
                            snake.move_up();
                            break;
                        case SDLK_s:
                            snake.move_down();
                            break;
                        case SDLK_a:
                            snake.move_left();
                            break;
                        case SDLK_d:
                            snake.move_right();
                            break;
                        case SDLK_ESCAPE:
                            if (game_state == GameState::game_paused) {
                                game_state = GameState::game_running;
                            } else if (game_state == GameState::game_running) {
                                game_state = GameState::game_paused;
                            }
                            break;
                    }
                    break;
            }
        }
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0); // Transparent if blend mode allows
        SDL_RenderClear(renderer);

        if (game_state == GameState::menu) {
            NewStaticCenterFrame();
            ImGui::Begin("Hello, welcome to snake!", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
            ImGui::Text("Try to collect as many apples as you can without hitting the wall or yourself.");

            if (ImGui::BeginTable("Controls", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Key");
                ImGui::TableSetupColumn("Action");
                ImGui::TableHeadersRow();

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("W");
                ImGui::TableSetColumnIndex(1); ImGui::Text("Move Up");

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("S");
                ImGui::TableSetColumnIndex(1); ImGui::Text("Move Down");

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("A");
                ImGui::TableSetColumnIndex(1); ImGui::Text("Move Left");

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("D");
                ImGui::TableSetColumnIndex(1); ImGui::Text("Move Right");

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("ESC");
                ImGui::TableSetColumnIndex(1); ImGui::Text("Pause Game");

                ImGui::EndTable();
            }

            ImGui::Text("Before we start, let us set some settings.");
            ImGui::SliderInt2("Size of grid", grid_size, 10, 128);
            ImGui::SliderInt("Cell size", &cell_size, 1, 10);
            ImGui::SliderInt("Starting length of snake", &starting_l, 3, 10);
            ImGui::SliderInt("Number of apples at once", &num_of_apples, 1, 5);

            if (ImGui::Button("Let's Play!")) {
                game_state = GameState::game_running;
                snake.set_settings(grid_size[0], grid_size[1], starting_l, cell_size);
                snake.init_tex(renderer);
                for (int i = 0; i < num_of_apples; i++) snake.spawn_apple();
            };
            ImGui::SameLine();
            if (ImGui::Button("Quit")) break;
            ImGui::End();
            ImGui::Render();
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        } else {
            SDL_Texture* tex = snake.render(renderer);
            SDL_RenderCopy(renderer, tex, nullptr, &game_rect);

            if (game_state == GameState::game_paused) {
                NewStaticCenterFrame();
                ImGui::Begin("Game Paused!", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
                ImGui::Text("Final Score: %d", (int)snake.body.size());
                if (ImGui::Button("Continue")) {
                    game_state = GameState::game_running;
                }
                ImGui::End();
                ImGui::Render();
                ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
            } else if (game_state == GameState::game_over) {
                SDL_Texture* tex = snake.render(renderer);
                SDL_RenderCopy(renderer, tex, nullptr, &game_rect);

                NewStaticCenterFrame();
                ImGui::Begin("Game Over!", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
                ImGui::Text("Final Score: %d", (int)snake.body.size());
                ImGui::Text("Apples Eaten: %d", (int)snake.body.size() - starting_l);

                if (ImGui::Button("Home")) {
                    game_state = GameState::menu;
                };
                ImGui::End();
                ImGui::Render();
                ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
            } else {
                if (!snake.step()) {
                    game_state = GameState::game_over;
                }
                SDL_Delay(100);
            }
        }
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}