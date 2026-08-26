#include <stdbool.h>
#include <X11/Xlib.h>

#include "structs.h"

enum InsertDirection {
    INSERT_BEFORE,
    INSERT_AFTER,
};

struct Manager manager_new(Display *display);
void manager_free(Display *display, struct Manager *m);
void track_remove_client(struct Track *t, struct Client *c);
void track_add_client
(struct Track *t, struct Client *c, enum InsertDirection direction, bool focus_it);
void workspace_remove_track(struct Workspace *w, struct Track *t);
void workspace_add_new_track
(struct Workspace *w, enum InsertDirection direction, bool focus_it);
void screen_arrange(Display *display, struct ScreenArea *s);
void manager_add_window(Display *display, struct Manager *m, Window window);
void manager_refocus(Display *display, struct Manager *m);
