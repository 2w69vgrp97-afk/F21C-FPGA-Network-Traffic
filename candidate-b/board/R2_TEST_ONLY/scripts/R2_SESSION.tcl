# Vitis 2025.2 bundled XSCT, Windows. Sourcing this file performs NO board action.
namespace eval ::r2 {
    variable root [file normalize [file join [file dirname [info script]] ..]]
    variable state NEW
    variable log ""
    variable session ""
    variable cable ""
    variable device ""
    variable cpu ""
    variable verify_count 0
}
proc ::r2::record {message} {
    variable log
    set text "[clock format [clock seconds] -format {%Y-%m-%dT%H:%M:%S%z}] $message"
    puts $text
    if {$log ne ""} { puts $log $text; flush $log }
}
proc ::r2::fail {message} {
    variable state; set state FAILED
    record "STOP $message"
    error $message
}
proc ::r2::need {expected} {
    variable state
    if {$state ne $expected} { fail "Expected state $expected, actual $state. Start a fresh session after any failure." }
}
proc ::r2::call {args} {
    record "COMMAND $args"
    if {[catch {uplevel #0 $args} result options]} { fail "$args : $result" }
    record "RESULT $result"
    return $result
}
proc ::r2::verify {} {
    variable root; variable session; variable verify_count
    incr verify_count
    set receipt [file join $session [format {package_verify_%03d.json} $verify_count]]
    if {[catch {exec powershell.exe -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File [file join $root scripts VERIFY_PACKAGE.ps1] -PackageRoot $root -ReceiptPath $receipt} output]} { fail "Package verification failed: $output" }
    record $output
}
proc ::r2::select {id name_pattern expected_device} {
    variable cable
    if {![string is integer -strict $id]} { fail {Use a current numeric target id from r2_targets, never a copied historical id.} }
    set matches {}
    foreach target [call targets -target-properties] {
        if {[dict exists $target target_id] && [dict get $target target_id] == $id} { lappend matches $target }
    }
    if {[llength $matches] != 1} { fail "Target id $id is missing or ambiguous" }
    set p [lindex $matches 0]
    foreach key {name jtag_cable_serial jtag_device_ctx} {
        if {![dict exists $p $key] || [dict get $p $key] eq ""} { fail "Target lacks identity property $key; do not guess" }
    }
    if {![regexp -nocase $name_pattern [dict get $p name]]} { fail "Wrong target type: $p" }
    if {[dict get $p jtag_cable_serial] ne $cable} { fail {JTAG cable serial differs from operator-confirmed session identity} }
    if {$expected_device ne "" && [dict get $p jtag_device_ctx] ne $expected_device} { fail {CPU is on a different JTAG device than the programmed FPGA} }
    call targets -set $id
    record "SELECTED_TARGET_PROPERTIES $p"
    return $p
}
proc r2_connect {{url tcp:127.0.0.1:3121}} {
    ::r2::need NEW
    ::r2::call connect -url $url
    set ::r2::state CONNECTED
    r2_targets
}
proc r2_targets {} {
    if {$::r2::state eq "NEW" || $::r2::state eq "FAILED"} { ::r2::fail {Connect explicitly before listing targets} }
    return [::r2::call targets -target-properties]
}
proc r2_begin {session_dir operator board_serial cable_serial approval_reference risk_review_reference} {
    ::r2::need CONNECTED
    foreach value [list $operator $board_serial $cable_serial $approval_reference $risk_review_reference] {
        if {[string trim $value] eq ""} { ::r2::fail {Operator, board/cable identity, approval and risk-review references are required} }
    }
    set d [file normalize $session_dir]
    if {[string equal -nocase $d $::r2::root] || [string match -nocase "$::r2::root/*" $d]} { ::r2::fail {Session evidence must be outside the immutable package} }
    if {[file exists $d]} { ::r2::fail {Refusing to reuse or overwrite any existing session directory} }
    file mkdir $d
    set ::r2::session $d
    set ::r2::cable $cable_serial
    set ::r2::log [open [file join $d jtag_actions.log] {WRONLY CREAT EXCL}]
    fconfigure $::r2::log -encoding utf-8 -buffering line
    ::r2::record "TEST_ONLY_OPERATOR_AUTHORIZATION operator=$operator board=$board_serial cable=$cable_serial approval=$approval_reference risk_review=$risk_review_reference"
    ::r2::record "PACKAGE_ROOT $::r2::root"
    ::r2::verify
    ::r2::call targets -target-properties
    set ::r2::state AUTHORIZED
    ::r2::record {READY_FOR_MANUAL_PROGRAM_COMMAND; no FPGA programming yet}
}
proc r2_program {fpga_target_id} {
    ::r2::need AUTHORIZED
    ::r2::verify
    set p [::r2::select $fpga_target_id {^xc7z020([_ ].*)?$} ""]
    set ::r2::device [dict get $p jtag_device_ctx]
    set bit [file join $::r2::root artifacts F21C_CANDIDATE_B_R2_TEST_ONLY.bit]
    ::r2::record "ACTUAL_BIT_PATH $bit SHA256=27ECB5D4CE74494B2220FEFD1FDEA6800F002B711C2834ACF4C2C991441CFFE8"
    ::r2::call fpga -file $bit
    set status [::r2::call fpga -state]
    if {$status ne {FPGA is configured}} { ::r2::fail "FPGA configuration status did not confirm startup: $status" }
    set ::r2::state PL_PROGRAMMED
}
proc r2_load {cpu_target_id} {
    ::r2::need PL_PROGRAMMED
    ::r2::verify
    ::r2::select $cpu_target_id {cortex-a9.*#0$} $::r2::device
    set ::r2::cpu $cpu_target_id
    set xsa [file join $::r2::root artifacts F21C_CANDIDATE_B_R2.xsa]
    set init [file join $::r2::root init ps7_init.tcl]
    set elf [file join $::r2::root artifacts F21C_CANDIDATE_B_R2.elf]
    ::r2::record "ACTUAL_XSA_PATH $xsa SHA256=8383B46D9B801CFBF39DD6AB74B5DAC483E58E8148EAF7F016E594EDBE747B4D"
    ::r2::call loadhw -hw $xsa
    ::r2::call rst -processor
    ::r2::record "ACTUAL_PS_INIT_PATH $init SHA256=F2BEA71C8E6C6D99D60C747101791B93FBD80B4E68F63DE8CB36090D9F780C98"
    ::r2::call source $init
    foreach procname {ps7_init ps7_post_config} {
        if {[llength [info procs ::$procname]] != 1} { ::r2::fail "Missing PS initialization procedure $procname" }
    }
    ::r2::call ps7_init
    ::r2::call ps7_post_config
    ::r2::record "ACTUAL_ELF_PATH $elf SHA256=DF32E985C053DB5FDB7893E06F72479D63BEBEC15C5C6CD55A35BC40A32C278C"
    ::r2::call dow $elf
    set ::r2::state ELF_LOADED
    ::r2::record {CPU remains stopped. Open UART capture at 115200 8N1, no flow control, then explicitly call r2_run.}
}
proc r2_run {} {
    ::r2::need ELF_LOADED
    ::r2::verify
    ::r2::select $::r2::cpu {cortex-a9.*#0$} $::r2::device
    ::r2::call con
    set ::r2::state RUNNING
    ::r2::record {ELF_RESUMED; board function/performance verdict remains PENDING until operator records real results.}
}
proc r2_status {} { return $::r2::state }
puts "R2 TEST_ONLY package: $::r2::root"
puts {Sourced definitions only: no connect, program, PS init, ELF download or run has occurred. Follow BOARD_TEST_GUIDE.md.}
