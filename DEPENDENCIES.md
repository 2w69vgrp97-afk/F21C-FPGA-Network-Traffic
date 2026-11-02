# 工具环境与工程依赖

F21C 的原型平台为 **AC7021B / Zynq-7020**，硬件开发环境以 **AMD Vivado、Vitis、Vitis HLS 2025.2** 为基准。

| 部分 | 主要用途 | 工程依赖 |
| --- | --- | --- |
| Vivado / PL | RTL、Block Design、约束、IP 集成、综合与实现 | XPR/BD/XCI、Tcl、RTL、XDC、Zynq-7020 器件支持 |
| Vitis HLS | 分层统计及相关任务综合、IP 打包 | C/C++、头文件、HLS 配置与测试数据 |
| Ethernet | PHY/RGMII 接收与 MAC/AXI Stream 接口 | AXI Ethernet 等 AMD IP 及对应许可 |
| Vitis / PS | 初始化、DMA 控制与流频次查询 | 应用源码、匹配的 XSA/平台、BSP 与驱动 |
| 板级调试 | 下载与运行状态观察 | 目标板、连接线、UART/JTAG、对应软硬件版本 |

## 工程配置

内部交付记录中已定位到 XPR、BD、XCI、HLS 组件配置与重建 Tcl。完整复现需要使用与源代码、IP 版本、硬件平台相匹配的配置。涉及 AMD 专有 IP 或其他第三方实现的文件，应按适用许可取得或由工具合法生成。

**当前 GitHub 仓库只发布技术说明文档，不包含上述工程源码与配置。** 因而仓库内容不能直接执行完整构建。具体的恢复步骤和验证边界参见 [复现说明](docs/REPRODUCTION_GUIDE.md)。

Funnel-Sketch 及项目中使用的第三方哈希实现应保留其各自的来源和许可条件；在授权明确前，不将有关实现文件作为本仓库的开源源码提供。
