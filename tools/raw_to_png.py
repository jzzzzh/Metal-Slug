#!/usr/bin/env python3
"""把 render_shot 导出的 raw 帧缓冲转成 PNG，方便直接查看。"""
import glob
import os
import struct
import sys

from PIL import Image

W, H = 1920, 1080


def main() -> int:
    paths = sys.argv[1:] or sorted(glob.glob("/tmp/ms_shot_*.raw"))
    for path in paths:
        with open(path, "rb") as f:
            data = f.read()
        if len(data) < W * H * 4:
            print(f"跳过 {path}（数据不足）")
            continue
        # 帧缓冲每个像素是一个 DWORD，排布为 0xAABBGGRR，
        # 小端写盘后字节顺序是 B,G,R,A；直接按 RGBA 读会红蓝颠倒。
        # 这里做一次 BGRA -> RGBA 重排，alpha 统一置 255
        # （帧缓冲已是合成好的画面，alpha 位没有意义）。
        rgba = bytearray(W * H * 4)
        rgba[0::4] = data[2::4]   # R <- B
        rgba[1::4] = data[1::4]   # G
        rgba[2::4] = data[0::4]   # B <- R
        rgba[3::4] = b"\xff" * (W * H)
        img = Image.frombytes("RGBA", (W, H), bytes(rgba))
        out = path.rsplit(".", 1)[0] + ".png"
        img.save(out)
        print(f"{path} -> {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
