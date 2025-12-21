#ifndef LIB_ENTITY_HPP
#define LIB_ENTITY_HPP

#include <SDL.h>
#include <iostream>
#include <vector>
#include <array>
#include <climits>
#include <unordered_map>
#include "utils.hpp"
#include "engine.hpp"
#include "atomic"

template <typename T>
struct Entity {
    SDL_Renderer* renderer = NULL;
    SDL_Texture* tex = NULL;
    SDL_PixelFormat* format = NULL;
    Network* net = NULL;
    SDL_Rect rect{-1, -1, -1, -1};

    Entity() = default;
    Entity(Network& net): net(&net) {}

    ~Entity() {
        if (tex != NULL) {
            SDL_DestroyTexture(tex);
            tex = NULL;
        }
        if (format != NULL) {
            SDL_FreeFormat(format);
            format = NULL;
        }
    }


    Entity(const Entity& other) = delete;
    Entity(Entity&& other): renderer(other.renderer), tex(other.tex), format(other.format), rect(other.rect) {
        other.renderer = NULL;
        other.tex = NULL;
        other.format = NULL;
    }

    Entity<T>& operator=(const Entity<T>& other) = delete;
    Entity<T>& operator=(Entity<T>&& other) noexcept {
        if (this == &other) return *this;
        if (tex) SDL_DestroyTexture(tex);
        if (format) SDL_FreeFormat(format);

        renderer = other.renderer;
        tex = other.tex;
        format = other.format;
        rect = other.rect;

        other.renderer = NULL;
        other.tex = NULL;
        other.format = NULL;
        return *this;
    }

    bool init_tex() {
        tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, rect.w, rect.h);
        if (tex == NULL) {
            std::cerr << "[Viewport::init] Error: " << SDL_GetError() << "\n";
            return false;
        }
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        uint32_t formatEnum;
        if (SDL_QueryTexture(tex, &formatEnum, NULL, NULL, NULL) != 0) {
            std::cerr << "[Viewport::init] Error: " << SDL_GetError() << "\n";
            return false;
        }
        format = SDL_AllocFormat(formatEnum);
        if (format == NULL) {
            std::cerr << "[Viewport::init] Error: " << SDL_GetError() << "\n";
            return false;
        }
        return true;
    }

    bool init(SDL_Renderer* renderer, int x, int y, int w, int h) {
        this->renderer = renderer;
        rect.x = x;
        rect.y = y;
        rect.w = w;
        rect.h = h;
        return true;
    }

    bool draw_tex() {
        if (tex == NULL) return false;
        if (SDL_RenderCopy(renderer, tex, NULL, &rect) != 0) {
            std::cerr << "[Viewport::draw] Error: " << SDL_GetError() << "\n";
            return false;
        }
        return true;
    }

    bool draw() { return draw_tex(); }

    bool step() { return false; }

    bool handle_network_event() {
        if (net == NULL) return false;
        T* self = static_cast<T*>(this);
        BotEvent e;
        if (net->bot_events.pop(e)) {
            switch(e.type) {
            case NetworkEventType::MOVE:
                self->handle_move(e.move);
                return true;
            case NetworkEventType::GAME_END:
                self->handle_game_end(e.game_end);
                return true;
            }
        }
        return false;
    }

    bool handle_event(SDL_Event& e, bool& quit) {
        T* self = static_cast<T*>(this);
        switch(e.type) {
            case SDL_QUIT:
                quit = true;
                self->handle_quit(e.quit);
                return true;
            case SDL_MOUSEWHEEL:
                self->handle_mouse_scroll(e.wheel);
                return true;
            case SDL_MOUSEMOTION:
                {
                    SDL_MouseMotionEvent me = e.motion;
                    me.x -= rect.x;
                    me.y -= rect.y;
                    if (me.x < 0 || me.x >= rect.w) return true;
                    if (me.y < 0 || me.y >= rect.h) return true;
                    self->handle_mouse_motion(me);
                }
                return true;
            case SDL_MOUSEBUTTONDOWN:
                self->handle_mouse_down(e.button);
                return true;
            case SDL_MOUSEBUTTONUP:
                self->handle_mouse_up(e.button);
                return true;
            case SDL_KEYDOWN:
                self->handle_key_down(e.key);
                return true;
            case SDL_KEYUP:
                self->handle_key_up(e.key);
                return true;
            default:
                return false;
        }
    }

    void handle_quit(const SDL_QuitEvent&) {}
    void handle_mouse_scroll(const SDL_MouseWheelEvent&) {}
    void handle_mouse_motion(const SDL_MouseMotionEvent&) {}
    void handle_mouse_down(const SDL_MouseButtonEvent&) {}
    void handle_mouse_up(const SDL_MouseButtonEvent&) {}
    void handle_key_down(const SDL_KeyboardEvent&) {}
    void handle_key_up(const SDL_KeyboardEvent&) {}
    void handle_move(const Move&) {}
    void handle_game_end(const GameEnd&) {}
};

