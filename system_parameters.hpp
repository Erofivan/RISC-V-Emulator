#ifndef SYSTEM_PARAMETERS_HPP_
#define SYSTEM_PARAMETERS_HPP_

#include <cstdint>

namespace system_parameters {

constexpr int32_t MEMORY_SIZE = 128 * 1024;
constexpr int32_t ADDRESS_LEN = 17;  // log2(MEMORY_SIZE)

constexpr int32_t CACHE_LINE_SIZE = 64;
constexpr int32_t CACHE_SET_COUNT = 16;
constexpr int32_t CACHE_LINE_COUNT = 64;  // kCacheSize / CACHE_LINE_SIZE
constexpr int32_t CACHE_WAY = CACHE_LINE_COUNT / CACHE_SET_COUNT;  // 4-way associative

constexpr int32_t CACHE_OFFSET_LEN = 6;  // log2(CACHE_LINE_SIZE)
constexpr int32_t CACHE_INDEX_LEN = 4;   // log2(CACHE_SET_COUNT)
constexpr int32_t CACHE_TAG_LEN = ADDRESS_LEN - CACHE_INDEX_LEN - CACHE_OFFSET_LEN;

constexpr int32_t INSTRUCTION_LEN = 4;  // RISC-V instruction length in bytes

}  // namespace system_parameters

#endif  // SYSTEM_PARAMETERS_HPP_