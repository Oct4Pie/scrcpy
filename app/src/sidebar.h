#ifndef SIDEBAR_H
#define SIDEBAR_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdint.h>
#include "android/keycodes.h"
#include "input_manager.h"
#include "screen.h"

struct sc_sidebar_params {
    int16_t window_x;
    int16_t window_y;
    uint16_t width;
    uint16_t height;
};

typedef struct {
    SDL_Texture* minimize;
    SDL_Texture* close;
    SDL_Texture* more;
    SDL_Texture* power;
    SDL_Texture* rotateLeft;
    SDL_Texture* rotateRight;
    SDL_Texture* rotate;
    SDL_Texture* volumeDown;
    SDL_Texture* volumeUp;
    SDL_Texture* circle;
    SDL_Texture* triangle;
    SDL_Texture* square;
    int num;
} SidebarIcons;

typedef struct {
    bool power;
} deviceState;

struct sc_sidebar {
    SidebarIcons icons;
    deviceState device_state;
    struct sc_screen* screen;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Rect iconsRect[12];
    double dpi_y, dpi_x;
    unsigned int win_id;
    bool isDragging;
    int dragOffsetX, dragOffsetY;
    int last_mouseX, last_mouseY;
    int last_WindowX, last_WindowY;
    int width, height;
};

bool sc_sidebar_init(struct sc_sidebar* sidebar, struct sc_screen* screen, struct sc_sidebar_params* params);
void sc_sidebar_render(struct sc_sidebar* sidebar);
void sc_sidebar_destroy(struct sc_sidebar* sidebar);
void sc_sidebar_handle_resize(struct sc_sidebar* sidebar, bool expose);
bool sc_sidebar_handle_event(struct sc_sidebar* sidebar, SDL_Event* event);

#endif  // SIDEBAR_H
