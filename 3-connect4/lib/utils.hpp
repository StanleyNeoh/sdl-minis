#ifndef LIB_UTILS_HPP
#define LIB_UTILS_HPP

#include <SDL2/SDL.h>
#include <iostream>

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

uint32_t map_color(SDL_PixelFormat* format, Color color) {
    return SDL_MapRGBA(format, color.r, color.g, color.b, color.a);
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


using Cell = char;
constexpr static Cell NoneKey = '.';
constexpr static Cell BotKey = 'B';
constexpr static Cell PlayerKey = 'R';

Cell other_player(Cell key) {
    switch(key) {
    case BotKey:
        return PlayerKey;
    case PlayerKey:
        return BotKey;
    default:
        return NoneKey;
    }
}

struct Score {
    int a;
    int b;
    int c;

    bool operator==(const Score& other) const {
        return a == other.a && b == other.b && c == other.c;
    }

    bool operator<(const Score& other) const {
        if (a != other.a) return a < other.a;
        if (b != other.b) return b < other.b;
        return c < other.c;
    }

    bool operator>(const Score& other) const {
        return other < *this;
    }

    bool operator<=(const Score& other) const {
        return *this == other || *this < other;
    }

    bool operator>=(const Score& other) const {
        return other <= *this;
    }

    Score operator-() const {
        return Score{-a, -b, -c};
    }

    friend std::ostream& operator<<(std::ostream& o, const Score& score) {
        o << score.a << "," << score.b << "," << score.c;
        return o;
    }
};

struct GridLoc { 
    int r;
    int c;
    bool operator==(const GridLoc& other) const {
        return r == other.r && c == other.c;
    }

    friend std::ostream& operator<<(std::ostream& o, const GridLoc& loc) {
        o << loc.r << "," << loc.c;
        return o;
    }
};

template<>
struct std::hash<GridLoc> {
    std::size_t operator()(const GridLoc& f) const {
        return std::hash<int>{}(f.r) ^ std::hash<int>{}(f.r);
    }
};

#endif