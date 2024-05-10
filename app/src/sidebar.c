#include "sidebar.h"
#include <stdio.h>
#define WIDTH 50

bool load_icons(struct sc_sidebar* sidebar, SDL_Renderer* renderer) {
    const char* basePath = "../resources/icons/";
    const char* paths[] = {
        "power.png",
        "volume_up.png",
        "volume_down.png",
        "rotate_left.png",
        "rotate_right.png",
        "rotate.png",
        "triangle.png",
        "circle.png",
        "square.png",
        "more.png",
        "close.png",
        "minimize.png",
    };
    SDL_Texture** icons[] = {
        &sidebar->icons.power,
        &sidebar->icons.volumeUp,
        &sidebar->icons.volumeDown,
        &sidebar->icons.rotateLeft,
        &sidebar->icons.rotateRight,
        &sidebar->icons.rotate,
        &sidebar->icons.triangle,
        &sidebar->icons.circle,
        &sidebar->icons.square,
        &sidebar->icons.more,
        &sidebar->icons.close,
        &sidebar->icons.minimize,
    };

    char fullPath[1024];
    sidebar->icons.num = sizeof(icons) / sizeof(icons[0]);

    for (int i = 0; i < sidebar->icons.num; ++i) {
        snprintf(fullPath, sizeof(fullPath), "%s%s", basePath, paths[i]);
        *icons[i] = IMG_LoadTexture(renderer, fullPath);
        if (!*icons[i]) {
            printf("Failed to load %s: %s\n", fullPath, IMG_GetError());
            return false;
        }
    }
    return true;
}

bool sc_sidebar_init(struct sc_sidebar* sidebar, struct sc_screen* screen, struct sc_sidebar_params* params) {
    sidebar->screen = screen;
    uint32_t window_flags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_BORDERLESS;
    sidebar->window = SDL_CreateWindow("Sidebar", params->window_x, params->window_x, params->width, params->height, window_flags);
    if (!sidebar->window) {
        printf("Could not create sidebar window: %s\n", SDL_GetError());
        return false;
    }

    sidebar->win_id = SDL_GetWindowID(sidebar->window);

    sidebar->renderer = SDL_CreateRenderer(sidebar->window, -1, SDL_RENDERER_ACCELERATED);
    if (!sidebar->renderer) {
        SDL_DestroyWindow(sidebar->window);
        printf("Could not create renderer for sidebar window: %s\n", SDL_GetError());
        return false;
    }
    SDL_ShowWindow(sidebar->window);
    sidebar->isDragging = false;

    if (!load_icons(sidebar, sidebar->renderer)) {
        printf("Could not load icons.\n");
        return false;
    }

    sidebar->device_state.power = true;

    int ww, wh, dw, dh;
    SDL_GetWindowSize(sidebar->window, &ww, &wh);
    SDL_Metal_GetDrawableSize(sidebar->window, &dw, &dh);
    sidebar->dpi_x = dw / ww;
    sidebar->dpi_y = dh / wh;

    return true;
}

void sc_sidebar_handle_resize(struct sc_sidebar* sidebar, bool expose) {
    int window_width, window_height, x, y;
    const int width = WIDTH;
    SDL_GetWindowSize(sidebar->screen->window, &window_width, &window_height);
    SDL_SetWindowSize(sidebar->window, width, width * 0.75 * (sizeof(sidebar->icons) / sizeof(sidebar->icons.more)));
    if (expose) {
        SDL_GetWindowPosition(sidebar->screen->window, &x, &y);
        SDL_SetWindowPosition(sidebar->window, x + window_width + 10, y);
    }
}

