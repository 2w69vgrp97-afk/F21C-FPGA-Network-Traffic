#ifndef _BOTTOM_H
#define _BOTTOM_H

#include <cassert>
#include <cstdint>

#include "ap_int.h"
#include "BOBHash32.h"
#include "param.h"

#ifdef __SYNTHESIS__
#define HLS_ASSERT(condition, msg) ((void)0)
#else
#define HLS_ASSERT(condition, msg) do { \
    if (!(condition)) { \
        assert(false && msg); \
    } \
} while (0)
#endif

template <uint32_t N>
constexpr bool is_power_of_two() {
    return (N > 0) && ((N & (N - 1)) == 0);
}

template <uint32_t W>
class HashTable_32 {
private:
    uint32_t hashTable32[W];
    uint32_t idx;
    uint32_t val;

    static_assert(W > 0 && W <= 65536,
                  "Hash table size W must be in range [1, 65536]");
    static constexpr bool IS_POWER_OF_TWO = is_power_of_two<W>();

    static void stage_bottom_update_control(
        uint32_t current_value,
        uint32_t increment,
        uint32_t &updated_value
    ) noexcept {
        #pragma HLS INLINE off
        #pragma HLS PIPELINE II=1
        #pragma HLS LATENCY min=1 max=1

        const bool can_accumulate =
            current_value <= UINT32_MAX - increment;

        updated_value =
            can_accumulate
                ? current_value + increment
                : UINT32_MAX;
    }

public:
    HashTable_32() noexcept : idx(0), val(0) {}

    ~HashTable_32() = default;

    HashTable_32(const HashTable_32&) = delete;
    HashTable_32& operator=(const HashTable_32&) = delete;
    HashTable_32(HashTable_32&&) noexcept = default;
    HashTable_32& operator=(HashTable_32&&) noexcept = default;

    void init() {
        #pragma HLS INLINE off
        #pragma HLS RESOURCE variable=hashTable32 core=RAM_2P_BRAM

        Init_Loop:
        for (uint32_t i = 0; i < W; ++i) {
            #pragma HLS PIPELINE II=1
            hashTable32[i] = 0;
        }
    }

    void insert(
        const uint32_t (&hashval)[3],
        uint32_t f,
        int judge_cm
    ) noexcept {
        #pragma HLS INLINE off
        #pragma HLS PIPELINE II=2
        #pragma HLS RESOURCE variable=hashTable32 core=RAM_2P_BRAM

        HLS_ASSERT(judge_cm >= 0 && judge_cm < 3, "judge_cm must be 0,1,2");
        HLS_ASSERT(f <= UINT32_MAX / 2, "f is too large, may cause overflow");

        if (IS_POWER_OF_TWO) {
            idx = hashval[judge_cm] & (W - 1);
        } else {
            idx = hashval[judge_cm] % W;
        }

        uint32_t current_value = hashTable32[idx];
        uint32_t updated_value = 0;

        stage_bottom_update_control(
            current_value,
            f,
            updated_value
        );

        hashTable32[idx] = updated_value;
    }

    void clear() noexcept {
        #pragma HLS INLINE off
        init();
    }

    uint32_t query(
        const uint32_t (&hashval)[3],
        int val_cm
    ) noexcept {
        #pragma HLS INLINE off
        #pragma HLS PIPELINE II=2
        #pragma HLS RESOURCE variable=hashTable32 core=RAM_2P_BRAM

        HLS_ASSERT(val_cm >= 0 && val_cm < 3, "val_cm must be 0,1,2");

        if (IS_POWER_OF_TWO) {
            idx = hashval[val_cm] & (W - 1);
        } else {
            idx = hashval[val_cm] % W;
        }

        val = hashTable32[idx];
        if (val <= UINT32_MAX - BOTTOM_OFFSET) {
            return val + BOTTOM_OFFSET;
        }
        return UINT32_MAX;
    }
};

#endif // _BOTTOM_H
