"""Convert supplied Aranea PNG textures into the game's bounded RGB cache.

Run with Python 3 + Pillow from any directory. No image tools are needed by
players. Original PNGs remain untouched. GLB UVs imported by Blender use
bottom-up V, so rows are flipped for OpenGL's conventional texture origin.

Format: 8 bytes ARATEX1\\0, two little-endian uint32 dimensions (512, 512),
then exactly 512 * 512 * 3 unsigned RGB bytes. Color bytes remain sRGB;
tangent normal bytes remain linear and are normalized after downsampling.
Source GLB materials use default OPAQUE alpha mode, so image alpha is ignored.
"""

from __future__ import annotations

import hashlib
import math
from pathlib import Path
import struct

from PIL import Image, ImageOps


SIZE = 512
ROOT = Path(__file__).resolve().parents[1]
ASSET = ROOT / "assets" / "models" / "aranea"


def convert(source_name: str, output_name: str, normal: bool = False) -> None:
    source = ASSET / "textures" / source_name
    with Image.open(source) as original:
        original.load()
        if original.size != (1024, 1024):
            raise ValueError(f"Unexpected source dimensions for {source}: {original.size}")
        # The GLB uses OPAQUE for all three materials. Its color PNG's alpha
        # range 238..249 is therefore unused, rather than visible transparency.
        pixels = ImageOps.flip(original.convert("RGB").resize((SIZE, SIZE), Image.Resampling.LANCZOS))
    data = bytearray(pixels.tobytes())
    if normal:
        for offset in range(0, len(data), 3):
            x, y, z = (data[offset + axis] / 127.5 - 1.0 for axis in range(3))
            magnitude = math.sqrt(x*x + y*y + z*z)
            if magnitude < 0.01:
                raise ValueError(f"Degenerate tangent normal in {source}")
            for axis, value in enumerate((x, y, z)):
                data[offset + axis] = max(0, min(255, int((value / magnitude * .5 + .5) * 255 + .5)))
    encoded = b"ARATEX1\0" + struct.pack("<II", SIZE, SIZE) + data
    assert len(encoded) == 16 + SIZE * SIZE * 3
    target = ASSET / output_name
    target.write_bytes(encoded)
    print(f"{target.relative_to(ROOT)}: {len(encoded)} bytes; SHA256 {hashlib.sha256(encoded).hexdigest()}")


if __name__ == "__main__":
    convert("UGC_Carpet_Berber_tex_1.png", "creature-color.rgb")
    convert("Carpet_LowPile_norm_0.png", "creature-normal.rgb", normal=True)
