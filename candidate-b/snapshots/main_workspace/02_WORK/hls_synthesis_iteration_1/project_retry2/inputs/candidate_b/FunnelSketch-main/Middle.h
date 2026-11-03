/**FileHeader
 * @Author: 赵祥�? * @Date: 2025/12/18 19:53:15
 * @LastEditors: 赵祥�? * @LastEditTime: 2026/3/13 10:05:23
 * @Description: HLS 2018.3 适配修复�? * @Copyright: Copyright (?)}) 2026 赵祥�? All rights reserved.
 * @Email: [contact redacted]
 * @Original Author: 刘金�?et al.
 */

#ifndef _MIDDLE_H
#define _MIDDLE_H

#include <cstdint>
#include <cstring>
#include <climits>
#include <cassert>
#include <type_traits>
#include "ap_int.h"
#include "BOBHash32.h"
#include "param.h"
#include "FastMod.h"

// ==================== 编译期常量定义（替代宏，类型安全�?====================
/** 哈希函数数量 */
static constexpr int D_M = 3;
/** 最大桶数量 */
static constexpr uint32_t MAX_W = 1004;

// ==================== 平台适配宏定�?====================
// 自定义硬件友好的断言（仿真时生效，综合时为空�?
#ifdef __SYNTHESIS__
#define HLS_ASSERT(condition, msg) ((void)0)
#else
#define HLS_ASSERT(condition, msg) do { \
    if (!(condition)) { \
        assert(false && "Middle: " msg); \
    } \
} while (0)
#endif

// ==================== 数据结构定义（硬件优化版�?====================
/**
 * @brief 键值对结构�?2位：8字节key + 2字节count�? * @note 内存对齐优化，避免HLS综合时的字节填充
 */
struct KV_72 {
    ap_uint<64> key;
    ap_uint<16> count;
};

/**
 * @brief Middle层存储单元结�? * @note 包含KV�?+ 8bit计数器数�?+ 16bit计数器数�? */
struct middle_t {
    KV_72 kv_72;
    ap_uint<8>  _8bit[COUNTER_PER_BUCKET_8BIT];
    ap_uint<16> _16bit[COUNTER_PER_BUCKET_16BIT];
};
// ==================== 工具函数（硬件优化版�?====================
/**
 * @brief 8bit数组清零（模板化，编译期确定长度�? * @tparam LEN 数组长度（编译期常量�? * @param arr 待清零的数组指针
 */
template <uint32_t LEN>
inline void memset_8bit(ap_uint<8>* arr) noexcept {
    #pragma HLS INLINE
    Memset8_Loop:
    for (uint32_t i = 0; i < LEN; ++i) {
        #pragma HLS UNROLL factor=8
        arr[i] = 0;
    }
}

/**
 * @brief 16bit数组清零（模板化，编译期确定长度�? * @tparam LEN 数组长度（编译期常量�? * @param arr 待清零的数组指针
 */
template <uint32_t LEN>
inline void memset_16bit(ap_uint<16>* arr) noexcept {
    #pragma HLS INLINE
    Memset16_Loop:
    for (uint32_t i = 0; i < LEN; ++i) {
        #pragma HLS UNROLL factor=8
        arr[i] = 0;
    }
}

/**
 * @brief 键数组清零（硬件优化版）
 * @param key_arr 待清零的键数组指�? */
inline void memset_key(ap_uint<8>* key_arr) noexcept {
    #pragma HLS INLINE
    MemsetKey_Loop:
    for (uint32_t i = 0; i < KEY_BYTE_LEN; ++i) {
        #pragma HLS UNROLL complete  // 完全展开，无循环开销
        key_arr[i] = 0;
    }
}

/**
 * @brief 硬件友好的逐字节拷贝（键拷贝专用）
 * @param dst 目标数组指针
 * @param src 源数组指�? * @param len 拷贝长度（固定为KEY_BYTE_LEN�? */
