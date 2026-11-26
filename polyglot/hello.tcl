# ============================================================================
# GovnoOS Tcl 8.6 Procedure Meta-Interpreter
# Computes affine glyph morphisms using list procs, expr arithmetic, and format.
# ============================================================================

proc eval_glyph_tcl {a b bias} {
    return [expr {($a * $b) + $bias}]
}

set glyph_specs {
    {8 9 0}
    {10 10 1}
    {9 12 0}
    {12 9 0}
    {11 10 1}
    {4 8 0}
    {11 11 -2}
    {10 11 1}
    {14 8 2}
    {6 18 0}
    {10 10 0}
}

set tcl_checksum 0
set tcl_result ""

foreach spec $glyph_specs {
    set a [lindex $spec 0]
    set b [lindex $spec 1]
    set c [lindex $spec 2]
    set code [eval_glyph_tcl $a $b $c]
    set tcl_checksum [expr {$tcl_checksum + $code}]
    append tcl_result [format %c $code]
}

if {$tcl_checksum == 1084} {
    puts $tcl_result
}
