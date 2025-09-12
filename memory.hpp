#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <istream>
#include <ostream>

#include "system_parameters.hpp"

// Simple byte-addressable memory implementation with contiguous storage.
class Memory {
public:
    std::byte* get_line(const AddressComponents& comps) noexcept {
        return data_ + create_address(comps.tag, comps.index);
    }

    std::byte read_byte(uint32_t addr) const noexcept { return data_[addr]; }

    std::uint16_t read_half_word(uint32_t addr) const noexcept {
        uint16_t value{};
        std::memcpy(&value, data_ + addr, sizeof(value));
        return value;
    }

    uint32_t read_word(uint32_t addr) const noexcept {
        uint32_t value{};
        std::memcpy(&value, data_ + addr, sizeof(value));
        return value;
    }

    void write_byte(uint32_t addr, std::byte byte) noexcept { data_[addr] = byte; }

    void write_half_word(uint32_t addr, uint16_t half_word) noexcept {
        std::memcpy(data_ + addr, &half_word, sizeof(half_word));
    }

    void write_word(uint32_t addr, uint32_t word) noexcept {
        std::memcpy(data_ + addr, &word, sizeof(word));
    }

    void write_bytes(uint32_t addr, const std::byte* data, size_t len) noexcept {
        std::memcpy(data_ + addr, data, len);
    }

    void read_bytes_from_stream(std::istream& in, uint32_t addr, size_t count) {
        in.read(reinterpret_cast<char*>(data_ + addr), static_cast<std::streamsize>(count));
    }

    void write_data_to_stream(std::ostream& out, uint32_t addr, size_t count) const {
        out.write(reinterpret_cast<const char*>(data_ + addr), static_cast<std::streamsize>(count));
    }

private:
    std::byte data_[system_parameters::MEMORY_SIZE] = {};
};