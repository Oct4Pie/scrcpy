#include "keymap.h"
#include <SDL2/SDL_image.h>
#include <unistd.h>
#include "../data/arrow-tip.h"
#include "cjson/cJSON.h"
#include "tinyfiledialogs.h"
#define MOUSE_SENSITIVITY 1.0

State state = {false, false, false, false, false, "", 0};
ApplicationContext context;

void prepare_directional_textures(struct sc_keymap_screen* screen, SDL_Renderer* renderer, SDL_Texture* arrow_left) {
    if (!arrow_left) {
        SDL_Log("Arrow texture is not available.");
        return;
    }

    int w, h;
    SDL_QueryTexture(arrow_left, NULL, NULL, &w, &h);  // Query the texture to get its width and height

    SDL_Point center = {w / 2, h / 2};
    SDL_Rect srcRect = {0, 0, w, h};
    SDL_Rect dstRect = {0, 0, w, h};

    // Create a target texture to hold rotated versions
    for (int i = 0; i < 4; i++) {
        context.arrowTextures[i] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);
        SDL_SetTextureBlendMode(context.arrowTextures[i], SDL_BLENDMODE_BLEND);
        SDL_SetRenderTarget(renderer, context.arrowTextures[i]);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);  // Transparent background
        SDL_RenderClear(renderer);

        double angle = 0.0;
        SDL_RendererFlip flip = SDL_FLIP_NONE;
        switch (i) {
            case 0:
                angle = 90;
                break;  // Up
            case 1:
                angle = -90;
                break;  // Down
            case 2:
                angle = 0;
                break;  // Right
            case 3:
                angle = 0;
                flip = SDL_FLIP_HORIZONTAL;
                break;  // Left (default)
        }

        SDL_RenderCopyEx(renderer, arrow_left, &srcRect, &dstRect, angle, &center, flip);
        SDL_SetRenderTarget(renderer, NULL);  // Reset render target
    }
}

void sc_keymap_screen_init(struct sc_keymap_screen* screen, SDL_Window* window, SDL_Renderer* renderer) {
    screen->window = window;
    screen->renderer = renderer;
    screen->keymappingMode = false;
    screen->isDragging = false;
    screen->num_circles = 0;
    screen->selectedCircleIndex = -1;
    screen->enabled = false;
    screen->gesture = -1;
    TTF_Init();
    context.font = TTF_OpenFont("/Library/Fonts/Arial Unicode.ttf", 12);
    if (context.font == NULL) {
        fprintf(stderr, "Failed to load font: %s\n", TTF_GetError());
        return;
    }

    int ww, wh, dw, dh;
    SDL_GetWindowSize(screen->window, &ww, &wh);
    SDL_Metal_GetDrawableSize(screen->window, &dw, &dh);
    screen->dpi_x = dw / ww;
    screen->dpi_y = dh / wh;

    // Load arrow texture
    SDL_RWops* rw = SDL_RWFromMem(arrow_tip_png, arrow_tip_png_len);
    SDL_Surface* surface = IMG_Load_RW(rw, 1);  // Automatically close RWops after use
    if (!surface) {
        fprintf(stderr, "Failed to load arrow image: %s\n", IMG_GetError());
        return;
    }
    SDL_Texture* left_arrow_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    prepare_directional_textures(screen, renderer, left_arrow_texture);

    // The left_arrow_texture is no longer needed
    SDL_DestroyTexture(left_arrow_texture);
}

