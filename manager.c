#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>

#include "bar.h"
#include "manager.h"

struct Client client_new(Window window) {
    struct Client c = {
        .next = NULL,
        .prev = NULL,

        .window = window,
        .weight = 1,
        .x = 0,
        .y = 0,
        .width = 0,
        .height = 0,
    };

    return c;
}

struct Track track_new() {
    struct Track t = {
        .next = NULL,
        .prev = NULL,

        .first_client = NULL,
        .focused_client = NULL,
        .total_clients = 0,
        .total_clients_weight = 0,
        .weight = 1,
    };

    return t;
}

struct Workspace workspace_new() {
    struct Workspace w = {
        .first_track = NULL,
        .focused_track = NULL,
        .total_tracks = 0,
        .total_tracks_weight = 0,
    };

    return w;
}

struct ScreenArea screen_new
(Display *display, int x, int y, int width, int height)
{
    Window root = XDefaultRootWindow(display);
    struct ScreenArea s = {
        .workspaces = malloc(WORKSPACES * sizeof(struct Workspace*)),
        .x = x,
        .y = y,
        .width = width,
        .height = height,
        .bar = malloc(sizeof(struct Bar*)),
        .guide_x = malloc(sizeof(struct Bar*)),
        .guide_y = malloc(sizeof(struct Bar*)),
        .virtual_window = XCreateSimpleWindow(
            display,
            root,
            0,
            BAR_SIZE,
            width,
            height - BAR_SIZE,
            0,
            0,
            0x000000
        ),
    };

    struct Workspace *workspaces = malloc(WORKSPACES * sizeof(struct Workspace));
    for (int i = 0; i < WORKSPACES; i++) {
        s.workspaces[i] = &workspaces[i];
        *s.workspaces[i] = workspace_new();
    }

    *s.bar = bar_new(display, 0, 0, width, BAR_SIZE);
    *s.guide_x = bar_new(display, 0, height - GUIDE_SIZE, width, GUIDE_SIZE);
    *s.guide_y = bar_new(display, width - GUIDE_SIZE, 0, GUIDE_SIZE, height);

    XMapWindow(display, s.virtual_window);

    return s;
}

struct Manager manager_new(Display *display) {
    struct Manager m = {
        .screens = malloc(SCREENS * sizeof(struct ScreenArea)),
        .screens_length = SCREENS,
        .screens_focus = 0,
    };

    Window root = XDefaultRootWindow(display);
    Screen *xscreen = XDefaultScreenOfDisplay(display);
    int width = XWidthOfScreen(xscreen);
    int height = XHeightOfScreen(xscreen);

    for (int i = 0; i < m.screens_length; i++) {
        struct ScreenArea s = screen_new(display, 0, 0, width, height);
        m.screens[i] = s;
    }

    return m;
}

void track_free(struct Track *t) {
    struct Client *c = t->first_client;
    while (c != NULL) {
        struct Client *next = c->next;
        free(c);
        c = next;
    }
}

void workspace_free(struct Workspace *w) {
    struct Track *t = w->first_track;
    while (t != NULL) {
        struct Track *next = t->next;
        track_free(t);
        free(t);
        t = next;
    }
}

void screen_free(Display *display, struct ScreenArea *s) {
    for (int i = 0; i < WORKSPACES; i++) {
        workspace_free(s->workspaces[i]);
    }

    free(*s->workspaces);
    free(s->workspaces);

    bar_free(display, s->bar);
    bar_free(display, s->guide_x);
    bar_free(display, s->guide_y);
    free(s->bar);
    free(s->guide_x);
    free(s->guide_y);

    XDestroyWindow(display, s->virtual_window);
}

void manager_free(Display *display, struct Manager *m) {
    for (int i = 0; i < m->screens_length; i++) {
        screen_free(display, &m->screens[i]);
    }

    free(m->screens);
}

void track_remove_client(struct Track *t, struct Client *c) {
    if (t->focused_client == c) {
        if (c->next != NULL) {
            t->focused_client = c->next;
        } else if (c->prev != NULL) {
            t->focused_client = c->prev;
        } else {
            t->focused_client = NULL;
            t->first_client = NULL;
        }
    }

    if (c->prev != NULL) {
        c->prev->next = c->next;
    } else {
        t->first_client = c->next;
    }

    if (c->next != NULL) {
        c->next->prev = c->prev;
    }

    c->prev = NULL;
    c->next = NULL;

    t->total_clients -= 1;
    t->total_clients_weight -= c->weight;
}

