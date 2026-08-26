#include <stdbool.h>

#include "bar.h"

#define MIN(a, b) (a < b ? a : b)

struct Bar bar_new(Display *display, int x, int y, int width, int height) {
    struct Bar bar;
    bar.window = XCreateSimpleWindow(
        display,
        XDefaultRootWindow(display),
        x,
        y,
        width,
        height,
        0,
        0,
        BAR_BACKGROUND
    );
    cairo_surface_t *surface = cairo_xlib_surface_create(
        display,
        bar.window,
        DefaultVisualOfScreen(DefaultScreenOfDisplay(display)),
        width,
        height
    );
    bar.visible = true;

    bar.cairo = cairo_create(surface);

    return bar;
}

void bar_free(Display *display, struct Bar *bar) {
    XDestroyWindow(display, bar->window);
    cairo_destroy(bar->cairo);
}

struct Color hex_to_color(long hex) {
    int r = (hex & 0xff0000) >> 16;
    int g = (hex & 0x00ff00) >> 8;
    int b = (hex & 0x0000ff);

    struct Color color = {
        .r = (double)r / 256,
        .g = (double)g / 256,
        .b = (double)b / 256,
    };

    return color;
}

void set_color(cairo_t *cairo, struct Color color) {
    cairo_set_source_rgb(cairo, color.r, color.g, color.b);
}

void draw_empty_workspace
(cairo_t *cairo, struct Color color, double x, double y, double width, double height)
{
    double midpoint_x = width / 2 + x;
    double midpoint_y = height / 2 + y;

    cairo_set_line_width(cairo, BAR_WORKSPACE_INACTIVE_STROKE_SIZE);
    cairo_arc
    (cairo, midpoint_x, midpoint_y, (double)BAR_WORKSPACE_INACTIVE_SIZE / 2,
     0, 2 * 3.14);
    cairo_stroke(cairo);
}

void draw_window
(cairo_t *cairo, struct Client *c, double x, double y, double x_ratio, double y_ratio)
{
    double client_x = (c->x + c->width / 2.0) * x_ratio + x;
    double client_y = (c->y + c->height / 2.0) * y_ratio + y;

    double client_width = c->width * x_ratio;
    double client_height = c->height * y_ratio;

    double radius = 0.9 * MIN(client_width, client_height) / 2;
    radius = MIN(radius, BAR_WORKSPACE_WINDOW_MAX_SIZE / 2.0);

    cairo_arc(cairo, client_x, client_y, radius, 0, 2 * 3.14);
    cairo_fill(cairo);
}

void draw_workspace
(struct Bar *bar, struct Color c_wksp, struct Color c_wksp_focused,
 struct Workspace *w, double x, double y, double width, double height,
 int screen_width, int screen_height)
{
    cairo_t *cairo = bar->cairo;

    double x_ratio = (double)width / screen_width;
    double y_ratio = (double)height / screen_height;

    //set_color(cairo, c_wksp);
    //cairo_set_line_width(cairo, 1);
    //cairo_set_antialias(cairo, CAIRO_ANTIALIAS_NONE);
    //cairo_rectangle(cairo, x + 1, y + 1, width - 1, height - 1);
    //cairo_stroke(cairo);
    //cairo_set_antialias(cairo, CAIRO_ANTIALIAS_DEFAULT);

    if (w->total_tracks == 0) {
        set_color(cairo, c_wksp);
        draw_empty_workspace(cairo, c_wksp, x, y, width, height);
    } else {
        struct Track *t = w->first_track;

        while (t != NULL) {
            struct Client *c = t->first_client;

            bool track_is_focused = t == w->focused_track;
            while (c != NULL) {
                bool client_is_focused = c == t->focused_client;
                if (track_is_focused && client_is_focused) {
                    set_color(cairo, c_wksp_focused);
                } else {
                    set_color(cairo, c_wksp);
                }

                draw_window(cairo, c, x, y, x_ratio, y_ratio);

                c = c->next;
            }

            t = t->next;
        }
    }
}

void bar_update(Display *display, struct ScreenArea *s) {
    struct Bar *bar = s->bar;
    XMapWindow(display, bar->window);
    XClearWindow(display, bar->window);

    struct Color c_wksp = hex_to_color(BAR_WORKSPACE_COLOR);
    struct Color c_wksp_focused = hex_to_color(BAR_WORKSPACE_COLOR_FOCUSED);
    double xy_ratio = (double)s->width / s->height;

    int wksp_width = BAR_SIZE;
    int wksp_height = BAR_SIZE;
    int wksp_margin = BAR_WORKSPACE_MARGIN;

    draw_workspace
    (bar, c_wksp, c_wksp_focused, s->workspaces[3], 0, 0,
     wksp_width, wksp_height, s->width, s->height);
    draw_workspace
    (bar, c_wksp, c_wksp_focused, s->workspaces[4], wksp_width + wksp_margin, 0,
     wksp_width, wksp_height, s->width, s->height);
    draw_workspace
    (bar, c_wksp, c_wksp_focused, s->workspaces[0], 2 * wksp_width + 2 * wksp_margin, 0,
     wksp_width, wksp_height, s->width, s->height);
    draw_workspace
    (bar, c_wksp, c_wksp_focused, s->workspaces[1], 3 * wksp_width + 3 * wksp_margin, 0,
     wksp_width, wksp_height, s->width, s->height);
    draw_workspace
    (bar, c_wksp, c_wksp_focused, s->workspaces[2], 4 * wksp_width + 4 * wksp_margin, 0,
     wksp_width, wksp_height, s->width, s->height);
}

void guide_x_update(struct Bar *guide_x) {
}

void guide_y_update() {
}

// Helper function for screen_arrange. Checks if any track has > 1 clients.
bool workspace_has_plural_track(struct Workspace *w) {
    struct Track *t = w->first_track;
    while (t != NULL) {
        if (t->total_clients > 1) return true;
        t = t->next;
    }
    return false;
}

void bar_update_guides() {
}