inline ap_uint<64> pack_key64(const ap_uint<8>* key_arr) noexcept {
    #pragma HLS INLINE
    ap_uint<64> packed = 0;
    PackKey64_Loop:
    for (uint32_t i = 0; i < KEY_BYTE_LEN; ++i) {
        #pragma HLS UNROLL complete
        packed.range((i + 1) * 8 - 1, i * 8) = key_arr[i];
    }
    return packed;
}

inline void unpack_key64(ap_uint<8>* key_arr, ap_uint<64> packed) noexcept {
    #pragma HLS INLINE
    UnpackKey64_Loop:
    for (uint32_t i = 0; i < KEY_BYTE_LEN; ++i) {
        #pragma HLS UNROLL complete
        key_arr[i] = packed.range((i + 1) * 8 - 1, i * 8);
    }
}

inline bool compare_key(ap_uint<64> key1, const ap_uint<8>* key2) noexcept {
    #pragma HLS INLINE
    return key1 == pack_key64(key2);
}

// ==================== Middle层核心类 ====================
/**
 * @brief Middle层存储核心类（HLS 2018.3深度适配�? * @note 核心架构：A1(8bit) -> KV -> A2(16bit) 三层存储
 * @优化点：
 * 1. 编译期常量替代宏，提升类型安�? * 2. HLS指令精细化，解决2018.3版本综合问题
 * 3. 循环优化（展开/流水线），提升硬件并行度
 * 4. 逻辑漏洞修复（溢出处理、提前终止）
 * 5. 资源约束优化，减少BRAM冲突
 */
class Middle
{
private:
    // 存储数组（核心资源）
    uint32_t w;
    middle_t A[D_M][MAX_W];
    BOBHash32 bobhash[D_M];                         // 哈希函数数组
    ap_uint<10> idx_sk[D_M];
    ap_uint<5> idx_ar_8[D_M];
    ap_uint<2> idx_ar_16[D_M];                        // 16bit数组索引缓存
    int row;
    int index;
    static const uint32_t HASH_PRIME_INDEX[D_M];
    // 编译期检查：确保参数合法
    static_assert(D_M > 0 && D_M <= 8, "D_M must be 1~8");
    static_assert(KEY_BYTE_LEN == 8, "KEY_BYTE_LEN must be 8");
    static_assert(MAX_W > 0 && MAX_W <= 1004, "MAX_W must be 1~1004");
    static void stage_a1_write_control(
        uint32_t a1_min,
        uint32_t f,
        ap_uint<8>& a1_min_stage,
        ap_uint<8>& write_value_stage,
        uint32_t& available_stage,
        uint32_t& overflow_stage,
        ap_uint<1>& can_insert_stage
    ) noexcept {
        #pragma HLS INLINE off
        #pragma HLS PIPELINE II=1
        #pragma HLS LATENCY min=1 max=1

        available_stage =
            static_cast<uint32_t>(UINT8_MAX) - a1_min;

        const bool can_insert =
            (available_stage >= f);

        a1_min_stage =
            static_cast<ap_uint<8> >(a1_min);

        write_value_stage = can_insert
            ? static_cast<ap_uint<8> >(a1_min + f)
            : static_cast<ap_uint<8> >(UINT8_MAX);

        overflow_stage = can_insert
            ? 0U
            : (f - available_stage);

        can_insert_stage = can_insert ? 1 : 0;
    }
    static void stage_a2_min_control(
        uint16_t value0,
        uint16_t value1,
        uint16_t value2,
        ap_uint<16> &a2_min_stage
    ) noexcept {
        #pragma HLS INLINE off
        #pragma HLS PIPELINE II=1
        #pragma HLS LATENCY min=1 max=1

        const uint16_t min01 =
            (value1 < value0) ? value1 : value0;

        const uint16_t min012 =
            (value2 < min01) ? value2 : min01;

        a2_min_stage = min012;
    }

