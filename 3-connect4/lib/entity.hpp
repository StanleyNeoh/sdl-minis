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
#include "textures/textures.hpp"

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

struct BoardCell {
    static constexpr Color RED{255, 0, 0};
    static constexpr Color BLUE{0, 0, 255};
    static constexpr Color HIGHLIGHT{0, 255, 0};

    Cell key = NoneKey;
    SDL_Rect rect;
    int final_y = 0;
    bool is_marked = false;

    BoardCell() = default;
    BoardCell(Cell key, const SDL_Rect& _rect): key(key), rect(_rect.x, 0, _rect.w, _rect.h), final_y(_rect.y) {}

    bool step() {
        if (rect.y == final_y) return false;
        rect.y = std::min(final_y, rect.y + 10);
        return true;
    }

    bool draw(SDL_Renderer* renderer) {
        switch (key) {
        case BotKey:
            SDL_SetTextureColorMod(Textures::BoardCell::tex, BLUE.r, BLUE.g, BLUE.b);
            break;
        case PlayerKey:
            SDL_SetTextureColorMod(Textures::BoardCell::tex, RED.r, RED.g, RED.b);
            break;
        default:
            break;
        }
        SDL_RenderCopy(renderer, Textures::BoardCell::tex, NULL, &rect);

        if (is_marked) {
            SDL_SetTextureColorMod(Textures::BoardCell::tex, HIGHLIGHT.r, HIGHLIGHT.g, HIGHLIGHT.b);
            int cx = rect.x + 0.5 * rect.w;
            int cy = rect.y + 0.5 * rect.h;
            int nw = rect.w / 5.0;
            int nh = rect.w / 5.0;
            SDL_Rect _rect{cx - nw / 2, cy - nh / 2, nw, nh};
            SDL_RenderCopy(renderer, Textures::BoardCell::tex, NULL, &_rect);
        }

        SDL_SetTextureColorMod(Textures::BoardCell::tex, 255, 255, 255);
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

    Board(Network& net): Par(net) {};

    int col_i = -1;
    int winner = NoneKey;

    bool init(SDL_Renderer* renderer, int x, int y, int w, int h) {
        if (!Par::init(renderer, x, y, w, h)) return false;
        Textures::BoardCell::init(renderer);
        Textures::BoardFrame::init(renderer);
        return true;
    }

    bool draw() {
        for (auto& cell: cells) {;
            if (!cell.second.draw(renderer)) return false;
            cell.second.step();
        }
        SDL_RenderCopy(renderer, Textures::BoardFrame::tex, NULL, &rect);
        return true;
    }

    void handle_mouse_motion(const SDL_MouseMotionEvent& e) {
        int cw = (rect.w - 2 * Textures::BoardFrame::br) / N;
        int ind =  (e.x - Textures::BoardFrame::br) / cw; 
        if (ind < 0 || ind >= N) {
            col_i = -1;
        } else {
            col_i = ind;
        };
    }

    void handle_mouse_up(const SDL_MouseButtonEvent& e) {
        if (winner != NoneKey) return;
        if (net != NULL) {
            UiEvent event{};
            event.move = {NetworkEventType::MOVE, PlayerKey, -1, col_i};
            net->ui_events.block_push(event);
        }
    }

    void handle_move(const Move& move) {
        int x = Textures::BoardFrame::cw * move.c + Textures::BoardFrame::br + Textures::BoardFrame::cbr;
        int y = Textures::BoardFrame::ch * move.r + Textures::BoardFrame::br + Textures::BoardFrame::cbr;
        int w = Textures::BoardFrame::cw - 2 * Textures::BoardFrame::cbr;
        int h = Textures::BoardFrame::ch - 2 * Textures::BoardFrame::cbr;
        SDL_Rect screen_rect = Textures::BoardFrame::to_screen_rect(rect, {x, y, w, h});
        cells[{move.r, move.c}] = BoardCell(move.key, screen_rect);
    }

    void handle_game_end(const GameEnd& game_end) {
        winner = game_end.winner;
        for (auto& loc: game_end.marked) {
            cells[loc].is_marked = true;
        }
    }
};
#endif