void sc_keymap_screen_add_circle(struct sc_keymap_screen* screen, int w, int h, char* text) {
    if (screen->num_circles >= MAX_CIRCLES) {
        fprintf(stderr, "Maximum number of circles reached.\n");
        return;
    }

    int radius = screen->window_width * 0.05;
    if (strcmp(text, "mouse") == 0) {
        radius *= 5;
    }

    double x_ratio = 0.5 * screen->dpi_x;
    double y_ratio = 0.5 * screen->dpi_y;

    // Create texture for the circle
    SDL_Texture* texture = SDL_CreateTexture(screen->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 2 * radius + 1, 2 * radius + 1);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(screen->renderer, texture);
    SDL_SetRenderDrawColor(screen->renderer, 0, 0, 0, 0);  // Clear with transparent
    SDL_RenderClear(screen->renderer);
    SDL_SetRenderDrawColor(screen->renderer, 53, 63, 79, 150);  // Circle fill color

    for (int dy = -radius; dy <= radius; dy++) {
        int dx_length = (int)sqrt((radius * radius) - (dy * dy));  // Calculate the horizontal line length
        SDL_RenderDrawLine(screen->renderer, radius - dx_length, radius + dy, radius + dx_length, radius + dy);
    }
    SDL_SetRenderTarget(screen->renderer, NULL);

    // Set circle properties
    screen->circles[screen->num_circles] = (Circle){
        .texture = texture,
        .position = {w * x_ratio, h * y_ratio},
        .x_ratio = x_ratio,
        .y_ratio = y_ratio,
        .isSelected = false,
        .radius = radius,
        .text = {0}};
    strcpy(screen->circles[screen->num_circles].text, text);

    for (int i = 0; i < screen->num_circles; i++) {
        screen->circles[i].isSelected = false;
    }
    screen->selectedCircleIndex = screen->num_circles;
    screen->num_circles++;
}

void draw_filled_circle(SDL_Renderer* renderer, SDL_Texture* texture, int x, int y, int radius) {
    printf("x: %d y: %d\n", x, y);
    if (texture == NULL) {
        SDL_Log("Texture not found");
        return;
    }
    SDL_Rect dst = {x - radius, y - radius, 2 * radius + 1, 2 * radius + 1};
    SDL_RenderCopy(renderer, texture, NULL, &dst);
}

void draw_circle_outline(SDL_Renderer* renderer, int centerX, int centerY, int radius) {
    SDL_SetRenderDrawColor(renderer, 128, 131, 135, 255);
    int x = radius, y = 0, decisionOver2 = 1 - x;

    while (x >= y) {
        SDL_RenderDrawPoint(renderer, x + centerX, y + centerY);
        SDL_RenderDrawPoint(renderer, y + centerX, x + centerY);
        SDL_RenderDrawPoint(renderer, -x + centerX, y + centerY);
        SDL_RenderDrawPoint(renderer, -y + centerX, x + centerY);
        SDL_RenderDrawPoint(renderer, -x + centerX, -y + centerY);
        SDL_RenderDrawPoint(renderer, -y + centerX, -x + centerY);
        SDL_RenderDrawPoint(renderer, x + centerX, -y + centerY);
        SDL_RenderDrawPoint(renderer, y + centerX, -x + centerY);
        y++;
        if (decisionOver2 <= 0) {
            decisionOver2 += 2 * y + 1;
        } else {
            x--;
            decisionOver2 += 2 * (y - x) + 1;
        }
    }
}

Uint32 release_mouse(Uint32 interval, void* param) {
    (void)interval;
    SDL_Event mouseEvent;
    SDL_zero(mouseEvent);
    mouseEvent.type = SDL_MOUSEBUTTONUP;
    mouseEvent.button.button = SDL_BUTTON_LEFT;
    mouseEvent.button.state = SDL_RELEASED;

    sc_input_manager_handle_event((struct sc_input_manager*)param, &mouseEvent);

    return 0;
}

void emulateMouseClick(struct sc_input_manager* im, int x, int y) {
    SDL_Event mouseEvent;
    SDL_zero(mouseEvent);
    mouseEvent.type = SDL_MOUSEBUTTONDOWN;
    mouseEvent.button.button = SDL_BUTTON_LEFT;
    mouseEvent.button.state = SDL_PRESSED;
    mouseEvent.button.x = x;
    mouseEvent.button.y = y;

    sc_input_manager_handle_event(im, &mouseEvent);
    SDL_AddTimer(125, release_mouse, im);
}

void save_json_to_file(const char* json_string, const char* filepath) {
    FILE* file = fopen(filepath, "w");
    if (file == NULL) {
        fprintf(stderr, "Failed to open file for writing.\n");
        return;
    }
    fprintf(file, "%s", json_string);
    fclose(file);
}

