#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sleep.h"
#include "xaxidma.h"
#include "xaxiethernet.h"
#include "xaxiethernet_hw.h"
#include "xfs_hls_test.h"
#include "xil_cache.h"
#include "xil_printf.h"
#include "xparameters.h"
#ifndef XPAR_CPU_CORTEXA9_0_CPU_CLK_FREQ_HZ
#define XPAR_CPU_CORTEXA9_0_CPU_CLK_FREQ_HZ XPAR_CPU_CORE_CLOCK_FREQ_HZ
#endif

#include "xiltimer.h"



#include "xuartps_hw.h"

#define PL_PHY_ADDR 1U

#define CMD_BUF_LEN 160U
#define QUERY_TIMEOUT 2000000U

#define PHY_REG_BMCR 0U
#define PHY_REG_BMSR 1U
#define PHY_REG_ID1  2U
#define PHY_REG_ID2  3U
#define PHY_REG_ANAR 4U
#define PHY_REG_GBCR 9U
#define PHY_REG_MMD_CTRL 13U
#define PHY_REG_MMD_DATA 14U
#define PHY_REG_KSZ9031_CTRL 0x1FU

#define KSZ9031_MMD_RGMII_DEV 2U
#define KSZ9031_MMD_RGMII_CTRL_SKEW 4U
#define KSZ9031_MMD_RGMII_RX_DATA_SKEW 5U
#define KSZ9031_MMD_RGMII_TX_DATA_SKEW 6U
#define KSZ9031_MMD_RGMII_CLK_SKEW 8U

static XAxiEthernet g_eth;
static XFs_hls_test g_fs;
static XAxiDma g_result_dma; /* HLS result stream -> DDR. */
static XAxiDma g_query_dma;  /* DDR query key -> HLS query stream. */

static uint64_t g_query_word __attribute__((aligned(64)));
static uint32_t g_result_word __attribute__((aligned(64)));

static uint32_t g_queries_sent;
static uint32_t g_query_timeouts;
static uint32_t g_reinit_count;
static uint16_t g_requested_speed = 1000;
static int g_bad_frame_enabled;
static int g_pl_ready;

static const uint8_t kPlMac[6] = {0x00, 0x0A, 0x35, 0x05, 0x01, 0x02};

static void early_uart_puts(const char *s)
{
    while (*s != '\0') {
        XUartPs_SendByte(STDOUT_BASEADDRESS, (uint8_t)*s++);
    }
}

static uint32_t parse_ipv4(const char *s)
{
    unsigned a = 0, b = 0, c = 0, d = 0;
    if (sscanf(s, "%u.%u.%u.%u", &a, &b, &c, &d) != 4) {
        return 0;
    }
    return ((a & 0xffU) << 24) | ((b & 0xffU) << 16) |
           ((c & 0xffU) << 8) | (d & 0xffU);
}

