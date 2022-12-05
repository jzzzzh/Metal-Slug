#pragma once
#include <graphics.h>
#include <math.h>
#define PI 3.14159

void MyputimagePng(int img_x, int img_y, int draw_width, int draw_height, IMAGE *pSrcImg, int left_top_x, int left_top_y)
{
	DWORD *dst = GetImageBuffer();
	DWORD *src = GetImageBuffer(pSrcImg);
	int src_width = pSrcImg->getwidth();
	int src_height = pSrcImg->getheight();
	int dst_width = getwidth();
	int dst_height = getheight();

	int im_width, im_height;
	if (img_x + draw_width > dst_width)
		im_width = dst_width - img_x;
	else
		im_width = draw_width;
	if (img_y + draw_height > dst_height)
		im_height = dst_height - img_y;
	else
		im_height = draw_height;

	src = src + src_width * left_top_y + left_top_x;

	if (img_x < 0)
	{
		src = src - img_x;
		im_width = im_width + img_x;
		img_x = 0;
	}
	if (img_y < 0)
	{
		src = src - src_width * img_y;
		im_height = im_height + img_y;
		img_y = 0;
	}

	dst = dst + dst_width * img_y + img_x;
	//im_width = 640;

	for (int i = 0; i < im_height; i++)
	{
		for (int j = 0; j < im_width; j++)
		{
			int src_r = ((src[j] & 0xff0000) >> 16);
			int src_g = ((src[j] & 0xff00) >> 8);
			int src_b = src[j] & 0xff;
			int src_a = ((src[j] & 0xff000000) >> 24);
			int dst_r = ((dst[j] & 0xff0000) >> 16);
			int dst_g = ((dst[j] & 0xff00) >> 8);
			int dst_b = dst[j] & 0xff;
			dst[j] = ((src_r + dst_r * (255 - src_a) / 255) << 16) | ((src_g + dst_g * (255 - src_a) / 255) << 8) | (src_b + dst_b * (255 - src_a) / 255);
		}
		dst = dst + dst_width;
		src = src + src_width;
	}
}
