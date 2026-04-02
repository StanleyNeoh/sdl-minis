#include <array>
#include <iostream>
#include "particle.hpp"

int main() {
    Pf2 a(
        Vf2{0, 1},
        Vf2{0, -1},
        1
    );

    Pf2 b(
        Vf2{0, 0},
        Vf2{0, 0},
        1
    );
    std::cout << a << "\n";
    std::cout << b << "\n";

    std::cout << a.is_overlap(b) << "\n";
    std::cout << a.is_approaching(b) << "\n";
} 