    static void stage_old_a2_update_control(
        uint32_t current_value,
        uint32_t increment,
        uint32_t maximum_value,
        uint16_t &updated_value
    ) noexcept {
        #pragma HLS INLINE off
        #pragma HLS PIPELINE II=1
        #pragma HLS LATENCY min=1 max=1

        const bool can_accumulate =
            current_value <= maximum_value - increment;

        updated_value = static_cast<uint16_t>(
            can_accumulate
                ? current_value + increment
                : maximum_value
        );
    }

    static void stage_old_a2_index_control(
        uint32_t old_hashval_value,
        ap_uint<10> &old_idx_sk_stage,
        ap_uint<2> &old_idx_ar_16_stage
    ) noexcept {
        #pragma HLS INLINE off
        #pragma HLS PIPELINE II=1
        #pragma HLS LATENCY min=3 max=3

        old_idx_sk_stage =
            fast_mod_1004(old_hashval_value);

        old_idx_ar_16_stage =
            static_cast<ap_uint<2> >(
                old_hashval_value &
                (COUNTER_PER_BUCKET_16BIT - 1)
            );
    }

    static void stage_a2_write_control(
        uint32_t a2_min,
        uint32_t overflow,
        uint32_t &available_stage,
        ap_uint<D_M> &update_mask_stage
    ) noexcept {
        #pragma HLS INLINE off
        #pragma HLS PIPELINE II=1
        #pragma HLS LATENCY min=1 max=1

        available_stage = static_cast<uint32_t>(UINT16_MAX) - a2_min;
        const bool can_accumulate = (available_stage >= overflow);
        update_mask_stage[0] = can_accumulate;
        update_mask_stage[1] = can_accumulate;
        update_mask_stage[2] = can_accumulate;
    }

    static void stage_kv_winner_control(
        uint16_t count0,
        uint16_t count1,
        uint16_t count2,
        ap_uint<2> &winner_row_stage
    ) noexcept {
        #pragma HLS INLINE off
        #pragma HLS PIPELINE II=1
        #pragma HLS LATENCY min=1 max=1

        const bool row1_less_row0 =
            count1 < count0;

        const bool row2_less_row0 =
            count2 < count0;

        const bool row2_less_row1 =
            count2 < count1;

        const bool choose_row0 =
            !row1_less_row0 && !row2_less_row0;

        const bool choose_row1 =
            row1_less_row0 && !row2_less_row1;

        winner_row_stage =
            choose_row0 ? 0 :
            (choose_row1 ? 1 : 2);
    }

    static void stage_kv_replace_control(
        uint32_t a2_min,
        uint32_t overflow,
        uint32_t v_min,
        int selected_row,
        int selected_index,
        ap_uint<64> incoming_key,
        ap_uint<1> &replace_stage,
        ap_uint<2> &row_stage,
        ap_uint<10> &index_stage,
        ap_uint<16> &count_stage,
        ap_uint<64> &key_stage
    ) noexcept {
        #pragma HLS INLINE off
        #pragma HLS PIPELINE II=1
        #pragma HLS LATENCY min=1 max=1

        const uint32_t new_count_stage = a2_min + overflow;
        replace_stage =
            new_count_stage > (THRESHOLD_REPLACEMENT * v_min);
        row_stage = selected_row;
        index_stage = selected_index;
        count_stage = new_count_stage;
        key_stage = incoming_key;
    }

public:
    /**
     * @brief 构造函数：初始化哈希函�?     */
    Middle() noexcept : w(MAX_W), row(0), index(0) {
        Hash_Init_Loop:
        for (int i = 0; i < D_M; ++i) {
            #pragma HLS UNROLL complete
            bobhash[i].initialize(HASH_PRIME_INDEX[i]);
        }
    }

    /**
     * @brief 析构函数：默认实现（无动态内存）
     */
    ~Middle() = default;

    Middle(const Middle&) = delete;
    Middle& operator=(const Middle&) = delete;

    Middle(Middle&&) noexcept = default;
    Middle& operator=(Middle&&) noexcept = default;

