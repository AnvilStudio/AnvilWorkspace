#pragma once
#include <cstdint>
namespace anv {
struct PushConstantRange {
    uint32_t stageMask = 0; // vertex=1, fragment=2
    uint32_t size = 0;
    uint32_t offset = 0;
};
}
