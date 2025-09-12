#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "policy_base.hpp"
#include "system_parameters.hpp"

// LRU replacement policy using logical timestamps.
class LruPolicy : public CacheReplacementPolicy {
    static constexpr uint64_t kInvalidTime = 0;

    struct LineInfo {
        uint16_t tag = 0;
        uint64_t last_use_time = kInvalidTime;
    };

public:
    static constexpr WayIndex kWayCount = system_parameters::CACHE_WAY;
    static constexpr WayIndex kInvalidWay = -1;

    LruPolicy() { lines_.fill(LineInfo{}); }

    // Marks a way as recently used by updating its timestamp.
    void update_usage(WayIndex way) {
        if (way == kInvalidWay) {
            return;
        }
        lines_[way].last_use_time = ++current_time_;
    }

    WayIndex get_way_index(uint16_t tag) override {
        WayIndex way = find_tag(tag);
        update_usage(way);
        return way;
    }

    bool is_in_cache(uint16_t tag) const override { return find_tag(tag) != kInvalidWay; }

    std::optional<uint16_t> get_tag(WayIndex way_index) const override {
        if (!is_line_valid(way_index)) {
            return std::nullopt;
        }
        return lines_[way_index].tag;
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
            evicted_tag = lines_[way].tag;
        }

        insert_new_line(tag, way);
        return {way, evicted_tag};
    }

private:
    void insert_new_line(uint16_t tag, WayIndex way) {
        lines_[way] = LineInfo{
            .tag = tag,
            .last_use_time = ++current_time_,
        };
    }

    WayIndex find_tag(uint16_t tag) const {
        for (WayIndex i = 0; i < kWayCount; ++i) {
            if (is_line_valid(i) && lines_[i].tag == tag) {
                return i;
            }
        }
        return kInvalidWay;
    }

    WayIndex find_way_to_replace() const {
        // Prefer invalid (empty) line first.
        for (WayIndex i = 0; i < kWayCount; ++i) {
            if (!is_line_valid(i)) {
                return i;
            }
        }
        // Otherwise, choose the least recently used.
        WayIndex lru_way = 0;
        for (WayIndex i = 1; i < kWayCount; ++i) {
            if (lines_[i].last_use_time < lines_[lru_way].last_use_time) {
                lru_way = i;
            }
        }
        return lru_way;
    }

    bool is_line_valid(WayIndex way) const {
        return way >= 0 && way < kWayCount && lines_[way].last_use_time != kInvalidTime;
    }

    std::array<LineInfo, kWayCount> lines_{};
    uint64_t current_time_ = kInvalidTime;
};