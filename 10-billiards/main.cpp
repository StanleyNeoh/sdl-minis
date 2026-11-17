#include "context.hpp"
#include "apps/stager.hpp"

#include "SDL.h"

// Main code
int main(int, char**)
{
    Context::init();
    Apps::Stager::run();

    Context::destroy();
    return 0;
}