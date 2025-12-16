#ifndef LIB_UTILS_HPP
#define LIB_UTILS_HPP

#include <SDL2/SDL.h>

template <typename T>
T* unsafe_shift(T* ptr, int n_bytes) {
    return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(ptr) + n_bytes);
}

struct Color {
    int r; 
    int g; 
    int b;
    int a = 255;
};

Color WHITE{255, 255, 255};
Color RED{255, 0, 0};
Color BLUE{0, 0, 255};
Color BOARD_BG{0, 0, 100};

enum class Player {
    None,
    PlayerRed,
    PlayerBlue,
};

uint32_t map_color(SDL_PixelFormat* format, Color& color) {
    return SDL_MapRGBA(format, color.r, color.g, color.b, color.a);
}

inline std::ostream& operator<<(std::ostream& os, Player p) {
    switch (p) {
        case Player::PlayerRed:  return os << "R";
        case Player::PlayerBlue: return os << "B";
        default:                 return os << ".";
    }
    return os;
}

template <typename T, size_t M, size_t N>
inline std::ostream& operator<<(std::ostream& o, const std::array<std::array<T, N>, M>& grid) {
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            o << grid[i][j] << " ";
        }
        o << "\n";
    }
    return o;
}

#endif