bool handle_button_click(struct sc_sidebar* sidebar, int mouseX, int mouseY) {
    // SDL_Texture* iconTextures[] = {
    //     sidebar->icons.power,
    //     sidebar->icons.volumeUp,
    //     sidebar->icons.volumeDown,
    //     sidebar->icons.rotateLeft,
    //     sidebar->icons.rotateRight,
    //     sidebar->icons.rotate,
    //     sidebar->icons.triangle,
    //     sidebar->icons.circle,
    //     sidebar->icons.square,
    //     sidebar->icons.more,
    // };
    mouseX *= sidebar->dpi_x;
    mouseY *= sidebar->dpi_y;
    // Check if the close button is clicked
    if (SDL_PointInRect(&(SDL_Point){mouseX, mouseY}, &sidebar->iconsRect[0])) {
        SDL_DestroyWindow(sidebar->window);  // Close the sidebar window
        return true;
    }

    // Check if the minimize button is clicked
    if (SDL_PointInRect(&(SDL_Point){mouseX, mouseY}, &sidebar->iconsRect[1])) {
        SDL_MinimizeWindow(sidebar->window);  // Minimize the sidebar window
        return true;
    }

    for (int i = 2; i < sidebar->icons.num; i++) {
        if (SDL_PointInRect(&(SDL_Point){mouseX, mouseY}, &sidebar->iconsRect[i])) {
            switch (i) {
                case 2:
                    // set_screen_power_mode(&sidebar->screen->im, sidebar->device_state.power
                    //                                                 ? SC_SCREEN_POWER_MODE_OFF
                    //                                                 : SC_SCREEN_POWER_MODE_NORMAL);
                    // sidebar->device_state.power = !sidebar->device_state.power;
                    send_keycode(&sidebar->screen->im, AKEYCODE_POWER, SC_ACTION_DOWN, "POWER");
                    break;
                case 3:

                    send_keycode(&sidebar->screen->im, AKEYCODE_VOLUME_UP, SC_ACTION_DOWN, "VOLUME_UP");
                    break;
                case 4:
                    send_keycode(&sidebar->screen->im, AKEYCODE_VOLUME_DOWN, SC_ACTION_DOWN, "VOLUME_DOWN");
                    break;
                case 5:
                    apply_orientation_transform(&sidebar->screen->im,
                                                SC_ORIENTATION_270);
                    break;
                case 6:
                    apply_orientation_transform(&sidebar->screen->im,
                                                SC_ORIENTATION_90);
                    break;
                case 7:
                    rotate_device(&sidebar->screen->im);
                    break;
                case 8:
                    send_keycode(&sidebar->screen->im, AKEYCODE_BACK, SC_ACTION_UP, "BACK");
                    break;
                case 9:
                    send_keycode(&sidebar->screen->im, AKEYCODE_HOME, SC_ACTION_UP, "HOME");
                    break;
                case 10:
                    send_keycode(&sidebar->screen->im, AKEYCODE_APP_SWITCH, SC_ACTION_UP, "APP_SWITCH");
                    break;

                case 11:
                    break;
            }
            return true;
        }
    }

    return false;  // No buttons matched
}

