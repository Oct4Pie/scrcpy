#ifndef KEYMAP_H
#define KEYMAP_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include "input_manager.h"

#define MAX_KEY_MAPPINGS 50
#define MAX_CIRCLES 30

typedef struct {
    SDL_Keycode keycode;
    const char* text;
} KeyMapping;

static const KeyMapping keyMappings[] = {
    {SDLK_TAB, "tab"},
    {SDLK_LCTRL, "lctl"},
    {SDLK_RCTRL, "rctl"},
    {SDLK_LSHIFT, "lshft"},
    {SDLK_RSHIFT, "rshft"},
    {SDLK_LALT, "lalt"},
    {SDLK_RALT, "ralt"},
    {SDLK_LGUI, "lcmd"},
    {SDLK_RGUI, "rcmd"},
    {SDLK_RETURN, "entr"},
    {SDLK_ESCAPE, "esc"},
    {SDLK_SPACE, "spc"},
    {SDLK_BACKSPACE, "dlt"},
    {SDLK_UP, "up"},
    {SDLK_DOWN, "dwn"},
    {SDLK_LEFT, "left"},
    {SDLK_RIGHT, "right"},
    {SDLK_CAPSLOCK, "clock"},
    {0, NULL}};

typedef struct {
    SDL_Texture* texture;
    SDL_Point position;
    char text[32];
    bool isSelected;
    float x_ratio;
    float y_ratio;
    int offsetX, offsetY;
    int radius;
    char direction[16];
} Circle;

typedef struct {
    struct sc_keymap_screen* screen;
    int currentStep;
    int steps;
    int startX;
    int startY;
    int endX;
    int endY;
} JoystickData;

typedef struct {
    struct sc_keymap_screen* screen;
    SDL_Window* window;
    int currentStep;
    int steps;
    int startX;
    int startY;
    int endX;
    int endY;
    int deltaX;
    int deltaY;
    bool active;
} SwipeData;

typedef struct {
    SDL_Texture* arrowTextures[4];
    TTF_Font* font;
} ApplicationContext;

typedef struct {
    bool m_being_displayed;
    bool s_active;
    bool j_done;
    bool j_active;
    bool j_in_progress;
    char j_direction[16];
    SDL_Keycode j_keycode;
} State;

struct sc_keymap_screen {
    struct sc_input_manager* im;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    TTF_Font* font;
    Circle circles[MAX_CIRCLES];
    char message[256];
    int window_height, window_width;
    int num_circles;
    int selectedCircleIndex;
    bool keymappingMode;
    bool isDragging;
    bool enabled;
    int gesture;
    double dpi_x, dpi_y;
};

void sc_keymap_screen_init(struct sc_keymap_screen* screen, SDL_Window* window, SDL_Renderer* renderer);
bool sc_keymap_screen_handle_event(struct sc_keymap_screen* screen, const SDL_Event* event);
void sc_keymap_screen_render(struct sc_keymap_screen* screen);
void sc_keymap_screen_handle_resize(struct sc_keymap_screen* screen);
void set_message(struct sc_keymap_screen* screen, const char* message);
#endif
