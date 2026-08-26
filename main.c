#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
//#include <X11/extensions/Xrandr.h>

#include "bar.h"
#include "manager.h"

Display *display;
Window root;

bool will_quit = false;
struct Manager manager;

void quit(void *arg) {
    will_quit = true;
}

void focus_left(void *arg) {
    struct Workspace *w = *manager.screens[manager.screens_focus].workspaces;
    struct Track *t = w->focused_track;

    if (t != NULL && t->prev != NULL) {
        w->focused_track = t->prev;
        manager_refocus(display, &manager);
        bar_update(display, &manager.screens[manager.screens_focus]);
    }
}

void focus_right(void *arg) {
    struct Workspace *w = *manager.screens[manager.screens_focus].workspaces;
    struct Track *t = w->focused_track;

    if (t != NULL && t->next != NULL) {
        w->focused_track = t->next;
        manager_refocus(display, &manager);
        bar_update(display, &manager.screens[manager.screens_focus]);
    }
}

void focus_up(void *arg) {
    struct Workspace *w = *manager.screens[manager.screens_focus].workspaces;
    struct Track *t = w->focused_track;
    if (t == NULL) return;
    struct Client *c = t->focused_client;

    if (c != NULL && c->prev != NULL) {
        t->focused_client = c->prev;
        manager_refocus(display, &manager);
        bar_update(display, &manager.screens[manager.screens_focus]);
    }
}

void focus_down(void *arg) {
    struct Workspace *w = *manager.screens[manager.screens_focus].workspaces;
    struct Track *t = w->focused_track;
    if (t == NULL) return;
    struct Client *c = t->focused_client;

    if (c != NULL && c->next != NULL) {
        t->focused_client = c->next;
        manager_refocus(display, &manager);
        bar_update(display, &manager.screens[manager.screens_focus]);
    }
}

void swap_client_trackwise(enum InsertDirection direction) {
    struct Workspace *w = *manager.screens[manager.screens_focus].workspaces;
    struct Track *t = w->focused_track;
    if (t == NULL) return;

    bool is_last_client = t->total_clients == 1;
    bool is_track_at_end;
    if (direction == INSERT_BEFORE) {
        is_track_at_end = t->prev == NULL;
    } else if (direction == INSERT_AFTER) {
        is_track_at_end = t->next == NULL;
    }

    if (!is_last_client || !is_track_at_end) {
        struct Client *c = t->focused_client;
        track_remove_client(t, t->focused_client);

        if (is_track_at_end) {
            workspace_add_new_track(w, direction, false);
        }
        if (direction == INSERT_BEFORE) {
            w->focused_track = w->focused_track->prev;
        } else if (direction == INSERT_AFTER) {
            w->focused_track = w->focused_track->next;
        }

        if (is_last_client) {
            workspace_remove_track(w, t);
        }

        // TODO: implement INSERT_END.
        // currently inserts after the focus of the new track
        track_add_client(w->focused_track, c, INSERT_AFTER, true);

        screen_arrange(display, &manager.screens[manager.screens_focus]);
        manager_refocus(display, &manager);
        bar_update(display, &manager.screens[manager.screens_focus]);
    }
}

void swap_left(void *arg) {
    swap_client_trackwise(INSERT_BEFORE);
}

void swap_right(void *arg) {
    swap_client_trackwise(INSERT_AFTER);
}

void swap_client_clientwise(enum InsertDirection direction) {
    struct Workspace *w = *manager.screens[manager.screens_focus].workspaces;
    struct Track *t = w->focused_track;
    if (t == NULL) return;
    struct Client *c = t->focused_client;

    // before <-> first  <-> second <-> after
    // before <-> second <-> first  <-> after
    struct Client *before;
    struct Client *first;
    struct Client *second;
    struct Client *after;

    // c.prev.prev <-> c.prev <-> c      <-> c.next
    // c.prev.prev <-> c      <-> c.prev <-> c.next
    if (direction == INSERT_BEFORE) {
        first = c->prev;
        if (first != NULL) {
            before = c->prev->prev;
        }
        second = c;
        after = c->next;
    }
    // c.prev <-> c      <-> c.next <-> c.next.next
    // c.prev <-> c.next <-> c      <-> c.next.next
    else if (direction == INSERT_AFTER) {
        before = c->prev;
        first = c;
        second = c->next;
        if (second != NULL) {
            after = c->next->next;
        }
    }

    if (first != NULL && second != NULL) {
        if (before != NULL) {
            before->next = second;
        } else {
            t->first_client = second;
        }

        second->prev = before;
        second->next = first;

        first->prev = second;
        first->next = after;

        if (after != NULL) {
            after->prev = first;
        }

        screen_arrange(display, &manager.screens[manager.screens_focus]);
        manager_refocus(display, &manager);
        bar_update(display, &manager.screens[manager.screens_focus]);
    }
}

void swap_up(void *arg) {
    swap_client_clientwise(INSERT_BEFORE);
}
void swap_down(void *arg) {
    swap_client_clientwise(INSERT_AFTER);
}