template <int M, int N>
struct BoardFrame: public Entity<BoardFrame<M, N>> {
    using Par = Entity<BoardFrame<M, N>>;
    using Par::rect;
    using Par::tex;
    using Par::format;
    using Par::draw_tex;
    using Par::init_tex;
    static constexpr Color BACKGROUND{0, 0, 100};

    int br = 20;
    int bw = -1;
    int bh = -1;
    float cw = -1;
    float cwr = -1;
    float ch = -1;
    float chr = -1;

    BoardFrame(int br): br(br) {}
    BoardFrame() = default;

    bool init(SDL_Renderer* renderer, int x, int y, int w, int h) {
        if (!Par::init(renderer, x, y, w, h)) return false;
        if (!init_tex()) return false;
        uint32_t* pixels;
        int pitch;
        if (SDL_LockTexture(tex, NULL, reinterpret_cast<void**>(&pixels), &pitch) != 0) {
            return false;
        }

        bw = rect.w - 2 * br;
        bh = rect.h - 2 * br;
        cw = static_cast<float>(bw) / N;
        cwr = cw / 2.0;
        ch = static_cast<float>(bh) / M;
        chr = ch / 2.0;
        uint32_t bgColor = map_color(format, BACKGROUND);

        for (int r = 0; r < br; r++) {
            int dy = br - r;
            int dx = br - SDL_sqrt(br * br - dy * dy);
            uint32_t* toppad = unsafe_shift(pixels, r * pitch);
            uint32_t* botpad = unsafe_shift(pixels, (rect.h - 1 - r) * pitch);
            for (int c = dx; c < rect.w - dx; c++) {
                botpad[c] = toppad[c] = bgColor;
            }
        }
        for (int r = br; r < rect.h - br; r++) {
            uint32_t* rowpix = unsafe_shift(pixels, r * pitch);
            for (int c = 0; c < br; c++) {
                rowpix[c] = bgColor;
            }
            for (int c = rect.w - br; c < rect.w; c++) {
                rowpix[c] = bgColor;
            }
        }

        for (int r = 0; r < M; r++) {
            for (int c = 0; c < N; c++) {
                float tlx = cw * c + br;
                float tly = ch * r + br;
                float cx = tlx + cwr;
                float cy = tly + chr;
                for (int y = tly; y < tly + ch; y++) {
                    uint32_t* rowpix = unsafe_shift(pixels, y * pitch);
                    for (int x = tlx; x < tlx + cw; x++) {
                        float dx = (x - cx) / cwr;
                        float dy = (y - cy) / chr;
                        float d2 = dx * dx + dy * dy;
                        if (d2 > 0.8) {
                            rowpix[x] = bgColor;
                        } 
                    }
                }
            }
        }
        SDL_UnlockTexture(tex);
        return true;
    }
};

struct BoardCell: public Entity<BoardCell> {
    using Par = Entity<BoardCell>;
    using Par::tex;
    using Par::format;
    using Par::rect;
    using Par::init_tex;

