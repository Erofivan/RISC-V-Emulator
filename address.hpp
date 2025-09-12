#pragma once

#include <cstdint>

#include "functions.hpp"
#include "system_parameters.hpp"

// Logical decomposition of a memory address into cache tag/index/offset.
struct AddressComponents {
    uint16_t tag;
    uint16_t index;
    uint16_t offset;

};

// Decompose a full address into cache tag, index, and byte offset.
inline AddressComponents decompose_address(uint32_t addr) {
    return AddressComponents{
        .tag = static_cast<uint16_t>(get_bits_shifted(
            addr, system_parameters::CACHE_OFFSET_LEN + system_parameters::CACHE_INDEX_LEN,
            system_parameters::ADDRESS_LEN)),
        .index = static_cast<uint16_t>(get_bits_shifted(
            addr, system_parameters::CACHE_OFFSET_LEN,
            system_parameters::CACHE_OFFSET_LEN + system_parameters::CACHE_INDEX_LEN)),
        .offset = static_cast<uint16_t>(get_first_bits(addr, system_parameters::CACHE_OFFSET_LEN)),
    };
}

// Construct an address from cache tag, index, and optional offset.
inline uint32_t create_address(uint16_t tag, uint16_t index, uint16_t offset = 0) {
    uint32_t res = tag;
    res = (res << system_parameters::CACHE_INDEX_LEN) | index;
    res = (res << system_parameters::CACHE_OFFSET_LEN) | offset;
    return res;
}