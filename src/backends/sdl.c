#include "backend.h"

#include <stdbool.h>
#include <stdint.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_render.h>
#include "SDL2/SDL_ttf.h"
#include "SDL2/SDL_video.h"

#include "panic.h"

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;

    int width;
    int height;

    int border_width;
    int border_height;

    uint16_t scale;

    SDL_Texture* texture;
} Screen;

SDL_AudioDeviceID g_audio_device;

Screen g_backend_screen = { 0 };

SDL_KeyCode g_default_keys[] = {
    SDLK_x,
    SDLK_1,
    SDLK_2,
    SDLK_3,
    SDLK_q,
    SDLK_w,
    SDLK_e,
    SDLK_a,
    SDLK_s,
    SDLK_d,
    SDLK_z,
    SDLK_c,
    SDLK_4,
    SDLK_r,
    SDLK_f,
    SDLK_v
};

bool g_key_states[16] = { false };

uint8_t g_pixel_map[64*32/8] = { 0 };

#define WINDOW_WIDTH 320
#define WINDOW_HEIGHT 240

void backend_audio_callback(void* userdata, uint8_t* stream, int len)
{
    (void) userdata;

    int16_t* data = (int16_t*) stream;
    static uint32_t running_sample_index = 0;
    const int32_t volume = 3000;
    const int32_t square_wave_period = 44100 / 440;
    const int32_t half_square_wave_period = square_wave_period / 2;

    for (int i=0; i<len/2; i++) data[i] = ((running_sample_index++ / half_square_wave_period) % 2) ? volume : -volume;
}

void backend_initialize()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) panic("Couldn't initialize SDL: %s", SDL_GetError());
    if (TTF_Init() < 0)               panic("Couldn't initialize TTF: %s", SDL_GetError());

	if (SDL_CreateWindowAndRenderer(WINDOW_WIDTH,
                                    WINDOW_HEIGHT, 
                                    SDL_WINDOW_RESIZABLE, 
                                    &g_backend_screen.window, 
                                    &g_backend_screen.renderer)) panic("Couldn't open a window: %s", SDL_GetError());

    SDL_SetWindowMinimumSize(g_backend_screen.window, 64, 32);

    g_backend_screen.width = WINDOW_WIDTH;
    g_backend_screen.height = WINDOW_HEIGHT;
    g_backend_screen.border_width = (WINDOW_WIDTH % 64) / 2;
    g_backend_screen.border_height = (WINDOW_HEIGHT % 32) / 2;
#define MIN(x, y) (x < y) ? x : y
    g_backend_screen.scale = MIN(((WINDOW_WIDTH - g_backend_screen.border_width) / 64), ((WINDOW_HEIGHT - g_backend_screen.border_height) / 32));
#undef MIN

    backend_clear_screen();

    // Initilaize Audio
    SDL_AudioSpec audio_spec;
    SDL_zero(audio_spec);

    audio_spec.freq = 44100;
    audio_spec.format = AUDIO_S16LSB;
    audio_spec.samples = 512;
    audio_spec.channels = 1;
    audio_spec.callback = backend_audio_callback;

    g_audio_device = SDL_OpenAudioDevice(NULL, 0, &audio_spec, NULL, 1);
    if (!g_audio_device) panic("Failed to open the audio device: %s", SDL_GetError());
}

void backend_redraw() {
    SDL_SetRenderTarget(g_backend_screen.renderer, g_backend_screen.texture);
    SDL_RenderClear(g_backend_screen.renderer);
    SDL_SetRenderDrawColor(g_backend_screen.renderer, 255, 255, 255, 255);

    SDL_Rect rect;
    rect.h = g_backend_screen.scale;
    rect.w = g_backend_screen.scale;

    for (int x=0; x<64; x++) for (int y=0; y<32; y++) {
        if (!(g_pixel_map[y*64/8+x/8] & (1<<(x%8)))) continue;

        rect.x = g_backend_screen.border_width + (x*g_backend_screen.scale);
        rect.y = g_backend_screen.border_height + (y*g_backend_screen.scale);

        SDL_RenderFillRect(g_backend_screen.renderer, &rect);
    }

    SDL_SetRenderTarget(g_backend_screen.renderer, NULL);
}

