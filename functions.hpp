#ifndef FUNCTIONS_HPP_
#define FUNCTIONS_HPP_

#include <cstdint>
#include <string_view>

// Returns a mask with 'size' low bits set to 1.
inline constexpr uint64_t get_mask(uint16_t size) noexcept {
    if (size >= 64) {
        return ~0ULL;
    }
    return (1ULL << size) - 1ULL;
}

// Returns a mask with ones in [from, to).
inline constexpr uint64_t get_mask(uint16_t from, uint16_t to) noexcept {
    const uint16_t size = static_cast<uint16_t>(to - from);
    return get_mask(size) << from;
}

// Extracts a single bit at a given position.
inline constexpr bool get_bit(uint64_t num, uint16_t pos) noexcept {
    return (num >> pos) & 1ULL;
}

// Extracts bits from [from, to) without shifting.
inline constexpr uint64_t get_bits(uint64_t num, uint16_t from, uint16_t to) noexcept {
    return num & get_mask(from, to);
}

// Extracts bits from [from, to) and shifts them to LSB.
inline constexpr uint64_t get_bits_shifted(uint64_t num, uint16_t from, uint16_t to) noexcept {
    return get_bits(num, from, to) >> from;
}

// Extracts the lowest 'size' bits.
inline constexpr uint64_t get_first_bits(uint64_t num, uint16_t size) noexcept {
    return num & get_mask(size);
}

// Sign-extends a number from a bit-width 'from_bit' to 32 bits.
inline constexpr uint32_t sign_extend32(uint32_t num, uint8_t from_bit) noexcept {
    if (from_bit == 0) {
        return 0;
    }
    if (from_bit >= 32) {
        return num;
    }
    const bool is_negative = ((num >> (from_bit - 1)) & 1U) != 0U;
    if (is_negative) {
        num |= ~((1U << from_bit) - 1U);
    }
    return num;
}

// Checks if a number matches a bit mask string of '0', '1', or '*'.
inline bool satisfy_mask(uint32_t num, std::string_view mask) {
    for (size_t i = 0; i < mask.length(); ++i) {
        const char mask_char = mask[mask.length() - 1 - i];
        if (mask_char == '*') {
            continue;
        }
        if (((num >> i) & 1U) != static_cast<uint32_t>(mask_char - '0')) {
            return false;
        }
    }
    return true;
}

#endif  // FUNCTIONS_HPP_