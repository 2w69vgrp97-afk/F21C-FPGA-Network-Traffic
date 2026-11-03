#include "hls_task.h"
#include "fs_hls_test.h"
#include "Funnelsketch.h"
#include "param.h"

// Provisional implementation depth for B1 functional and synthesis evaluation.
// The production depth remains an evidence gap until the wire-level burst
// envelope and post-synthesis service rates are measured.
static const int UPDATE_REQ_FIFO_DEPTH = 16;
static const int CONTROL_FIFO_DEPTH = 2;
static const int PRECOMPUTED_FIFO_DEPTH = 2;

static const ap_uint<2> kCommandUpdate = 0;
static const ap_uint<2> kCommandQuery = 1;
static const ap_uint<2> kCommandReset = 2;

struct SketchCommand {
    ap_uint<2> operation;
    // Mutually exclusive payload: 104-bit tuple for updates, low 64 bits for
    // queries, and zero for reset. This keeps the ordered FIFO at 106 bits.
    ap_uint<104> payload;
};

struct PrecomputedCommand {
    ap_uint<2> operation;
    // UPDATE: folded tuple; QUERY/op3: original key; RESET: zero.
    ap_uint<64> key64;
};

typedef hls::stream<SketchCommand, UPDATE_REQ_FIFO_DEPTH> command_stream_t;
typedef hls::stream<PrecomputedCommand, PRECOMPUTED_FIFO_DEPTH> precomputed_stream_t;
typedef hls::stream<ap_uint<1>, CONTROL_FIFO_DEPTH> control_stream_t;

static const ap_uint<16> kTupleInjectUdpPort = 9001;
static const ap_uint<8> kInjectMagic0 = 0x46; // 'F'
static const ap_uint<8> kInjectMagic1 = 0x53; // 'S'
static const ap_uint<8> kInjectMagic2 = 0x49; // 'I'
static const ap_uint<8> kInjectMagic3 = 0x31; // '1'
static const ap_uint<8> kInjectVersion = 1;
static const ap_uint<8> kInjectBatchType = 5;
static const ap_uint<16> kInjectMaxBatchRecords = 32;

static void clear_hash_vals(uint32_t (&hash_vals)[HASH_FUNC_NUM]) {
    for (int i = 0; i < HASH_FUNC_NUM; ++i) {
        #pragma HLS UNROLL
        hash_vals[i] = 0;
    }
}

static void clear_parser_state(
    bool &l4_capture_active,
    bool &inject_candidate,
    bool &inject_header_active,
    bool &inject_active,
    bool &inject_suppress_normal
) {
    #pragma HLS INLINE
    // Only carried control state must be cleared at a frame boundary.  Data
    // fields are fully overwritten before their validity controls can consume
    // them in the next frame.  Avoiding redundant data clears removes a wide
    // TLAST/reset mux from every parser data register.
    l4_capture_active = false;
    inject_candidate = false;
    inject_header_active = false;
    inject_active = false;
    inject_suppress_normal = false;
}

static ap_uint<64> fold_five_tuple(
    const ap_uint<8> (&src_ip)[4],
    const ap_uint<8> (&dst_ip)[4],
    const ap_uint<8> (&src_port)[2],
    const ap_uint<8> (&dst_port)[2],
    ap_uint<8> protocol
) {
    #pragma HLS INLINE
    ap_uint<8> tuple_bytes[13];
    #pragma HLS ARRAY_PARTITION variable=tuple_bytes complete

    tuple_bytes[0]  = src_ip[0];
    tuple_bytes[1]  = src_ip[1];
    tuple_bytes[2]  = src_ip[2];
    tuple_bytes[3]  = src_ip[3];
    tuple_bytes[4]  = dst_ip[0];
    tuple_bytes[5]  = dst_ip[1];
    tuple_bytes[6]  = dst_ip[2];
    tuple_bytes[7]  = dst_ip[3];
    tuple_bytes[8]  = src_port[0];
    tuple_bytes[9]  = src_port[1];
    tuple_bytes[10] = dst_port[0];
    tuple_bytes[11] = dst_port[1];
    tuple_bytes[12] = protocol;

    ap_uint<64> hash = 0xcbf29ce484222325ULL;
    const ap_uint<64> fnv_prime = 0x100000001b3ULL;

    for (int i = 0; i < 13; ++i) {
        hash ^= static_cast<ap_uint<64> >(tuple_bytes[i]);
        hash *= fnv_prime;
    }
    return hash;
}

