# Reconstruct review project only. It deliberately does not launch impl_1.
set root [file normalize [file join [file dirname [info script]] ..]]
set install $::env(XILINX_VIVADO)
lappend ::auto_path [file join $install data XilinxTclStore support appinit]
package require ::tclapp::support::appinit 1.2
foreach {app version} {xsim 2.520 modelsim 2.375 questa 2.339 ies 4.79 vcs 17.7 xcelium 11.194} {
    lappend ::auto_path [file join $install data XilinxTclStore tclapp xilinx $app]
    package require ::tclapp::xilinx::$app $version
}
foreach app {activehdl riviera} {
    lappend ::auto_path [file join $install data XilinxTclStore tclapp aldec $app]
    package require ::tclapp::aldec::$app 1.42
}
set run [file join $root validation recreation_[clock seconds]]
if {[file exists $run]} {error {Fresh copy required: project_recreation exists}}
file mkdir $run
cd $run
set ::origin_dir_loc $root
set ::user_project_name B
set ::argv {};set ::argc 0
if {[catch {
    source [file join $root validation vivado recreate_review_project.tcl]
    update_compile_order -fileset sources_1
    set f [open [file join $run RESULT.txt] w]
    puts $f {PROJECT_RECREATION=PASS}
    puts $f "TOP=[get_property TOP [get_filesets sources_1]]"
    puts $f "PART=[get_property PART [current_project]]"
    close $f
    close_project
} message options]} {
    set f [open [file join $run RESULT.txt] w];puts $f {PROJECT_RECREATION=PENDING};puts $f $message;puts $f [dict get $options -errorinfo];close $f
    puts stderr $message;exit 1
}
exit 0
