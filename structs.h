#pragma once

#include <X11/Xlib.h>

struct Client {
    struct Client *next;
    struct Client *prev;

    Window window;
    double weight;
    int x;
    int y;
    int width;
    int height;
};

struct Track {
    struct Track *next;
    struct Track *prev;

    struct Client *first_client;
    struct Client *focused_client;
    int total_clients;
    double total_clients_weight;
    double weight;
};

struct Workspace {
    struct Track *first_track;
    struct Track *focused_track;
    int total_tracks;
    double total_tracks_weight;
};

// "Screen" conflicts with Xlib's "Screen"
struct ScreenArea {
    // Array of pointers so that they can be swapped easily.
    // The i-th pointer points to the workspace in the corresponding position:
    //     [3]
    // [4] [0] [1]
    //     [2]
    struct Workspace **workspaces;
    int x;
    int y;
    int width;
    int height;
    struct Bar *bar;
    struct Bar *guide_x;
    struct Bar *guide_y;
    Window virtual_window;
};

struct Manager {
    struct ScreenArea *screens;
    int screens_length;
    int screens_focus;
};
