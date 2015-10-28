#include <flif.h>
#include <stdlib.h>
#include <stdio.h>
#include <SDL.h>

#pragma pack(push,1)
typedef struct RGBA
{
    uint8_t r,g,b,a;
} RGBA;
#pragma pack(pop)

FLIF_DECODER* d = NULL;
SDL_Window *window = NULL;
SDL_Surface *surf = NULL;
int quit = 0;

int do_event(SDL_Event e) {
       if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE) {printf("Closed\n"); quit=1; return 0;}
       if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_q) {printf("Quit\n"); quit=1; return 0;}
       return 1;
}

uint32_t progressive_render(int quality, int bytes_read) {
    printf("%i bytes read, rendering at quality=%.2f%%\n",bytes_read, 0.01*quality);
    FLIF_IMAGE* image = flif_decoder_get_image(d, 0);
    if (!image) { printf("Error: No decoded image found\n"); return 1; }
    uint32_t w = flif_image_get_width(image);
    uint32_t h = flif_image_get_height(image);
    if (!window) window = SDL_CreateWindow("FLIF Viewer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, 0);
    if (!window) { printf("Error: Could not create window\n"); return 2; }
    char title[100];
    sprintf(title,"%ix%i FLIF image [read %i bytes, quality=%.2f%%]",w,h,bytes_read, 0.01*quality);
    SDL_SetWindowTitle(window,title);
    SDL_Surface *canvas = SDL_GetWindowSurface(window);
    if (!canvas) { printf("Error: Could not get canvas\n"); return 2; }
    if (!surf) surf = SDL_CreateRGBSurface(0,w,h,32,0x000000FF,0x0000FF00,0x00FF0000,0xFF000000);
    if (!surf) { printf("Error: Could not create surface\n"); return 2; }
    void* pp = surf->pixels;
    for (uint32_t r=0; r<h; r++) {
        flif_image_read_row_RGBA8(image, r, pp, w * sizeof(RGBA));
        pp += surf->pitch;
    }
    if (flif_image_get_nb_channels(image) > 3) {
      // SDL_FillRect(canvas,NULL,0);
      // Draw checkerboard background for image with alpha channel
      SDL_Rect sq; sq.w=20; sq.h=20;
      for (sq.y=0; sq.y<h; sq.y+=sq.h) for (sq.x=0; sq.x<w; sq.x+=sq.w)
          SDL_FillRect(canvas,&sq,((sq.y/sq.h + sq.x/sq.w)&1 ? 0xFF606060 : 0xFFA0A0A0));
    }
    SDL_BlitSurface(surf,NULL,canvas,NULL);
    SDL_UpdateWindowSurface(window);
    SDL_Event e;
    while (SDL_PollEvent(&e)) {if (!do_event(e)) return 0;} // stop decoding
    //SDL_Delay(500);
    return quality + 500; // call me back when you have at least 5% better quality
}

int main(int argc, char **argv) {
    if (argc < 2 || argc > 2) {
        printf("Usage:  %s  image.flif\n",argv[0]);
        return 0;
    }
    d = flif_create_decoder();
    if (!d) return 1;

    SDL_Init(SDL_INIT_VIDEO);

    flif_decoder_set_quality(d, 100);
    flif_decoder_set_scale(d, 1);
    flif_decoder_set_callback(d, &(progressive_render));
    printf("Decoding...\n");
    if (flif_decoder_decode_file(d, argv[1]) == 0) {
        printf("Error: decoding failed\n");
        return 1;
    }
    if (!quit) {
      printf("Done.\n");
      SDL_Event e;
      while (1) {
        SDL_WaitEvent(&e);
        if (!do_event(e)) break;
      }
    }
    flif_destroy_decoder(d);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
