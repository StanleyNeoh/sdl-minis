#include "context.hpp"
#include "stager.hpp"

// Main code
int main(int, char**)
{
    Context::init();
    Stager stager;
    stager.run();

    Context::deinit();
    return 0;
}