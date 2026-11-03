/**FileHeader
 * @Author: 赵祥�? * @Date: 2025/12/18 19:53:15
 * @LastEditors: 赵祥�? * @LastEditTime: 2026/4/21 09:59:19
 * @Description: HLS 2018.3 适配XC7Z020CLG484-2最终版（无告警�? * @Copyright: Copyright (?)}) 2026 赵祥�? All rights reserved.
 * @Email: [contact redacted]
 * @Original Author: 刘金�?et al.
 */

#ifndef _FUNNELSKETCH_H
#define _FUNNELSKETCH_H

#include <cstdint>
#include <cassert>
#include <cstring>
#include "ap_int.h"
#include "Bottom.h"
#include "Middle.h"
#include "Top.h"
#include "param.h"          // 系统参数配置头文�?  

// ==================== 平台适配宏定�?====================
// 自定义硬件友好的断言（仿真时生效，综合时为空�?
#ifdef __SYNTHESIS__
#define HLS_ASSERT(condition, msg) ((void)0)
#else
#define HLS_ASSERT(condition, msg) do { \
    if (!(condition)) { \
        assert(false && "FunnelSketch: " msg); \
    } \
} while (0)
#endif

// ==================== 编译期常量定义（保留原有值，替换宏的类型安全封装�?====================
/** Bottom层最大桶数量 */
static constexpr uint32_t MAX_W_HT_32 = 8192;
/** Middle层最大桶数量 */
static constexpr uint32_t MAX_W_D = 1004;
/** Top层最大桶数量 */
static constexpr uint32_t MAX_W_U = 87040;
/** Middle层总容�?*/
static constexpr uint32_t MIDDLE_TOTAL_CAPACITY = 65790;

// ==================== 工具函数（优化HLS 2018.3兼容性） ====================
/**
 * @brief 硬件友好的哈希值拷贝函数（消除依赖告警，HLS 2018.3适配�? * @param dst 目标哈希值数�? * @param src 源哈希值数�? * @note 核心逻辑保留，优化HLS指令和依赖约�? */
inline void copy_hash_vals(
    uint32_t (&dst)[HASH_FUNC_NUM],
    const uint32_t (&src)[HASH_FUNC_NUM]
) noexcept {

    Hash_Copy_Loop:
    for (uint32_t i = 0; i < HASH_FUNC_NUM; ++i) {
        #pragma HLS UNROLL complete
        dst[i] = src[i];
    }
}

// ==================== FunnelSketch核心�?====================
/**
 * @brief FunnelSketch核心类（HLS 2018.3深度适配版）
 * @note 核心业务逻辑100%保留，仅优化HLS指令、消除依赖告警、提升资源效�? */
class FunnelSketch
{
private:
    // 完全保留原有成员变量声明
    uint32_t val_m;
    uint32_t val_t;
    uint32_t val_b;
    uint32_t overflow;

    // 核心实例对象（无任何数组映射约束，保留原有声明）
    HashTable_32<MAX_W_HT_32> hashtable_32;
    Middle middle;
    Top top;

    uint32_t w_of_HT_32;
    uint32_t w_d;
    uint32_t w_u;

    // 编译期检查：确保哈希函数数量匹配各层定义
    static_assert(HASH_FUNC_NUM == 3, "HASH_FUNC_NUM must be 3 (match Bottom/Middle/Top)");
    static_assert(KEY_BYTE_LEN == 8, "KEY_BYTE_LEN must be 8");

public:
    /**
     * @brief 默认构造函数（完全保留原有逻辑�?     */
    FunnelSketch() noexcept :
        val_m(0), val_t(0), val_b(0), overflow(0),
        w_of_HT_32(MAX_W_HT_32), w_d(MAX_W_D), w_u(MAX_W_U)
    {}

    /**
     * @brief 带参数构造函数（完全保留原有逻辑�?     * @param _w_of_HT_32 Bottom层桶数量
     * @param _w_d Middle层桶数量
     * @param _w_u Top层桶数量
     */
    FunnelSketch(uint32_t _w_of_HT_32, uint32_t _w_d, uint32_t _w_u) noexcept :
        val_m(0), val_t(0), val_b(0), overflow(0),
        w_of_HT_32(_w_of_HT_32), w_d(_w_d), w_u(_w_u)
    {}

    /**
     * @brief 析构函数（默认实现，符合C++规范�?     */
    ~FunnelSketch() = default;