static uint64_t fnv_fold_five_tuple(uint32_t src_ip, uint32_t dst_ip,
                                    uint16_t src_port, uint16_t dst_port,
                                    uint8_t proto)
{
    uint8_t bytes[13];
    bytes[0] = (uint8_t)(src_ip >> 24);
    bytes[1] = (uint8_t)(src_ip >> 16);
    bytes[2] = (uint8_t)(src_ip >> 8);
    bytes[3] = (uint8_t)(src_ip);
    bytes[4] = (uint8_t)(dst_ip >> 24);
    bytes[5] = (uint8_t)(dst_ip >> 16);
    bytes[6] = (uint8_t)(dst_ip >> 8);
    bytes[7] = (uint8_t)(dst_ip);
    bytes[8] = (uint8_t)(src_port >> 8);
    bytes[9] = (uint8_t)(src_port);
    bytes[10] = (uint8_t)(dst_port >> 8);
    bytes[11] = (uint8_t)(dst_port);
    bytes[12] = proto;

    uint64_t hash = 0xcbf29ce484222325ULL;
    for (unsigned i = 0; i < sizeof(bytes); ++i) {
        hash ^= bytes[i];
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

static void phy_read_print(uint32_t reg)
{
    uint16_t val = 0;
    XAxiEthernet_PhyRead(&g_eth, PL_PHY_ADDR, reg, &val);
    xil_printf("  phy[%u]=0x%04x\r\n",
               (unsigned int)reg, (unsigned int)val);
}

static void phy_scan_print(void)
{
    xil_printf("[physcan] addr  bmsr   id1    id2    ctrl31\r\n");
    for (uint32_t addr = 0; addr < 32U; ++addr) {
        uint16_t bmsr = 0;
        uint16_t id1 = 0;
        uint16_t id2 = 0;
        uint16_t ctrl31 = 0;
        XAxiEthernet_PhyRead(&g_eth, addr, PHY_REG_BMSR, &bmsr);
        XAxiEthernet_PhyRead(&g_eth, addr, PHY_REG_ID1, &id1);
        XAxiEthernet_PhyRead(&g_eth, addr, PHY_REG_ID2, &id2);
        XAxiEthernet_PhyRead(&g_eth, addr, PHY_REG_KSZ9031_CTRL, &ctrl31);
        if ((bmsr != 0U) || (id1 != 0U) || (id2 != 0U) || (ctrl31 != 0U) ||
            (addr == PL_PHY_ADDR)) {
            xil_printf("[physcan] %2u   0x%04x 0x%04x 0x%04x 0x%04x\r\n",
                       (unsigned int)addr,
                       (unsigned int)bmsr,
                       (unsigned int)id1,
                       (unsigned int)id2,
                       (unsigned int)ctrl31);
        }
    }
}

static int phy_mmd_read(uint16_t dev, uint16_t reg, uint16_t *val)
{
    XAxiEthernet_PhyWrite(&g_eth, PL_PHY_ADDR, PHY_REG_MMD_CTRL,
                          dev & 0x1fU);
    XAxiEthernet_PhyWrite(&g_eth, PL_PHY_ADDR, PHY_REG_MMD_DATA, reg);
    XAxiEthernet_PhyWrite(&g_eth, PL_PHY_ADDR, PHY_REG_MMD_CTRL,
                          0x4000U | (dev & 0x1fU));
    XAxiEthernet_PhyRead(&g_eth, PL_PHY_ADDR, PHY_REG_MMD_DATA, val);
    return XST_SUCCESS;
}

static int phy_mmd_write(uint16_t dev, uint16_t reg, uint16_t val)
{
    XAxiEthernet_PhyWrite(&g_eth, PL_PHY_ADDR, PHY_REG_MMD_CTRL,
                          dev & 0x1fU);
    XAxiEthernet_PhyWrite(&g_eth, PL_PHY_ADDR, PHY_REG_MMD_DATA, reg);
    XAxiEthernet_PhyWrite(&g_eth, PL_PHY_ADDR, PHY_REG_MMD_CTRL,
                          0x4000U | (dev & 0x1fU));
    XAxiEthernet_PhyWrite(&g_eth, PL_PHY_ADDR, PHY_REG_MMD_DATA, val);
    return XST_SUCCESS;
}

static void print_rgmii_skew(void)
{
    uint16_t ctrl = 0, rx = 0, tx = 0, clk = 0;
    int s0 = phy_mmd_read(KSZ9031_MMD_RGMII_DEV, KSZ9031_MMD_RGMII_CTRL_SKEW, &ctrl);
    int s1 = phy_mmd_read(KSZ9031_MMD_RGMII_DEV, KSZ9031_MMD_RGMII_RX_DATA_SKEW, &rx);
    int s2 = phy_mmd_read(KSZ9031_MMD_RGMII_DEV, KSZ9031_MMD_RGMII_TX_DATA_SKEW, &tx);
    int s3 = phy_mmd_read(KSZ9031_MMD_RGMII_DEV, KSZ9031_MMD_RGMII_CLK_SKEW, &clk);
    xil_printf("[rgmii] status=%d/%d/%d/%d ctrl=0x%04x rx_data=0x%04x tx_data=0x%04x clk=0x%04x\r\n",
               s0, s1, s2, s3,
               (unsigned int)ctrl,
               (unsigned int)rx,
               (unsigned int)tx,
               (unsigned int)clk);
    xil_printf("[rgmii] clk fields: gtx_clk=0x%02x rx_clk=0x%02x\r\n",
               (unsigned int)((clk >> 5) & 0x1fU),
               (unsigned int)(clk & 0x1fU));
}

static void apply_rgmii_rx_skew(uint16_t rx_clk_skew, uint16_t rx_data_skew)
{
    uint16_t clk = 0;
    uint16_t ctrl = (uint16_t)(((rx_data_skew & 0x0fU) << 4) | 0x0007U);
    uint16_t rx = (uint16_t)((rx_data_skew & 0x0fU) |
                             ((rx_data_skew & 0x0fU) << 4) |
                             ((rx_data_skew & 0x0fU) << 8) |
                             ((rx_data_skew & 0x0fU) << 12));

    (void)phy_mmd_read(KSZ9031_MMD_RGMII_DEV, KSZ9031_MMD_RGMII_CLK_SKEW, &clk);
    clk = (uint16_t)((clk & 0x03e0U) | (rx_clk_skew & 0x1fU));

    int s0 = phy_mmd_write(KSZ9031_MMD_RGMII_DEV, KSZ9031_MMD_RGMII_CTRL_SKEW, ctrl);
    int s1 = phy_mmd_write(KSZ9031_MMD_RGMII_DEV, KSZ9031_MMD_RGMII_RX_DATA_SKEW, rx);
    int s2 = phy_mmd_write(KSZ9031_MMD_RGMII_DEV, KSZ9031_MMD_RGMII_CLK_SKEW, clk);
    xil_printf("[rgmii] set rx_clk=0x%02x rx_data=0x%x status=%d/%d/%d\r\n",
               (unsigned int)(rx_clk_skew & 0x1fU),
               (unsigned int)(rx_data_skew & 0x0fU),
               s0, s1, s2);
    print_rgmii_skew();
}

static int wait_phy_link(void)
{
    uint16_t status = 0;

    if (g_requested_speed == 1000U) {
        xil_printf("[TRACE] phy: write ANAR\r\n");
        XAxiEthernet_PhyWrite(&g_eth, PL_PHY_ADDR, PHY_REG_ANAR, 0x0de1U);
        xil_printf("[TRACE] phy: write GBCR\r\n");
        XAxiEthernet_PhyWrite(&g_eth, PL_PHY_ADDR, PHY_REG_GBCR, 0x0300U);
        xil_printf("[TRACE] phy: restart autoneg\r\n");
        XAxiEthernet_PhyWrite(&g_eth, PL_PHY_ADDR, PHY_REG_BMCR, 0x1340U);
    } else if (g_requested_speed == 100U) {
        xil_printf("[TRACE] phy: autoneg 100M full-duplex only\r\n");
        // Keep the PC adapter in autonegotiation while constraining the PHY
        // advertisement to 100BASE-TX full duplex plus pause capability.
        XAxiEthernet_PhyWrite(&g_eth, PL_PHY_ADDR, PHY_REG_ANAR, 0x0d01U);
        XAxiEthernet_PhyWrite(&g_eth, PL_PHY_ADDR, PHY_REG_GBCR, 0x0000U);
        XAxiEthernet_PhyWrite(&g_eth, PL_PHY_ADDR, PHY_REG_BMCR, 0x1200U);
    } else {
        uint16_t bmcr = 0x0100U;
        xil_printf("[TRACE] phy: force %uM full-duplex BMCR=0x%04x\r\n",
                   (unsigned int)g_requested_speed,
                   (unsigned int)bmcr);
        XAxiEthernet_PhyWrite(&g_eth, PL_PHY_ADDR, PHY_REG_GBCR, 0x0000U);
        XAxiEthernet_PhyWrite(&g_eth, PL_PHY_ADDR, PHY_REG_BMCR, bmcr);
    }

    xil_printf("[PLPHY] waiting for link");
    for (unsigned i = 0; i < 30; ++i) {
        XAxiEthernet_PhyRead(&g_eth, PL_PHY_ADDR, PHY_REG_BMSR, &status);
        xil_printf("(%u:0x%04x)", i, (unsigned int)status);
        if ((g_requested_speed == 1000U) && ((status & 0x0024U) == 0x0024U)) {
            xil_printf(" done, bmsr=0x%04x\r\n", (unsigned int)status);
            return XST_SUCCESS;
        }
        if ((g_requested_speed != 1000U) && ((status & 0x0004U) == 0x0004U)) {
            xil_printf(" done, bmsr=0x%04x\r\n", (unsigned int)status);
            return XST_SUCCESS;
        }
        xil_printf(".");
        usleep(100000);
    }

    xil_printf(" timeout, bmsr=0x%04x\r\n", (unsigned int)status);
    return XST_FAILURE;
}

static void apply_bad_frame_option(void)
{
    if (g_bad_frame_enabled) {
        XAxiEthernet_SetBadFrmRcvOption(&g_eth);
    } else {
        XAxiEthernet_ClearBadFrmRcvOption(&g_eth);
    }
}

static int init_dma_one(XAxiDma *dma, UINTPTR base_address)
{
    XAxiDma_Config *cfg = XAxiDma_LookupConfig(base_address);
    if (cfg == NULL) {
        return XST_FAILURE;
    }
    int status = XAxiDma_CfgInitialize(dma, cfg);
    if (status != XST_SUCCESS) {
        return status;
    }
    if (XAxiDma_HasSg(dma)) {
        return XST_FAILURE;
    }
    XAxiDma_IntrDisable(dma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
    XAxiDma_IntrDisable(dma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
    return XST_SUCCESS;
}

static int init_hls(void)
{
    int status = XFs_hls_test_Initialize(&g_fs, XPAR_FS_HLS_TEST_0_BASEADDR);
    if (status != XST_SUCCESS) {
        xil_printf("[ERR] XFs_hls_test_Initialize failed: %d\r\n", status);
        return status;
    }

    XFs_hls_test_DisableAutoRestart(&g_fs);
    XFs_hls_test_Set_init_rst(&g_fs, 1);
    XFs_hls_test_Start(&g_fs);
    int clear_done = 0;
    for (unsigned i = 0; i < 200000000U; ++i) {
        if (XFs_hls_test_IsDone(&g_fs)) {
            clear_done = 1;
            break;
        }
    }
    if (!clear_done) {
        xil_printf("[ERR] PL FS HLS clear timeout\r\n");
        return XST_FAILURE;
    }
    XFs_hls_test_Set_init_rst(&g_fs, 0);
    XFs_hls_test_EnableAutoRestart(&g_fs);
    XFs_hls_test_Start(&g_fs);

    xil_printf("[OK] PL FS HLS started, auto_restart=on\r\n");
    return XST_SUCCESS;
}

static int init_hls_no_clear(void)
{
    int status = XFs_hls_test_Initialize(&g_fs, XPAR_FS_HLS_TEST_0_BASEADDR);
    if (status != XST_SUCCESS) {
        xil_printf("[ERR] XFs_hls_test_Initialize failed: %d\r\n", status);
        return status;
    }

    XFs_hls_test_DisableAutoRestart(&g_fs);
    XFs_hls_test_Set_init_rst(&g_fs, 0);
    XFs_hls_test_EnableAutoRestart(&g_fs);
    XFs_hls_test_Start(&g_fs);

    xil_printf("[OK] PL FS HLS started without dynamic clear, auto_restart=on\r\n");
    return XST_SUCCESS;
}

static int init_pl_eth(void)
{
    xil_printf("[TRACE] eth: LookupConfig\r\n");
    XAxiEthernet_Config *cfg =
        XAxiEthernet_LookupConfig(XPAR_XAXIETHERNET_0_BASEADDR);
    if (cfg == NULL) {
        xil_printf("[ERR] XAxiEthernet_LookupConfig failed\r\n");
        return XST_FAILURE;
    }
    xil_printf("[TRACE] eth: cfg base=0x%08x dma=0x%08x phyaddr=%u\r\n",
               (unsigned int)cfg->BaseAddress,
               (unsigned int)cfg->AxiDevBaseAddress,
               (unsigned int)XPAR_XAXIETHERNET_0_PHYADDR);

    xil_printf("[TRACE] eth: CfgInitialize enter\r\n");
    int status = XAxiEthernet_CfgInitialize(&g_eth, cfg, cfg->BaseAddress);
    xil_printf("[TRACE] eth: CfgInitialize exit status=%d\r\n", status);
    if (status != XST_SUCCESS) {
        xil_printf("[ERR] XAxiEthernet_CfgInitialize failed: %d\r\n", status);
        return status;
    }

    xil_printf("[TRACE] eth: Stop\r\n");
    XAxiEthernet_Stop(&g_eth);
    xil_printf("[TRACE] eth: MdioDivisor\r\n");
    XAxiEthernet_PhySetMdioDivisor(&g_eth, XAE_MDIO_DIV_DFT);
    xil_printf("[TRACE] eth: SetMacAddress\r\n");
    status = XAxiEthernet_SetMacAddress(&g_eth, (void *)kPlMac);
    xil_printf("[TRACE] eth: SetMacAddress exit status=%d\r\n", status);
    if (status != XST_SUCCESS) {
        xil_printf("[ERR] SetMacAddress failed: %d\r\n", status);
        return status;
    }

    xil_printf("[TRACE] eth: SetOptions\r\n");
    status = XAxiEthernet_SetOptions(&g_eth,
        XAE_DEFAULT_OPTIONS | XAE_PROMISC_OPTION | XAE_JUMBO_OPTION);
    xil_printf("[TRACE] eth: SetOptions exit status=%d\r\n", status);
    if (status != XST_SUCCESS) {
        xil_printf("[ERR] SetOptions failed: %d\r\n", status);
        return status;
    }

    xil_printf("[TRACE] eth: wait_phy_link\r\n");
    (void)wait_phy_link();
    xil_printf("[TRACE] eth: native RGMII IO; no external gmii_to_rgmii programming\r\n");
    apply_bad_frame_option();
    xil_printf("[TRACE] eth: SetOperatingSpeed %u\r\n",
               (unsigned int)g_requested_speed);
    status = XAxiEthernet_SetOperatingSpeed(&g_eth, g_requested_speed);
    xil_printf("[TRACE] eth: SetOperatingSpeed exit status=%d\r\n", status);
    if (status != XST_SUCCESS) {
        xil_printf("[WARN] SetOperatingSpeed(%u) failed: %d\r\n",
                   (unsigned int)g_requested_speed, status);
    }
    xil_printf("[TRACE] eth: Start\r\n");
    XAxiEthernet_Start(&g_eth);
    xil_printf("[TRACE] eth: Start returned\r\n");

    xil_printf("[OK] PL AXI Ethernet started. MAC %02x:%02x:%02x:%02x:%02x:%02x speed=%uM\r\n",
               (unsigned int)kPlMac[0],
               (unsigned int)kPlMac[1],
               (unsigned int)kPlMac[2],
               (unsigned int)kPlMac[3],
               (unsigned int)kPlMac[4],
               (unsigned int)kPlMac[5],
               (unsigned int)XAxiEthernet_GetOperatingSpeed(&g_eth));
    return XST_SUCCESS;
}

static int init_all(void)
{
    int status = init_dma_one(&g_result_dma, XPAR_XAXIDMA_0_BASEADDR);
    if (status != XST_SUCCESS) {
        xil_printf("[ERR] result DMA init failed: %d\r\n", status);
        return status;
    }
    status = init_dma_one(&g_query_dma, XPAR_XAXIDMA_1_BASEADDR);
    if (status != XST_SUCCESS) {
        xil_printf("[ERR] query DMA init failed: %d\r\n", status);
        return status;
    }
    status = init_hls();
    if (status != XST_SUCCESS) {
        return status;
    }
    xil_printf("[TRACE] init_all: init_pl_eth\r\n");
    status = init_pl_eth();
    if (status != XST_SUCCESS) {
        return status;
    }
    ++g_reinit_count;
    return XST_SUCCESS;
}

static int init_all_no_clear(void)
{
    int status = init_dma_one(&g_result_dma, XPAR_XAXIDMA_0_BASEADDR);
    if (status != XST_SUCCESS) {
        xil_printf("[ERR] result DMA init failed: %d\r\n", status);
        return status;
    }
    status = init_dma_one(&g_query_dma, XPAR_XAXIDMA_1_BASEADDR);
    if (status != XST_SUCCESS) {
        xil_printf("[ERR] query DMA init failed: %d\r\n", status);
        return status;
    }
    status = init_hls_no_clear();
    if (status != XST_SUCCESS) {
        return status;
    }
    xil_printf("[TRACE] init_all_no_clear: init_pl_eth\r\n");
    status = init_pl_eth();
    if (status != XST_SUCCESS) {
        return status;
    }
    ++g_reinit_count;
    return XST_SUCCESS;
}

static int query_key_internal(uint64_t key, uint32_t *estimate, int verbose)
{
    g_query_word = key;
    g_result_word = 0;

    Xil_DCacheFlushRange((UINTPTR)&g_query_word, sizeof(g_query_word));
    Xil_DCacheFlushRange((UINTPTR)&g_result_word, sizeof(g_result_word));

    if (verbose) {
        xil_printf("[QDBG] start key=0x%08x%08x\r\n",
                   (unsigned int)(g_query_word >> 32),
                   (unsigned int)(g_query_word & UINT64_C(0xffffffff)));
    }

    int status = XAxiDma_SimpleTransfer(&g_result_dma, (UINTPTR)&g_result_word,
                                        sizeof(g_result_word), XAXIDMA_DEVICE_TO_DMA);
    if (verbose) {
        xil_printf("[QDBG] result_dma start status=%d\r\n", status);
    }
    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    status = XAxiDma_SimpleTransfer(&g_query_dma, (UINTPTR)&g_query_word,
                                    sizeof(g_query_word), XAXIDMA_DMA_TO_DEVICE);
    if (verbose) {
        xil_printf("[QDBG] query_dma start status=%d\r\n", status);
    }
    if (status != XST_SUCCESS) {
        XAxiDma_Reset(&g_result_dma);
        while (!XAxiDma_ResetIsDone(&g_result_dma)) {
        }
        return XST_FAILURE;
    }

    for (unsigned i = 0; i < QUERY_TIMEOUT; ++i) {
        if (!XAxiDma_Busy(&g_result_dma, XAXIDMA_DEVICE_TO_DMA) &&
            !XAxiDma_Busy(&g_query_dma, XAXIDMA_DMA_TO_DEVICE)) {
            Xil_DCacheInvalidateRange((UINTPTR)&g_result_word, sizeof(g_result_word));
            *estimate = g_result_word;
            ++g_queries_sent;
            return XST_SUCCESS;
        }
        if (verbose && ((i & 0x3ffffU) == 0x3ffffU)) {
            xil_printf("[QDBG] polling i=%u rd_busy=%d wr_busy=%d\r\n",
                       i,
                       (int)XAxiDma_Busy(&g_result_dma,
                                             XAXIDMA_DEVICE_TO_DMA),
                       (int)XAxiDma_Busy(&g_query_dma,
                                             XAXIDMA_DMA_TO_DEVICE));
        }
    }

    ++g_query_timeouts;
    if (verbose) {
        xil_printf("[QDBG] timeout; resetting query/result DMA\r\n");
    }
    XAxiDma_Reset(&g_query_dma);
    while (!XAxiDma_ResetIsDone(&g_query_dma)) {
    }
    XAxiDma_Reset(&g_result_dma);
    while (!XAxiDma_ResetIsDone(&g_result_dma)) {
    }
    return XST_FAILURE;
}

static int query_flow_internal(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port,
                               uint16_t dst_port, uint8_t proto, uint32_t *estimate,
                               int verbose)
{
    uint64_t key = fnv_fold_five_tuple(src_ip, dst_ip, src_port, dst_port, proto);
    return query_key_internal(key, estimate, verbose);
}

static int query_flow(uint32_t src_ip, uint32_t dst_ip, uint16_t src_port,
                      uint16_t dst_port, uint8_t proto, uint32_t *estimate)
{
    return query_flow_internal(src_ip, dst_ip, src_port, dst_port, proto, estimate, 1);
}

static void split_fixed6(double value, uint32_t *whole, uint32_t *frac)
{
    uint32_t whole_value = (uint32_t)value;
    double frac_d = value - (double)whole_value;
    if (frac_d < 0.0) {
        frac_d = 0.0;
    }
    uint32_t frac_value = (uint32_t)(frac_d * 1000000.0 + 0.5);
    if (frac_value >= 1000000U) {
        ++whole_value;
        frac_value -= 1000000U;
    }
    *whole = whole_value;
    *frac = frac_value;
}

static void print_fixed6(const char *name, double value)
{
    uint32_t whole = 0U;
    uint32_t frac = 0U;

    split_fixed6(value, &whole, &frac);
    xil_printf("%s=%u.%06u", name,
               (unsigned int)whole,
               (unsigned int)frac);
}

static void bench_query(uint32_t count)
{
    if (!g_pl_ready) {
        xil_printf("[ERR] PL path is not initialized yet. Run 'reinit' first.\r\n");
        return;
    }
    if (count == 0U) {
        xil_printf("usage: benchquery <count>\r\n");
        return;
    }

    uint32_t estimate = 0;
    uint32_t ok = 0;
    uint32_t fail = 0;
    uint32_t sink = 0;
    XTime t0 = 0;
    XTime t1 = 0;

    XTime_GetTime(&t0);
    for (uint32_t i = 0; i < count; ++i) {
        uint16_t src_port = (uint16_t)(50000U + (i & 0x3ffU));
        int status = query_flow_internal(0xC0A86464U, 0xC0A86414U,
                                         src_port, 1234U, 17U,
                                         &estimate, 0);
        if (status == XST_SUCCESS) {
            ++ok;
            sink ^= estimate;
        } else {
            ++fail;
        }
    }
    XTime_GetTime(&t1);

    double seconds = (double)(t1 - t0) / (double)COUNTS_PER_SECOND;
    double qps = (seconds > 0.0) ? ((double)ok / seconds) : 0.0;
    uint32_t seconds_whole = 0U;
    uint32_t seconds_frac = 0U;
    split_fixed6(seconds, &seconds_whole, &seconds_frac);
    xil_printf("[benchquery] requested=%u ok=%u fail=%u seconds=%u.%06u qps=%u sink=%u\r\n",
               (unsigned int)count,
               (unsigned int)ok,
               (unsigned int)fail,
               (unsigned int)seconds_whole,
               (unsigned int)seconds_frac,
               (unsigned int)(uint32_t)qps,
               (unsigned int)sink);
}

static void batch_score(UINTPTR keys_addr, UINTPTR exact_addr, uint32_t count)
{
    if (!g_pl_ready) {
        xil_printf("[ERR] PL path is not initialized yet. Run 'reinit' first.\r\n");
        return;
    }
    if (count == 0U) {
        xil_printf("usage: batchscore <keys_addr> <exact_addr> <count>\r\n");
        return;
    }

    uint64_t *keys = (uint64_t *)keys_addr;
    uint32_t *exact = (uint32_t *)exact_addr;
    Xil_DCacheInvalidateRange(keys_addr, count * sizeof(uint64_t));
    Xil_DCacheInvalidateRange(exact_addr, count * sizeof(uint32_t));

    uint32_t estimate = 0;
    uint32_t ok = 0;
    uint32_t fail = 0;
    double abs_sum = 0.0;
    double rel_sum = 0.0;
    uint32_t max_estimate = 0;
    uint32_t max_expected = 0;
    uint32_t max_diff_u32 = 0;
    uint32_t max_index = 0;
    XTime t0 = 0;
    XTime t1 = 0;

    XTime_GetTime(&t0);
    for (uint32_t i = 0; i < count; ++i) {
        int status = query_key_internal(keys[i], &estimate, 0);
        if (status == XST_SUCCESS) {
            uint32_t expected = exact[i];
            double diff = (estimate > expected)
                              ? (double)(estimate - expected)
                              : (double)(expected - estimate);
            if (i < 10U) {
                xil_printf("[batchscore] sample%u key=0x%08x%08x exact=%u estimate=%u\r\n",
                           (unsigned int)i,
                           (unsigned int)(keys[i] >> 32),
                           (unsigned int)(keys[i] & UINT64_C(0xffffffff)),
                           (unsigned int)expected,
                           (unsigned int)estimate);
            }
            abs_sum += diff;
            if (expected != 0U) {
                rel_sum += diff / (double)expected;
            }
            if (diff > (double)max_diff_u32) {
                max_diff_u32 = (diff > 4294967295.0) ? 0xffffffffU : (uint32_t)diff;
                max_estimate = estimate;
                max_expected = expected;
                max_index = i;
            }
            ++ok;
        } else {
            ++fail;
        }
        if (((i + 1U) % 50000U) == 0U) {
            xil_printf("[batchscore] progress=%u/%u\r\n",
                       (unsigned int)(i + 1U), (unsigned int)count);
        }
    }
    XTime_GetTime(&t1);

    double seconds = (double)(t1 - t0) / (double)COUNTS_PER_SECOND;
    double qps = (seconds > 0.0) ? ((double)ok / seconds) : 0.0;
    double are = (ok > 0U) ? (rel_sum / (double)ok) : 0.0;
    double aae = (ok > 0U) ? (abs_sum / (double)ok) : 0.0;

    uint32_t seconds_whole = 0U;
    uint32_t seconds_frac = 0U;
    split_fixed6(seconds, &seconds_whole, &seconds_frac);
    xil_printf("[batchscore] count=%u ok=%u fail=%u seconds=%u.%06u qps=%u\r\n",
               (unsigned int)count,
               (unsigned int)ok,
               (unsigned int)fail,
               (unsigned int)seconds_whole,
               (unsigned int)seconds_frac,
               (unsigned int)(uint32_t)qps);
    xil_printf("[batchscore] max_index=%u max_exact=%u max_estimate=%u max_abs_error=%u\r\n",
               (unsigned int)max_index,
               (unsigned int)max_expected,
               (unsigned int)max_estimate,
               (unsigned int)max_diff_u32);
    xil_printf("[batchscore] ");
    print_fixed6("ARE", are);
    xil_printf(" ");
    print_fixed6("AAE", aae);
    xil_printf("\r\n");
}

static uint8_t uart_recv_blocking(void)
{
    while (!XUartPs_IsReceiveData(STDIN_BASEADDRESS)) {
    }
    return XUartPs_RecvByte(STDIN_BASEADDRESS);
}

static void load_batch(UINTPTR keys_addr, UINTPTR exact_addr, uint32_t count)
{
    if (count == 0U) {
        xil_printf("usage: loadbatch <keys_addr> <exact_addr> <count>\r\n");
        return;
    }

    uint8_t *keys = (uint8_t *)keys_addr;
    uint8_t *exact = (uint8_t *)exact_addr;
    uint32_t key_bytes = count * (uint32_t)sizeof(uint64_t);
    uint32_t exact_bytes = count * (uint32_t)sizeof(uint32_t);

    xil_printf("[loadbatch] ready keys_bytes=%u exact_bytes=%u\r\n",
               (unsigned int)key_bytes, (unsigned int)exact_bytes);
    const uint32_t ack_interval = 4096U;
    uint32_t total_bytes = key_bytes + exact_bytes;
    uint32_t received = 0;

    while (received < total_bytes) {
        uint32_t chunk_end = received + ack_interval;
        if (chunk_end > total_bytes) {
            chunk_end = total_bytes;
        }
        while (received < chunk_end) {
            if (received < key_bytes) {
                keys[received] = uart_recv_blocking();
            } else {
                exact[received - key_bytes] = uart_recv_blocking();
            }
            ++received;
        }
        xil_printf("[loadbatch] ack=%u\r\n", (unsigned int)received);
    }
    xil_printf("[loadbatch] done count=%u\r\n", (unsigned int)count);
}

static void load_keys(UINTPTR keys_addr, uint32_t count)
{
    if (count == 0U) {
        xil_printf("usage: loadkeys <keys_addr> <count>\r\n");
        return;
    }

    uint8_t *keys = (uint8_t *)keys_addr;
    uint32_t key_bytes = count * (uint32_t)sizeof(uint64_t);
    const uint32_t ack_interval = 4096U;
    uint32_t received = 0;
    uint32_t checksum = 0;

    xil_printf("[loadkeys] ready bytes=%u\r\n", (unsigned int)key_bytes);
    while (received < key_bytes) {
        uint32_t chunk_end = received + ack_interval;
        if (chunk_end > key_bytes) {
            chunk_end = key_bytes;
        }
        while (received < chunk_end) {
            uint8_t byte = uart_recv_blocking();
            keys[received] = byte;
            checksum += byte;
            ++received;
        }
        xil_printf("[loadkeys] ack=%u\r\n", (unsigned int)received);
    }
    xil_printf("[loadkeys] done count=%u checksum=0x%08x\r\n",
               (unsigned int)count, (unsigned int)checksum);
}

static void uart_send_u32_le(uint32_t value)
{
    XUartPs_SendByte(STDOUT_BASEADDRESS, (uint8_t)(value & 0xffU));
    XUartPs_SendByte(STDOUT_BASEADDRESS, (uint8_t)((value >> 8) & 0xffU));
    XUartPs_SendByte(STDOUT_BASEADDRESS, (uint8_t)((value >> 16) & 0xffU));
    XUartPs_SendByte(STDOUT_BASEADDRESS, (uint8_t)((value >> 24) & 0xffU));
}

static void dump_query(UINTPTR keys_addr, uint32_t count)
{
    if (!g_pl_ready) {
        xil_printf("[ERR] PL path is not initialized yet. Run 'reinit' first.\r\n");
        return;
    }
    if (count == 0U) {
        xil_printf("usage: dumpquery <keys_addr> <count>\r\n");
        return;
    }

    uint64_t *keys = (uint64_t *)keys_addr;
    uint32_t estimate = 0;
    uint32_t ok = 0;
    uint32_t fail = 0;
    XTime t0 = 0;
    XTime t1 = 0;

    xil_printf("[dumpquery] ready count=%u bytes=%u\r\n",
               (unsigned int)count,
               (unsigned int)(count * (uint32_t)sizeof(uint32_t)));
    (void)uart_recv_blocking(); /* Host sends one byte after it is ready for raw output. */

    XTime_GetTime(&t0);
    for (uint32_t i = 0; i < count; ++i) {
        int status = query_key_internal(keys[i], &estimate, 0);
        if (status == XST_SUCCESS) {
            ++ok;
        } else {
            estimate = 0;
            ++fail;
        }
        uart_send_u32_le(estimate);
    }
    XTime_GetTime(&t1);

    double seconds = (double)(t1 - t0) / (double)COUNTS_PER_SECOND;
    double qps = (seconds > 0.0) ? ((double)ok / seconds) : 0.0;
    uint32_t seconds_whole = 0U;
    uint32_t seconds_frac = 0U;
    split_fixed6(seconds, &seconds_whole, &seconds_frac);
    xil_printf("\r\n[dumpquery] done count=%u ok=%u fail=%u seconds=%u.%06u qps=%u\r\n",
               (unsigned int)count,
               (unsigned int)ok,
               (unsigned int)fail,
               (unsigned int)seconds_whole,
               (unsigned int)seconds_frac,
               (unsigned int)(uint32_t)qps);
}

static void print_help(void)
{
    xil_printf("commands:\r\n");
    xil_printf("  help\r\n");
    xil_printf("  plstats\r\n");
    xil_printf("  ethstats\r\n");
    xil_printf("  phy\r\n");
    xil_printf("  physcan\r\n");
    xil_printf("  rgmiiskew [rx_clk_hex rx_data_hex]\r\n");
    xil_printf("  mmdread <dev> <reg>\r\n");
    xil_printf("  mmdwrite <dev> <reg> <value>\r\n");
    xil_printf("  speed <10|100|1000>\r\n");
    xil_printf("  badframe <on|off>\r\n");
    xil_printf("  resetfs\r\n");
    xil_printf("  reinit\r\n");
    xil_printf("  reinitnoclear\r\n");
    xil_printf("  benchquery <count>\r\n");
    xil_printf("  loadbatch <keys_addr> <exact_addr> <count>\r\n");
    xil_printf("  loadkeys <keys_addr> <count>\r\n");
    xil_printf("  batchscore <keys_addr> <exact_addr> <count>\r\n");
    xil_printf("  dumpquery <keys_addr> <count>\r\n");
    xil_printf("  plquery <src_ip> <dst_ip> <src_port> <dst_port> <proto>\r\n");
    xil_printf("note: PL side is raw Ethernet parser; ping is not required/implemented.\r\n");
}

static void print_stats(void)
{
    uint32_t hls_ctrl = 0;
    uint16_t eth_speed = 0;

    if (g_pl_ready) {
        hls_ctrl = XFs_hls_test_ReadReg(g_fs.Ctrl_bus_BaseAddress,
                                        XFS_HLS_TEST_CTRL_BUS_ADDR_AP_CTRL);
        eth_speed = XAxiEthernet_GetOperatingSpeed(&g_eth);
    }

    xil_printf("[plstats] queries=%u timeouts=%u reinit=%u ready=%s hls_ctrl=0x%08x eth_speed=%u requested_speed=%u badframe=%s\r\n",
               (unsigned int)g_queries_sent,
               (unsigned int)g_query_timeouts,
               (unsigned int)g_reinit_count,
               g_pl_ready ? "yes" : "no",
               (unsigned int)hls_ctrl,
               (unsigned int)eth_speed,
               (unsigned int)g_requested_speed,
               g_bad_frame_enabled ? "on" : "off");
}

static uint32_t eth_reg(uint32_t offset)
{
    return XAxiEthernet_ReadReg(g_eth.Config.BaseAddress, offset);
}

static void print_eth_stats(void)
{
    xil_printf("[eth] RAF=0x%08x IS=0x%08x RCW1=0x%08x EMMC=0x%08x PHYC=0x%08x\r\n",
               (unsigned int)eth_reg(XAE_RAF_OFFSET),
               (unsigned int)eth_reg(XAE_IS_OFFSET),
               (unsigned int)eth_reg(XAE_RCW1_OFFSET),
               (unsigned int)eth_reg(XAE_EMMC_OFFSET),
               (unsigned int)eth_reg(XAE_PHYC_OFFSET));
    xil_printf("[eth] mode=native_rgmii_io\r\n");
    xil_printf("[eth] rx_bytes=%u rx_frames=%u rx_fcs_err=%u rx_len_err=%u rx_align_err=%u\r\n",
               (unsigned int)eth_reg(XAE_RXBL_OFFSET),
               (unsigned int)eth_reg(XAE_RXFL_OFFSET),
               (unsigned int)eth_reg(XAE_RXFCSERL_OFFSET),
               (unsigned int)eth_reg(XAE_RXLTERL_OFFSET),
               (unsigned int)eth_reg(XAE_RXAERL_OFFSET));
    xil_printf("[eth] rx_64=%u rx_65_127=%u rx_128_255=%u rx_bcst=%u rx_mcst=%u\r\n",
               (unsigned int)eth_reg(XAE_RX64BL_OFFSET),
               (unsigned int)eth_reg(XAE_RX65B127L_OFFSET),
               (unsigned int)eth_reg(XAE_RX128B255L_OFFSET),
               (unsigned int)eth_reg(XAE_RXBCSTFL_OFFSET),
               (unsigned int)eth_reg(XAE_RXMCSTFL_OFFSET));
}

static void handle_command(char *cmd)
{
    char *argv[8];
    int argc = 0;

    for (char *tok = strtok(cmd, " \t\r\n"); tok && argc < 8;
         tok = strtok(NULL, " \t\r\n")) {
        argv[argc++] = tok;
    }
    if (argc == 0) {
        return;
    }

    if (strcmp(argv[0], "help") == 0) {
        print_help();
    } else if (strcmp(argv[0], "plstats") == 0 || strcmp(argv[0], "stats") == 0) {
        print_stats();
    } else if (strcmp(argv[0], "ethstats") == 0 || strcmp(argv[0], "eth") == 0) {
        print_eth_stats();
    } else if (strcmp(argv[0], "phy") == 0) {
        phy_read_print(PHY_REG_BMCR);
        phy_read_print(PHY_REG_BMSR);
        phy_read_print(PHY_REG_ID1);
        phy_read_print(PHY_REG_ID2);
        phy_read_print(PHY_REG_ANAR);
        phy_read_print(PHY_REG_GBCR);
        phy_read_print(PHY_REG_KSZ9031_CTRL);
    } else if (strcmp(argv[0], "physcan") == 0) {
        phy_scan_print();
    } else if (strcmp(argv[0], "rgmiiskew") == 0) {
        if (argc == 1) {
            print_rgmii_skew();
        } else if (argc == 3) {
            uint16_t rx_clk = (uint16_t)strtoul(argv[1], NULL, 0);
            uint16_t rx_data = (uint16_t)strtoul(argv[2], NULL, 0);
            if ((rx_clk > 0x1fU) || (rx_data > 0x0fU)) {
                xil_printf("usage: rgmiiskew [rx_clk_hex<=0x1f rx_data_hex<=0xf]\r\n");
                return;
            }
            apply_rgmii_rx_skew(rx_clk, rx_data);
        } else {
            xil_printf("usage: rgmiiskew [rx_clk_hex rx_data_hex]\r\n");
        }
    } else if (strcmp(argv[0], "mmdread") == 0) {
        if (argc != 3) {
            xil_printf("usage: mmdread <dev> <reg>\r\n");
            return;
        }
        uint16_t dev = (uint16_t)strtoul(argv[1], NULL, 0);
        uint16_t reg = (uint16_t)strtoul(argv[2], NULL, 0);
        uint16_t val = 0;
        int status = phy_mmd_read(dev, reg, &val);
        xil_printf("mmd[%u.%u]=0x%04x status=%d\r\n",
                   (unsigned int)dev,
                   (unsigned int)reg,
                   (unsigned int)val,
                   status);
    } else if (strcmp(argv[0], "mmdwrite") == 0) {
        if (argc != 4) {
            xil_printf("usage: mmdwrite <dev> <reg> <value>\r\n");
            return;
        }
        uint16_t dev = (uint16_t)strtoul(argv[1], NULL, 0);
        uint16_t reg = (uint16_t)strtoul(argv[2], NULL, 0);
        uint16_t val = (uint16_t)strtoul(argv[3], NULL, 0);
        int status = phy_mmd_write(dev, reg, val);
        xil_printf("mmd[%u.%u] <= 0x%04x status=%d\r\n",
                   (unsigned int)dev,
                   (unsigned int)reg,
                   (unsigned int)val,
                   status);
    } else if (strcmp(argv[0], "speed") == 0) {
        if (argc != 2) {
            xil_printf("usage: speed <10|100|1000>\r\n");
            return;
        }
        uint16_t speed = (uint16_t)strtoul(argv[1], NULL, 0);
        if (!((speed == 10U) || (speed == 100U) || (speed == 1000U))) {
            xil_printf("[ERR] speed must be 10, 100, or 1000\r\n");
            return;
        }
        g_requested_speed = speed;
        xil_printf("[OK] requested_speed=%u. Run 'reinit' to apply PHY/MAC speed.\r\n",
                   (unsigned int)g_requested_speed);
    } else if (strcmp(argv[0], "badframe") == 0) {
        if (argc != 2 ||
            !((strcmp(argv[1], "on") == 0) || (strcmp(argv[1], "off") == 0))) {
            xil_printf("usage: badframe <on|off>\r\n");
            return;
        }
        g_bad_frame_enabled = (strcmp(argv[1], "on") == 0);
        if (g_pl_ready) {
            apply_bad_frame_option();
        }
        xil_printf("[OK] badframe=%s RAF=0x%08x\r\n",
                   g_bad_frame_enabled ? "on" : "off",
                   (unsigned int)eth_reg(XAE_RAF_OFFSET));
    } else if (strcmp(argv[0], "resetfs") == 0) {
        g_pl_ready = (init_hls() == XST_SUCCESS);
    } else if (strcmp(argv[0], "reinit") == 0) {
        g_pl_ready = (init_all() == XST_SUCCESS);
        xil_printf(g_pl_ready ? "[OK] PL path is ready\r\n" : "[ERR] PL path bring-up failed\r\n");
    } else if (strcmp(argv[0], "reinitnoclear") == 0) {
        g_pl_ready = (init_all_no_clear() == XST_SUCCESS);
        xil_printf(g_pl_ready ? "[OK] PL path is ready (no dynamic clear)\r\n" : "[ERR] PL path bring-up failed\r\n");
    } else if (strcmp(argv[0], "benchquery") == 0) {
        if (argc != 2) {
            xil_printf("usage: benchquery <count>\r\n");
            return;
        }
        bench_query((uint32_t)strtoul(argv[1], NULL, 0));
    } else if (strcmp(argv[0], "loadbatch") == 0) {
        if (argc != 4) {
            xil_printf("usage: loadbatch <keys_addr> <exact_addr> <count>\r\n");
            return;
        }
        load_batch((UINTPTR)strtoul(argv[1], NULL, 0),
                   (UINTPTR)strtoul(argv[2], NULL, 0),
                   (uint32_t)strtoul(argv[3], NULL, 0));
    } else if (strcmp(argv[0], "loadkeys") == 0) {
        if (argc != 3) {
            xil_printf("usage: loadkeys <keys_addr> <count>\r\n");
            return;
        }
        load_keys((UINTPTR)strtoul(argv[1], NULL, 0),
                  (uint32_t)strtoul(argv[2], NULL, 0));
    } else if (strcmp(argv[0], "batchscore") == 0) {
        if (argc != 4) {
            xil_printf("usage: batchscore <keys_addr> <exact_addr> <count>\r\n");
            return;
        }
        batch_score((UINTPTR)strtoul(argv[1], NULL, 0),
                    (UINTPTR)strtoul(argv[2], NULL, 0),
                    (uint32_t)strtoul(argv[3], NULL, 0));
    } else if (strcmp(argv[0], "dumpquery") == 0) {
        if (argc != 3) {
            xil_printf("usage: dumpquery <keys_addr> <count>\r\n");
            return;
        }
        dump_query((UINTPTR)strtoul(argv[1], NULL, 0),
                   (uint32_t)strtoul(argv[2], NULL, 0));
    } else if (strcmp(argv[0], "plquery") == 0 || strcmp(argv[0], "query") == 0) {
        if (!g_pl_ready) {
            xil_printf("[ERR] PL path is not initialized yet. Run 'reinit' first.\r\n");
            return;
        }
        if (argc != 6) {
            xil_printf("usage: plquery <src_ip> <dst_ip> <src_port> <dst_port> <proto>\r\n");
            return;
        }
        uint32_t src_ip = parse_ipv4(argv[1]);
        uint32_t dst_ip = parse_ipv4(argv[2]);
        uint16_t src_port = (uint16_t)strtoul(argv[3], NULL, 0);
        uint16_t dst_port = (uint16_t)strtoul(argv[4], NULL, 0);
        uint8_t proto = (uint8_t)strtoul(argv[5], NULL, 0);
        uint32_t estimate = 0;
        int status = query_flow(src_ip, dst_ip, src_port, dst_port, proto, &estimate);
        if (status == XST_SUCCESS) {
            xil_printf("plquery %s:%u -> %s:%u proto=%u = %u\r\n",
                       argv[1], (unsigned int)src_port,
                       argv[2], (unsigned int)dst_port,
                       (unsigned int)proto, (unsigned int)estimate);
        } else {
            xil_printf("[ERR] plquery timeout/failure\r\n");
        }
    } else {
        xil_printf("unknown command: %s\r\n", argv[0]);
    }
}

int main(void)
{
    early_uart_puts("\r\n[EARLY] pl_eth_funnelsketch_app entered main\r\n");
    Xil_DCacheDisable();
    xil_printf("\r\nFunnelSketch PL Ethernet control app. [BUILD 2026-09-09-CLI-SYNC-001]\r\n");
    xil_printf("UART: AX7021B PS UART, 115200 baud. PL port: PHY5 / AXI Ethernet.\r\n");
    xil_printf("[SAFE] PL bring-up is manual. Run 'reinit' after confirming UART is alive.\r\n");
    print_help();
    xil_printf("plfs> ");

    char cmd[CMD_BUF_LEN];
    unsigned len = 0;
    while (1) {
        if (!XUartPs_IsReceiveData(STDIN_BASEADDRESS)) {
            continue;
        }

        char ch = (char)XUartPs_RecvByte(STDIN_BASEADDRESS);
        if (ch == '\0') {
            continue;
        }
        if (ch == '\r' || ch == '\n') {
            xil_printf("\r\n");
            cmd[len] = '\0';
            handle_command(cmd);
            len = 0;
            xil_printf("plfs> ");
        } else if ((ch == '\b') || (ch == 0x7f)) {
            if (len > 0) {
                --len;
                xil_printf("\b \b");
            }
        } else if (len + 1 < CMD_BUF_LEN) {
            cmd[len++] = ch;
            xil_printf("%c", ch);
        }
    }
}





