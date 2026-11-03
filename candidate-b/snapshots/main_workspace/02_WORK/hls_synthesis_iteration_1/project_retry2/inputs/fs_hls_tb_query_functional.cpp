#include "hls_task.h"
#include "ap_axi_sdata.h"
#include "ap_int.h"
#include "fs_hls_test.h"
#include "hls_stream.h"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct Flow {
    std::uint32_t src_ip;
    std::uint32_t dst_ip;
    std::uint16_t src_port;
    std::uint16_t dst_port;
};

net_axis_t NetByte(std::uint8_t value, bool last) {
    net_axis_t beat;
    beat.data = value;
    beat.keep = 1;
    beat.strb = 1;
    beat.last = last ? 1 : 0;
    return beat;
}

pkt_axis_t QueryBeat(ap_uint<64> key) {
    pkt_axis_t beat;
    beat.data = key;
    beat.keep = 0xff;
    beat.strb = 0xff;
    beat.last = 1;
    return beat;
}

ap_uint<64> FoldKey(const Flow& flow) {
    std::uint8_t bytes[13] = {
        static_cast<std::uint8_t>(flow.src_ip >> 24),
        static_cast<std::uint8_t>(flow.src_ip >> 16),
        static_cast<std::uint8_t>(flow.src_ip >> 8),
        static_cast<std::uint8_t>(flow.src_ip),
        static_cast<std::uint8_t>(flow.dst_ip >> 24),
        static_cast<std::uint8_t>(flow.dst_ip >> 16),
        static_cast<std::uint8_t>(flow.dst_ip >> 8),
        static_cast<std::uint8_t>(flow.dst_ip),
        static_cast<std::uint8_t>(flow.src_port >> 8),
        static_cast<std::uint8_t>(flow.src_port),
        static_cast<std::uint8_t>(flow.dst_port >> 8),
        static_cast<std::uint8_t>(flow.dst_port),
        17
    };
    ap_uint<64> hash = 0xcbf29ce484222325ULL;
    for (const std::uint8_t value : bytes) {
        hash ^= static_cast<ap_uint<64> >(value);
        hash *= static_cast<ap_uint<64> >(0x100000001b3ULL);
    }
    return hash;
}

std::vector<std::uint8_t> BuildFrame(const Flow& flow) {
    std::vector<std::uint8_t> frame(74, 0);
    const std::uint8_t dst_mac[6] = {0x00, 0x0a, 0x35, 0x05, 0x01, 0x02};
    const std::uint8_t src_mac[6] = {0x98, 0xfa, 0x9b, 0xaf, 0x6a, 0x58};
    for (int i = 0; i < 6; ++i) {
        frame[i] = dst_mac[i];
        frame[6 + i] = src_mac[i];
    }
    frame[12] = 0x08;
    frame[13] = 0x00;
    frame[14] = 0x45;
    frame[16] = 0x00;
    frame[17] = 0x3c;
    frame[18] = 0xd0;
    frame[19] = 0x50;
    frame[22] = 0x40;
    frame[23] = 0x11;
    frame[26] = static_cast<std::uint8_t>(flow.src_ip >> 24);
    frame[27] = static_cast<std::uint8_t>(flow.src_ip >> 16);
    frame[28] = static_cast<std::uint8_t>(flow.src_ip >> 8);
    frame[29] = static_cast<std::uint8_t>(flow.src_ip);
    frame[30] = static_cast<std::uint8_t>(flow.dst_ip >> 24);
    frame[31] = static_cast<std::uint8_t>(flow.dst_ip >> 16);
    frame[32] = static_cast<std::uint8_t>(flow.dst_ip >> 8);
    frame[33] = static_cast<std::uint8_t>(flow.dst_ip);
    frame[34] = static_cast<std::uint8_t>(flow.src_port >> 8);
    frame[35] = static_cast<std::uint8_t>(flow.src_port);
    frame[36] = static_cast<std::uint8_t>(flow.dst_port >> 8);
    frame[37] = static_cast<std::uint8_t>(flow.dst_port);
    frame[38] = 0x00;
    frame[39] = 0x28;
    for (std::size_t i = 0; i < 32; ++i) frame[42 + i] = static_cast<std::uint8_t>(i);
    return frame;
}

class Harness {
public:
    void Initialize() {
        fs_hls_test(pkt_in_, query_in_, query_out_, 1);
        fs_hls_test(pkt_in_, query_in_, query_out_, 0);
    }

    void Update(const Flow& flow, int count) {
        const std::vector<std::uint8_t> frame = BuildFrame(flow);
        for (int packet = 0; packet < count; ++packet) {
            for (std::size_t i = 0; i < frame.size(); ++i) {
                pkt_in_.write(NetByte(frame[i], i + 1 == frame.size()));
            }
        }
    }

    std::uint32_t Query(const Flow& flow) {
        query_in_.write(QueryBeat(FoldKey(flow)));
        return static_cast<std::uint32_t>(query_out_.read().data);
    }

private:
    hls::stream<net_axis_t> pkt_in_;
    hls::stream<pkt_axis_t> query_in_;
    hls::stream<result_axis_t> query_out_;
};

bool Check(Harness& dut, const char* name, const Flow& flow,
           int update_count, std::uint32_t expected) {
    dut.Update(flow, update_count);
    const std::uint32_t actual = dut.Query(flow);
    const std::uint64_t key = static_cast<std::uint64_t>(FoldKey(flow));
    std::cout << "QUERY_CASE name=" << name
              << " updates=" << update_count
              << " key=0x" << std::hex << std::uppercase << std::setw(16)
              << std::setfill('0') << key << std::dec
              << " expected=" << expected << " actual=" << actual
              << " status=" << (actual == expected ? "PASS" : "FAIL") << '\n';
    return actual == expected;
}

}  // namespace

int main() {
    Harness dut;
    dut.Initialize();

    const Flow one{0x0a000001, 0x0a000002, 1234, 4321};
    const Flow ten{0x0a010001, 0x0a010002, 2234, 5321};
    const Flow c3{0xc6125001, 0xc6135001, 46001, 56001};

    bool pass = true;
    const std::uint32_t baseline = dut.Query(c3);
    std::cout << "QUERY_CASE name=c3_baseline updates=0 key=0x"
              << std::hex << std::uppercase << std::setw(16) << std::setfill('0')
              << static_cast<std::uint64_t>(FoldKey(c3)) << std::dec
              << " expected=0 actual=" << baseline
              << " status=" << (baseline == 0 ? "PASS" : "FAIL") << '\n';
    pass &= baseline == 0;
    pass &= Check(dut, "one_update", one, 1, 1);
    pass &= Check(dut, "ten_updates", ten, 10, 10);
    pass &= Check(dut, "c3_eighty_eight_updates", c3, 88, 88);

    std::cout << "QUERY_FUNCTIONAL_CSIM = " << (pass ? "PASS" : "FAIL") << '\n';
    return pass ? 0 : 1;
}