static void tuple_to_key_bytes(ap_uint<64> folded_key, ap_uint<8> (&key_bytes)[KEY_BYTE_LEN]) {
    #pragma HLS INLINE
    for (int i = 0; i < KEY_BYTE_LEN; ++i) {
        #pragma HLS UNROLL
        key_bytes[i] = folded_key.range((i + 1) * 8 - 1, i * 8);
    }
}

static SketchCommand make_update_command(
    const ap_uint<8> (&src_ip)[4],
    const ap_uint<8> (&dst_ip)[4],
    const ap_uint<8> (&src_port)[2],
    const ap_uint<8> (&dst_port)[2],
    ap_uint<8> protocol
) {
    #pragma HLS INLINE
    SketchCommand command;
    command.operation = kCommandUpdate;
    command.payload = 0;

    for (int i = 0; i < 4; ++i) {
        #pragma HLS UNROLL
        command.payload.range((i + 1) * 8 - 1, i * 8) = src_ip[i];
        command.payload.range(32 + (i + 1) * 8 - 1, 32 + i * 8) = dst_ip[i];
    }
    for (int i = 0; i < 2; ++i) {
        #pragma HLS UNROLL
        command.payload.range(64 + (i + 1) * 8 - 1, 64 + i * 8) = src_port[i];
        command.payload.range(80 + (i + 1) * 8 - 1, 80 + i * 8) = dst_port[i];
    }
    command.payload.range(103, 96) = protocol;
    return command;
}

static void unpack_update_tuple(
    ap_uint<104> tuple,
    ap_uint<8> (&src_ip)[4],
    ap_uint<8> (&dst_ip)[4],
    ap_uint<8> (&src_port)[2],
    ap_uint<8> (&dst_port)[2],
    ap_uint<8> &protocol
) {
    #pragma HLS INLINE
    for (int i = 0; i < 4; ++i) {
        #pragma HLS UNROLL
        src_ip[i] = tuple.range((i + 1) * 8 - 1, i * 8);
        dst_ip[i] = tuple.range(32 + (i + 1) * 8 - 1, 32 + i * 8);
    }
    for (int i = 0; i < 2; ++i) {
        #pragma HLS UNROLL
        src_port[i] = tuple.range(64 + (i + 1) * 8 - 1, 64 + i * 8);
        dst_port[i] = tuple.range(80 + (i + 1) * 8 - 1, 80 + i * 8);
    }
    protocol = tuple.range(103, 96);
}

