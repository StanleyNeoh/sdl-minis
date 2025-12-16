#ifndef LIB_UTILS_HPP
#define LIB_UTILS_HPP

#include <SDL2/SDL.h>
#include <array>
#include <type_traits>
#include <climits>

template <typename T>
T cast_to(const char* arg) {
    T out;
    std::stringstream ss;
    ss << arg;
    ss >> out;
    return out;
}

template <typename T>
int clamp(T x, T l, T r) {
    if (x < l) return l;
    if (x > r) return r;
    return x;
}

template <typename T>
T* unsafe_shift(T* obj, int n_bytes) {
    return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(obj) + n_bytes);
}

int dist2(SDL_Point p0, SDL_Point p1) {
    int dx = p0.x - p1.x;
    int dy = p0.y - p1.y;
    return dx * dx + dy * dy;
}

double boundedLineDist2(SDL_Point e0, SDL_Point e1, SDL_Point p) {
    p.x -= e0.x;
    p.y -= e0.y;
    e1.x -= e0.x;
    e1.y -= e0.y;
    int pd = p.x * e1.x + p.y * e1.y;
    int pp = p.x * p.x + p.y * p.y;
    int dd = e1.x * e1.x + e1.y * e1.y;
    if (pd <= 0 || pd >= dd) {
        return std::min(pp, dist2(p, e1));
    }
    double dl2 = static_cast<double>(pp * dd - pd * pd) / dd;
    return dl2;
}


template <int N>
void get_bounds(
    const std::array<SDL_Point, N>& points, 
    SDL_Point& top_left, 
    SDL_Point& bottom_right, 
    int padding,
    int w, int h
) {
    int minx = INT_MAX;
    int miny = INT_MAX;
    int maxx = INT_MIN;
    int maxy = INT_MIN;
    for (int i = 0; i < N; i++) {
        minx = std::min(minx, points[i].x);
        miny = std::min(miny, points[i].y);
        maxx = std::max(maxx, points[i].x);
        maxy = std::max(maxy, points[i].y);
    }

    top_left.x = std::max(0, minx - padding);
    top_left.y = std::max(0, miny - padding);
    bottom_right.x = std::min(w-1, maxx + padding);
    bottom_right.y = std::min(h-1, maxy + padding);
}

// Trailing returns style
template<std::size_t I = 0, typename Func, typename... Tp>
auto tuple_for_each(std::tuple<Tp...>& t, Func f) -> std::enable_if_t<I == sizeof...(Tp), void> { }


template<std::size_t I = 0, typename Func, typename... Tp>
auto tuple_for_each(std::tuple<Tp...>& t, Func f) -> std::enable_if_t<I < sizeof...(Tp), void> {
    f(std::get<I>(t));
    tuple_for_each<I + 1, Func, Tp...>(t, f);
}

#endif