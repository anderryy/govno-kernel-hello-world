// ============================================================================
// GovnoOS Dart Sound-Null-Safe Glyph Stream Kernel
// Evaluates const classes, functional map/fold, and UTF-8 char reconstruction.
// ============================================================================

class DartGlyph {
  final int a;
  final int b;
  final int bias;

  const DartGlyph(this.a, this.b, this.bias);

  int eval() => (a * b) + bias;
}

void main() {
  const specs = <DartGlyph>[
    DartGlyph(8, 9, 0),
    DartGlyph(10, 10, 1),
    DartGlyph(9, 12, 0),
    DartGlyph(12, 9, 0),
    DartGlyph(11, 10, 1),
    DartGlyph(4, 8, 0),
    DartGlyph(11, 11, -2),
    DartGlyph(10, 11, 1),
    DartGlyph(14, 8, 2),
    DartGlyph(6, 18, 0),
    DartGlyph(10, 10, 0),
  ];

  final codes = specs.map((g) => g.eval()).toList();
  final total = codes.fold<int>(0, (acc, v) => acc + v);

  if (total == 1084) {
    final result = String.fromCharCodes(codes);
    print(result);
  }
}