static void ingress_parser(
    hls::stream<net_axis_t> &pkt_in,
    hls::stream<pkt_axis_t> &query_id_in,
    control_stream_t &reset_request,
    control_stream_t &reset_release,
    command_stream_t &command_fifo
) {
    #pragma HLS PIPELINE II=1 style=flp

    static ap_uint<16> byte_index = 0;
    static ap_uint<16> ether_type = 0;
    static ap_uint<8> l2_header_bytes = 14;
    // IPv4 L4 starts at most at byte 18 + (15 * 4) = 78.  Keep the
    // carried base in its exact unsigned range; ip_header_valid is the
    // validity guard, so an all-ones sentinel is neither needed nor safe.
    static ap_uint<7> l4_header_base = 0;
    static bool l4_capture_active = false;
    static ap_uint<2> l4_capture_index = 0;
    static bool saw_vlan = false;
    static bool is_ipv4 = false;
    static bool ip_header_valid = false;
    static bool inject_candidate = false;
    static bool inject_header_ok = true;
    static bool inject_header_active = false;
    static ap_uint<3> inject_header_byte = 0;
    static bool inject_active = false;
    static bool inject_suppress_normal = false;
    static ap_uint<16> inject_payload_base = 0;
    static ap_uint<16> inject_count = 0;
    static ap_uint<16> inject_records_remaining = 0;
    static ap_uint<5> inject_record_byte = 0;
    static ap_uint<8> src_ip[4];
    static ap_uint<8> dst_ip[4];
    static ap_uint<8> src_port[2];
    static ap_uint<8> dst_port[2];
    static ap_uint<32> inject_value = 0;
    // Assemble tuples as fields arrive so the enqueue path reads one
    // registered 104-bit value instead of rebuilding it from byte arrays.
    static ap_uint<32> tuple_src_ip = 0;
    static ap_uint<32> tuple_dst_ip = 0;
    static ap_uint<32> tuple_ports = 0;
    static ap_uint<8> tuple_protocol = 0;
    static ap_uint<104> inject_tuple_payload = 0;
    // One-entry output stage.  Event detection updates this register while the
    // command captured by the preceding invocation is the only value that can
    // drive the physical FIFO write.  This keeps one write port and one event
    // per cycle, but breaks the long parser-decode-to-FIFO control cone.
    static SketchCommand pending_command;
    static bool pending_command_valid = false;
    static bool pending_command_is_reset = false;
    static bool reset_waiting_release = false;
    #pragma HLS ARRAY_PARTITION variable=src_ip complete
    #pragma HLS ARRAY_PARTITION variable=dst_ip complete
    #pragma HLS ARRAY_PARTITION variable=src_port complete
    #pragma HLS ARRAY_PARTITION variable=dst_port complete

    // Retire at most one command from the registered output stage.  A newly
    // detected command below replaces it for the next invocation, so sustained
    // query traffic can still enqueue at one command per cycle.
    const bool output_pending = pending_command_valid;
    const bool output_is_reset = pending_command_is_reset;
    if (output_pending) {
        command_fifo.write(pending_command);
    }
    pending_command_valid = false;
    pending_command_is_reset = false;

    // Do not place the reset-command retirement and release wait in the same
    // auto-rewound pipeline transaction.  Once the staged reset command has
    // retired, enter the wait microphase in a later invocation.
    if (output_is_reset) {
        pending_command_valid = false;
        reset_waiting_release = true;
        return;
    }

    if (reset_waiting_release) {
        ap_uint<1> release_token;
        if (reset_release.read_nb(release_token)) {
            reset_waiting_release = false;
        }
        return;
    }

    ap_uint<1> reset_token;
    if (reset_request.read_nb(reset_token)) {
        // The next accepted packet byte performs the full parser-control
        // initialization in its dedicated byte-zero microphase.
        byte_index = 0;
        pending_command.operation = kCommandReset;
        pending_command.payload = 0;
        pending_command_valid = true;
        pending_command_is_reset = true;
        return;
    }

    // Snapshot carried parser state before decoding this byte. Earlier source
    // read variables again after assigning them, which forced HLS to build one
    // long same-cycle next-state/control chain from byte_index through the
    // injection decoder. Protocol fields are separated by at least one input
    // byte, so all decode predicates can use the registered pre-byte state.
    const ap_uint<16> current_byte_index = byte_index;
    const ap_uint<16> current_ether_type = ether_type;
    const ap_uint<8> current_l2_header_bytes = l2_header_bytes;
    const ap_uint<7> current_l4_header_base = l4_header_base;
    const bool current_l4_capture_active = l4_capture_active;
    const ap_uint<2> current_l4_capture_index = l4_capture_index;
    const bool current_saw_vlan = saw_vlan;
    const bool current_is_ipv4 = is_ipv4;
    const bool current_ip_header_valid = ip_header_valid;
    const bool current_inject_candidate = inject_candidate;
    const bool current_inject_header_ok = inject_header_ok;
    const bool current_inject_header_active = inject_header_active;
    const ap_uint<3> current_inject_header_byte = inject_header_byte;
    const bool current_inject_active = inject_active;
    const bool current_inject_suppress_normal = inject_suppress_normal;
    const ap_uint<16> current_inject_payload_base = inject_payload_base;
    const ap_uint<16> current_inject_count = inject_count;
    const ap_uint<16> current_inject_records_remaining =
        inject_records_remaining;
    const ap_uint<5> current_inject_record_byte = inject_record_byte;
    const ap_uint<32> current_inject_value = inject_value;
    const ap_uint<32> current_tuple_src_ip = tuple_src_ip;
    const ap_uint<32> current_tuple_dst_ip = tuple_dst_ip;
    const ap_uint<32> current_tuple_ports = tuple_ports;
    const ap_uint<8> current_tuple_protocol = tuple_protocol;
    const ap_uint<104> current_inject_tuple_payload = inject_tuple_payload;
    const ap_uint<8> current_inject_protocol =
        current_inject_tuple_payload.range(103, 96);
    const ap_uint<8> current_ip_protocol = current_tuple_protocol;
    const bool current_l4_supported =
        (current_ip_protocol == 6) || (current_ip_protocol == 17);
    net_axis_t beat;
    beat.data = 0;
    beat.keep = 0;
    beat.strb = 0;
    beat.last = 0;
    const bool packet_available = pkt_in.read_nb(beat);
    const ap_uint<8> data_byte = beat.data;
    const bool byte_valid =
        packet_available && (beat.keep == 1) && (beat.strb == 1);
    pkt_axis_t query_buf;
    query_buf.data = 0;
    bool query_available = false;

    if (packet_available) {

        if (byte_valid) {
            // Initialize the next frame's grouped tuple at its first byte.
            // Return immediately after consuming byte zero so these clears do
            // not become inputs to the normal decode/update mux tree.
            if (current_byte_index == 0) {
                clear_parser_state(
                    l4_capture_active,
                    inject_candidate, inject_header_active,
                    inject_active, inject_suppress_normal
                );
                tuple_src_ip = 0;
                tuple_dst_ip = 0;
                tuple_ports = 0;
                tuple_protocol = 0;
                byte_index = (beat.last == 1) ? 0 : 1;
                return;
            }

            if (current_byte_index == 12) {
                ether_type.range(15, 8) = data_byte;
            } else if (current_byte_index == 13) {
                const ap_uint<16> ether_type_now =
                    (current_ether_type.range(15, 8), data_byte);
                ether_type = ether_type_now;
                if ((ether_type_now == 0x8100) || (ether_type_now == 0x88A8)) {
                    saw_vlan = true;
                    l2_header_bytes = 18;
                } else {
                    saw_vlan = false;
                    l2_header_bytes = 14;
                    is_ipv4 = (ether_type_now == 0x0800);
                }
            } else if (current_saw_vlan && (current_byte_index == 16)) {
                ether_type.range(15, 8) = data_byte;
            } else if (current_saw_vlan && (current_byte_index == 17)) {
                const ap_uint<16> ether_type_now =
                    (current_ether_type.range(15, 8), data_byte);
                ether_type = ether_type_now;
                is_ipv4 = (ether_type_now == 0x0800);
            }

            // Keep first-byte IPv4 validation/base capture in its own shallow
            // control block.  Folding this state update into the large offset
            // decoder made HLS carry l4_header_base through every switch arm.
            if (current_is_ipv4 &&
                (current_byte_index == current_l2_header_bytes)) {
                const ap_uint<8> version = data_byte.range(7, 4);
                const ap_uint<4> ihl = data_byte.range(3, 0);
                const bool header_ok =
                    (version == 4) && (ihl >= static_cast<ap_uint<4> >(5));
                ip_header_valid = header_ok;
                if (header_ok) {
                    const ap_uint<7> ihl_bytes =
                        static_cast<ap_uint<7> >(ihl) << 2;
                    l4_header_base = static_cast<ap_uint<7> >(
                        static_cast<ap_uint<7> >(current_l2_header_bytes) +
                        ihl_bytes);
                }
            }

            if (current_is_ipv4 &&
                (current_byte_index >= current_l2_header_bytes)) {
                const ap_uint<16> ip_offset =
                    current_byte_index - current_l2_header_bytes;
                if (ip_offset == 9) {
                    tuple_protocol = data_byte;
                } else if (ip_offset == 12) {
                    src_ip[0] = data_byte;
                    tuple_src_ip.range(7, 0) = data_byte;
                } else if (ip_offset == 13) {
                    src_ip[1] = data_byte;
                    tuple_src_ip.range(15, 8) = data_byte;
                } else if (ip_offset == 14) {
                    src_ip[2] = data_byte;
                    tuple_src_ip.range(23, 16) = data_byte;
                } else if (ip_offset == 15) {
                    src_ip[3] = data_byte;
                    tuple_src_ip.range(31, 24) = data_byte;
                } else if (ip_offset == 16) {
                    dst_ip[0] = data_byte;
                    tuple_dst_ip.range(7, 0) = data_byte;
                } else if (ip_offset == 17) {
                    dst_ip[1] = data_byte;
                    tuple_dst_ip.range(15, 8) = data_byte;
                } else if (ip_offset == 18) {
                    dst_ip[2] = data_byte;
                    tuple_dst_ip.range(23, 16) = data_byte;
                } else if (ip_offset == 19) {
                    dst_ip[3] = data_byte;
                    tuple_dst_ip.range(31, 24) = data_byte;
                }
            }

            // Protocol and L4 base are registered before the transport bytes
            // arrive.  Run this four-byte microphase from those registers,
            // independent of the live IP-offset decoder.
            // Capturing four bytes at the registered L4 base is harmless for
            // unsupported/non-IP frames; commit and injection checks below
            // still enforce protocol validity.
            const ap_uint<7> current_byte_offset =
                static_cast<ap_uint<7> >(current_byte_index.range(6, 0));
            const bool at_l4_header_base =
                (current_byte_index < static_cast<ap_uint<16> >(128)) &&
                (current_byte_offset == current_l4_header_base);
            if (current_is_ipv4 && current_ip_header_valid &&
                !current_l4_capture_active &&
                at_l4_header_base) {
                src_port[0] = data_byte;
                tuple_ports.range(7, 0) = data_byte;
                l4_capture_active = true;
                l4_capture_index = 1;
            } else if (current_l4_capture_active) {
                if (current_l4_capture_index == 1) {
                    src_port[1] = data_byte;
                    tuple_ports.range(15, 8) = data_byte;
                    l4_capture_index = 2;
                } else if (current_l4_capture_index == 2) {
                    dst_port[0] = data_byte;
                    tuple_ports.range(23, 16) = data_byte;
                    l4_capture_index = 3;
                } else {
                    dst_port[1] = data_byte;
                    tuple_ports.range(31, 24) = data_byte;
                    l4_capture_active = false;
                    l4_capture_index = 0;
                    const ap_uint<16> dst_port_now =
                        (static_cast<ap_uint<16> >(dst_port[0]) << 8) |
                        static_cast<ap_uint<16> >(data_byte);
                    if ((current_ip_protocol == 17) &&
                        (dst_port_now == kTupleInjectUdpPort)) {
                        inject_candidate = true;
                        inject_suppress_normal = true;
                        inject_payload_base = static_cast<ap_uint<16> >(
                            static_cast<ap_uint<16> >(
                                current_l4_header_base) +
                            static_cast<ap_uint<16> >(8));
                    }
                }
            }

            // Once the injection state machine has been armed by the UDP
            // header, its header/record phases no longer depend on the live
            // L2/IP offset decoder.  Keeping this carried-state update outside
            // that decoder prevents the L2-header select from feeding every
            // injection-state hold/update mux while preserving byte order.
            if (current_inject_candidate) {
                if (!current_inject_header_active &&
                    !current_inject_active &&
                    (current_byte_index == current_inject_payload_base)) {
                            inject_header_ok = (data_byte == kInjectMagic0);
                            inject_header_active = true;
                            inject_header_byte = 1;
                } else if (current_inject_header_active) {
                            if (current_inject_header_byte == 1) {
                                inject_header_ok = current_inject_header_ok &&
                                                   (data_byte == kInjectMagic1);
                                inject_header_byte = 2;
                            } else if (current_inject_header_byte == 2) {
                                inject_header_ok = current_inject_header_ok &&
                                                   (data_byte == kInjectMagic2);
                                inject_header_byte = 3;
                            } else if (current_inject_header_byte == 3) {
                                inject_header_ok = current_inject_header_ok &&
                                                   (data_byte == kInjectMagic3);
                                inject_header_byte = 4;
                            } else if (current_inject_header_byte == 4) {
                                inject_header_ok = current_inject_header_ok &&
                                                   (data_byte == kInjectVersion);
                                inject_header_byte = 5;
                            } else if (current_inject_header_byte == 5) {
                                inject_header_ok = current_inject_header_ok &&
                                                   (data_byte == kInjectBatchType);
                                inject_header_byte = 6;
                            } else if (current_inject_header_byte == 6) {
                                inject_count.range(15, 8) = data_byte;
                                inject_header_byte = 7;
                            } else {
                                const ap_uint<16> inject_count_now =
                                    (current_inject_count.range(15, 8), data_byte);
                                inject_count = inject_count_now;
                                inject_active = current_inject_header_ok &&
                                                (inject_count_now != 0) &&
                                                (inject_count_now <= kInjectMaxBatchRecords);
                                // The baseline deliberately consumes one record
                                // for counts 1/2 and count-1 records otherwise.
                                // Store that legacy termination rule directly so
                                // record completion needs no 17-bit index add.
                                inject_records_remaining =
                                    (inject_count_now <= 1) ?
                                        static_cast<ap_uint<16> >(1) :
                                        static_cast<ap_uint<16> >(
                                            inject_count_now - 1);
                                inject_record_byte = 0;
                                inject_header_active = false;
                                inject_header_byte = 0;
                            }
                } else if (current_inject_active) {
                            bool advance_record_byte = true;
                            if (current_inject_record_byte == 0) {
                                inject_tuple_payload.range(103, 96) = data_byte;
                            } else if ((current_inject_record_byte >= 4) &&
                                       (current_inject_record_byte < 16)) {
                                // Bytes 4..15 arrive in increasing payload-bit
                                // order.  A fixed shift replaces twelve sparse
                                // writes and fully overwrites the tuple each record.
                                inject_tuple_payload.range(95, 0) =
                                    (data_byte,
                                     current_inject_tuple_payload.range(95, 8));
                            } else if (current_inject_record_byte >= 16) {
                                const ap_uint<32> inject_value_now =
                                    (current_inject_value.range(23, 0), data_byte);
                                inject_value = inject_value_now;
                                if (current_inject_record_byte == 19) {
                                    inject_record_byte = 0;
                                    advance_record_byte = false;
                                    if (current_inject_records_remaining <= 1) {
                                        inject_active = false;
                                    } else {
                                        inject_records_remaining =
                                            current_inject_records_remaining - 1;
                                    }
                                }
                            }

                            if (advance_record_byte) {
                                inject_record_byte =
                                    current_inject_record_byte + 1;
                            }
                        }
            }

            byte_index = (beat.last == 1) ?
                static_cast<ap_uint<16> >(0) :
                static_cast<ap_uint<16> >(current_byte_index + 1);
        } else if (beat.last) {
            byte_index = 0;
        }
    } else {
        query_available = query_id_in.read_nb(query_buf);
    }

    // The three command sources are mutually exclusive in one parser
    // invocation.  Form their predicates directly, then use one physical FIFO
    // write.  This avoids both the prior deep valid-flag phi chain and the
    // multiple-write resource conflict that forced parser II=2.
    const ap_uint<32> completed_inject_value =
        (current_inject_value.range(23, 0), data_byte);
    const bool emit_inject =
        byte_valid && current_inject_candidate && current_inject_active &&
        (current_inject_record_byte == 19) &&
        (completed_inject_value != 0) &&
        ((current_inject_protocol == 6) ||
         (current_inject_protocol == 17));
    const bool emit_normal =
        packet_available && (beat.last == 1) && byte_valid &&
        current_is_ipv4 && current_ip_header_valid && current_l4_supported &&
        !current_inject_suppress_normal;
    const bool emit_command = emit_inject || emit_normal || query_available;

    pending_command_valid = emit_command;
    pending_command_is_reset = false;
    if (emit_command) {
        pending_command.operation =
            query_available ? kCommandQuery : kCommandUpdate;
        pending_command.payload = 0;
        if (emit_inject) {
            pending_command.payload = current_inject_tuple_payload;
        } else if (emit_normal) {
            pending_command.payload.range(31, 0) = current_tuple_src_ip;
            pending_command.payload.range(63, 32) = current_tuple_dst_ip;
            pending_command.payload.range(95, 64) = current_tuple_ports;
            pending_command.payload.range(103, 96) = current_tuple_protocol;
        } else {
            pending_command.payload.range(63, 0) = query_buf.data;
        }
    }
}

