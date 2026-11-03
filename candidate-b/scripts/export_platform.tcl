# Run from the release root in Vivado 2025.2 batch mode.
set root [file normalize [file join [file dirname [info script]] ..]]
set project [file join $root project FINAL_RUNTIME.xpr]
set repo [file join $root project candidate_b_hls_repo fs_hls_test_2_0]
if {![file isfile [file join $repo component.xml]]} {
    error "Export and unpack Candidate-B fs_hls_test 2.0 into $repo first"
}
open_project $project
set_property ip_repo_paths [list $repo] [current_project]
update_ip_catalog
set bd [get_files */FS_Design.bd]
if {[llength $bd] != 1} {error "Expected one FS_Design.bd"}
open_bd_design $bd
validate_bd_design
generate_target all $bd
set build [file join $root build]
file mkdir $build
report_ip_status -file [file join $build ip_status.rpt]
write_hw_platform -fixed -force -file [file join $build FINAL_RUNTIME.xsa]
close_project
