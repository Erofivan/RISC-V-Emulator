#pragma once

#include <cstdint>
#include <optional>

// Abstract base class for cache replacement policies.
// Policies track tags per set and decide on hit/miss/eviction.
class CacheReplacementPolicy {
public:
    using WayIndex = int16_t;

    struct EvictionResult {
        WayIndex way_index;
        std::optional<uint16_t> evicted_tag;
    };

    virtual ~CacheReplacementPolicy() = default;

    // Returns the way index for the given tag if present; may update usage.
    virtual WayIndex get_way_index(uint16_t tag) = 0;

    // Records an access: returns the way index used and the evicted tag (if any).
    virtual EvictionResult record_access(uint16_t tag) = 0;

    // Checks whether the tag exists in the set.
    virtual bool is_in_cache(uint16_t tag) const = 0;

    // Returns the tag by way index if valid; std::nullopt otherwise.
    virtual std::optional<uint16_t> get_tag(WayIndex way_index) const = 0;
};