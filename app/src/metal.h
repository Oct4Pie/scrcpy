#ifndef SC_METAL_H
#define SC_METAL_H

#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>
#include <SDL2/SDL_metal.h>

struct sc_metal {
    id<MTLDevice> device;
    id<MTLCommandQueue> commandQueue;
    id<MTLRenderPipelineState> pipelineState;
    id<MTLTexture> texture;
    CAMetalLayer *metalLayer;
};

void sc_metal_init(struct sc_metal *metal, CAMetalLayer *layer);
bool sc_metal_create_resources(struct sc_metal *metal);
void sc_metal_cleanup(struct sc_metal *metal);

#endif