void export_keymap_config(const struct sc_keymap_screen* screen) {
    const char* filterPatterns[] = {"*."};
    const char* defaultSavePath = "keymap_config.json";
    const char* saveFilePath = tinyfd_saveFileDialog(
        "Save Keymap Configuration",
        defaultSavePath,
        1,
        filterPatterns,
        "JSON files");

    if (!saveFilePath) {
        printf("No file was selected or the dialog was cancelled.\n");
        return;
    }

    cJSON* root = cJSON_CreateObject();
    cJSON* circles = cJSON_AddArrayToObject(root, "circles");
    for (int i = 0; i < screen->num_circles; i++) {
        cJSON* circle = cJSON_CreateObject();
        cJSON_AddNumberToObject(circle, "x_ratio", (double)screen->circles[i].position.x / screen->window_width);
        cJSON_AddNumberToObject(circle, "y_ratio", (double)screen->circles[i].position.y / screen->window_height);
        cJSON_AddStringToObject(circle, "text", screen->circles[i].text);
        cJSON_AddStringToObject(circle, "direction", screen->circles[i].direction);
        cJSON_AddItemToArray(circles, circle);
    }

    char* json_string = cJSON_PrintUnformatted(root);  // Use Unformatted for compact storage
    save_json_to_file(json_string, saveFilePath);
    printf("Configuration saved to: %s\n", saveFilePath);

    cJSON_Delete(root);
    free(json_string);
}

void sc_keymap_screen_handle_resize(struct sc_keymap_screen* screen) {
    int window_width, window_height;
    SDL_GetWindowSize(screen->window, &window_width, &window_height);

    for (int i = 0; i < screen->num_circles; i++) {
        screen->circles[i].radius *= (double)(window_width) / (double)(screen->window_width);
        screen->circles[i].position.x = (int)(screen->circles[i].x_ratio * window_width);
        screen->circles[i].position.y = (int)(screen->circles[i].y_ratio * window_height);
    }

    screen->window_height = window_height;
    screen->window_width = window_width;
    int fsize = window_width * 0.025;

    if (abs(window_width * screen->window_width - fsize) > 3) {
        if (context.font) {
            TTF_CloseFont(context.font);
            context.font = NULL;  // Set to NULL to avoid using a dangling pointer
            context.font = TTF_OpenFont("/Library/Fonts/Arial Unicode.ttf", fsize);
            if (context.font == NULL) {
                fprintf(stderr, "Failed to load font: %s\n", TTF_GetError());
                return;
            }
        }
    }
}

Uint32 reset_message(Uint32 interval, void* param) {
    (void)interval;
    state.m_being_displayed = false;
    struct sc_keymap_screen* screen = (struct sc_keymap_screen*)param;
    memset(screen->message, 0, sizeof(screen->message));
    return 0;
}

void set_message(struct sc_keymap_screen* screen, const char* message) {
    if (message == NULL) {
        printf("Invalid input to set_message function.\n");
        return;
    }
    // strncpy(screen->message, message, sizeof(screen->message) - 1);
    // screen->message[sizeof(screen->message) - 1] = '\0';
    snprintf(screen->message, sizeof(screen->message), "%s", message);
}

