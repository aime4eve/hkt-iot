#!/usr/bin/env python3
"""HKT-DeviceHub 管理控制台 favicon 生成器（生成器即源码）。

设计源文件：src/main/resources/static/console/favicon.svg（改设计改它）。
用法：在仓库根目录执行  python3 scripts/make_favicon.py

产物：
  src/main/resources/static/console/favicon.svg          # 设计源（脚本不覆盖）
  src/main/resources/static/console/favicon-{16,32,48}.png
  src/main/resources/static/console/apple-touch-icon.png  # 180
  src/main/resources/static/console/icon-{192,512}.png
  src/main/resources/static/favicon.ico                   # 16+32+48，供浏览器默认请求 /favicon.ico

依赖：rsvg-convert（brew librsvg）、Pillow。
"""
import re
import subprocess
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
SVG = ROOT / "src/main/resources/static/console/favicon.svg"
CONSOLE_DIR = ROOT / "src/main/resources/static/console"
STATIC_ROOT = ROOT / "src/main/resources/static"

PNG_SIZES = [16, 32, 48, 180, 192, 512]  # 180 = apple-touch-icon
ICO_SIZES = [16, 32, 48]
# 小尺寸剥离投影（<g id="shadow">）：深蓝叠加会吃掉白符文的对比度
FLAT_SIZES = {16, 32}


def flat_svg(tmp: Path) -> Path:
    text = re.sub(r'<g id="shadow".*?</g>', "", SVG.read_text(), flags=re.S)
    tmp.write_text(text)
    return tmp


def render_pngs():
    tmp = flat_svg(Path("/tmp/favicon-flat.svg")) if FLAT_SIZES else None
    for size in PNG_SIZES:
        name = "apple-touch-icon.png" if size == 180 else f"favicon-{size}.png" if size < 180 else f"icon-{size}.png"
        out = CONSOLE_DIR / name
        src = tmp if size in FLAT_SIZES else SVG
        subprocess.run(
            ["rsvg-convert", "-w", str(size), "-h", str(size), str(src), "-o", str(out)],
            check=True,
        )
        print(f"png  {out.relative_to(ROOT)}  {size}x{size}{'  (flat)' if size in FLAT_SIZES else ''}")


def render_ico():
    pngs = [CONSOLE_DIR / f"favicon-{s}.png" for s in ICO_SIZES]
    images = [Image.open(p) for p in pngs]
    out = STATIC_ROOT / "favicon.ico"
    images[0].save(out, format="ICO", append_images=images[1:])
    print(f"ico  {out.relative_to(ROOT)}  {ICO_SIZES}")


if __name__ == "__main__":
    if not SVG.exists():
        sys.exit(f"design source missing: {SVG}")
    render_pngs()
    render_ico()