void track_add_client
(struct Track *t, struct Client *c, enum InsertDirection direction, bool focus_it)
{
    if (t->total_clients == 0) {
        t->first_client = c;
        t->focused_client = c;
    } else {
        struct Client *before;
        struct Client *after;
        if (direction == INSERT_BEFORE) {
            before = t->focused_client->prev;
            after = t->focused_client;
        } else if (direction == INSERT_AFTER) {
            before = t->focused_client;
            after = t->focused_client->next;
        }

        if (before != NULL) {
            before->next = c;
            c->prev = before;
        }

        if (after != NULL) {
            after->prev = c;
            c->next = after;
        }

        if (direction == INSERT_BEFORE && before == NULL) {
            t->first_client = c;
        }

        if (focus_it) {
            t->focused_client = c;
        }
    }

    t->total_clients += 1;
    t->total_clients_weight += c->weight;
}

void workspace_remove_track(struct Workspace *w, struct Track *t) {
    if (w->focused_track == t) {
        if (t->next != NULL) {
            w->focused_track = t->next;
        } else if (t->prev != NULL) {
            w->focused_track = t->prev;
        } else {
            w->first_track = NULL;
            w->focused_track = NULL;
        }
    }

    if (t->prev != NULL) {
        t->prev->next = t->next;
    } else {
        w->first_track = t->next;
    }

    if (t->next != NULL) {
        t->next->prev = t->prev;
    }

    t->prev = NULL;
    t->next = NULL;

    w->total_tracks -= 1;
    w->total_tracks_weight -= t->weight;
}

void workspace_add_track
(struct Workspace *w, struct Track *t, enum InsertDirection direction, bool focus_it)
{
    if (w->total_tracks == 0) {
        w->first_track = t;
        w->focused_track = t;
    } else {
        struct Track *before;
        struct Track *after;
        if (direction == INSERT_BEFORE) {
            before = w->focused_track->prev;
            after = w->focused_track;
        } else if (direction == INSERT_AFTER) {
            before = w->focused_track;
            after = w->focused_track->next;
        }

        if (before != NULL) {
            before->next = t;
            t->prev = before;
        }

        if (after != NULL) {
            after->prev = t;
            t->next = after;
        }

        if (direction == INSERT_BEFORE && before == NULL) {
            w->first_track = t;
        }

        if (focus_it) {
            w->focused_track = t;
        }
    }

    w->total_tracks += 1;
    w->total_tracks_weight += 1;
}

void workspace_add_new_track
(struct Workspace *w, enum InsertDirection direction, bool focus_it)
{
    struct Track *t = malloc(sizeof(struct Track));
    *t = track_new();
    workspace_add_track(w, t, direction, focus_it);
}

void screen_arrange(Display *display, struct ScreenArea *s) {
    XUnmapSubwindows(display, s->virtual_window);
    struct Workspace *w = *s->workspaces;

    bool guide_x_visible = w->total_tracks > 1;
    bool guide_y_visible = workspace_has_plural_track(w);
    int guide_height = guide_x_visible ? GUIDE_SIZE : 0;
    int guide_width = guide_y_visible ? GUIDE_SIZE : 0;
    int virtual_width = s->width - guide_width;
    int virtual_height = s->height - guide_height - BAR_SIZE;

    struct Track *t = w->first_track;
    double x = 0;
    while (t != NULL) {
        struct Client *c = t->first_client;
        double relative_track_weight = t->weight / w->total_tracks_weight;
        int width = virtual_width * relative_track_weight;
        double y = 0;

        while (c != NULL) {
            double relative_client_weight = c->weight / t->total_clients_weight;
            int height = virtual_height * relative_client_weight;

            printf("move resize: x=%.1f y=%.1f width=%d height=%d\n", x, y, width, height);
            XMoveResizeWindow(display, c->window, x, y, width, height);
            XMapWindow(display, c->window);

            c->x = x;
            c->y = y;
            c->width = width;
            c->height = height;

            y += height;
            c = c->next;
        }

        x += width;
        t = t->next;
    }
}

// Client insertion method:
// If there are no clients, add a track, focus it, and put the client in it.
// If there is one track, add a track, focus it, and and put the client in it.
// If there are > 1 tracks, put the client in the focused track.
// Rationale:
// Generally you want at least two tracks, so prioritize that. The problem is
// that it's harder to move a client from a singleton track into a populated
// track into the right place than from a populated track into a new track.
void manager_add_window(Display *display, struct Manager *m, Window window) {
    struct ScreenArea *s = &m->screens[m->screens_focus];
    struct Workspace *w = *s->workspaces;

    XReparentWindow(display, window, s->virtual_window, 0, 0);

    if (w->total_tracks < 2) {
        workspace_add_new_track(w, INSERT_AFTER, true);
    }

    struct Client *c = malloc(sizeof(struct Client));
    *c = client_new(window);
    track_add_client(w->focused_track, c, INSERT_AFTER, true);

    XSelectInput(display, window, KeyPressMask);
}

void manager_refocus(Display *display, struct Manager *m) {
    struct Workspace *w = *m->screens[m->screens_focus].workspaces;
    struct Client *c = w->focused_track->focused_client;
    XSetInputFocus(display, c->window, RevertToPointerRoot, CurrentTime);
}