    static constexpr Color RED{255, 0, 0};
    static constexpr Color BLUE{0, 0, 255};
    static constexpr Color HIGHLIGHT{0, 255, 0};

    Cell key = NoneKey;
    int final_y = 0;

    BoardCell() = default;
    BoardCell(Cell key, int final_y): key(key), final_y(final_y) {}

    bool init(SDL_Renderer* renderer, int x, int y, int w, int h) {
        if (!Par::init(renderer, x, y, w, h)) return false;
        if (!init_tex()) return false;
        if (!update_tex()) return false;
        return true;
    }

    bool update_tex(bool marked=false) {
        uint32_t* pixels;
        int pitch;
        if (SDL_LockTexture(tex, NULL, reinterpret_cast<void**>(&pixels), &pitch) != 0) {
            return false;
        }
        float cwr = rect.w / 2.0;
        float chr = rect.h / 2.0;
        uint32_t hl_color = map_color(format, HIGHLIGHT);
        uint32_t cell_color;
        switch(key) {
        case BotKey:
            cell_color = map_color(format, {0,0,255});
            break;
        case PlayerKey:
            cell_color = map_color(format, {255,0,0});
            break;
        default:
            return true;
        }
        for (int r = 0; r < rect.h; r++) {
            uint32_t* rowpix = unsafe_shift(pixels, r * pitch);
            for (int c = 0; c < rect.w; c++) {
                float dy = (r - chr) / chr;
                float dx = (c - cwr) / cwr;
                float d2 = dx * dx + dy * dy;
                if (marked && d2 < 0.05) {
                    rowpix[c] = hl_color;
                } else if (d2 <= 1.0) {
                    rowpix[c] = cell_color;
                }
            }
        }
        SDL_UnlockTexture(tex);
        return true;
    }

    bool step() {
        if (rect.y == final_y) return false;
        rect.y = std::min(final_y, rect.y + 10);
        return true;
    }
};

template <int M, int N>
struct Board: public Entity<Board<M, N>> {
    using Par = Entity<Board<M, N>>;
    using Par::renderer;
    using Par::rect;
    using Par::tex;
    using Par::format;
    using Par::net;
    
    std::unordered_map<GridLoc, BoardCell> cells;
    BoardFrame<M, N> boardframe;

    Board(Network& net): Par(net) {};

    int col_i = -1;
    int winner = NoneKey;

    bool init(SDL_Renderer* renderer, int x, int y, int w, int h) {
        if (!Par::init(renderer, x, y, w, h)) return false;
        if (!boardframe.init(renderer, x, y, w, h)) return false;
        return true;
    }

    bool draw() {
        for (auto& cell: cells) {;
            if (!cell.second.draw()) return false;
            cell.second.step();
        }
        return boardframe.draw();
    }

    void handle_mouse_motion(const SDL_MouseMotionEvent& e) {
        int cw = (rect.w - 2 * boardframe.br) / N;
        int ind =  (e.x - boardframe.br) / cw; 
        if (ind < 0 || ind >= N) {
            col_i = -1;
        } else {
            col_i = ind;
        };
    }

    void handle_mouse_up(const SDL_MouseButtonEvent& e) {
        if (winner != NoneKey) return;
        if (net != NULL) {
            net->ui_events.block_push({move: {NetworkEventType::MOVE, PlayerKey, -1, col_i}});
        }
    }

    void handle_move(const Move& move) {
        int r = move.r;
        int c = move.c;
        float cw = boardframe.cw;
        float ch = boardframe.ch;
        float tlx = cw * c + boardframe.br;
        float tly = ch * r + boardframe.br;
        cells[{r, c}] = BoardCell(move.key, tly);
        cells[{r, c}].init(renderer, tlx, 0, cw, ch);
    }

    void handle_game_end(const GameEnd& game_end) {
        winner = game_end.winner;
        for (auto& loc: game_end.marked) {
            cells[loc].update_tex(true);
        }
    }
};
#endif