// ============================================================================
//  toggle.h —— 水平镜像的 PNG 绘制
//
//  原版在此处重复实现了一份逐像素混合，并且镜像索引用的是"裁剪后的宽度"，
//  贴图贴近屏幕边缘时会算错源像素甚至越界读。现改为复用 EasyXPng.h 的
//  putimagePngEx，镜像基数恒为贴图真实宽度。
// ============================================================================
#pragma once

#include "EasyXPng.h"

inline void putToggleImagePng(int img_x, int img_y, IMAGE* pSrcImg)
{
    putimagePngEx(img_x, img_y, pSrcImg, true, 0);
}

inline void putToggleImagePngFlash(int img_x, int img_y, IMAGE* pSrcImg, int flash)
{
    putimagePngEx(img_x, img_y, pSrcImg, true, flash);
}
