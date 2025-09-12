#pragma once

#include <array>
#include <bitset>
#include <cstdint>
#include <optional>

#include "policy_base.hpp"
#include "system_parameters.hpp"

// Bit-based pseudo-LRU (bpLRU) replacement policy.
// Uses per-way MRU flags to approximate LRU with lower overhead.
class BitLruPolicy : public CacheReplacementPolicy {
public:
    static constexpr WayIndex kWayCount = system_parameters::CACHE_WAY;
    static constexpr WayIndex kInvalidWay = -1;

    BitLruPolicy() {
        tags_.fill(0);
        is_valid_.reset();
        mru_flags_.reset();
    }

    // Updates MRU flags on access; when all become true, reset and keep current as MRU.
    void update_usage(WayIndex way) {
        if (way == kInvalidWay) {
            return;
        }
        mru_flags_[way] = true;
        if (mru_flags_.all()) {
            mru_flags_.reset();
            mru_flags_[way] = true;
        }
    }

    bool is_in_cache(uint16_t tag) const override { return find_tag(tag) != kInvalidWay; }

    WayIndex get_way_index(uint16_t tag) override {
        WayIndex way = find_tag(tag);
        update_usage(way);
        return way;
    }

    std::optional<uint16_t> get_tag(WayIndex way_index) const override {
        if (!is_line_valid(way_index)) {
            return std::nullopt;
        }
        return tags_[way_index];
    }

    EvictionResult record_access(uint16_t tag) override {
        WayIndex way = find_tag(tag);
        if (way != kInvalidWay) {
            update_usage(way);
            return {way, std::nullopt};
        }

        way = find_way_to_replace();
        std::optional<uint16_t> evicted_tag;
        if (is_line_valid(way)) {
            evicted_tag = tags_[way];
        }

        insert_new_line(tag, way);
        return {way, evicted_tag};
    }

private:
    void insert_new_line(uint16_t tag, WayIndex way) {
        tags_[way] = tag;
        is_valid_[way] = true;
        update_usage(way);
    }

    WayIndex find_tag(uint16_t tag) const {
        for (WayIndex i = 0; i < kWayCount; ++i) {
            if (is_line_valid(i) && tags_[i] == tag) {
                return i;
            }
        }
        return kInvalidWay;
    }

    WayIndex find_way_to_replace() const {
        // Prefer an invalid line first.
        for (WayIndex i = 0; i < kWayCount; ++i) {
            if (!is_valid_[i]) {
                return i;
            }
        }
        // Choose a line that is not MRU.
        for (WayIndex i = 0; i < kWayCount; ++i) {
            if (!mru_flags_[i]) {
                return i;
            }
        }
        // Fallback (should not happen with proper MRU maintenance).
        return 0;
    }

    bool is_line_valid(WayIndex way) const {
        return way >= 0 && way < kWayCount && is_valid_[way];
    }

    std::bitset<kWayCount> is_valid_{};
    std::bitset<kWayCount> mru_flags_{};
    std::array<uint16_t, kWayCount> tags_{};
};