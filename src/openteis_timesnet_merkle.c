#define _GNU_SOURCE
#include <stdint.h>

typedef struct {
    uint64_t timesnet_2d_tensor_hash;
    uint64_t merkle_root;
    uint8_t atomic_rollup_ready;
} TimesNetMerkleResult_t;

TimesNetMerkleResult_t evaluate_timesnet_merkle(const uint32_t* leaf_hashes, uint8_t count) {
    TimesNetMerkleResult_t res;
    res.timesnet_2d_tensor_hash = 0xCBF29CE484222325ULL;
    res.merkle_root = 0;
    res.atomic_rollup_ready = (count >= 8) ? 1 : 0;

    for (uint8_t i = 0; i < count; i++) {
        uint64_t folded = ((uint64_t)leaf_hashes[i] << 16) ^ (uint64_t)(i * 110);
        res.timesnet_2d_tensor_hash ^= folded;
        res.timesnet_2d_tensor_hash *= 0x100000001B3ULL;
        res.merkle_root ^= res.timesnet_2d_tensor_hash;
    }
    return res;
}