    // 禁用拷贝构造和赋值（避免HLS生成冗余硬件�?    FunnelSketch(const FunnelSketch&) = delete;
    FunnelSketch& operator=(const FunnelSketch&) = delete;

    FunnelSketch(FunnelSketch&&) noexcept = default;
    FunnelSketch& operator=(FunnelSketch&&) noexcept = default;

    /**
     * @brief 初始化FunnelSketch（完全保留原有逻辑，优化HLS指令�?     * @param _w_of_HT_32 Bottom层桶数量
     * @param _w_d Middle层桶数量
     * @param _w_u Top层桶数量
     */
    void init(uint32_t _w_of_HT_32, uint32_t _w_d, uint32_t _w_u) noexcept {

        HLS_ASSERT(_w_of_HT_32 > 0 && _w_of_HT_32 <= MAX_W_HT_32,
                   "init: Bottom buckets exceed limit (1~8192)");
        HLS_ASSERT(_w_d > 0 && _w_d <= MAX_W_D, 
                   "init: Middle buckets exceed limit (1~1004)");
        HLS_ASSERT(_w_u > 0 && _w_u <= MAX_W_U, 
                   "init: Top buckets exceed limit (1~87040)");

        // 核心逻辑完全保留
        w_of_HT_32 = _w_of_HT_32;
        w_d = _w_d;
        w_u = _w_u;

        hashtable_32.init();
        middle.init(w_d);
        top.init(w_u);

        val_m = 0;
        val_t = 0;
        val_b = 0;
        overflow = 0;
    }

    /**
     * @brief 清空FunnelSketch（完全保留原有逻辑，优化HLS指令�?     */
    void clear() noexcept {

        // 核心逻辑完全保留
        hashtable_32.clear();
        middle.clear();
        top.clear();

        val_m = 0;
        val_t = 0;
        val_b = 0;
        overflow = 0;
    }

    /**
     * @brief 插入数据到FunnelSketch�?00%保留核心逻辑，消除依赖告警）
     * @param key 输入键（8字节数组�?     * @param key_len 键长度（必须�?�?     * @param f 要插入的计数值（>0�?     * @param hash_vals 哈希值数组（输出参数�?     */
    void insert(
        const ap_uint<8> (&key)[KEY_BYTE_LEN],
        uint16_t key_len,
        uint32_t f,
        uint32_t (&hash_vals)[HASH_FUNC_NUM]
    ) noexcept {

        uint32_t hashval[HASH_FUNC_NUM];

        HLS_ASSERT(key_len == KEY_BYTE_LEN, "insert: key_len must be 8");
        HLS_ASSERT(f > 0, "insert: f must be positive");

        // 核心逻辑100%保留（Top→Middle→Bottom的漏斗插入流程）
        overflow = top.insert(key, key_len, f, hashval);
        copy_hash_vals(hash_vals, hashval);
        if (overflow == 0) return;

        overflow = middle.insert(key, key_len, overflow, hashval);
        copy_hash_vals(hash_vals, hashval);
        if (overflow == 0) return;

        hashtable_32.insert(hashval, overflow, 0);
        copy_hash_vals(hash_vals, hashval);
    }

    /**
     * @brief 查询FunnelSketch数据�?00%保留核心逻辑，消除依赖告警）
     * @param key 输入键（8字节数组�?     * @param key_len 键长度（必须�?�?     * @param hash_vals 哈希值数组（输出参数�?     * @return 查询结果（Top/Middle/Bottom层的有效值）
     */
    uint32_t query(
        const ap_uint<8> (&key)[KEY_BYTE_LEN],
        uint16_t key_len,
        uint32_t (&hash_vals)[HASH_FUNC_NUM]
    ) noexcept {

        uint32_t hashval[HASH_FUNC_NUM];

        HLS_ASSERT(key_len == KEY_BYTE_LEN, "query: key_len must be 8");

        // 核心逻辑100%保留（Top→Middle→Bottom的漏斗查询流程）
        val_t = top.query(key, key_len, hashval);
        copy_hash_vals(hash_vals, hashval);
        if (val_t < TOP_THRESHOLD) return val_t;

        val_m = middle.query(key, key_len, hashval);
        copy_hash_vals(hash_vals, hashval);

        val_b = hashtable_32.query(hashval, 0);
        if (val_m > BOTTOM_OFFSET) {
            return val_b;
        } else {
            return val_m;
        }
    }
};

#endif // _FUNNELSKETCH_H



