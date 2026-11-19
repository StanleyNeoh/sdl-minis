#ifndef APPS_CONNECT4_CACHE_HPP
#define APPS_CONNECT4_CACHE_HPP

#include "board.hpp"

namespace Apps {
namespace _Connect4 {

struct CacheData {
    _Board board;
    Score score;
}

}}

#endif