void display_message(struct sc_keymap_screen* screen) {
    SDL_Color textColor = {255, 255, 255, 255};
    SDL_Surface* textSurface = TTF_RenderText_Solid(context.font, screen->message, textColor);
    if (!textSurface) {
        SDL_Log("Unable to create surface from text: %s", TTF_GetError());
        return;
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(screen->renderer, textSurface);
    if (!textTexture) {
        SDL_Log("Unable to create texture from surface: %s", SDL_GetError());
        SDL_FreeSurface(textSurface);
        return;
    }

    // Calculate the position to center the text on the screen
    int textWidth = textSurface->w;
    int textHeight = textSurface->h;
    int textX = (screen->window_width - textWidth / screen->dpi_x);
    int textY = (screen->window_height - textHeight - screen->window_height * (1.5 / screen->dpi_y));

    SDL_Rect rect = {
        textX - screen->window_width * 0.025,
        textY - screen->window_height * 0.025,
        textWidth + screen->window_width * 0.05,
        textHeight + screen->window_height * 0.05};
    SDL_SetRenderDrawColor(screen->renderer, 0, 0, 0, 128);  // Semi-transparent black
    SDL_SetRenderDrawBlendMode(screen->renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderFillRect(screen->renderer, &rect);

    SDL_Rect textRect = {textX, textY, textWidth, textHeight};
    SDL_RenderCopy(screen->renderer, textTexture, NULL, &textRect);

    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);

    if (!state.m_being_displayed) {
        state.m_being_displayed = true;
        SDL_AddTimer(3000, reset_message, screen);
    }
}

void add_joystick(struct sc_keymap_screen* screen, int centerX, int centerY) {
    centerX /= 2;
    centerY *= 1.5;
    const char* directions[] = {"up", "dn", "lt", "rt"};
    int JOYSTICK_RADIUS = (int)(centerX * 0.2);
    int offsets[][2] = {{0, -JOYSTICK_RADIUS}, {0, JOYSTICK_RADIUS}, {-JOYSTICK_RADIUS, 0}, {JOYSTICK_RADIUS, 0}};

    for (int i = 0; i < 4; i++) {
        if (screen->num_circles < MAX_CIRCLES) {
            Circle* c = &screen->circles[screen->num_circles++];
            c->position.x = centerX + offsets[i][0];
            c->position.y = centerY + offsets[i][1];
            strncpy(c->text, directions[i], sizeof(c->text));  // Potentially redundant if using textures
            strncpy(c->direction, directions[i], sizeof(c->direction));
            c->isSelected = false;
            c->radius = screen->window_width * 0.05;
            c->x_ratio = (float)(centerX + offsets[i][0]) / screen->window_width;
            c->y_ratio = (float)(centerY + offsets[i][1]) / screen->window_height;

            switch (i) {
                case 0:  // Up
                    c->texture = context.arrowTextures[0];
                    break;
                case 1:  // Down
                    c->texture = context.arrowTextures[1];
                    break;
                case 2:  // Left
                    c->texture = context.arrowTextures[2];
                    break;
                case 3:  // Right
                    c->texture = context.arrowTextures[3];
                    break;
            }
        }
    }
}

void render_text(struct sc_keymap_screen* screen, SDL_Renderer* renderer, int x, int y, const char* text) {
    SDL_Color color = {255, 255, 255, 255};
    SDL_Surface* surface = TTF_RenderText_Solid(context.font, text, color);
    if (surface == NULL) {
        fprintf(stderr, "Failed to create surface from text: %s\n", TTF_GetError());
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == NULL) {
        fprintf(stderr, "Failed to create texture from surface: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect renderQuad = {x - surface->w / screen->dpi_x, y - surface->h / screen->dpi_y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &renderQuad);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void sc_keymap_screen_remove_selected_circle(struct sc_keymap_screen* screen) {
    if (screen->selectedCircleIndex == -1) {
        return;
    }

    // if (strcmp(screen->circles[screen->selectedCircleIndex].text, "mouse") == 0) {
    //     screen->gesture = -1;
    // }

    if (screen->circles[screen->selectedCircleIndex].texture != NULL && !(strlen(screen->circles[screen->selectedCircleIndex].direction) > 0)) {
        SDL_DestroyTexture(screen->circles[screen->selectedCircleIndex].texture);
        screen->circles[screen->selectedCircleIndex].texture = NULL;
    }

    for (int i = screen->selectedCircleIndex; i < screen->num_circles - 1; i++) {
        screen->circles[i] = screen->circles[i + 1];
    }

    screen->num_circles--;
    screen->selectedCircleIndex = -1;

    bool mousePresent = false;

    for (int i = 0; i < screen->num_circles; i++) {
        if (strcmp(screen->circles[i].text, "mouse") == 0) {
            mousePresent = true;
            break;
        }
    }

    if (!mousePresent) {
        screen->gesture = -1;
    }
}

const char* getKeyText(SDL_Keycode keycode) {
    for (const KeyMapping* map = keyMappings; map->text != NULL; map++) {
        if (map->keycode == keycode) {
            return map->text;
        }
    }
    return NULL;
}

void load_keymap_config(struct sc_keymap_screen* screen) {
    const char* openFilePath = tinyfd_openFileDialog(
        "Open Keymap Configuration",
        "",
        0,
        NULL,
        "JSON files",
        0);

    if (!openFilePath) {
        printf("No file selected or dialog cancelled.\n");
        return;
    }

    FILE* file = fopen(openFilePath, "r");
    if (!file) {
        fprintf(stderr, "Failed to open file for reading: %s\n", openFilePath);
        return;
    }

    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* jsonString = malloc(fsize + 1);
    fread(jsonString, 1, fsize, file);
    fclose(file);
    jsonString[fsize] = '\0';

    cJSON* root = cJSON_Parse(jsonString);
    if (!root) {
        fprintf(stderr, "Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        free(jsonString);
        return;
    }

    cJSON* circles = cJSON_GetObjectItemCaseSensitive(root, "circles");
    cJSON* circle;
    screen->num_circles = 0;
    cJSON_ArrayForEach(circle, circles) {
        Circle newCircle;
        cJSON* x_ratio = cJSON_GetObjectItemCaseSensitive(circle, "x_ratio");
        cJSON* y_ratio = cJSON_GetObjectItemCaseSensitive(circle, "y_ratio");
        cJSON* text = cJSON_GetObjectItemCaseSensitive(circle, "text");
        cJSON* direction = cJSON_GetObjectItemCaseSensitive(circle, "direction");  // Ensure to load direction

        if (strcmp(newCircle.text, "mouse") == 0) {
            screen->gesture = screen->num_circles - 1;
        }

        newCircle.position.x = (int)(x_ratio->valuedouble * screen->window_width);
        newCircle.position.y = (int)(y_ratio->valuedouble * screen->window_height);
        strncpy(newCircle.text, text->valuestring, sizeof(newCircle.text) - 1);
        strncpy(newCircle.direction, direction->valuestring, sizeof(newCircle.direction) - 1);
        newCircle.text[sizeof(newCircle.text) - 1] = '\0';
        newCircle.direction[sizeof(newCircle.direction) - 1] = '\0';
        screen->circles[screen->num_circles++] = newCircle;
    }

    cJSON_Delete(root);
    free(jsonString);
    printf("Configuration loaded from: %s\n", openFilePath);
}

Uint32 performSwipeStep(Uint32 interval, void* param) {
    SwipeData* data = (SwipeData*)param;
    int newX = data->startX + data->deltaX * data->currentStep / data->steps;
    int newY = data->startY + data->deltaY * data->currentStep / data->steps;

    SDL_Event event;
    SDL_zero(event);
    event.type = SDL_MOUSEMOTION;
    event.motion.state = SDL_BUTTON_LEFT;
    event.motion.x = newX;
    event.motion.y = newY;
    // SDL_PushEvent(&event);
    sc_input_manager_handle_event(data->screen->im, &event);

    if (data->currentStep >= data->steps) {
        SDL_zero(event);
        event.type = SDL_MOUSEBUTTONUP;
        event.button.button = SDL_BUTTON_LEFT;
        event.button.state = SDL_RELEASED;
        event.button.x = data->endX;
        event.button.y = data->endY;
        // SDL_PushEvent(&event);
        sc_input_manager_handle_event(data->screen->im, &event);
        free(data);
        state.s_active = false;
        // SDL_AddTimer(1, resetSyntheticFlag, &swipeState.active);
        return 0;
    }

    data->currentStep++;

    return interval;
}

void simulate_swipe(struct sc_keymap_screen* screen, int startX, int startY, int endX, int endY) {
    if (state.s_active) {
        return;
    }
    state.s_active = true;
    int deltaX = endX - startX;
    int deltaY = endY - startY;
    double distance = sqrt(deltaX * deltaX + deltaY * deltaY);
    int steps = distance;
    int interval = 50;

    if (distance >= 200) {
        steps /= 3;
        interval = 1;
    } else if (distance >= 100) {
        steps /= 2;
        interval = 2;
    } else if (distance >= 100) {
        steps /= 2;
        interval = 3;
    } else if (distance >= 50) {
        steps /= 1.5;
        interval = 5;
    } else if (distance >= 10) {
        interval = 7;
    } else if (distance >= 5) {
        interval = 15;
    }

    SDL_Event event;
    SDL_zero(event);
    event.type = SDL_MOUSEBUTTONDOWN;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.state = SDL_PRESSED;
    event.button.x = startX;
    event.button.y = startY;
    // SDL_PushEvent(&event);
    sc_input_manager_handle_event(screen->im, &event);

    // Prepare swipe data
    SwipeData* data = (SwipeData*)malloc(sizeof(SwipeData));
    if (data == NULL) {
        fprintf(stderr, "Memory allocation failure for SwipeState\n");
        state.s_active = false;
        return;
    }

    data->screen = screen;
    data->currentStep = 0;
    data->steps = steps;
    data->startX = startX;
    data->startY = startY;
    data->endX = endX;
    data->endY = endY;
    data->deltaX = endX - startX;
    data->deltaY = endY - startY;

    SDL_AddTimer(interval, performSwipeStep, data);
}

bool handle_mouse_motion(struct sc_keymap_screen* screen, const SDL_Event* event, int w, int h) {
    if (screen->gesture != -1) {
        Circle gestureCircle = screen->circles[screen->gesture];
        int centerX = gestureCircle.position.x / screen->dpi_x;
        int centerY = gestureCircle.position.y / screen->dpi_y;
        printf("centerX: %d centerY: %d\n", centerX, centerY);

        int deltaX = event->motion.xrel * MOUSE_SENSITIVITY;
        int deltaY = event->motion.yrel * MOUSE_SENSITIVITY;
        int newCenterX = centerX + deltaX;
        int newCenterY = centerY + deltaY;

        if (deltaX != 0 || deltaY != 0) {
            simulate_swipe(screen, centerX, centerY, newCenterX, newCenterY);
            SDL_WarpMouseInWindow(screen->window, (int)w / screen->dpi_x, (int)h / screen->dpi_y);
        }
        return true;
    }
    return false;
}

Uint32 perform_directional_swipe(Uint32 interval, void* param) {
    JoystickData* data = (JoystickData*)param;
    SDL_Event event;

    if (data->currentStep >= data->steps) {
        if (state.j_done) {
            SDL_zero(event);
            event.type = SDL_MOUSEBUTTONUP;
            event.button.button = SDL_BUTTON_LEFT;
            event.button.state = SDL_RELEASED;
            event.button.x = data->endX;
            event.button.y = data->endY;
            state.j_in_progress = false;
            // SDL_PushEvent(&event);
            sc_input_manager_handle_event(data->screen->im, &event);
            free(data);
            return 0;
        }
        return interval;
    }

    SDL_zero(event);
    event.type = SDL_MOUSEMOTION;
    event.motion.state = SDL_BUTTON_LEFT;
    event.motion.x = data->endX;
    event.motion.y = data->endY;
    // SDL_PushEvent(&event);
    sc_input_manager_handle_event(data->screen->im, &event);

    data->currentStep++;

    return interval;
}

void handle_joystick_key(struct sc_keymap_screen* screen, Circle circle, SDL_Keycode key) {
    if (state.j_in_progress) {
        return;
    }
    state.j_keycode = key;
    state.j_in_progress = true;
    state.j_done = false;
    JoystickData* data = (JoystickData*)malloc(sizeof(JoystickData));
    if (!data) {
        printf("Failed to allocate memory for SwipeState.\n");
        return;
    }

    data->startX = (int)circle.position.x / screen->dpi_x;
    data->startY = (int)circle.position.y / screen->dpi_y;
    data->endX = data->startX;
    data->endY = data->startY;
    data->currentStep = 0;
    data->steps = 50;
    data->screen = screen;

    int magnitude = screen->window_width * 0.05;

    if (strcmp(circle.direction, "up") == 0) {
        data->endX = data->startX;
        data->endY -= magnitude / screen->dpi_y;
        data->startY += magnitude / screen->dpi_y;
    } else if (strcmp(circle.direction, "dn") == 0) {
        data->endX = data->startX;
        data->endY += magnitude / screen->dpi_y;
        data->startY -= magnitude / screen->dpi_y;
    } else if (strcmp(circle.direction, "lt") == 0) {
        data->endY = data->startY;
        data->endX -= magnitude / screen->dpi_x;
        data->startX += magnitude / screen->dpi_x;
    } else if (strcmp(circle.direction, "rt") == 0) {
        data->endY = data->startY;
        data->endX += magnitude / screen->dpi_x;
        data->startX -= magnitude / screen->dpi_x;
    } else {
        free(data);
        return;
    }

    SDL_Event event;
    SDL_zero(event);
    event.type = SDL_MOUSEBUTTONDOWN;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.state = SDL_PRESSED;
    event.button.x = data->startX;
    event.button.y = data->startY;
    // SDL_PushEvent(&event);
    sc_input_manager_handle_event(screen->im, &event);
    SDL_AddTimer(1, perform_directional_swipe, data);
}

bool sc_keymap_screen_handle_event(struct sc_keymap_screen* screen, const SDL_Event* event) {
    int circle_r = screen->window_width * 0.05;

    if (screen->enabled && !screen->keymappingMode) {
        switch (event->type) {
            case SDL_KEYDOWN:
                if (screen->num_circles > 0) {
                    const char* keyText = getKeyText(event->key.keysym.sym);
                    char keyChar[2] = {'\0', '\0'};  // Buffer for single character

                    if (event->key.keysym.sym >= 32 && event->key.keysym.sym <= 126) {
                        keyChar[0] = (char)event->key.keysym.sym;
                    }

                    for (int i = 0; i < screen->num_circles; i++) {
                        Circle* circle = &screen->circles[i];

                        const char* compareText = keyText ? keyText : keyChar;

                        if (compareText[0] != '\0' && strcmp(circle->text, compareText) == 0) {
                            if (strlen(circle->direction) > 0) {
                                handle_joystick_key(screen, *circle, event->key.keysym.sym);
                            } else {
                                emulateMouseClick(screen->im, circle->position.x / screen->dpi_x, circle->position.y / screen->dpi_y);
                            }
                            return true;
                        }
                    }
                }
                break;

            case SDL_MOUSEMOTION:
                return handle_mouse_motion(screen, event, screen->window_width, screen->window_height);
                break;

            case SDL_KEYUP:

                if (event->key.keysym.sym == state.j_keycode) {
                    state.j_done = true;
                }
                break;
        }
        return true;
    }

    switch (event->type) {
        case SDL_WINDOWEVENT:
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            if (event->button.button == SDL_BUTTON_LEFT) {
                for (int i = 0; i < screen->num_circles; i++) {
                    int dx = event->button.x * screen->dpi_x - screen->circles[i].position.x;
                    int dy = event->button.y * screen->dpi_y - screen->circles[i].position.y;
                    if (strcmp(screen->circles[i].text, "mouse") == 0) {
                        if (dx * dx + dy * dy <= (circle_r * 5) * (circle_r * 5)) {
                            screen->selectedCircleIndex = i;

                            for (int j = 0; j < screen->num_circles; j++) {
                                screen->circles[j].isSelected = (j == i);
                            }
                            screen->circles[i].offsetX = dx;
                            screen->circles[i].offsetY = dy;
                            screen->isDragging = event->type == SDL_MOUSEBUTTONDOWN;
                            return true;
                        }
                    }

                    if (dx * dx + dy * dy <= circle_r * circle_r) {
                        screen->selectedCircleIndex = i;

                        for (int j = 0; j < screen->num_circles; j++) {
                            screen->circles[j].isSelected = (j == i);
                        }
                        screen->circles[i].offsetX = dx;
                        screen->circles[i].offsetY = dy;
                        screen->isDragging = event->type == SDL_MOUSEBUTTONDOWN;
                        return true;
                    }
                }
                if (event->type == SDL_MOUSEBUTTONUP) {
                    screen->isDragging = false;
                }
            }
            break;
        case SDL_MOUSEMOTION:
            if (screen->isDragging && screen->selectedCircleIndex != -1) {
                Circle* selectedCircle = &screen->circles[screen->selectedCircleIndex];
                selectedCircle->position.x = event->motion.x * screen->dpi_x - selectedCircle->offsetX;
                selectedCircle->position.y = event->motion.y * screen->dpi_y - selectedCircle->offsetY;

                selectedCircle->x_ratio = (float)selectedCircle->position.x / screen->window_width;
                selectedCircle->y_ratio = (float)selectedCircle->position.y / screen->window_height;
                return true;
            }
            break;
        case SDL_KEYDOWN:
            if (event->key.keysym.sym == SDLK_EQUALS && (event->key.keysym.mod & KMOD_GUI)) {
                sc_keymap_screen_add_circle(screen, screen->window_width, screen->window_height, "lctl");
                return true;
            } else if (screen->keymappingMode && (event->key.keysym.mod & KMOD_GUI) && event->key.keysym.sym == SDLK_l) {
                sc_keymap_screen_add_circle(screen, screen->window_width, screen->window_height, "mouse");
                screen->gesture = screen->num_circles - 1;
            } else if (screen->keymappingMode && (event->key.keysym.mod & KMOD_GUI) && event->key.keysym.sym == SDLK_j) {
                add_joystick(screen, screen->window_width, screen->window_height);
                return true;
            } else if (event->key.keysym.sym == SDLK_MINUS && (event->key.keysym.mod & KMOD_GUI)) {
                sc_keymap_screen_remove_selected_circle(screen);
                return true;
            }

            else if (screen->keymappingMode && (event->key.keysym.mod & KMOD_GUI) && event->key.keysym.sym == SDLK_o) {
                load_keymap_config(screen);
            }

            else if (screen->keymappingMode && (event->key.keysym.mod & KMOD_GUI) && event->key.keysym.sym == SDLK_s) {
                export_keymap_config(screen);
            }

            else if (screen->selectedCircleIndex != -1) {
                const char* keyText = NULL;

                if (event->key.keysym.mod & (KMOD_LGUI | KMOD_RGUI)) {
                    return true;  // Cmd key is pressed, skip processing for adding/modifying text
                }

                if (event->key.keysym.sym >= 33 && event->key.keysym.sym <= '~' && strcmp(screen->circles[screen->selectedCircleIndex].text, "mouse") != 0) {  // ASCII printable characters range from 32 (space) to 126 (tilde)
                    snprintf(screen->circles[screen->selectedCircleIndex].text, sizeof(screen->circles[screen->selectedCircleIndex].text), "%c", (char)event->key.keysym.sym);
                    return true;
                } else {
                    keyText = getKeyText(event->key.keysym.sym);
                    if (keyText) {
                        snprintf(screen->circles[screen->selectedCircleIndex].text, sizeof(screen->circles[screen->selectedCircleIndex].text), "%s", keyText);
                        return true;
                    }
                }
            }

            break;
    }
    return true;
}

void sc_keymap_screen_render(struct sc_keymap_screen* screen) {
    if (strlen(screen->message) > 0) {
        display_message(screen);
    }
    if (screen->keymappingMode) {
        for (int i = 0; i < screen->num_circles; i++) {
            draw_filled_circle(screen->renderer, screen->circles[i].texture, screen->circles[i].position.x, screen->circles[i].position.y, screen->circles[i].radius);
            render_text(screen, screen->renderer, screen->circles[i].position.x, screen->circles[i].position.y, screen->circles[i].text);
            if (screen->circles[i].isSelected) {
                draw_circle_outline(screen->renderer, screen->circles[i].position.x, screen->circles[i].position.y, screen->circles[i].radius);
            }
        }
    }
}
