# 面向数字基础设施的 FPGA 网络流量感知系统

**2026 年 FPGA 创新设计赛道 · AMD 自主选题**

基于 **Zynq-7020** 的网络流级报文频次测量原型，采用 Funnel-Sketch 漏斗式分层统计方法，并通过 **PS/PL 协同、硬件报文解析和三级 DATAFLOW** 完成在线流量统计与指定流查询。

## 项目概述

网络流量由大量不同通信流组成。项目在 FPGA 数据通路中解析 IPv4 TCP/UDP 五元组，以紧凑的片上统计状态估计各流报文频次。PL 承担报文接入、流键计算与计数更新；PS 承担初始化、运行控制和按需查询。

本项目的技术工作重点是 **Funnel-Sketch 的 FPGA 工程映射、任务级流水组织、以太网数据通路集成和软硬件查询接口**。Funnel-Sketch 方法本身有既有研究来源，本项目不将原算法作为原创发明。

## 系统特点

- **分层统计：** Top、Middle、Bottom 三级结构分别承担不同频次范围的状态维护，约 **286 KiB** 逻辑统计状态。
- **报文到流标识：** 从 IPv4 TCP/UDP 报文提取源/目的 IP、源/目的端口及协议号，构成五元组。
- **三级 DATAFLOW：** 将解析、哈希预计算与有状态统计维护分离，以 FIFO 连接任务阶段，重叠相邻命令处理。
- **有序状态访问：** UPDATE、QUERY、RESET 经统一后端维护一致的统计状态访问顺序。
- **PS/PL 协同：** PS 通过 AXI/DMA 通路发起查询并读取流频次估计结果。

## 已有测试记录摘要

| 项目 | 记录结果 | 口径 |
| --- | --- | --- |
| 片上逻辑统计容量 | 约 286 KiB | Top/Middle/Bottom 的逻辑计数状态 |
| 真实流样本 | 22,709 条流 | 既有测量分析记录 |
| 真实流平均相对误差 | 0.33533% | 按既有统计分析口径计算 |
| 内部 UPDATE 完成间隔对照 | 缩短 29.17%～43.05% | 100 MHz、纯 UPDATE 连续供给下的设计内对照；不是端到端 Ethernet 吞吐率 |

这些数字摘自项目既有技术与分析记录，**不代表本 GitHub 仓库已独立重新运行测试**。详细验证条件及版本边界见 [验证记录](docs/VALIDATION_STATUS.md)。

## 系统数据路径

\`\`\`mermaid
flowchart LR
    A[以太网报文] --> B[PHY / RGMII]
    B --> C[Ethernet MAC / AXI Stream]
    C --> D[IPv4 TCP/UDP 解析]
    D --> E[流键与哈希预计算]
    E --> F[有序统计后端]
    F --> G[Top / Middle / Bottom]
    H[PS 控制与查询] -->|DMA 查询键| F
    F -->|DMA 频次结果| H
\`\`\`

## 技术资料导航

- [系统架构与设计逻辑](docs/SYSTEM_ARCHITECTURE.md)
- [已有验证结果与适用范围](docs/VALIDATION_STATUS.md)
- [工程依赖与构建环境](DEPENDENCIES.md)
- [工程恢复与复现说明](docs/REPRODUCTION_GUIDE.md)
- [AI 辅助工程实践](docs/AI_COLLABORATION.md)
- [FPGA 设计证据核查工作流](skill/fpga-design-review/SKILL.md)

## 仓库范围

本仓库当前提供**项目架构、设计说明和已有验证记录的文档化整理**。RTL/HLS/PS 源码、Vivado 工程配置和第三方 IP 实现尚未纳入，因此**当前版本不能独立编译、生成 BIT/ELF 或复现整机**；不能将它当作完整源码交付。相关工程材料的共享范围需按照团队权属和第三方许可确定。

对 Candidate-B 特定版本的 BIT/ELF 复现、板级版本配对及 CDC 事项，按 [验证记录](docs/VALIDATION_STATUS.md) 的范围与状态理解；不将内部阶段性成果等同于最终版本全部验收通过。
