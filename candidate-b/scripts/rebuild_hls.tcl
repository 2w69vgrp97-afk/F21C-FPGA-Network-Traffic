# Run with: vitis-run --mode hls --tcl scripts/rebuild_hls.tcl
# New build directory; source bytes stay unchanged.
set root [file normalize [file join [file dirname [info script]] ..]]
set project [file join $root validation hls_[clock seconds]]
if {[file exists $project]} {error {Use a fresh release copy: hls_rebuild already exists}}
file mkdir $project
set original [file join $root snapshots main_workspace 02_WORK hls_synthesis_iteration_1 project_retry2 inputs]
file copy $original [file join $project inputs]
cd $project
open_project $project
set_top fs_hls_test
set inc "-std=c++11 -I[file join $project inputs candidate_b HLS_Design] -I[file join $project inputs candidate_b FunnelSketch-main]"
add_files [file join $project inputs candidate_b HLS_Design fs_hls_test.cpp] -cflags $inc
add_files -tb [file join $project inputs fs_hls_tb_query_functional.cpp] -cflags "$inc -DCANDIDATE_TASK_MODEL"
open_solution hls -flow_target vivado
set_part xc7z020-clg484-2
create_clock -period 10 -name default
csim_design
puts {F21C_RELEASE_CSIM_PASS}
set stage csim
if {[info exists ::env(F21C_HLS_STAGE)]} {set stage $::env(F21C_HLS_STAGE)}
if {$stage ni {csim synth export}} {error {F21C_HLS_STAGE must be csim, synth, or export}}
if {$stage in {synth export}} {
    csynth_design
    puts {F21C_RELEASE_CSYNTH_PASS}
}
if {$stage eq {export}} {
    export_design -flow none -format ip_catalog -rtl verilog -vendor xilinx.com -library hls -ipname fs_hls_test -version 2.0 -display_name {F21C Candidate-B Balanced Iteration-1} -output [file join $project fs_hls_test_candidate_b_2_0.zip]
    puts {F21C_RELEASE_HLS_EXPORT_PASS}
}
exit 0
