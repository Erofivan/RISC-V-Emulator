#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>

#include "address.hpp"
#include "memory.hpp"
#include "system_parameters.hpp"

// Represents a single cache line (a fixed-size block of bytes).
struct CacheLine {
    std::byte data[system_parameters::CACHE_LINE_SIZE];
};

// Represents a set of the cache; contains multiple ways (lines).
struct CacheSet {
    CacheLine lines[system_parameters::CACHE_WAY];
};

// Cache template parametrized by a replacement policy.
// Implements write-back, write-allocate behavior and line-based transfers.
template <typename ReplacementPolicy>
class Cache {
public:
    explicit Cache(Memory* mem_ptr) : memory_(mem_ptr) {}

    uint32_t read_word(uint32_t addr) {
        std::byte* byte_ptr = get_byte_address(addr);
        uint32_t value{};
        std::memcpy(&value, byte_ptr, sizeof(value));
        return value;
    }

    uint16_t read_half_word(uint32_t addr) {
        std::byte* byte_ptr = get_byte_address(addr);
        uint16_t value{};
        std::memcpy(&value, byte_ptr, sizeof(value));
        return value;
    }

    std::byte read_byte(uint32_t addr) {
        std::byte* byte_ptr = get_byte_address(addr);
        return *byte_ptr;
    }

    void write_byte(uint32_t addr, std::byte byte) {
        std::byte* byte_ptr = get_byte_address(addr);
        *byte_ptr = byte;
    }

    void write_half_word(uint32_t addr, uint16_t half_word) {
        std::byte* byte_ptr = get_byte_address(addr);
        std::memcpy(byte_ptr, &half_word, sizeof(half_word));
    }

    void write_word(uint32_t addr, uint32_t word) {
        std::byte* byte_ptr = get_byte_address(addr);
        std::memcpy(byte_ptr, &word, sizeof(word));
    }

    // Write all valid cache lines back to memory (simulate cache flush).
    void sync_with_memory() {
        for (uint16_t set_idx = 0; set_idx < system_parameters::CACHE_SET_COUNT; ++set_idx) {
            auto& set_lines = sets_[set_idx].lines;
            for (typename ReplacementPolicy::WayIndex way_idx = 0;
                 way_idx < system_parameters::CACHE_WAY; ++way_idx) {
                auto tag_opt = policies_[set_idx].get_tag(way_idx);
                if (tag_opt.has_value()) {
                    write_line_to_memory(set_lines[way_idx], create_address(*tag_opt, set_idx));
                }
            }
        }
    }

    bool last_op_was_miss() const noexcept { return last_op_was_miss_; }

private:
    using WayIndex = typename ReplacementPolicy::WayIndex;

    // Returns a pointer into the cache line for the given address (after ensuring residency).
    std::byte* get_byte_address(uint32_t addr) {
        const auto comps = decompose_address(addr);

        last_op_was_miss_ = false;
        if (!policies_[comps.index].is_in_cache(comps.tag)) {
            last_op_was_miss_ = true;
            load_from_memory(comps);
        }

        WayIndex way_idx = policies_[comps.index].get_way_index(comps.tag);
        return sets_[comps.index].lines[way_idx].data + comps.offset;
    }

    // Load the corresponding memory line into the cache set. If a line is evicted,
    // write it back (write-back policy).
    void load_from_memory(const AddressComponents& comps) {
        auto& policy = policies_[comps.index];
        auto result = policy.record_access(comps.tag);

        // If a valid line was evicted, write it back to memory.
        if (result.evicted_tag.has_value()) {
            auto& line_to_evict = sets_[comps.index].lines[result.way_index];
            write_line_to_memory(line_to_evict, create_address(*result.evicted_tag, comps.index));
        }

        // Read the new line from memory into the cache.
        read_line_to_set(comps, result.way_index);
    }

    void read_line_to_set(const AddressComponents& comps, WayIndex way_idx) {
        auto& line_data = sets_[comps.index].lines[way_idx].data;
        std::byte* memory_data = memory_->get_line(comps);
        std::memcpy(line_data, memory_data, system_parameters::CACHE_LINE_SIZE);
    }

    void write_line_to_memory(const CacheLine& line, uint32_t addr) {
        memory_->write_bytes(addr, line.data, system_parameters::CACHE_LINE_SIZE);
    }

    bool last_op_was_miss_ = false;
    std::array<ReplacementPolicy, system_parameters::CACHE_SET_COUNT> policies_{};
    std::array<CacheSet, system_parameters::CACHE_SET_COUNT> sets_{};
    Memory* memory_;
};