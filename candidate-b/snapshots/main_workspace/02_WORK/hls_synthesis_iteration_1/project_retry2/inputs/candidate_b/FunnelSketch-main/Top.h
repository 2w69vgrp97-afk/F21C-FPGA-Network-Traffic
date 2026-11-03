/**FileHeader
 * @Author: 赵祥�? * @Date: 2025/12/18 19:53:15
 * @LastEditors: 赵祥�? * @LastEditTime: 2026/3/13 10:12:41
 * @Description: HLS 2018.3 适配修复�? * @Copyright: Copyright (?)}) 2026 赵祥�? All rights reserved.
 * @Email: [contact redacted]
 * @Original Author: 刘金�?et al.
 */

#ifndef _TOP_H
#define _TOP_H

#include <cstdint>
#include <cstring>
#include <cassert>
#include "ap_int.h"
#include "BOBHash32.h"
#include "param.h"
#include "FastMod.h"

// ==================== 编译期常量定义（替代宏，解决链接问题�?====================
/** 哈希函数数量（显式指定为3，避免宏解析歧义�?*/
static constexpr int D_T = 3;
/** Top层最大桶数量 */
static constexpr uint32_t MAX_W_T = 87040;
/** 键字节长�?*/

// ==================== 平台适配宏定�?====================
// 自定义硬件友好的断言（仿真时生效，综合时为空�?
#ifdef __SYNTHESIS__
#define HLS_ASSERT(condition, msg) ((void)0)
#else
#define HLS_ASSERT(condition, msg) do { \
    if (!(condition)) { \
        assert(false && "Top: " msg); \
    } \
} while (0)
#endif

// ==================== Top层核心类 ====================
/**
 * @brief Top层存储核心类（HLS 2018.3深度适配�? * @note 核心逻辑完全保留，仅优化HLS指令、修复链接问题、提升资源效�? */
class Top
{
private:
    // 存储数组（核心资源）
    uint32_t w_t;
    ap_uint<4> A[D_T][MAX_W_T];
    BOBHash32 bobhash[D_T];
    ap_uint<17> idx_sk_t[D_T];                            // 桶索引缓�?
    // 【关键修复】静态常量数组：显式指定长度�?，解决链接未定义问题
    static const uint32_t HASH_PRIME_INDEX[3];

    // 编译期检查：确保参数合法
    static_assert(D_T == 3, "D_T must be 3 (match HASH_PRIME_INDEX length)");
    static_assert(KEY_BYTE_LEN == 8, "KEY_BYTE_LEN must be 8");
    static_assert(MAX_W_T == 87040, "MAX_W_T must be 87040");

public:
    /**
     * @brief 构造函数：初始化哈希函�?     */
    Top() noexcept : w_t(MAX_W_T) {
        Hash_Init_Loop:
        for (int i = 0; i < D_T; ++i) {
            #pragma HLS UNROLL complete
            bobhash[i].initialize(HASH_PRIME_INDEX[i]);
        }
    }

    /**
     * @brief 析构函数：默认实现（无动态内存）
     */
    ~Top() = default;

    Top(const Top&) = delete;
    Top& operator=(const Top&) = delete;

    Top(Top&&) noexcept = default;
    Top& operator=(Top&&) noexcept = default;

    /**
     * @brief 初始化Top�?     * @param w_u 桶数量（范围�?~MAX_W_T�?     * @note 核心逻辑不变，优化HLS指令适配2018.3，避免资源浪�?     */
    void init(uint32_t w_u) noexcept {
        #pragma HLS INLINE off
        // #pragma HLS PIPELINE II=16  // 保留原II值，适配大数组初始化
        #pragma HLS LOOP_TRIPCOUNT min=1 max=87040 avg=43520  // 保留原循环计�?
        // HLS 2018.3资源约束优化
        #pragma HLS RESOURCE variable=A core=RAM_2P_BRAM
        #pragma HLS ARRAY_PARTITION variable=A complete dim=1
        #pragma HLS ARRAY_PARTITION variable=idx_sk_t complete dim=1  // 索引数组分区

        HLS_ASSERT(w_u > 0 && w_u <= MAX_W_T, "init: w_u out of range");
        w_t = w_u;

        Init_Outer_Loop:
        for (int i = 0; i < D_T; ++i) {
            // 内层循环：遍历桶（移除不必要的展开，节省资源）
            Init_Inner_Loop:
            for (uint32_t j = 0; j < w_t; ++j) {
                #pragma HLS PIPELINE II=1
                A[i][j] = 0;
            }
        }
    }

    /**
     * @brief 清空Top层（复用init逻辑，核心逻辑不变�?     */
    void clear() noexcept {
        #pragma HLS INLINE off
        init(w_t);
    }