static void fnv_precompute(
    command_stream_t &command_fifo,
    precomputed_stream_t &precomputed_fifo
) {
    #pragma HLS INLINE off

    SketchCommand command = command_fifo.read();
    PrecomputedCommand precomputed;
    precomputed.operation = command.operation;
    precomputed.key64 = 0;

    if (command.operation == kCommandUpdate) {
        ap_uint<8> src_ip[4];
        ap_uint<8> dst_ip[4];
        ap_uint<8> src_port[2];
        ap_uint<8> dst_port[2];
        ap_uint<8> protocol;
        #pragma HLS ARRAY_PARTITION variable=src_ip complete
        #pragma HLS ARRAY_PARTITION variable=dst_ip complete
        #pragma HLS ARRAY_PARTITION variable=src_port complete
        #pragma HLS ARRAY_PARTITION variable=dst_port complete

        unpack_update_tuple(
            command.payload, src_ip, dst_ip, src_port, dst_port, protocol);
        precomputed.key64 =
            fold_five_tuple(src_ip, dst_ip, src_port, dst_port, protocol);
    } else if (command.operation != kCommandReset) {
        // Preserve the existing query fallthrough, including operation 3.
        precomputed.key64 = command.payload.range(63, 0);
    }

    // Every command uses the same ordered path, including query and reset.
    precomputed_fifo.write(precomputed);
}

