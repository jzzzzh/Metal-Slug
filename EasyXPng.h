// ============================================================================
//  EasyXPng.h —— 带透明通道 PNG 的混合绘制
//
//  重构要点：
//    1. 修正原版 putToggleImagePng 的镜像索引错误
//       （原版用裁剪后的宽度做镜像基数，贴图贴到屏幕左/右边缘时会读越界）
//    2. 逐像素不再做整数除法：/255 换成乘法位移
//    3. 全透明 / 全不透明像素走快速路径
//    4. 新增命中闪白（flash）与镜像绘制，供游戏层复用
//
//  约定：所有 PNG 贴图的左上角为 (img_x, img_y)，alpha 通道 255 为不透明。
// ============================================================================
#pragma once

#include <graphics.h>
#include <cstddef>

// 单通道混合：out = src + dst * (255 - a) / 255
// 用 (t + 1 + (t >> 8)) >> 8 代替 t / 255，结果一致且省掉整数除法。
inline BYTE blendChannel(int src, int dst, int a)
{
    const int t = dst * (255 - a);
    return (BYTE)(src + ((t + 1 + (t >> 8)) >> 8));
}

// 合成一个像素。flash > 0 时先向白色混合（命中闪白）。
inline DWORD blendPixel(DWORD sp, DWORD dp, int flash)
{
    const int a = (sp >> 24) & 0xff;
    if (a == 0)
        return dp;                       // 全透明：保留底图

    int sr = (sp >> 16) & 0xff;
    int sg = (sp >> 8) & 0xff;
    int sb = sp & 0xff;

    if (flash > 0)
    {
        sr += ((255 - sr) * flash) / 255;
        sg += ((255 - sg) * flash) / 255;
        sb += ((255 - sb) * flash) / 255;
    }

    if (a == 255)                          // 全不透明：直接覆盖
        return (DWORD)((sr << 16) | (sg << 8) | sb);

    const int dr = (dp >> 16) & 0xff;
    const int dg = (dp >> 8) & 0xff;
    const int db = dp & 0xff;
    return (DWORD)((blendChannel(sr, dr, a) << 16) |
                   (blendChannel(sg, dg, a) << 8) |
                    blendChannel(sb, db, a));
}

// ---------------------------------------------------------------------------
//  通用绘制入口
//    flip  : 水平镜像（贴图默认朝右时，朝左需要镜像）
//    flash : 向白色混合的强度 0~255，用于受击闪白
// ---------------------------------------------------------------------------
inline void putimagePngEx(int img_x, int img_y, IMAGE* pSrcImg, bool flip, int flash)
{
    if (pSrcImg == NULL)
        return;

    DWORD* dst = GetImageBuffer();
    DWORD* src = GetImageBuffer(pSrcImg);
    const int src_w = pSrcImg->getwidth();
    const int src_h = pSrcImg->getheight();
    const int dst_w = getwidth();
    const int dst_h = getheight();

    if (src_w <= 0 || src_h <= 0 || dst == NULL || src == NULL)
        return;

    // 与屏幕求交集
    int x0 = img_x < 0 ? 0 : img_x;
    int y0 = img_y < 0 ? 0 : img_y;
    int x1 = img_x + src_w; if (x1 > dst_w) x1 = dst_w;
    int y1 = img_y + src_h; if (y1 > dst_h) y1 = dst_h;
    if (x0 >= x1 || y0 >= y1)
        return;

    for (int y = y0; y < y1; ++y)
    {
        const int j = y - img_y;                       // 贴图内的行号
        DWORD* drow = dst + (size_t)dst_w * y;
        const DWORD* srow = src + (size_t)src_w * j;

        for (int x = x0; x < x1; ++x)
        {
            const int i = x - img_x;                   // 贴图内的列号
            const int si = flip ? (src_w - 1 - i) : i; // 镜像时取对列
            drow[x] = blendPixel(srow[si], drow[x], flash);
        }
    }
}

inline void putimagePng(int img_x, int img_y, IMAGE* pSrcImg)
{
    putimagePngEx(img_x, img_y, pSrcImg, false, 0);
}

inline void putimagePngFlash(int img_x, int img_y, IMAGE* pSrcImg, int flash)
{
    putimagePngEx(img_x, img_y, pSrcImg, false, flash);
}
