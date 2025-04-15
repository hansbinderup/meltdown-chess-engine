#pragma once

#include <cstdlib>

/* std::aligned_alloc and std::free are not available for windows
 * create helpers to make cross compilation easier */

namespace helper {

constexpr void* alignedAlloc(size_t alignment, size_t size)
{
#ifdef _WIN32
    return _aligned_malloc(size, alignment);
#else
    return std::aligned_alloc(alignment, size);
#endif
}

constexpr void alignedFree(void* ptr)
{
#ifdef _WIN32
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif
}

}