    /**
     * @brief 初始化Middle�?     * @param w_d 桶数量（范围�?~MAX_W�?     * @note HLS优化：BRAM资源约束，数组分区，循环流水�?     */
    void init(uint32_t w_d) noexcept {
    #pragma HLS INLINE off
    #pragma HLS RESOURCE variable=A core=RAM_2P_BRAM
    #pragma HLS ARRAY_PARTITION variable=A complete dim=1
    #pragma HLS ARRAY_PARTITION variable=bobhash complete dim=1
    #pragma HLS ARRAY_PARTITION variable=idx_sk complete dim=1

    HLS_ASSERT(w_d > 0 && w_d <= MAX_W, "init: w_d out of range");
    w = w_d;

    Init_Outer_Loop:
    for (int i = 0; i < D_M; ++i) {
        Init_Inner_Loop:
        for (uint32_t j = 0; j < w; ++j) {
            memset_8bit<COUNTER_PER_BUCKET_8BIT>(A[i][j]._8bit);
            memset_16bit<COUNTER_PER_BUCKET_16BIT>(A[i][j]._16bit);
            A[i][j].kv_72.key = 0;
            A[i][j].kv_72.count = 0;
        }
    }
}

    /**
     * @brief 清空Middle层（复用init逻辑�?     */
    void clear() noexcept {
        #pragma HLS INLINE off
        init(w);
    }