static void update_consumer(
    precomputed_stream_t &precomputed_fifo,
    hls::stream<result_axis_t> &query_size_out,
    control_stream_t &reset_release,
    control_stream_t &reset_done
) {
    #pragma HLS INLINE off

    static FunnelSketch fs;
    static uint32_t hash_vals[HASH_FUNC_NUM];
    static ap_uint<8> key_pkt[KEY_BYTE_LEN];
    static ap_uint<8> key_query[KEY_BYTE_LEN];
    #pragma HLS ARRAY_PARTITION variable=hash_vals complete
    #pragma HLS ARRAY_PARTITION variable=key_pkt complete
    #pragma HLS ARRAY_PARTITION variable=key_query complete

    PrecomputedCommand command = precomputed_fifo.read();

    if (command.operation == kCommandReset) {
        fs.init(MAX_W_HT_32, MAX_W_D, MAX_W_U);
        clear_hash_vals(hash_vals);
        for (int i = 0; i < KEY_BYTE_LEN; ++i) {
            #pragma HLS UNROLL
            key_pkt[i] = 0;
            key_query[i] = 0;
        }

        // Release ingress only after the sketch memories have been cleared;
        // acknowledge the control transaction at the same semantic boundary.
        reset_release.write(1);
        reset_done.write(1);
        return;
    }

    if (command.operation == kCommandUpdate) {
        tuple_to_key_bytes(command.key64, key_pkt);
        fs.insert(key_pkt, KEY_BYTE_LEN, 1, hash_vals);
        return;
    }

    for (int i = 0; i < KEY_BYTE_LEN; ++i) {
        #pragma HLS UNROLL
        key_query[i] = command.key64.range((i + 1) * 8 - 1, i * 8);
    }
    uint32_t flow_size = fs.query(key_query, KEY_BYTE_LEN, hash_vals);

    result_axis_t out_data;
    out_data.data = flow_size;
    out_data.keep = 0xf;
    out_data.strb = 0xf;
    out_data.last = 1;
    query_size_out.write(out_data);
}

