#ifndef FS_HLS_TEST_H
#define FS_HLS_TEST_H

#include "ap_int.h"
#include "hls_stream.h"
#include "ap_axi_sdata.h"

#define PKT_TOTAL_CNT 1024
#define W_HT_32       8192
#define W_D           1004
#define W_U           87040

typedef ap_axis<8, 0, 0, 0>  net_axis_t;
typedef ap_axis<64, 0, 0, 0> pkt_axis_t;
typedef ap_axis<32, 0, 0, 0> result_axis_t;

void fs_hls_test(
    hls::stream<net_axis_t> &pkt_in,
    hls::stream<pkt_axis_t> &query_id_in,
    hls::stream<result_axis_t> &query_size_out,
    ap_uint<1> init_rst
);

#endif