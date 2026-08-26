#include <stdbool.h>
#include <X11/Xlib.h>
#include <cairo.h>
#include <cairo-xlib.h>

#include "config.h"
#include "structs.h"

struct Color {
    float r;
    float g;
    float b;
};

struct Bar {
    Window window;
    cairo_t *cairo;
    bool visible;
};

struct Bar bar_new(Display *display, int x, int y, int width, int height);
void bar_free(Display *display, struct Bar *bar);
void bar_update(Display *display, struct ScreenArea *s);