void sc_sidebar_render(struct sc_sidebar* sidebar) {
    int window_width, window_height;
    SDL_GetWindowSize(sidebar->window, &window_width, &window_height);
    SDL_SetRenderDrawColor(sidebar->renderer, 245, 245, 245, 255);
    SDL_RenderClear(sidebar->renderer);

    int buttonWidth = window_width * 0.3;
    int buttonHeight = buttonWidth;
    int buttonY = 10;
    int minimizeX = window_width * sidebar->dpi_x - (buttonWidth)*sidebar->dpi_x - 10;
    int closeX = 10;
    // Close and minimize buttons
    SDL_Rect closeRect = {closeX, buttonY, buttonWidth * sidebar->dpi_x, buttonHeight * sidebar->dpi_y};
    SDL_Rect minimizeRect = {minimizeX, buttonY, buttonWidth * sidebar->dpi_x, buttonHeight * sidebar->dpi_y};
    sidebar->iconsRect[0] = closeRect;
    sidebar->iconsRect[1] = minimizeRect;
    SDL_SetTextureColorMod(sidebar->icons.close, 100, 100, 100);
    SDL_SetTextureColorMod(sidebar->icons.minimize, 100, 100, 100);
    SDL_RenderCopy(sidebar->renderer, sidebar->icons.close, NULL, &closeRect);
    SDL_RenderCopy(sidebar->renderer, sidebar->icons.minimize, NULL, &minimizeRect);

    int icons = (sizeof(sidebar->icons) / sizeof(sidebar->icons.more));
    int iconWidth = (int)(window_width) * 0.5;
    int iconHeight = iconWidth;
    int startX = (window_width - iconWidth);
    SDL_Rect iconRect = {startX, iconHeight * sidebar->dpi_y, iconWidth * sidebar->dpi_x, iconHeight * sidebar->dpi_y};

    SDL_Texture* iconTextures[] = {
        sidebar->icons.power,
        sidebar->icons.volumeUp,
        sidebar->icons.volumeDown,
        sidebar->icons.rotateLeft,
        sidebar->icons.rotateRight,
        sidebar->icons.rotate,
        sidebar->icons.triangle,
        sidebar->icons.circle,
        sidebar->icons.square,
        sidebar->icons.more,
    };

    for (int i = 0; i < (int)(sizeof(iconTextures) / sizeof(iconTextures[0])); ++i) {
        SDL_SetTextureColorMod(iconTextures[i], 100, 100, 100);  // darken the icons
        sidebar->iconsRect[i + 2] = iconRect;
        SDL_RenderCopy(sidebar->renderer, iconTextures[i], NULL, &iconRect);

        iconRect.y += (window_height / icons * (sidebar->dpi_y + sidebar->dpi_y / 5));  // Move down for the next icon
    }

    SDL_RenderPresent(sidebar->renderer);
}

void sc_sidebar_destroy(struct sc_sidebar* sidebar) {
    if (sidebar->renderer) {
        SDL_DestroyRenderer(sidebar->renderer);
    }
    if (sidebar->window) {
        SDL_DestroyWindow(sidebar->window);
    }
}

bool sc_sidebar_handle_event(struct sc_sidebar* sidebar, SDL_Event* event) {
    switch (event->type) {
        case SDL_WINDOWEVENT:
            if (event->window.windowID == sidebar->win_id) {
                switch (event->window.event) {
                    case SDL_WINDOWEVENT_EXPOSED:
                        sc_sidebar_render(sidebar);
                };
                return true;
            }

            break;
        case SDL_MOUSEBUTTONDOWN:
            if (event->button.windowID == sidebar->win_id && event->button.button == SDL_BUTTON_LEFT) {
                int mouseX = event->button.x;
                int mouseY = event->button.y;

                if (!handle_button_click(sidebar, mouseX, mouseY)) {
                    sidebar->isDragging = true;
                    sidebar->dragOffsetX = event->button.x;  // Local coordinates
                    sidebar->dragOffsetY = event->button.y;  // Local coordinates
                    SDL_GetWindowPosition(sidebar->window, &sidebar->last_WindowX, &sidebar->last_WindowY);
                    sidebar->last_mouseX = event->button.x;
                    sidebar->last_mouseY = event->button.y;
                    return true;
                }
                sc_sidebar_render(sidebar);
                return true;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (event->button.windowID == sidebar->win_id && event->button.button == SDL_BUTTON_LEFT) {
                sidebar->isDragging = false;
                sc_sidebar_render(sidebar);
                return true;
            }
            break;
        case SDL_MOUSEMOTION:
            if (sidebar->isDragging && event->motion.windowID == sidebar->win_id) {
                // Calculate new window position based on the drag
                int mouseX, mouseY;
                SDL_GetGlobalMouseState(&mouseX, &mouseY);

                int deltaX = mouseX - (sidebar->last_WindowX + sidebar->dragOffsetX);
                int deltaY = mouseY - (sidebar->last_WindowY + sidebar->dragOffsetY);

                SDL_SetWindowPosition(sidebar->window, sidebar->last_WindowX + deltaX, sidebar->last_WindowY + deltaY);

                // Update last window position for smoothness
                sidebar->last_WindowX += deltaX;
                sidebar->last_WindowY += deltaY;
                sc_sidebar_render(sidebar);
                return true;
            }
            break;
    }

    return false;
}