static void issue_reset(ap_uint<1> init_rst, control_stream_t &reset_request) {
    #pragma HLS INLINE off
    if (init_rst == 1) {
        reset_request.write(1);
    }
}

static void wait_for_reset(ap_uint<1> init_rst, control_stream_t &reset_done) {
    #pragma HLS INLINE off
    if (init_rst == 1) {
        reset_done.read();
    }
}

void fs_hls_test(
    hls::stream<net_axis_t> &pkt_in,
    hls::stream<pkt_axis_t> &query_id_in,
    hls::stream<result_axis_t> &query_size_out,
    ap_uint<1> init_rst
) {
    #pragma HLS INTERFACE s_axilite port=return bundle=CTRL_BUS offset=0x04
    #pragma HLS INTERFACE mode=axis port=pkt_in name=pkt_in
    #pragma HLS INTERFACE mode=axis port=query_id_in name=query_id_in
    #pragma HLS INTERFACE mode=axis port=query_size_out name=query_size_out
    #pragma HLS INTERFACE s_axilite port=init_rst bundle=CTRL_BUS offset=0x00
    #pragma HLS DATAFLOW

    hls_thread_local control_stream_t reset_request("reset_request");
    hls_thread_local control_stream_t reset_release("reset_release");
    hls_thread_local control_stream_t reset_done("reset_done");
    hls_thread_local command_stream_t command_fifo("command_fifo");
    hls_thread_local precomputed_stream_t precomputed_fifo("precomputed_fifo");
    #pragma HLS AGGREGATE variable=precomputed_fifo compact=bit

    issue_reset(init_rst, reset_request);

    hls_thread_local hls::task parser_task(
        ingress_parser,
        pkt_in,
        query_id_in,
        reset_request,
        reset_release,
        command_fifo);
    hls_thread_local hls::task fnv_task(
        fnv_precompute,
        command_fifo,
        precomputed_fifo);
    hls_thread_local hls::task consumer_task(
        update_consumer,
        precomputed_fifo,
        query_size_out,
        reset_release,
        reset_done);

    wait_for_reset(init_rst, reset_done);
}
