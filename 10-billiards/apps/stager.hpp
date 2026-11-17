#ifndef APPS_STAGER_HPP
#define APPS_STAGER_HPP

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "demo.hpp"

namespace Apps {
namespace Stager {
    extern bool done;
    extern int selectIndex;
    void run();
}
}

#endif