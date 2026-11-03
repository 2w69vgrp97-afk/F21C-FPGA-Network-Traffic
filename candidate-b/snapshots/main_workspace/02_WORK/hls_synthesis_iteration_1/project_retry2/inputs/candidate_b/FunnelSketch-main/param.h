/**FileHeader
 * @Author: 赵祥宇
 * @Date: 2025/12/18 19:53:15
 * @LastEditors: 赵祥宇
 * @LastEditTime: 2026/2/26 20:47:39
 * @Description: 
 * @Copyright: Copyright (?)}) 2026 赵祥宇. All rights reserved.
 * @Email: [contact redacted]
 * @Original Author: 刘金典 et al.
 */

#ifndef _PARAM_H_
#define _PARAM_H_

#define KEY_BYTE_LEN     8       // 流Key长度（8字节）
#define HASH_FUNC_NUM    3       // 哈希函数数量

#define TOP_THRESHOLD 15
#define COUNTER_PER_BUCKET_8BIT 25
#define COUNTER_PER_BUCKET_16BIT 4
#define THRESHOLD_REPLACEMENT 2

#define OFFSET_A2_KV (UINT8_MAX + TOP_THRESHOLD) // 270
#define BOTTOM_OFFSET 65805

#define MEMORY_PROP 0.51
#endif