    /**
     * @brief 插入数据到Middle�?     * @param key 输入键（8字节数组�?     * @param key_len 键长度（必须�?�?     * @param f 要插入的计数值（>0�?     * @param hashval 预计算的哈希值数组（长度D_M�?     * @return 溢出值（0表示无溢出）
     * @note HLS优化：II=4流水线，资源约束，依赖消�?     */
    uint32_t insert(
        const ap_uint<8> (&key)[KEY_BYTE_LEN],
        uint16_t key_len,
        uint32_t f,
        uint32_t (&hashval)[D_M]
    ) noexcept {
        #pragma HLS INLINE off
        // Relax II slightly so HLS can share more logic instead of
        // aggressively duplicating arithmetic/control for throughput.
        #pragma HLS PIPELINE II=8
        #pragma HLS RESOURCE variable=A core=RAM_2P_BRAM
    #pragma HLS ARRAY_PARTITION variable=A complete dim=1
    #pragma HLS ARRAY_PARTITION variable=bobhash complete dim=1
        #pragma HLS ARRAY_PARTITION variable=idx_sk complete dim=1
        #pragma HLS ARRAY_PARTITION variable=idx_ar_8 complete dim=1
        #pragma HLS ARRAY_PARTITION variable=idx_ar_16 complete dim=1

        HLS_ASSERT(key_len == KEY_BYTE_LEN, "insert: key_len must be 8");
        HLS_ASSERT(f > 0, "insert: f must be positive");
        HLS_ASSERT(f <= UINT32_MAX / 2, "insert: f too large, overflow risk");

        uint32_t A1_min = UINT32_MAX;
        Calc_Index_Loop:
        for (int i = 0; i < D_M; ++i) {
            #pragma HLS UNROLL complete
            idx_sk[i] = fast_mod_1004(hashval[i]);
            idx_ar_8[i] = fast_mod_25(hashval[i]);
            idx_ar_16[i] = hashval[i] & (COUNTER_PER_BUCKET_16BIT - 1);
            
            uint32_t curr_a1 = static_cast<uint32_t>(A[i][idx_sk[i]]._8bit[idx_ar_8[i]]);
            A1_min = (curr_a1 < A1_min) ? curr_a1 : A1_min;
        }

        // 2. 尝试插入A1层（无溢出）
        const uint32_t A1_MAX = UINT8_MAX;

        ap_uint<8> a1_min_stage = 0;
        ap_uint<8> a1_write_value_stage = 0;
        uint32_t available = 0;
        uint32_t overflow = 0;
        ap_uint<1> a1_can_insert_stage = 0;

        stage_a1_write_control(
            A1_min,
            f,
            a1_min_stage,
            a1_write_value_stage,
            available,
            overflow,
            a1_can_insert_stage
        );

        if (a1_can_insert_stage[0]) {
            Insert_A1_Loop:
            for (int i = 0; i < D_M; ++i) {
                #pragma HLS UNROLL complete

                if (
                    A[i][idx_sk[i]]._8bit[idx_ar_8[i]] ==
                    a1_min_stage
                ) {
                    A[i][idx_sk[i]]._8bit[idx_ar_8[i]] =
                        a1_write_value_stage;
                }
            }

            return 0;
        }

        Fill_A1_Loop:
        for (int i = 0; i < D_M; ++i) {
            #pragma HLS UNROLL complete
            A[i][idx_sk[i]]._8bit[idx_ar_8[i]] = A1_MAX;
        }

        // 4. 尝试插入KV对（优化：提前终止，防溢出）
        bool kv_written = false;
        uint32_t kv_overflow = 0;
        const uint32_t KV_MAX = UINT16_MAX;
        
        KV_Check_Loop:
        for (int i = 0; i < D_M; ++i) {
            #pragma HLS UNROLL complete
            if (kv_written) continue;

            // 空KV槽：直接写入
            if (A[i][idx_sk[i]].kv_72.count == 0) {
                A[i][idx_sk[i]].kv_72.key = pack_key64(key);
                A[i][idx_sk[i]].kv_72.count = (overflow > KV_MAX) ? KV_MAX : overflow;
                kv_written = true;
                // 记录溢出值（如果有）
                kv_overflow = (overflow > KV_MAX) ? (overflow - KV_MAX) : 0;
            } 
        else {
                bool key_equal = compare_key(A[i][idx_sk[i]].kv_72.key, key);
                if (key_equal) {
                    uint32_t curr_kv = static_cast<uint32_t>(A[i][idx_sk[i]].kv_72.count);
                    available = KV_MAX - curr_kv;
                    
                    // KV已达最大值：记录溢出
                    if (curr_kv == KV_MAX) {
                        kv_overflow = overflow;
                    }
                    // 可插入：累加
                    else if (available >= overflow) {
                        A[i][idx_sk[i]].kv_72.count += overflow;
                        kv_written = true;
                    }
        else {
                        A[i][idx_sk[i]].kv_72.count = KV_MAX;
                        kv_overflow = overflow - available;
                    }
                }
            }
        }

        // KV写入成功：返回溢出值（0表示无溢出）
        if (kv_written) return kv_overflow;
        if (kv_overflow > 0) return kv_overflow;

        uint16_t a2_value_0 =
            A[0][idx_sk[0]]._16bit[idx_ar_16[0]];

        uint16_t a2_value_1 =
            A[1][idx_sk[1]]._16bit[idx_ar_16[1]];

        uint16_t a2_value_2 =
            A[2][idx_sk[2]]._16bit[idx_ar_16[2]];

        ap_uint<16> a2_min_stage = 0;

        stage_a2_min_control(
            a2_value_0,
            a2_value_1,
            a2_value_2,
            a2_min_stage
        );

        uint32_t A2_min =
            static_cast<uint32_t>(a2_min_stage);

        const uint32_t A2_MAX = UINT16_MAX;

        ap_uint<D_M> a2_update_mask = 0;
        stage_a2_write_control(A2_min, overflow, available, a2_update_mask);
        // A2可插入：累加
        if (a2_update_mask[0]) {
            Insert_A2_Loop:
            for (int i = 0; i < D_M; ++i) {
                #pragma HLS UNROLL complete
                if (a2_update_mask[i] &&
                    A[i][idx_sk[i]]._16bit[idx_ar_16[i]] == A2_min) {
                    A[i][idx_sk[i]]._16bit[idx_ar_16[i]] += overflow;
                }
            }

            // D_M is fixed at 3. Compare all candidates in parallel
            // and preserve the original first-row-wins tie rule.
            uint16_t kv_count_0 =
                A[0][idx_sk[0]].kv_72.count;
            uint16_t kv_count_1 =
                A[1][idx_sk[1]].kv_72.count;
            uint16_t kv_count_2 =
                A[2][idx_sk[2]].kv_72.count;

            ap_uint<64> kv_key_0 =
                A[0][idx_sk[0]].kv_72.key;
            ap_uint<64> kv_key_1 =
                A[1][idx_sk[1]].kv_72.key;
            ap_uint<64> kv_key_2 =
                A[2][idx_sk[2]].kv_72.key;

            ap_uint<2> winner_row_stage = 0;

            stage_kv_winner_control(
                kv_count_0,
                kv_count_1,
                kv_count_2,
                winner_row_stage
            );

            const bool choose_row0 =
                winner_row_stage == 0;

            const bool choose_row1 =
                winner_row_stage == 1;

            row =
                choose_row0 ? 0 :
                (choose_row1 ? 1 : 2);

            index =
                choose_row0 ? idx_sk[0] :
                (choose_row1 ? idx_sk[1] : idx_sk[2]);

            uint16_t selected_old_count =
                choose_row0 ? kv_count_0 :
                (choose_row1 ? kv_count_1 : kv_count_2);

            ap_uint<64> selected_old_key =
                choose_row0 ? kv_key_0 :
                (choose_row1 ? kv_key_1 : kv_key_2);

            uint32_t v_min =
                static_cast<uint32_t>(selected_old_count);

            ap_uint<1> kv_replace_stage = 0;
            ap_uint<2> kv_replace_row_stage = 0;
            ap_uint<10> kv_replace_index_stage = 0;
            ap_uint<16> kv_replace_count_stage = 0;
            ap_uint<64> kv_replace_key_stage = 0;

            stage_kv_replace_control(
                A2_min,
                overflow,
                v_min,
                row,
                index,
                pack_key64(key),
                kv_replace_stage,
                kv_replace_row_stage,
                kv_replace_index_stage,
                kv_replace_count_stage,
                kv_replace_key_stage
            );

            if (kv_replace_stage) {
                ap_uint<8> old_key[KEY_BYTE_LEN];
                unpack_key64(
                    old_key,
                    selected_old_key
                );

                uint16_t old_count = selected_old_count;

                A[kv_replace_row_stage]
                 [kv_replace_index_stage].kv_72.key =
                    kv_replace_key_stage;

                A[kv_replace_row_stage]
                 [kv_replace_index_stage].kv_72.count =
                    kv_replace_count_stage;

                uint32_t old_hashval[D_M] = {0};
                Calc_Old_Hash_Loop:
                for (int i = 0; i < D_M; ++i) {
                    #pragma HLS UNROLL complete
                    old_hashval[i] = bobhash[i].run(reinterpret_cast<const ByteType*>(old_key), KEY_BYTE_LEN);

                    ap_uint<10> old_idx_sk_stage = 0;
                    ap_uint<2> old_idx_ar_16_stage = 0;

                    stage_old_a2_index_control(
                        old_hashval[i],
                        old_idx_sk_stage,
                        old_idx_ar_16_stage
                    );

                    uint32_t curr_a2 = static_cast<uint32_t>(
                        A[i][old_idx_sk_stage]
                         ._16bit[old_idx_ar_16_stage]
                    );

                    uint16_t updated_a2_stage = 0;

                    stage_old_a2_update_control(
                        curr_a2,
                        old_count,
                        static_cast<uint32_t>(A2_MAX),
                        updated_a2_stage
                    );

                    A[i][old_idx_sk_stage]
                     ._16bit[old_idx_ar_16_stage] =
                        updated_a2_stage;
                }
            }
            return 0;
        } 
        else {
            Fill_A2_Loop:
            for (int i = 0; i < D_M; ++i) {
                #pragma HLS UNROLL complete
                A[i][idx_sk[i]]._16bit[idx_ar_16[i]] = A2_MAX;
            }
            return overflow - available;
        }
    }