    /**
     * @brief 插入数据到Top�?     * @param key 输入键（8字节数组�?     * @param key_len 键长度（必须�?�?     * @param f 要插入的计数值（>0�?     * @param hashval 哈希值数组（输出参数，长度D_T�?     * @return 溢出值（0表示无溢出）
     * @note 核心逻辑完全保留，优化HLS指令和资源约�?     */
    uint32_t insert(
        const ap_uint<8> (&key)[KEY_BYTE_LEN],
        uint16_t key_len,
        uint32_t f,
        uint32_t (&hashval)[D_T]
    ) noexcept {
        #pragma HLS INLINE off
        // Trade a little throughput for much lower control/arithmetic duplication.
        #pragma HLS PIPELINE II=8
        // HLS 2018.3资源约束优化
        #pragma HLS RESOURCE variable=A core=RAM_2P_BRAM
        #pragma HLS ARRAY_PARTITION variable=A complete dim=1
        #pragma HLS ARRAY_PARTITION variable=bobhash complete dim=1
        #pragma HLS ARRAY_PARTITION variable=idx_sk_t complete dim=1

        HLS_ASSERT(key_len == KEY_BYTE_LEN, "insert: key_len must be 8");
        HLS_ASSERT(f > 0, "insert: f must be positive");
        HLS_ASSERT(f <= TOP_THRESHOLD, "insert: f exceeds TOP_THRESHOLD");

        // 类型安全转换（保留原逻辑，优化数组分区）
        ByteType key_int8[KEY_BYTE_LEN];
        #pragma HLS ARRAY_PARTITION variable=key_int8 complete dim=1
        
        Key_Convert_Loop:
        for (uint32_t k = 0; k < key_len; ++k) {
            #pragma HLS UNROLL complete
            key_int8[k] = static_cast<ByteType>(key[k]);
        }

        uint32_t min_count = UINT32_MAX;
        Hash_Calc_Loop:
        for (int i = 0; i < D_T; ++i) {
            #pragma HLS UNROLL complete
            hashval[i] = bobhash[i].run(key_int8, static_cast<uint32_t>(key_len));
            idx_sk_t[i] = fast_mod_87040(hashval[i]);
            uint32_t curr_count = static_cast<uint32_t>(A[i][idx_sk_t[i]]);
            if (curr_count < min_count) {
                min_count = curr_count;
            }
        }

        uint32_t available = TOP_THRESHOLD - min_count;
        if (available >= f) {
            Insert_Full_Loop:
            for (int i = 0; i < D_T; ++i) {
                #pragma HLS UNROLL complete
                if (A[i][idx_sk_t[i]] == min_count) {
                    A[i][idx_sk_t[i]] += f;
                }
            }
            return 0;
        } else {
            Fill_Top_Loop:
            for (int i = 0; i < D_T; ++i) {
                #pragma HLS UNROLL complete
                A[i][idx_sk_t[i]] = TOP_THRESHOLD;
            }
            return f - available;
        }
    }

    /**
     * @brief 查询Top层数�?     * @param key 输入键（8字节数组�?     * @param key_len 键长度（必须�?�?     * @param hashval 哈希值数组（输出参数，长度D_T�?     * @return 查询到的最小�?     * @note 核心逻辑完全保留，优化HLS指令
     */
    uint32_t query(
        const ap_uint<8> (&key)[KEY_BYTE_LEN],
        uint16_t key_len,
        uint32_t (&hashval)[D_T]
    ) noexcept {
        #pragma HLS INLINE off
        #pragma HLS PIPELINE II=8
        // HLS 2018.3资源约束优化
        #pragma HLS RESOURCE variable=A core=RAM_2P_BRAM
        #pragma HLS ARRAY_PARTITION variable=A complete dim=1
        #pragma HLS ARRAY_PARTITION variable=bobhash complete dim=1
        #pragma HLS ARRAY_PARTITION variable=idx_sk_t complete dim=1

        HLS_ASSERT(key_len == KEY_BYTE_LEN, "query: key_len must be 8");

        ByteType key_int8[KEY_BYTE_LEN];
        #pragma HLS ARRAY_PARTITION variable=key_int8 complete dim=1
        
        Query_Key_Convert_Loop:
        for (uint32_t k = 0; k < key_len; ++k) {
            #pragma HLS UNROLL complete
            key_int8[k] = static_cast<ByteType>(key[k]);
        }

        uint32_t min_count = UINT32_MAX;
        Query_Hash_Calc_Loop:
        for (int i = 0; i < D_T; ++i) {
            #pragma HLS UNROLL complete
            hashval[i] = bobhash[i].run(key_int8, static_cast<uint32_t>(key_len));
            idx_sk_t[i] = fast_mod_87040(hashval[i]);
            uint32_t curr_count = static_cast<uint32_t>(A[i][idx_sk_t[i]]);
            if (curr_count < min_count) {
                min_count = curr_count;
            }
        }

        return min_count;
    }
};

// 【关键修复】静态常量数组类外定义：显式指定长度3，解决链接未定义错误
const uint32_t Top::HASH_PRIME_INDEX[3] = {1, 5, 11};

#endif // _TOP_H






