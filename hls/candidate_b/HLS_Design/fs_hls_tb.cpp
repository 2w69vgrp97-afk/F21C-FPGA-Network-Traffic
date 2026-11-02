#include "ap_int.h"
#include "hls_stream.h"
#include "ap_axi_sdata.h"
#include "fs_hls_test.h"
#include "param.h"

#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

static pkt_axis_t pack_query_axis(ap_uint<64> key) {
    pkt_axis_t pkt;
    pkt.data = key;
    pkt.keep = 0xff;
    pkt.strb = 0xff;
    pkt.last = 1;
    return pkt;
}

static net_axis_t pack_net_axis(ap_uint<8> data, bool last) {
    net_axis_t pkt;
    pkt.data = data;
    pkt.keep = 1;
    pkt.strb = 1;
    pkt.last = last ? 1 : 0;
    return pkt;
}

static ap_uint<64> fold_five_tuple(
    uint32_t src_ip,
    uint32_t dst_ip,
    uint16_t src_port,
    uint16_t dst_port,
    uint8_t protocol
) {
    ap_uint<8> tuple_bytes[13];
    tuple_bytes[0]  = (src_ip >> 24) & 0xff;
    tuple_bytes[1]  = (src_ip >> 16) & 0xff;
    tuple_bytes[2]  = (src_ip >> 8) & 0xff;
    tuple_bytes[3]  = src_ip & 0xff;
    tuple_bytes[4]  = (dst_ip >> 24) & 0xff;
    tuple_bytes[5]  = (dst_ip >> 16) & 0xff;
    tuple_bytes[6]  = (dst_ip >> 8) & 0xff;
    tuple_bytes[7]  = dst_ip & 0xff;
    tuple_bytes[8]  = (src_port >> 8) & 0xff;
    tuple_bytes[9]  = src_port & 0xff;
    tuple_bytes[10] = (dst_port >> 8) & 0xff;
    tuple_bytes[11] = dst_port & 0xff;
    tuple_bytes[12] = protocol;

    ap_uint<64> hash = 0xcbf29ce484222325ULL;
    const ap_uint<64> fnv_prime = 0x100000001b3ULL;
    for (int i = 0; i < 13; ++i) {
        hash ^= static_cast<ap_uint<64> >(tuple_bytes[i]);
        hash *= fnv_prime;
    }
    return hash;
}

static std::vector<uint8_t> build_ipv4_udp_frame(
    uint32_t src_ip,
    uint32_t dst_ip,
    uint16_t src_port,
    uint16_t dst_port,
    const std::vector<uint8_t> &payload
) {
    const uint16_t ip_total_len = static_cast<uint16_t>(20 + 8 + payload.size());
    std::vector<uint8_t> frame;
    frame.reserve(14 + ip_total_len);

    const uint8_t dst_mac[6] = {0x00, 0x0a, 0x35, 0x01, 0x02, 0x03};
    const uint8_t src_mac[6] = {0x00, 0x0a, 0x35, 0x04, 0x05, 0x06};
    for (int i = 0; i < 6; ++i) frame.push_back(dst_mac[i]);
    for (int i = 0; i < 6; ++i) frame.push_back(src_mac[i]);
    frame.push_back(0x08);
    frame.push_back(0x00);

    frame.push_back(0x45);
    frame.push_back(0x00);
    frame.push_back((ip_total_len >> 8) & 0xff);
    frame.push_back(ip_total_len & 0xff);
    frame.push_back(0x00);
    frame.push_back(0x01);
    frame.push_back(0x00);
    frame.push_back(0x00);
    frame.push_back(0x40);
    frame.push_back(0x11);
    frame.push_back(0x00);
    frame.push_back(0x00);
    frame.push_back((src_ip >> 24) & 0xff);
    frame.push_back((src_ip >> 16) & 0xff);
    frame.push_back((src_ip >> 8) & 0xff);
    frame.push_back(src_ip & 0xff);
    frame.push_back((dst_ip >> 24) & 0xff);
    frame.push_back((dst_ip >> 16) & 0xff);
    frame.push_back((dst_ip >> 8) & 0xff);
    frame.push_back(dst_ip & 0xff);

    const uint16_t udp_len = static_cast<uint16_t>(8 + payload.size());
    frame.push_back((src_port >> 8) & 0xff);
    frame.push_back(src_port & 0xff);
    frame.push_back((dst_port >> 8) & 0xff);
    frame.push_back(dst_port & 0xff);
    frame.push_back((udp_len >> 8) & 0xff);
    frame.push_back(udp_len & 0xff);
    frame.push_back(0x00);
    frame.push_back(0x00);

    for (size_t i = 0; i < payload.size(); ++i) {
        frame.push_back(payload[i]);
    }

    return frame;
}