    /**
     * @brief 查询Middle层数�?     * @param key 输入键（8字节数组�?     * @param key_len 键长度（必须�?�?     * @param hashval 预计算的哈希值数组（长度D_M�?     * @return 查询结果（带偏移�?     * @note HLS优化：II=8流水线，减少访存延迟
     */
    uint32_t query(
        const ap_uint<8> (&key)[KEY_BYTE_LEN],
        uint16_t key_len,
        uint32_t (&hashval)[D_M]
    ) noexcept {
        #pragma HLS INLINE off
        #pragma HLS PIPELINE II=10
        #pragma HLS RESOURCE variable=A core=RAM_2P_BRAM
        #pragma HLS ARRAY_PARTITION variable=A complete dim=1
        #pragma HLS ARRAY_PARTITION variable=idx_sk complete dim=1
        #pragma HLS ARRAY_PARTITION variable=idx_ar_8 complete dim=1
        #pragma HLS ARRAY_PARTITION variable=idx_ar_16 complete dim=1

        HLS_ASSERT(key_len == KEY_BYTE_LEN, "query: key_len must be 8");

        uint32_t A1_min = UINT32_MAX;
        Query_Calc_Index_Loop:
        for (int i = 0; i < D_M; ++i) {
            #pragma HLS UNROLL complete
            // 索引计算优化
            idx_sk[i] = fast_mod_1004(hashval[i]);
            idx_ar_8[i] = fast_mod_25(hashval[i]);
            idx_ar_16[i] = hashval[i] & (COUNTER_PER_BUCKET_16BIT - 1);
            
            uint32_t curr_a1 = static_cast<uint32_t>(A[i][idx_sk[i]]._8bit[idx_ar_8[i]]);
            A1_min = (curr_a1 < A1_min) ? curr_a1 : A1_min;
        }

        if (A1_min < UINT8_MAX) {
            return A1_min + TOP_THRESHOLD;
        }

        // 2. 检查KV对（优化：提前终止，防溢出）
        uint32_t kv_min = UINT32_MAX;
        bool kv_matched = false;
        bool kv_overflow = false;
        const uint32_t KV_MAX = UINT16_MAX;
        
        Query_Check_KV_Loop:
        for (int i = 0; i < D_M; ++i) {
            #pragma HLS UNROLL complete
            if (kv_matched && !kv_overflow) continue;

            bool key_equal = compare_key(A[i][idx_sk[i]].kv_72.key, key);
            if (key_equal) {
                kv_matched = true;
                uint32_t curr_kv = static_cast<uint32_t>(A[i][idx_sk[i]].kv_72.count);
                
                if (curr_kv >= KV_MAX) {
                    kv_overflow = true;
                }
                else if (curr_kv < kv_min) {
                    kv_min = curr_kv;
                }
            }
        }

        if (kv_matched) {
            if (kv_overflow) {
                return KV_MAX + 1 + OFFSET_A2_KV;
            } else {
                return kv_min + OFFSET_A2_KV;
            }
        }

        uint32_t A2_min = UINT32_MAX;
        const uint32_t A2_MAX = UINT16_MAX;
        
        Query_Calc_A2_Min_Loop:
        for (int i = 0; i < D_M; ++i) {
            #pragma HLS UNROLL complete
            uint32_t curr_a2 = static_cast<uint32_t>(A[i][idx_sk[i]]._16bit[idx_ar_16[i]]);
            A2_min = (curr_a2 < A2_min) ? curr_a2 : A2_min;
        }

        if (A2_min <= A2_MAX) {
            return A2_min + OFFSET_A2_KV;
        } else {
            return A2_MAX + 2 + OFFSET_A2_KV;
        }
    }
};

const uint32_t Middle::HASH_PRIME_INDEX[D_M] = {1, 5, 11};

#endif // _MIDDLE_H






