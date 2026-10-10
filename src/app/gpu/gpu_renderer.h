// Hardware renderer (SDL_GPU): draws a frame on the GPU instead of the CPU
// rasterizer (src/runtime/raster.cpp, the exact reference) and tilemap
// drawing (Video::draw): the back tilemap layers, the Model 2 3D layer, the
// front tilemap layers (Video's external-3D desktop mode).
//
// The tilemap layers are composed per pixel (m2.hlsl ps_tiles_back,
// ps_tiles_front) from the pixmaps the CPU decodes, uploaded by the rows of
// tiles that changed, and the frame's tile registers and pens. With the HUD
// moved to the widescreen edges the front layers come from the CPU instead.
//
// Polygons are projected exactly as model2_3d_project, drawn as a fan from
// their first vertex in the rasterizer's order (window, then z, newest first)
// and clipped to their window; the depth test stands in for the rasterizer's
// "first polygon to fill a pixel wins". Solid polygons take their palette
// colour from the CPU; textured ones are shaded per pixel by a port of the
// rasterizer's draw_tex_span (m2.hlsl) reading texture RAM, the luma RAM and
// the colour translation from storage buffers.
//
// A texture pack (gpu/texpack.h, opt in) replaces textures by identity: a
// polygon's ID pair (runtime/texid.h) names its picture, which the pixel
// shader samples, filtered by the GPU, where it would read the game's 4-bit
// texture. Polygons are then drawn in groups by picture; the depth test, not
// the draw order, decides what shows, so the image is the same. With no pack,
// or no polygon matching it, the frame is drawn exactly as without.
#pragma once

#include "app/gpu/texpack.h"
#include "runtime/video.h"

#include <SDL3/SDL.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace app {

class GpuRenderer {
public:
    // Creates the pipelines for drawing into textures of `format`. False (with
    // error()) if the GPU or its shader format is not usable.
    bool init(SDL_GPUDevice *dev, SDL_GPUTextureFormat format);
    void shutdown();
    bool ok() const { return poly_pipe_ != nullptr; }
    // Replacement pictures for textured polygons; null turns them off. The pack
    // must outlive its use here (or until set again). Its pictures are decoded
    // and uploaded as polygons first need them.
    void set_texture_pack(const TexPack *pack);
    // Anisotropic filtering of the pack's pictures, up to `max` (1: off). Only
    // for a device made with the anisotropy feature (app/gpu_device.h).
    void set_pack_anisotropy(float max);
    // How many pictures may be decoded and uploaded in one frame (default 4:
    // decoding is a few milliseconds each; polygons without their picture yet
    // show the game's own texture). Tools that want every frame complete raise it.
    void set_pack_uploads_per_frame(int n) { pack_uploads_ = n; }
    // Pictures drawn from the pack in the last frame, and resident on the GPU.
    int pack_polygons() const { return pack_polygons_; }
    int pack_resident() const { return pack_resident_; }
    const std::string &error() const { return error_; }

    // Draws the frame into `target` (at least w x h times `scale`, created
    // with COLOR_TARGET usage): the back tilemap layers, the 3D polygons, the
    // front tilemap layers, all from `video` in external-3D mode. scale > 1
    // (the internal resolution enhancement): the 3D drawn at w x h times
    // scale, textures a mip level finer per doubling, the tilemaps' pixels
    // repeated.
    void render(SDL_GPUCommandBuffer *cmd, SDL_GPUTexture *target, int w, int h, const rt::Video &video, int scale = 1);

private:
    struct PolyVertex {
        float x, y, depth;
        float ooz, uoz, voz; // render_one's p[0..2]: 1/z, u/z, v/z
        float r, g, b, a;
        uint32_t info[4], more[2]; // texture state (m2.hlsl PolyIn)
    };
    struct QuadVertex {
        float x, y, u, v;
    };
    struct Batch { // consecutive polygons sharing one clip rectangle (and one pack picture)
        uint32_t first, count;
        SDL_Rect clip;
        int pack = -1; // the pack entry whose picture they use, or none
    };
    struct PolyRec { // a polygon's vertices, when a pack may regroup them
        uint32_t first, count;
        SDL_Rect clip;
        int pack;
    };
    struct PackTexture {
        SDL_GPUTexture *tex = nullptr;
        bool failed = false;
    };

    SDL_GPUDevice *dev_ = nullptr;
    SDL_GPUGraphicsPipeline *poly_pipe_ = nullptr, *quad_pipe_ = nullptr, *back_pipe_ = nullptr, *front_pipe_ = nullptr;
    SDL_GPUSampler *sampler_ = nullptr;
    SDL_GPUTexture *front_ = nullptr; // the front layers from the CPU (HUD at the edges)
    SDL_GPUTexture *depth_ = nullptr;
    int depth_w_ = 0, depth_h_ = 0, front_w_ = 0, front_h_ = 0;
    SDL_GPUBuffer *vbuf_ = nullptr, *qbuf_ = nullptr;
    // storage buffers: texture RAM (both sheets), luma RAM, colour translation with gamma
    SDL_GPUBuffer *texram_ = nullptr, *luma_ = nullptr, *xlat_ = nullptr;
    uint64_t texram_generation_ = ~0ull; // the texture RAM generation texram_ holds
    // the tilemaps: pixmaps (4 x 512 x 512 u16), header + pens + tile words
    SDL_GPUBuffer *tilepix_ = nullptr, *tiledata_ = nullptr;
    uint64_t tile_instance_ = 0, tile_generation_ = 0; // the Video and its tile generation tilepix_ holds
    struct Rows { uint32_t first, count; }; // pixmap rows (of pixels) to upload, per layer
    Rows tile_rows_[4];
    std::vector<uint32_t> tiledata_words_;
    std::vector<uint32_t> xlat_table_;
    uint32_t vbuf_size_ = 0;
    SDL_GPUTransferBuffer *upload_ = nullptr;
    uint32_t upload_size_ = 0;
    std::vector<PolyVertex> verts_;
    std::vector<Batch> batches_;
    // texture pack
    const TexPack *pack_ = nullptr;
    std::vector<PackTexture> pack_tex_;     // per pack entry
    SDL_GPUTexture *blank_ = nullptr;       // what a draw without a picture binds
    SDL_GPUSampler *pack_sampler_ = nullptr;
    std::vector<PolyRec> recs_;
    std::vector<PolyVertex> regrouped_;
    std::unordered_map<uint32_t, int> pack_ids_; // ID1 -> pack entry (or -1), for this texture RAM generation
    uint64_t pack_ids_generation_ = ~0ull;
    int pack_polygons_ = 0, pack_resident_ = 0, pack_uploads_ = 4;
    std::vector<size_t> order_;
    uint8_t gamma_[256];
    std::string error_;

    SDL_GPUShader *shader(const unsigned char *spv, size_t spv_len, const unsigned char *dxil, size_t dxil_len,
                          const char *msl, const char *dxil_entry, SDL_GPUShaderStage stage, int uniforms, int samplers,
                          int storage_buffers = 0);
    bool ensure(int w, int h, int scale, uint32_t vert_bytes);
    void build(const rt::Video &video, int w, int h, int scale);
    int pack_entry(const rt::VideoMem &mem, uint16_t th0, uint16_t th2);
    bool pack_texture(SDL_GPUCommandBuffer *cmd, int entry);
    void regroup(SDL_GPUCommandBuffer *cmd);
};

} // namespace app
