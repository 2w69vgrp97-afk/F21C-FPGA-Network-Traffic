# Run from repository root:
#   vitis_hls -f verification/hls/run_fastmod_csim.tcl
# This uses AMD ap_int semantics and the original FastMod.h.
# A successful C simulation is not an FPGA synthesis, timing, or board PASS.
open_project -reset fastmod_csim_work
set_top fastmod_check
add_files verification/hls/fastmod_check.cpp
add_files -tb verification/hls/fastmod_check_tb.cpp
open_solution -reset solution1 -flow_target vivado
set_part {xc7z020clg484-2}
create_clock -period 10 -name default
if {[catch {csim_design} err]} {
    puts stderr "FAIL: FastMod C simulation: $err"
    exit 1
}
puts "PASS: Vitis HLS C simulation returned successfully"
exit 0
