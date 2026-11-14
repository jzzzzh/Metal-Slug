#!/usr/bin/env python3
"""
把项目里的 PNG/JPG 素材转成桩头文件能直接读取的原始格式，
让无头渲染能画出真实画面（而不是随机噪声），用于离线目视检查构图。

输出：tools/.texcache/<扁平化路径>.raw
格式：int32 宽 | int32 高 | 宽*高 个 RGBA 字节
"""
import os
import sys
import struct

from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CACHE = os.path.join(ROOT, "tools", ".texcache")


def flat_key(rel_path: str) -> str:
    rel = rel_path.replace(os.sep, "/")
    while rel.startswith("./"):
        rel = rel[2:]
    return rel.replace("/", "_") + ".raw"


def convert(rel_path: str) -> bool:
    src = os.path.join(ROOT, rel_path)
    dst = os.path.join(CACHE, flat_key(rel_path))
    try:
        im = Image.open(src).convert("RGBA")
    except Exception as exc:                      # noqa: BLE001
        print(f"  跳过 {rel_path}: {exc}")
        return False
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    with open(dst, "wb") as f:
        f.write(struct.pack(">ii", im.width, im.height))
        f.write(im.tobytes())
    return True


def main() -> int:
    os.makedirs(CACHE, exist_ok=True)
    targets = []
    for dirpath, dirnames, filenames in os.walk(ROOT):
        dirnames[:] = [d for d in dirnames if d not in (".git", "tools", ".texcache")]
        for name in filenames:
            if name.lower().endswith((".png", ".jpg", ".jpeg")):
                full = os.path.join(dirpath, name)
                targets.append(os.path.relpath(full, ROOT))

    ok = 0
    for rel in sorted(targets):
        if convert(rel):
            ok += 1
    print(f"已转换 {ok} / {len(targets)} 张素材 -> {CACHE}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
