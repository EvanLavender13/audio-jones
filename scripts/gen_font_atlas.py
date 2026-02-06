"""Generate a 1024x1024 bitmap font atlas (16x16 grid) from Roboto-Medium."""

from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

FONT_PATH = Path(__file__).resolve().parent.parent / "fonts" / "Roboto-Medium.ttf"
OUTPUT_PATH = Path(__file__).resolve().parent.parent / "fonts" / "font_atlas.png"

ATLAS_SIZE = 1024
GRID = 16
CELL = ATLAS_SIZE // GRID  # 64px per glyph

# Printable ASCII 32-126, then Latin-1 supplement 160-255 for visual variety.
# Remaining slots filled with geometric/math symbols.
CHARS = []
for cp in range(32, 127):       # Standard ASCII printable
    CHARS.append(chr(cp))
for cp in range(160, 256):      # Latin-1 supplement (accented, symbols)
    CHARS.append(chr(cp))
# Fill remaining slots to reach 256
FILLER = list("+-*/=<>[]{}()#@&%$!?~^|\\:;.,0123456789")
while len(CHARS) < 256:
    CHARS.append(FILLER[len(CHARS) % len(FILLER)])


def main():
    font = ImageFont.truetype(str(FONT_PATH), size=int(CELL * 0.75))
    atlas = Image.new("L", (ATLAS_SIZE, ATLAS_SIZE), 0)
    draw = ImageDraw.Draw(atlas)

    for idx, ch in enumerate(CHARS):
        col = idx % GRID
        row = idx // GRID
        cx = col * CELL + CELL // 2
        cy = row * CELL + CELL // 2
        draw.text((cx, cy), ch, fill=255, font=font, anchor="mm")

    atlas.save(str(OUTPUT_PATH))
    print(f"Saved {ATLAS_SIZE}x{ATLAS_SIZE} atlas ({GRID}x{GRID} grid, {len(CHARS)} glyphs)")
    print(f"  -> {OUTPUT_PATH}")


if __name__ == "__main__":
    main()