void backend_handle_screenevent(SDL_Event e)
{
    if (e.window.event == SDL_WINDOWEVENT_EXPOSED) { backend_redraw(); return; }
    if (e.window.event != SDL_WINDOWEVENT_SIZE_CHANGED) return;

    g_backend_screen.width  = e.window.data1;
    g_backend_screen.height = e.window.data2;
    g_backend_screen.border_width = (g_backend_screen.width % 64) / 2;
    g_backend_screen.border_height = (g_backend_screen.height % 32) / 2;
#define MIN(x, y) (x < y) ? x : y
    g_backend_screen.scale = MIN(((g_backend_screen.width - g_backend_screen.border_width) / 64), ((g_backend_screen.height - g_backend_screen.border_height) / 32));
#undef MIN
    
    backend_redraw();
}

void backend_render()
{
    SDL_SetRenderDrawColor(g_backend_screen.renderer, 0, 0, 0, 255);
    SDL_RenderCopy(g_backend_screen.renderer, g_backend_screen.texture, NULL, NULL);
	SDL_RenderPresent(g_backend_screen.renderer);
}

void backend_handle_keyup(SDL_Event e)
{
    for (int i=0; i<sizeof(g_default_keys); i++) {
        if (e.key.keysym.sym == g_default_keys[i]) g_key_states[i] = false;
    }
}

void backend_handle_keydown(SDL_Event e)
{
    for (int i=0; i<sizeof(g_default_keys); i++) {
        if (e.key.keysym.sym == g_default_keys[i]) g_key_states[i] = true;
    }
}

bool backend_loop()
{
    SDL_Event event;
    SDL_PollEvent(&event);
    if      (event.type == SDL_QUIT)        return true;
    else if (event.type == SDL_WINDOWEVENT) backend_handle_screenevent(event);
    else if (event.type == SDL_KEYUP)       backend_handle_keyup(event);
    else if (event.type == SDL_KEYDOWN)     backend_handle_keydown(event);

    backend_render();

    return false;
}

void backend_destroy()
{
	SDL_DestroyRenderer(g_backend_screen.renderer);
	SDL_DestroyWindow(g_backend_screen.window);

    SDL_CloseAudioDevice(g_audio_device);

    SDL_Quit();
}

uint8_t** backend_get_pixel_map()
{
    return (uint8_t**) &g_pixel_map;
}

/* Y and X start with 0,
 * returns the current value of the pixel that is changed. */
bool backend_flip_pixel(uint8_t x, uint8_t y)
{
    /* get the pixel index */
    int pixel = y*64+x;
    int index = pixel/8;
    int shift = pixel%8;

    uint8_t mask = pow(2, shift);
    uint8_t newval = g_pixel_map[index] ^ mask;
    bool set = newval > g_pixel_map[index];
    g_pixel_map[index] = newval;

    SDL_SetRenderTarget(g_backend_screen.renderer, g_backend_screen.texture);

    SDL_Rect rect = { x, y, g_backend_screen.scale, g_backend_screen.scale };
    rect.x = g_backend_screen.border_width + (x*g_backend_screen.scale);
    rect.y = g_backend_screen.border_height + (y*g_backend_screen.scale);

    SDL_SetRenderDrawColor(g_backend_screen.renderer, 255*set, 255*set, 255*set, 255);
    SDL_RenderFillRect(g_backend_screen.renderer, &rect);

    SDL_SetRenderTarget(g_backend_screen.renderer, NULL);

    return !set;
}

void backend_toggle_beep(bool beep)
{
    SDL_PauseAudioDevice(g_audio_device, !beep);
}

bool backend_is_pressed(Chip8Key key)
{
    if (key > 15) panic("No such key: %d", key);
    return g_key_states[key];
}

void backend_clear_screen()
{
    SDL_SetRenderTarget(g_backend_screen.renderer, g_backend_screen.texture);
    SDL_SetRenderDrawColor(g_backend_screen.renderer, 0, 0, 0, 255);
    SDL_RenderClear(g_backend_screen.renderer);
    SDL_SetRenderTarget(g_backend_screen.renderer, NULL);

    for (int i=0; i<64*32/8; i++) g_pixel_map[i] = 0;
}

void backend_delay(uint32_t ms) 
{
    SDL_Delay(ms);
}
