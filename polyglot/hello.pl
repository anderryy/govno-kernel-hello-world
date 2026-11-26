#!/usr/bin/perl
# ============================================================================
# GovnoOS Perl 5 Scalar/Array Morphism Pipeline
# Uses Perl references, affine lambdas, checksum invariant, and chr projection.
# ============================================================================
use strict;
use warnings;

my @glyph_matrix = (
    [8, 9, 0],
    [10, 10, 1],
    [9, 12, 0],
    [12, 9, 0],
    [11, 10, 1],
    [4, 8, 0],
    [11, 11, -2],
    [10, 11, 1],
    [14, 8, 2],
    [6, 18, 0],
    [10, 10, 0],
);

sub eval_perl_tuple {
    my ($a, $b, $bias) = @_;
    return ($a * $b) + $bias;
}

my $sum = 0;
my $out = "";
foreach my $row (@glyph_matrix) {
    my $code = eval_perl_tuple($row->[0], $row->[1], $row->[2]);
    $sum += $code;
    $out .= chr($code);
}

print "$out\n" if $sum == 1084;
