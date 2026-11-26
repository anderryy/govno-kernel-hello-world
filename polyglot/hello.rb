# ============================================================================
# GovnoOS Ruby Object-Oriented Glyph Metamodel
# Uses Struct blocks, Enumerable folding, and ASCII invariant verification.
# ============================================================================

GlyphSpec = Struct.new(:a, :b, :bias) do
  def code
    (a * b) + bias
  end
end

pipeline = [
  GlyphSpec.new(8, 9, 0),
  GlyphSpec.new(10, 10, 1),
  GlyphSpec.new(9, 12, 0),
  GlyphSpec.new(12, 9, 0),
  GlyphSpec.new(11, 10, 1),
  GlyphSpec.new(4, 8, 0),
  GlyphSpec.new(11, 11, -2),
  GlyphSpec.new(10, 11, 1),
  GlyphSpec.new(14, 8, 2),
  GlyphSpec.new(6, 18, 0),
  GlyphSpec.new(10, 10, 0)
]

codes = pipeline.map(&:code)
checksum = codes.sum

if checksum == 1084
  puts codes.map(&:chr).join
end