//     [3]            [3]    
// [4] [0] [1] -> [1] [4] [0]
//     [2]            [2]    
void workspace_left(void *arg) {
    struct ScreenArea *s = &manager.screens[manager.screens_focus];
    struct Workspace *w = *s->workspaces;

    struct Workspace *temp = s->workspaces[1];
    s->workspaces[1] = s->workspaces[0];
    s->workspaces[0] = s->workspaces[4];
    s->workspaces[4] = temp;

    screen_arrange(display, &manager.screens[manager.screens_focus]);
    bar_update(display, &manager.screens[manager.screens_focus]);
}

//     [3]            [3]    
// [4] [0] [1] -> [0] [1] [4]
//     [2]            [2]    
void workspace_right(void *arg) {
    struct ScreenArea *s = &manager.screens[manager.screens_focus];
    struct Workspace *w = *s->workspaces;

    struct Workspace *temp = s->workspaces[4];
    s->workspaces[4] = s->workspaces[0];
    s->workspaces[0] = s->workspaces[1];
    s->workspaces[1] = temp;

    screen_arrange(display, &manager.screens[manager.screens_focus]);
    bar_update(display, &manager.screens[manager.screens_focus]);
}

//     [3]            [2]    
// [4] [0] [1] -> [4] [3] [1]
//     [2]            [0]    
void workspace_up(void *arg) {
    struct ScreenArea *s = &manager.screens[manager.screens_focus];
    struct Workspace *w = *s->workspaces;

    struct Workspace *temp = s->workspaces[2];
    s->workspaces[2] = s->workspaces[0];
    s->workspaces[0] = s->workspaces[3];
    s->workspaces[3] = temp;

    screen_arrange(display, &manager.screens[manager.screens_focus]);
    bar_update(display, &manager.screens[manager.screens_focus]);
}

//     [3]            [0]    
// [4] [0] [1] -> [4] [2] [1]
//     [2]            [3]    
void workspace_down(void *arg) {
    struct ScreenArea *s = &manager.screens[manager.screens_focus];
    struct Workspace *w = *s->workspaces;

    struct Workspace *temp = s->workspaces[3];
    s->workspaces[3] = s->workspaces[0];
    s->workspaces[0] = s->workspaces[2];
    s->workspaces[2] = temp;

    screen_arrange(display, &manager.screens[manager.screens_focus]);
    bar_update(display, &manager.screens[manager.screens_focus]);
}

// TODO reap
void launch(void *arg) {
    if (fork() == 0) {
        execlp((char*)arg, "");
        exit(EXIT_SUCCESS);
    }
}

struct KeyBinding {
    KeySym keysym;
    int mask;
    void (*effect)(void*);
    void *arg;
};

struct KeyBinding bindings[] = {
    {XK_h,      Mod1Mask,             &focus_left,      NULL},
    {XK_l,      Mod1Mask,             &focus_right,     NULL},
    {XK_k,      Mod1Mask,             &focus_up,        NULL},
    {XK_j,      Mod1Mask,             &focus_down,      NULL},
    {XK_q,      Mod1Mask,             &quit,            NULL},
    {XK_h,      Mod1Mask | ShiftMask, &swap_left,       NULL},
    {XK_l,      Mod1Mask | ShiftMask, &swap_right,      NULL},
    {XK_k,      Mod1Mask | ShiftMask, &swap_up,         NULL},
    {XK_j,      Mod1Mask | ShiftMask, &swap_down,       NULL},
    {XK_a,      Mod1Mask,             &workspace_left,  NULL},
    {XK_d,      Mod1Mask,             &workspace_right, NULL},
    {XK_w,      Mod1Mask,             &workspace_up,    NULL},
    {XK_s,      Mod1Mask,             &workspace_down,  NULL},
    {XK_Return, Mod1Mask,             &launch,          TERMINAL},
};
int num_bindings = sizeof(bindings) / sizeof(struct KeyBinding);

void key_handler(XKeyEvent event) {
    for (int i = 0; i < num_bindings; i++) {
        KeyCode keycode = XKeysymToKeycode(display, bindings[i].keysym);

        bool keycode_matches = keycode == event.keycode;
        bool mask_matches = bindings[i].mask == event.state;
        if (keycode_matches && mask_matches) {
            bindings[i].effect(bindings[i].arg);
            return;
        }
    }
}

void maprequest_handler(XMapRequestEvent event) {
    manager_add_window(display, &manager, event.window);
    screen_arrange(display, &manager.screens[manager.screens_focus]);
    manager_refocus(display, &manager);
    bar_update(display, &manager.screens[manager.screens_focus]);
}

void init() {
    display = XOpenDisplay(NULL);
    root = XRootWindow(display, 0);
    manager = manager_new(display);
    for (int i = 0; i < manager.screens_length; i++) {
        bar_update(display, &manager.screens[i]);
    }

    XSelectInput(display, root, KeyPressMask | SubstructureRedirectMask);
    XSetInputFocus(display, root, RevertToPointerRoot, CurrentTime);
}

void cleanup() {
    XCloseDisplay(display);
    manager_free(display, &manager);
}

void event_loop() {
    XEvent event;
    while (!will_quit) {
        XNextEvent(display, &event);
        switch (event.type) {
        case KeyPress:
            key_handler(event.xkey);
            break;
        case MapRequest:
            maprequest_handler(event.xmaprequest);
            break;
        }
    }
}

int main() {
    init();
    event_loop();
    cleanup();
    return EXIT_SUCCESS;
}
