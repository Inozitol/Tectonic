#pragma once
#include "TecCore.h"
#include "vulkan/VktCache.h"

#include <cstddef>

constexpr std::size_t SIZE_WINDOW    = sizeof(Window)+sizeof(Window)%alignof(Window);
constexpr std::size_t SIZE_VKT_CORE  = sizeof(VktCore)+sizeof(VktCore)%alignof(VktCore);
constexpr std::size_t SIZE_VKT_CACHE = sizeof(VktCache)+sizeof(VktCache)%alignof(VktCache);
constexpr std::size_t SIZE_TEC_CORE  = sizeof(TecCore)+sizeof(TecCore)%alignof(TecCore);

constexpr std::size_t OFFSET_WINDOW    = 0;
constexpr std::size_t OFFSET_VKT_CORE  = OFFSET_WINDOW + SIZE_WINDOW;
constexpr std::size_t OFFSET_VKT_CACHE = OFFSET_VKT_CORE + SIZE_VKT_CORE;
constexpr std::size_t OFFSET_TEC_CORE  = OFFSET_VKT_CACHE + SIZE_VKT_CACHE;

constexpr std::size_t TOTAL_SIZE = OFFSET_TEC_CORE + SIZE_TEC_CORE;

alignas(16) inline std::byte memory[TOTAL_SIZE];

inline Window* WindowPtr       = reinterpret_cast<Window*>(memory + OFFSET_WINDOW);
inline VktCore* VktCorePtr     = reinterpret_cast<VktCore*>(memory + OFFSET_VKT_CORE);
inline VktCache* VktCachePtr   = reinterpret_cast<VktCache*>(memory + OFFSET_VKT_CACHE);
inline TecCore* TecCorePtr     = reinterpret_cast<TecCore*>(memory + OFFSET_TEC_CORE);

inline void initMemory() {
    new (WindowPtr) Window();
    new (VktCachePtr) VktCache();
    new (VktCorePtr) VktCore();
    new (TecCorePtr) TecCore();
}