static void feed_frame(
    const std::vector<uint8_t> &frame,
    hls::stream<net_axis_t> &pkt_in,
    hls::stream<pkt_axis_t> &query_id_in,
    hls::stream<result_axis_t> &query_size_out,
    ap_uint<1> init_rst
) {
    for (size_t i = 0; i < frame.size(); ++i) {
        pkt_in.write(pack_net_axis(frame[i], i + 1 == frame.size()));
        fs_hls_test(pkt_in, query_id_in, query_size_out, init_rst);
    }

    // Allow the packet-end fold and insert stages to retire.
    for (int i = 0; i < 3; ++i) {
        fs_hls_test(pkt_in, query_id_in, query_size_out, init_rst);
    }
}

static uint32_t query_flow(
    ap_uint<64> key,
    hls::stream<net_axis_t> &pkt_in,
    hls::stream<pkt_axis_t> &query_id_in,
    hls::stream<result_axis_t> &query_size_out,
    ap_uint<1> init_rst
) {
    query_id_in.write(pack_query_axis(key));
    fs_hls_test(pkt_in, query_id_in, query_size_out, init_rst);

    for (int i = 0; i < 4 && query_size_out.empty(); ++i) {
        fs_hls_test(pkt_in, query_id_in, query_size_out, init_rst);
    }

    if (query_size_out.empty()) {
        return 0;
    }
    result_axis_t result_pkt = query_size_out.read();
    return static_cast<uint32_t>(result_pkt.data);
}

int main() {
    hls::stream<net_axis_t> pkt_in;
    hls::stream<pkt_axis_t> query_id_in;
    hls::stream<result_axis_t> query_size_out;
    ap_uint<1> init_rst = 0;

    fs_hls_test(pkt_in, query_id_in, query_size_out, 1);

    const uint32_t flow1_src_ip = 0x0a000001;
    const uint32_t flow1_dst_ip = 0x0a000002;
    const uint16_t flow1_src_port = 1234;
    const uint16_t flow1_dst_port = 4321;
    const uint8_t  flow1_proto = 17;

    const uint32_t flow2_src_ip = 0xc0a8010a;
    const uint32_t flow2_dst_ip = 0xc0a80114;
    const uint16_t flow2_src_port = 3456;
    const uint16_t flow2_dst_port = 9000;
    const uint8_t  flow2_proto = 17;

    std::vector<uint8_t> payload1(12, 0x5a);
    std::vector<uint8_t> payload2(20, 0xa5);

    std::vector<uint8_t> frame1 = build_ipv4_udp_frame(
        flow1_src_ip, flow1_dst_ip, flow1_src_port, flow1_dst_port, payload1);
    std::vector<uint8_t> frame2 = build_ipv4_udp_frame(
        flow2_src_ip, flow2_dst_ip, flow2_src_port, flow2_dst_port, payload2);

    for (int i = 0; i < 5; ++i) {
        feed_frame(frame1, pkt_in, query_id_in, query_size_out, init_rst);
    }
    for (int i = 0; i < 2; ++i) {
        feed_frame(frame2, pkt_in, query_id_in, query_size_out, init_rst);
    }

    ap_uint<64> flow1_key = fold_five_tuple(
        flow1_src_ip, flow1_dst_ip, flow1_src_port, flow1_dst_port, flow1_proto);
    ap_uint<64> flow2_key = fold_five_tuple(
        flow2_src_ip, flow2_dst_ip, flow2_src_port, flow2_dst_port, flow2_proto);

    uint32_t flow1_count = query_flow(flow1_key, pkt_in, query_id_in, query_size_out, init_rst);
    uint32_t flow2_count = query_flow(flow2_key, pkt_in, query_id_in, query_size_out, init_rst);

    std::cout << "flow1_count=" << flow1_count << std::endl;
    std::cout << "flow2_count=" << flow2_count << std::endl;

    bool pass = (flow1_count == 5) && (flow2_count == 2);
    std::cout << (pass ? "PASS" : "FAIL") << std::endl;
    return pass ? 0 : 1;
}
