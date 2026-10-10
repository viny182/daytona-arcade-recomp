#pragma once
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_properties.h>

namespace app {
// `anisotropy`: keep the anisotropic sampling device feature (a texture pack's
// pictures use it); mobile leaves it off, as the game and ImGui don't need it.
inline SDL_GPUDevice *create_gpu_device(bool anisotropy = false) {
    const SDL_PropertiesID props = SDL_CreateProperties();
    if (!props) return nullptr;
    // Same shaders on every platform; no shader clip distances or indirect
    // draws are used by the game or ImGui.
    const bool configured =
        SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true) &&
        SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_DXIL_BOOLEAN, true) &&
        SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_MSL_BOOLEAN, true) &&
        SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, false) &&
        SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_VERBOSE_BOOLEAN, true) &&
        SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_CLIP_DISTANCE_BOOLEAN, false) &&
        SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_INDIRECT_DRAW_FIRST_INSTANCE_BOOLEAN, false) &&
        SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_ANISOTROPY_BOOLEAN, anisotropy);
    // Keep depth clamping: current pipelines (including pinned ImGui) set
    // enable_depth_clip=false. Disabling the feature alone is not valid.
    SDL_GPUDevice *device = configured ? SDL_CreateGPUDeviceWithProperties(props) : nullptr;
    SDL_DestroyProperties(props);
    return device;
}
} // namespace app
