
#ifndef APPS_DEMO_HPP
#define APPS_DEMO_HPP

namespace Apps {
namespace Demo {
    extern bool done;
    extern bool show_demo_window;
    extern bool show_another_window;
    extern float f = 0.0f;
    extern int counter = 0;

    void init();
    void draw();
    void deinit();
}
}

#endif