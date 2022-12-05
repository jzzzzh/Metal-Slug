// 添加boss okk
// 添加人质 okk
// 添加桶的伤害 okk
// 添加不能通过桶 okk
// 添加血条 okk
// 添加按键提示 okk
// 添加胜利失败展示 okk
// 添加音乐 okk
// 显示鼠标和子弹跟随鼠标发射 ok
// 士兵死了要有//动画 okk
// // 添加进入游戏和退出游戏
// //添加爆炸效果
// 添加捡子弹和子弹容量
// 打完一个可以往右移动
// 添加两张地图
#include <graphics.h>
#include <iostream>
#include <conio.h>
#include <time.h>
#include <vector>
#include <math.h>
#include <list>
#include "EasyXPng.h" // 用于显示带透明通道的png图片
#include "toggle.h"
#include "mmsystem.h"
#include <Windows.h>
#include "MyEasyPng.h"
#pragma warning(disable : 4996)
#pragma comment(lib, "Winmm.lib")
using namespace std;
const int roundNum = 3;
int roundnum = 0;
int WIDTH = 1920;		 // 画面宽度
int HEIGHT = 1080;		 // 画面高度
#define MaxBulletNum 200 // 最多子弹个数
// const int MAXLIFE = 100;
const int bk1_Width = 8800;
const int bk1_Height = 1080;
const int TIME = 100;
const int ldbNum = 5;
const int zjbdyqiang = 3;
const int dangrange_x = 10;
const int zjbdydaqiang = 3;
const int zjbdypao = 3;
const int zjbdydapao = 3;
const int zjfootstand = 1;
const int zjfootrun = 9;
const int zjfootdun = 6;
const int zjfootjump = 5;
const int tiehecd = 7;
const int v = 20;
const int bosstimes = 10;
const int zidan_v = 50;
int zidan_vy = 0;
int zidan_vx = zidan_v;
const int my_g = 3;
const int feijidan_vy = 20;
const int jump_v = 30;
const int down_y = 5;
const int g = 0;
const int range_x = 20;
const int range_y = 30;
const int MaxLife = 200;
// const int MaxLife = 2;
const int BossMaxLife = 70;
int groundy = 800;
const int direnv = 5;
const int myfeiji_vx = 15;
const int myfeiji_vy = 10;
const int feiji_v = 5;
const int qiangNum = 7;
const int xblife = 5;
int feiji_y = 200;
const int feiji_life = 10;
const int daoNum = 7;
const int sldsNum = 9;
const int shouliudan_vx = 30;
const int shouliudan_vy = 6;
const int shouliudan_g = 2;
const int zidanshanghai = 1;
const int dazidanshanghai = 3;
const int direnzidanshanghai = 1;
const int shouliudanshanghai = 10;
const int feijidanshanghai = 10;
const int daoshanghai = 1;
const int direnliudanshanghai = 3;
const int qiangshouPosRange = 40;
const int liudanpaoshouPosRange = 100;
const int feijiPosRange = 50;
const int shouliudanshouPosRange = 20;
const int tongLife = 50;
const int poTongLife = 20;
const int tongNum = 3;
const int shadainum = 2;
const int bossNum = 10;
const int bossAngerNum = 2;
const int bossstandNum = 4;
const int bossdieNum = 5;
const int DAOSHOUTIME = 5;
const int oldmanbangNum = 3;
const int oldmanfreeNum = 4;
const int shibingdieNum = 5;
const int shibingdie2Num = 8;
//const int roundkillnum = 10;
const int roundkillnum = 4;
const int zidanweizhi = 10;
int GameTime = 0;
int roundtruekillnum = 0;
int daoshouTime = DAOSHOUTIME;
IMAGE im_fi;
IMAGE im_bk1;
IMAGE im_bk2;
IMAGE im_bk3;
IMAGE im_zj_body_qiang[zjbdyqiang];
IMAGE im_zj_body_daqiang[zjbdydaqiang];
IMAGE im_zj_body_pao[zjbdypao];
IMAGE im_zj_body_dapao[zjbdydapao];
IMAGE im_zj_foot_stand[zjfootstand];
IMAGE im_zj_foot_run[zjfootrun];
IMAGE im_zj_foot_dun[zjfootdun];
IMAGE im_zj_foot_jump[zjfootjump];
IMAGE liuDanBing[ldbNum];
IMAGE zidan, Dazidan, drzidan;
IMAGE im_qiangdiren[qiangNum];
IMAGE im_dao[daoNum];
IMAGE im_feijidan;
IMAGE im_liudan;
IMAGE im_shouliudan;
IMAGE im_feiji;
IMAGE im_slds[sldsNum];
IMAGE im_myfeiji;
IMAGE im_tong;
IMAGE im_poTong;
IMAGE im_shadai;
IMAGE im_LetterE;
IMAGE im_shibingdie2[shibingdie2Num];
IMAGE im_shibingdie1[shibingdieNum];
IMAGE im_boss[bossNum];
IMAGE im_bossdie[bossdieNum];
IMAGE im_bossstand[bossstandNum];
IMAGE im_bossanger[bossAngerNum];
IMAGE im_oldmanbang[oldmanbangNum];
IMAGE im_oldmanfree[oldmanfreeNum];
IMAGE WIN;
IMAGE FAIL;
int gametime = 0;
int finshedtime = 0;
TCHAR filename[100];
bool isbossdied = false;
bool is_feiji = false;
bool isBossShowed = false;
bool isRenzhiShowed = false;
const int stopTime = 10;
int stoptime = stopTime;
bool changeFlag = false;
bool canIhaveFeiji = true;
TCHAR s[20];
void calScreenSize()
{
	int no_menu_bar_width = GetSystemMetrics(SM_CXFULLSCREEN);
	int no_menu_bar_height = GetSystemMetrics(SM_CYFULLSCREEN);
	HEIGHT = no_menu_bar_height;
	WIDTH = no_menu_bar_width;
	groundy = int(0.8 * HEIGHT);
	feiji_y = int(0.2 * HEIGHT);
}
class Zidan
{
public:
	int x;
	int y;
	int vx;
	int vy;
	int shot_to;
	// shot to : 0 left, 1 right
	bool is_shoted;
	Zidan()
	{
		x = 0;
		y = 0;
		vx = zidan_v;
		vy = 0;
		is_shoted = false;
		shot_to = 0;
	}
	Zidan(int xx, int yy, int ss, int vx, int vy) : x(xx), y(yy), vx(vx), vy(vy), is_shoted(false), shot_to(ss)
	{
		y = yy + zidanweizhi;
	}
	void update()
	{
		if (is_shoted == false)
		{
			vy = vy + g;
			if (shot_to == 1)
				x = x + vx;
			else
			{
				x -= vx;
			}
			y = y + vy;
			if (y > HEIGHT || x < 0 || x > WIDTH)
			{
				is_shoted = true;
			}
		}
	}
	void show()
	{
		if (shot_to == 1)
			putimagePng(x, y, &zidan);
		else
		{
			putToggleImagePng(x, y, &zidan);
		}
	}
};
class DaZidan
{
public:
	int x;
	int y;
	int vx;
	int vy;
	int shot_to;
	// shot to : 0 left, 1 right
	bool is_shoted;
	DaZidan()
	{
		x = 0;
		y = 0;
		vy = 0;
		vx = zidan_vx;
		is_shoted = false;
		shot_to = 0;
	}
	DaZidan(int xx, int yy, int ss, int vx, int vy) : x(xx), y(yy), vx(vx), vy(vy), is_shoted(false), shot_to(ss)
	{
		y = yy + zidanweizhi;
	}
	void update()
	{
		if (is_shoted == false)
		{
			vy = vy + g;
			if (shot_to == 1)
				x = x + vx;
			else
			{
				x -= vx;
			}
			y = y + vy;
			if (y > HEIGHT || x < 0 || x > WIDTH)
			{
				is_shoted = true;
			}
		}
	}
	void show()
	{
		if (shot_to == 1)
			putimagePng(x, y, &Dazidan);
		else
		{
			putToggleImagePng(x, y, &Dazidan);
		}
	}
};
class DRZidan
{
public:
	int x;
	int y;
	int vx;
	int vy;
	int shot_to;
	// shot to : 0 left, 1 right
	bool is_shoted;
	DRZidan()
	{
		x = 0;
		y = 0;
		vx = zidan_v;
		vy = 0;
		is_shoted = false;
		shot_to = 0;
	}
	DRZidan(int xx, int yy, int ss) : x(xx), y(yy), vy(0), is_shoted(false), shot_to(ss)
	{
		y = yy + zidanweizhi;
	}
	void update()
	{
		if (is_shoted == false)
		{
			vy = vy + g;
			if (shot_to == 1)
				x = x + 20;
			else
			{
				x -= 20;
			}
			y = y + vy;
			if (y > HEIGHT || x < 0 || x > WIDTH)
			{
				is_shoted = true;
			}
		}
	}
	void show()
	{
		if (is_shoted == false)
		{
			if (shot_to == 1)
				putimagePng(x, y, &drzidan);
			else
			{
				putToggleImagePng(x, y, &drzidan);
			}
		}
	}
};
class zhujue
{
public:
	int x;
	int y;
	int pos;
	// pos: 0 is left, 1 is right, 2 is left jump,  3 is right jump, 4 is left down, 5 is right down, 6 is left stand, 7 is right stand
	int wq;
	int vy;
	// wq: 0 is shouqiang, 1 is liudanpao
	int shot;
	// shot: 0 is no shot, 1 is shot
	int life;
	list<Zidan> zidanlist;
	list<DaZidan> dazidanlist;
	zhujue()
	{
		x = 200;
		vy = 0;
		y = groundy;
		pos = 1;
		wq = 0;
		shot = 0;
		life = MaxLife;
	}
	void changexy(int xx, int yy)
	{
		x = xx;
		y = yy;
	}
	void changePos(int poss)
	{
		pos = poss;
	}
	void changeWq(int wqq)
	{
		wq = wqq;
	}
	void draw()
	{
		setlinecolor(RGB(255, 255, 255));
		fillrectangle(9, 9, 11 + 200, 31);
		setfillcolor(RGB(18, 150, 70));
		fillrectangle(10, 10, 10 + ((200 * life) / MaxLife), 30);
		setfillcolor(RGB(255, 255, 255));
		setlinecolor(RGB(255, 255, 255));
		swprintf_s(s, _T("%d"), int(life));
		// outtextxy(100, 100, s);
		if (shot == 0)
		{
			if (pos == 0)
			{
				if (wq == 0)
				{
					putimagePng(x, y, &im_zj_body_qiang[gametime % zjbdyqiang]);
					putimagePng(x, y + im_zj_body_qiang[gametime % zjbdyqiang].getheight() - tiehecd, &im_zj_foot_run[gametime % zjfootrun]);
				}
				else
				{
					putimagePng(x, y, &im_zj_body_pao[gametime % zjbdypao]);
					putimagePng(x, y + im_zj_body_pao[gametime % zjbdypao].getheight() - tiehecd, &im_zj_foot_run[gametime % zjfootrun]);
				}
			}
			else if (pos == 1)
			{
				if (wq == 0)
				{
					putToggleImagePng(x, y, &im_zj_body_qiang[gametime % zjbdyqiang]);
					putToggleImagePng(x, y + im_zj_body_qiang[gametime % zjbdyqiang].getheight() - tiehecd, &im_zj_foot_run[gametime % zjfootrun]);
				}
				else
				{
					putToggleImagePng(x, y, &im_zj_body_pao[gametime % zjbdypao]);
					putToggleImagePng(x, y + im_zj_body_pao[gametime % zjbdypao].getheight() - tiehecd, &im_zj_foot_run[gametime % zjfootrun]);
				}
			}
			else if (pos == 2)
			{
				if (wq == 0)
				{
					putimagePng(x, y, &im_zj_body_qiang[gametime % zjbdyqiang]);
					putimagePng(x, y + im_zj_body_qiang[gametime % zjbdyqiang].getheight() - tiehecd, &im_zj_foot_jump[gametime % zjfootjump]);
				}
				else
				{
					putimagePng(x, y, &im_zj_body_pao[gametime % zjbdypao]);
					putimagePng(x, y + im_zj_body_pao[gametime % zjbdypao].getheight() - tiehecd, &im_zj_foot_jump[gametime % zjfootjump]);
				}
			}
			else if (pos == 3)
			{
				if (wq == 0)
				{
					putToggleImagePng(x, y, &im_zj_body_qiang[gametime % zjbdyqiang]);
					putToggleImagePng(x, y + im_zj_body_qiang[gametime % zjbdyqiang].getheight() - tiehecd, &im_zj_foot_jump[gametime % zjfootjump]);
				}
				else
				{
					putToggleImagePng(x, y, &im_zj_body_pao[gametime % zjbdypao]);
					putToggleImagePng(x, y + im_zj_body_pao[gametime % zjbdypao].getheight() - tiehecd, &im_zj_foot_jump[gametime % zjfootjump]);
				}
			}
			else if (pos == 4)
			{
				if (wq == 0)
				{
					putimagePng(x, y, &im_zj_body_qiang[gametime % zjbdyqiang]);
					putimagePng(x, y + im_zj_body_qiang[gametime % zjbdyqiang].getheight() - tiehecd, &im_zj_foot_dun[gametime % zjfootdun]);
				}
				else
				{
					putimagePng(x, y, &im_zj_body_pao[gametime % zjbdypao]);
					putimagePng(x, y + im_zj_body_pao[gametime % zjbdypao].getheight() - tiehecd, &im_zj_foot_dun[gametime % zjfootdun]);
				}
			}
			else if (pos == 5)
			{
				if (wq == 0)
				{
					putToggleImagePng(x, y, &im_zj_body_qiang[gametime % zjbdyqiang]);
					putToggleImagePng(x, y + im_zj_body_qiang[gametime % zjbdyqiang].getheight() - tiehecd, &im_zj_foot_dun[gametime % zjfootdun]);
				}
				else
				{
					putToggleImagePng(x, y, &im_zj_body_pao[gametime % zjbdypao]);
					putToggleImagePng(x, y + im_zj_body_pao[gametime % zjbdypao].getheight() - tiehecd, &im_zj_foot_dun[gametime % zjfootdun]);
				}
			}
			else if (pos == 6)
			{
				if (wq == 0)
				{
					putimagePng(x, y, &im_zj_body_qiang[gametime % zjbdyqiang]);
					putimagePng(x, y + im_zj_body_qiang[gametime % zjbdyqiang].getheight() - tiehecd, &im_zj_foot_stand[gametime % zjfootstand]);
				}
				else
				{
					putimagePng(x, y, &im_zj_body_pao[gametime % zjbdypao]);
					putimagePng(x, y + im_zj_body_pao[gametime % zjbdypao].getheight() - tiehecd, &im_zj_foot_stand[gametime % zjfootstand]);
				}
			}
			else if (pos == 7)
			{
				if (wq == 0)
				{
					putToggleImagePng(x, y, &im_zj_body_qiang[gametime % zjbdyqiang]);
					putToggleImagePng(x, y + im_zj_body_qiang[gametime % zjbdyqiang].getheight() - tiehecd, &im_zj_foot_stand[gametime % zjfootstand]);
				}
				else
				{
					putToggleImagePng(x, y, &im_zj_body_pao[gametime % zjbdypao]);
					putToggleImagePng(x, y + im_zj_body_pao[gametime % zjbdypao].getheight() - tiehecd, &im_zj_foot_stand[gametime % zjfootstand]);
				}
			}
		}
		else
		{
			if (pos == 0)
			{
				if (wq == 0)
				{
					putimagePng(x, y, &im_zj_body_daqiang[gametime % zjbdydaqiang]);
					putimagePng(x, y + im_zj_body_daqiang[gametime % zjbdydaqiang].getheight() - tiehecd, &im_zj_foot_run[gametime % zjfootrun]);
				}
				else
				{
					putimagePng(x, y, &im_zj_body_dapao[gametime % zjbdydapao]);
					putimagePng(x, y + im_zj_body_dapao[gametime % zjbdydapao].getheight() - tiehecd, &im_zj_foot_run[gametime % zjfootrun]);
				}
			}
			else if (pos == 1)
			{
				if (wq == 0)
				{
					putToggleImagePng(x, y, &im_zj_body_daqiang[gametime % zjbdydaqiang]);
					putToggleImagePng(x, y + im_zj_body_daqiang[gametime % zjbdydaqiang].getheight() - tiehecd, &im_zj_foot_run[gametime % zjfootrun]);
				}
				else
				{
					putToggleImagePng(x, y, &im_zj_body_dapao[gametime % zjbdydapao]);
					putToggleImagePng(x, y + im_zj_body_dapao[gametime % zjbdydapao].getheight() - tiehecd, &im_zj_foot_run[gametime % zjfootrun]);
				}
			}
			else if (pos == 2)
			{
				if (wq == 0)
				{
					putimagePng(x, y, &im_zj_body_daqiang[gametime % zjbdydaqiang]);
					putimagePng(x, y + im_zj_body_daqiang[gametime % zjbdydaqiang].getheight() - tiehecd, &im_zj_foot_jump[gametime % zjfootjump]);
				}
				else
				{
					putimagePng(x, y, &im_zj_body_dapao[gametime % zjbdydapao]);
					putimagePng(x, y + im_zj_body_dapao[gametime % zjbdydapao].getheight() - tiehecd, &im_zj_foot_jump[gametime % zjfootjump]);
				}
			}
			else if (pos == 3)
			{
				if (wq == 0)
				{
					putToggleImagePng(x, y, &im_zj_body_daqiang[gametime % zjbdydaqiang]);
					putToggleImagePng(x, y + im_zj_body_daqiang[gametime % zjbdydaqiang].getheight() - tiehecd, &im_zj_foot_jump[gametime % zjfootjump]);
				}
				else
				{
					putToggleImagePng(x, y, &im_zj_body_dapao[gametime % zjbdydapao]);
					putToggleImagePng(x, y + im_zj_body_dapao[gametime % zjbdydapao].getheight() - tiehecd, &im_zj_foot_jump[gametime % zjfootjump]);
				}
			}
			else if (pos == 4)
			{
				if (wq == 0)
				{
					putimagePng(x, y, &im_zj_body_daqiang[gametime % zjbdydaqiang]);
					putimagePng(x, y + im_zj_body_daqiang[gametime % zjbdydaqiang].getheight() - tiehecd, &im_zj_foot_dun[gametime % zjfootdun]);
				}
				else
				{
					putimagePng(x, y, &im_zj_body_dapao[gametime % zjbdydapao]);
					putimagePng(x, y + im_zj_body_dapao[gametime % zjbdydapao].getheight() - tiehecd, &im_zj_foot_dun[gametime % zjfootdun]);
				}
			}
			else if (pos == 5)
			{
				if (wq == 0)
				{
					putToggleImagePng(x, y, &im_zj_body_daqiang[gametime % zjbdydaqiang]);
					putToggleImagePng(x, y + im_zj_body_daqiang[gametime % zjbdydaqiang].getheight() - tiehecd, &im_zj_foot_dun[gametime % zjfootdun]);
				}
				else
				{
					putToggleImagePng(x, y, &im_zj_body_dapao[gametime % zjbdydapao]);
					putToggleImagePng(x, y + im_zj_body_dapao[gametime % zjbdydapao].getheight() - tiehecd, &im_zj_foot_dun[gametime % zjfootdun]);
				}
			}
			else if (pos == 6)
			{
				if (wq == 0)
				{
					putimagePng(x, y, &im_zj_body_daqiang[gametime % zjbdydaqiang]);
					putimagePng(x, y + im_zj_body_daqiang[gametime % zjbdydaqiang].getheight() - tiehecd, &im_zj_foot_stand[gametime % zjfootstand]);
				}
				else
				{
					putimagePng(x, y, &im_zj_body_dapao[gametime % zjbdydapao]);
					putimagePng(x, y + im_zj_body_dapao[gametime % zjbdydapao].getheight() - tiehecd, &im_zj_foot_stand[gametime % zjfootstand]);
				}
			}
			else if (pos == 7)
			{
				if (wq == 0)
				{
					putToggleImagePng(x, y, &im_zj_body_daqiang[gametime % zjbdydaqiang]);
					putToggleImagePng(x, y + im_zj_body_daqiang[gametime % zjbdydaqiang].getheight() - tiehecd, &im_zj_foot_stand[gametime % zjfootstand]);
				}
				else
				{
					putToggleImagePng(x, y, &im_zj_body_dapao[gametime % zjbdydapao]);
					putToggleImagePng(x, y + im_zj_body_dapao[gametime % zjbdydapao].getheight() - tiehecd, &im_zj_foot_stand[gametime % zjfootstand]);
				}
			}
		}
	}
};
zhujue zj;
class qiangshou
{
public:
	int x;
	int y;
	int pos;
	// pos : 0 left, 1 right;
	int life;
	int is_mov;
	list<DRZidan> drzdlist;
	qiangshou()
	{
		x = rand() % WIDTH;
		y = groundy;
		pos = rand() % 2;
		life = xblife;
		is_mov = 0;
	}
	void changexy(int xx, int yy)
	{
		x = xx;
		y = yy;
	}
	void changePos(int poss)
	{
		pos = poss;
	}
	void update()
	{
		if (abs(x - zj.x) < qiangshouPosRange || rand() % 10 == 0)
		{
			is_mov = 1;
		}
		if (abs(x - zj.x) > qiangshouPosRange && rand() % 5 == 0)
		{
			is_mov = 0;
		}
		if (x > zj.x)
		{
			pos = 0;
			if (is_mov == 0)
			{
				x -= rand() % direnv;
			}
			if (gametime % 10 == 0)
			{
				drzdlist.push_back(DRZidan(x, y, 0));
			}
		}
		else
		{
			pos = 1;
			if (is_mov == 0)
			{
				x += rand() % direnv;
			}
			if (gametime % 10 == 0)
			{
				drzdlist.push_back(DRZidan(x, y, 1));
			}
		}
		for (auto i = drzdlist.begin(); i != drzdlist.end(); i++)
		{
			(*i).update();
			//	i.show();
		}
	}
	void show()
	{
		if (pos == 0)
		{
			putimagePng(x, y, &im_qiangdiren[(gametime + rand()) % qiangNum]);
		}
		else
		{
			putToggleImagePng(x, y, &im_qiangdiren[(gametime + rand()) % qiangNum]);
		}
		for (auto i : drzdlist)
		{
			// i.update();
			i.show();
		}
	}
};
list<qiangshou> qiangshoulist;
class liudanpao
{
public:
	int x;
	int y;
	int vx;
	int vy;
	int shot_to;
	// shot to : 0 left, 1 right
	bool is_shoted;
	liudanpao()
	{
		x = 0;
		y = 0;
		vx = zidan_v;
		vy = 0;
		is_shoted = false;
		shot_to = 0;
	}
	liudanpao(int xx, int yy, int ss) : x(xx), y(yy), vy(0), is_shoted(false), shot_to(ss) {}
	void update()
	{
		if (is_shoted == false)
		{
			vy = vy + g;
			if (shot_to == 1)
				x = x + 20;
			else
			{
				x -= 20;
			}
			y = y + vy;
			if (y > HEIGHT || x < 0 || x > WIDTH)
			{
				is_shoted = true;
			}
		}
	}
	void show()
	{
		if (is_shoted == false)
		{
			if (shot_to == 0)
				putimagePng(x, y, &im_liudan);
			else
			{
				putToggleImagePng(x, y, &im_liudan);
			}
		}
	}
};
class liudanpaoshou
{
public:
	int x;
	int y;
	int pos;
	// pos : 0 left, 1 right;
	int life;
	int is_mov;
	list<liudanpao> drzdlist;
	liudanpaoshou()
	{
		x = rand() % WIDTH;
		y = groundy;
		pos = rand() % 2;
		is_mov = 0;
		life = xblife;
	}
	void changexy(int xx, int yy)
	{
		x = xx;
		y = yy;
	}
	void changePos(int poss)
	{
		pos = poss;
	}
	void update()
	{
		if (abs(x - zj.x) < liudanpaoshouPosRange || rand() % 10 == 0)
		{
			is_mov = 1;
		}
		if (abs(x - zj.x) > liudanpaoshouPosRange && rand() % 5 == 0)
		{
			is_mov = 0;
		}
		if (x > zj.x)
		{
			pos = 0;
			if (is_mov == 0)
				x -= rand() % direnv;
			if (gametime % 10 == 0)
			{
				drzdlist.push_back(liudanpao(x, y, 0));
			}
		}
		else
		{
			pos = 1;
			if (is_mov == 0)
				x += rand() % direnv;
			if (gametime % 10 == 0)
			{
				drzdlist.push_back(liudanpao(x, y, 1));
			}
		}
		for (auto i = drzdlist.begin(); i != drzdlist.end(); i++)
		{
			(*i).update();
			//	i.show();
		}
	}
	void show()
	{
		if (pos == 0)
		{
			putimagePng(x, y, &liuDanBing[(gametime + rand()) % ldbNum]);
		}
		else
		{
			putToggleImagePng(x, y, &liuDanBing[(gametime + rand()) % ldbNum]);
		}
		for (auto i : drzdlist)
		{
			// i.update();
			i.show();
		}
	}
};
list<liudanpaoshou> ldps;
class feijidan
{
public:
	int x;
	int y;
	int vy;
	bool is_bombed;

	feijidan()
	{
		x = 0;
		y = 0;
		vy = feijidan_vy;
		is_bombed = false;
	}
	feijidan(int xx, int yy) : x(xx), y(yy), vy(feijidan_vy), is_bombed(false) {}
	void update()
	{
		if (is_bombed == false)
		{
			y += vy;
			if (y >= groundy)
			{
				is_bombed = true;
			}
		}
	}
	void show()
	{
		if (is_bombed == false)
		{
			putimagePng(x, y, &im_feijidan);
		}
	}
};
class feiji
{
public:
	int x;
	int y;
	int pos;
	// pos : 0 left, 1 right
	int life;
	int is_shoted;
	int is_mov;
	list<feijidan> fjdlist;
	feiji()
	{
		is_mov = 0;
		x = rand() % WIDTH;
		y = feiji_y;
		pos = 0;
		life = feiji_life;
		is_shoted = 0;
	}
	feiji(int xx, int poss) : x(xx), is_mov(0), pos(poss), y(feiji_y), life(feiji_life), is_shoted(0)
	{
	}
	void update()
	{
		if (abs(x - zj.x) <= feijiPosRange || rand() % 10 == 0)
		{
			is_mov = 1;
		}
		if (abs(x - zj.x) > feijiPosRange && rand() % 25 == 0)
		{
			is_mov = 0;
		}
		if (is_shoted == 0)
		{
			if (x > zj.x)
			{
				pos = 0;
				if (is_mov == 0)
					x -= feiji_v;
			}
			else if (x < zj.x)
			{
				pos = 1;
				if (is_mov == 0)
					x += feiji_v;
			}
			if (gametime % 20 == 0)
			{
				fjdlist.push_back(feijidan(x, y));
			}
			for (auto i = fjdlist.begin(); i != fjdlist.end(); i++)
			{
				(*i).update();
			}
		}
	}
	void show()
	{
		if (is_shoted == 0)
		{
			if (pos == 0)
			{
				putimagePng(x, y, &im_feiji);
			}
			else
			{
				putToggleImagePng(x, y, &im_feiji);
			}
			for (auto i : fjdlist)
			{
				i.show();
			}
		}
	}
};
list<feiji> fjlist;
class myfeiji
{
public:
	int x;
	int y;
	int pos;
	// pos : 0 left, 1 right
	int life;
	int is_shoted;
	list<feijidan> fjdlist;
	myfeiji()
	{
		x = rand() % WIDTH;
		y = groundy;
		pos = 0;
		life = feiji_life;
		is_shoted = 0;
	}
	myfeiji(int xx, int poss) : x(xx), pos(poss), y(feiji_y), life(feiji_life), is_shoted(0)
	{
	}
	void update(int xx, int yy, int poss)
	{
		x = xx;
		y = yy;
		pos = poss;
	}
	void show()
	{
		if (is_shoted == 0)
		{
			if (pos == 0)
			{
				putimagePng(x, y, &im_myfeiji);
			}
			else
			{
				putToggleImagePng(x, y, &im_myfeiji);
			}
			for (auto i : fjdlist)
			{
				i.show();
			}
		}
	}
};
myfeiji myfj;
class daoshou
{
public:
	int x;
	int y;
	int pos;
	// pos : 0 left, 1 right;
	int life;
	daoshou()
	{
		x = rand() % WIDTH;
		y = groundy;
		pos = rand() % 2;
		life = xblife;
	}
	void changexy(int xx, int yy)
	{
		x = xx;
		y = yy;
	}
	void changePos(int poss)
	{
		pos = poss;
	}
	void update()
	{
		if (x > zj.x)
		{
			pos = 0;
			x -= rand() % direnv;
		}
		else
		{
			pos = 1;
			x += rand() % direnv;
		}
	}
	void show()
	{
		if (pos == 0)
		{
			putimagePng(x, y, &im_dao[(gametime + rand()) % daoNum]);
		}
		else
		{
			putToggleImagePng(x, y, &im_dao[(gametime + rand()) % daoNum]);
		}
	}
};
list<daoshou> dslist;
class shouliudan
{
public:
	int x;
	int y;
	int vx;
	int vy;
	int shot_to;
	// shot to : 0 left, 1 right
	bool is_shoted;
	shouliudan()
	{
		x = 0;
		y = 0;
		vx = shouliudan_vx;
		vy = -shouliudan_vy;
		is_shoted = false;
		shot_to = 0;
	}
	shouliudan(int xx, int yy, int ss) : x(xx), y(yy), vy(-shouliudan_vy), vx(shouliudan_vx), is_shoted(false), shot_to(ss) {}
	void update()
	{
		if (is_shoted == false)
		{
			vy = vy + shouliudan_g;
			if (shot_to == 1)
				x = x + shouliudan_vx;
			else
			{
				x -= shouliudan_vx;
			}
			y = y + vy;
			if (y > groundy + 100 || x < 0 || x > WIDTH)
			{
				is_shoted = true;
			}
		}
	}
	void show()
	{
		if (is_shoted == false)
		{
			if (shot_to == 0)
				putimagePng(x, y, &im_shouliudan);
			else
			{
				putToggleImagePng(x, y, &im_shouliudan);
			}
		}
	}
};
class rengshouliudanshou
{
public:
	int x;
	int y;
	int pos;
	// pos : 0 left, 1 right;
	int life;
	int is_mov;
	list<shouliudan> sldlist;
	rengshouliudanshou()
	{
		x = rand() % WIDTH;
		y = groundy;
		pos = rand() % 2;
		life = xblife;
		is_mov = 0;
	}
	void changexy(int xx, int yy)
	{
		x = xx;
		y = yy;
	}
	void changePos(int poss)
	{
		pos = poss;
	}
	void update()
	{
		if (abs(x - zj.x) <= shouliudanshouPosRange || rand() % 10 == 0)
		{
			is_mov = 1;
		}
		if (abs(x - zj.x) > shouliudanshouPosRange && rand() % 5 == 0)
		{
			is_mov = 0;
		}
		if (x > zj.x)
		{
			pos = 0;
			if (is_mov == 0)
				x -= rand() % direnv;
			if (gametime % 10 == 0)
			{
				sldlist.push_back(shouliudan(x, y, 0));
			}
		}
		else
		{
			pos = 1;
			if (is_mov == 0)
				x += rand() % direnv;
			if (gametime % 10 == 0)
			{
				sldlist.push_back(shouliudan(x, y, 1));
			}
		}
		for (auto i = sldlist.begin(); i != sldlist.end(); i++)
		{
			(*i).update();
			//	i.show();
		}
	}
	void show()
	{
		if (pos == 0)
		{
			putimagePng(x, y, &im_slds[(gametime + rand()) % sldsNum]);
		}
		else
		{
			putToggleImagePng(x, y, &im_slds[(gametime + rand()) % sldsNum]);
		}
		for (auto i : sldlist)
		{
			// i.update();
			i.show();
		}
	}
};
list<rengshouliudanshou> rsldlist;
class tong
{
public:
	int x;
	int y;
	int life;
	tong()
	{
		do
		{
			x = rand() % WIDTH;
		} while (abs(zj.x - x) <= 30);
		y = groundy + 10;
		life = tongLife;
	}
	void show()
	{
		if (life > poTongLife)
		{
			putimagePng(x, y, &im_tong);
		}
		else
		{
			putimagePng(x, y, &im_poTong);
		}
	}
};
list<tong> tongList;
class shadai
{
public:
	int x;
	int y;
	shadai()
	{
		do
		{
			x = rand() % WIDTH;
		} while (abs(zj.x - x) <= 30);
		y = groundy + 20;
	}
	void show()
	{
		putimagePng(x, y, &im_shadai);
	}
};
list<shadai> shadailist;
class boss
{
public:
	int x;
	int y;
	int time;
	int life;
	int imgnum;
	boss()
	{
		x = 0.7 * WIDTH;
		y = groundy;
		time = 0;
		life = BossMaxLife;
		imgnum = 0;
	}
	boss(int xx, int yy, int ttime, int llife)
	{
		x = xx;
		y = yy;
		time = ttime;
		life = llife;
		imgnum = 0;
	}
	bool show()
	{
		if (imgnum < bossNum * bosstimes)
		{
			putimagePng(x, y, &im_boss[(imgnum++) % bossNum]);
			return true;
		}
		else
		{
			return false;
		}
	}
};
list<boss> bosslist;
class bossstand
{
public:
	int x;
	int y;
	int time;
	int life;
	bossstand()
	{
		x = 0.7 * WIDTH;
		y = groundy;
		time = 0;
		life = BossMaxLife;
	}
	bossstand(int xx, int yy, int ttime, int llife)
	{
		x = xx;
		y = yy;
		time = ttime;
		life = llife;
	}
	void show()
	{
		putimagePng(x, y, &im_bossstand[(gametime) % bossstandNum]);
	}
};
list<bossstand> bossstandlist;
class bossdie
{
public:
	int x;
	int y;
	int time;
	int life;
	int imgnum;
	bossdie()
	{
		x = 0.7 * WIDTH;
		y = groundy;
		time = 0;
		life = 0;
		imgnum = 0;
	}
	bool show()
	{
		if (imgnum < bosstimes * bossdieNum)
		{
			putimagePng(x, y, &im_bossdie[imgnum++ % bossdieNum]);
			return true;
		}
		else
		{
			return false;
		}
	}
};
list<bossdie> bossdielist;
class bossAnger
{
public:
	int x;
	int y;
	int time;
	int life;
	int imgnum;
	bossAnger()
	{
		x = 0.7 * WIDTH;
		y = groundy;
		time = 0;
		life = 0;
		imgnum = 0;
	}
	bossAnger(int xx, int yy, int ttime, int llife)
	{
		x = xx;
		y = yy;
		time = ttime;
		life = llife;
		imgnum = 0;
	}
	bossAnger(int ttime, int llife)
	{
		time = ttime;
		x = 0.7 * WIDTH;
		y = groundy;
		life = llife;
		imgnum = 0;
	}
	bool show()
	{
		if (imgnum < bosstimes * bossAngerNum)
		{
			putimagePng(x, y, &im_bossanger[imgnum++ % bossAngerNum]);
			return true;
		}
		else
		{
			return false;
		}
	}
};
list<bossAnger> bossangerlist;
class oldmanbang
{
public:
	int x;
	int y;
	int imgnum;
	oldmanbang()
	{
		x = int(0.85 * WIDTH);
		y = groundy;
		imgnum = 0;
	}
	bool show()
	{
		putimagePng(x, y, &im_oldmanbang[imgnum % oldmanbangNum]);
		imgnum++;
		return true;
	}
};
list<oldmanbang> oldmanbanglist;
class oldmanfree
{
public:
	int x;
	int y;
	int imgnum;
	oldmanfree()
	{
		x = int(0.85 * WIDTH);
		y = groundy;
		imgnum = 0;
	}
	oldmanfree(int xx)
	{
		x = xx;
		y = groundy;
		imgnum = 0;
	}
	bool show()
	{
		if (imgnum < oldmanfreeNum)
		{
			putimagePng(x, y, &im_oldmanfree[imgnum++]);
			return true;
		}
		else
		{
			return false;
		}
	}
};
list<oldmanfree> oldmanfreelist;
class die1man
{
public:
	int x;
	int y;
	int imgnum;
	die1man()
	{
		x = 0;
		y = groundy;
		imgnum = 0;
	}
	die1man(int xx, int yy)
	{
		x = xx;
		y = yy;
		imgnum = 0;
	}
	bool show()
	{
		if (imgnum < shibingdieNum)
		{
			putimagePng(x, y, &im_shibingdie1[imgnum++]);
			return true;
		}
		else
		{
			return false;
		}
	}
};
list<die1man> die1manlist;
class die2man
{
public:
	int x;
	int y;
	int imgnum;
	die2man()
	{
		x = 0;
		y = groundy;
		imgnum = 0;
	}
	die2man(int xx, int yy)
	{
		x = xx;
		y = yy;
		imgnum = 0;
	}
	bool show()
	{
		if (imgnum < shibingdie2Num)
		{
			putimagePng(x, y, &im_shibingdie2[imgnum++]);
			return true;
		}
		else
		{
			return false;
		}
	}
};
list<die2man> die2manlist;
class LetterE {
public:
	int x;
	int y;
	bool is_show;
	LetterE()
	{
		x = 0;
		y = groundy - 40;
		is_show = false;
	}
	LetterE(int xx)
	{
		x = xx;
		y = groundy - 40;
		is_show = false;
	}
	bool change(int xx)
	{
		x = xx;
		return true;
	}
	bool isshow()
	{
		is_show = true;
		return true;
	}
	bool isnotshow()
	{
		is_show = false;
		return true;
	}
	bool show()
	{
		putimagePng(x, y, &im_LetterE);
		return true;
	}
};
LetterE E;
void startup() //  初始化函数
{
	srand(time(0));												// 初始化随机种子
	loadimage(&im_fi, _T("firstImg.jpg"), WIDTH, HEIGHT, true); // 导入背景图片
	loadimage(&im_bk1, _T("background1.png"), bk1_Width * HEIGHT / bk1_Height, HEIGHT, true);
	loadimage(&im_bk2, _T("background2.png"), bk1_Width * HEIGHT / bk1_Height, HEIGHT, true);
	loadimage(&im_bk3, _T("background3.png"), bk1_Width * HEIGHT / bk1_Height, HEIGHT, true);
	loadimage(&zidan, _T("zidan.png"));
	loadimage(&im_feijidan, _T("./feiji/zhadan.png"));
	loadimage(&Dazidan, _T("dazidan.png"));
	loadimage(&drzidan, _T("enemyzidan.png"));
	loadimage(&im_liudan, _T("liudan.png"));
	loadimage(&im_feiji, _T("./feiji/feiji.png"));
	loadimage(&im_shouliudan, _T("./shouliudan/sld.png"));
	loadimage(&im_myfeiji, _T("myFeiji.png"));
	loadimage(&im_tong, _T("tong.png"));
	loadimage(&im_poTong, _T("potong.png"));
	loadimage(&im_shadai, _T("shadai.png"));
	loadimage(&WIN, _T("win.png"));
	loadimage(&FAIL, _T("fail.png"));
	loadimage(&im_LetterE, _T("E.png"));
	myfj.y = groundy;
	for (int i = 0; i < ldbNum; i++)
	{
		swprintf_s(filename, _T("./liudan/liudan (%d).png"), i + 1);
		loadimage(&liuDanBing[i], filename);
	}
	for (int i = 0; i < zjbdydapao; i++)
	{
		swprintf_s(filename, _T("./zhujue/body/leftdapao (%d).png"), i + 1);
		loadimage(&im_zj_body_dapao[i], filename);
	}
	for (int i = 0; i < zjbdypao; i++)
	{
		swprintf_s(filename, _T("./zhujue/body/leftpao (%d).png"), i + 1);
		loadimage(&im_zj_body_pao[i], filename);
	}
	for (int i = 0; i < zjbdydaqiang; i++)
	{
		swprintf_s(filename, _T("./zhujue/body/leftdaqiang (%d).png"), i + 1);
		loadimage(&im_zj_body_daqiang[i], filename);
	}
	for (int i = 0; i < zjbdyqiang; i++)
	{
		swprintf_s(filename, _T("./zhujue/body/leftqiang (%d).png"), i + 1);
		loadimage(&im_zj_body_qiang[i], filename);
	}
	for (int i = 0; i < zjfootdun; i++)
	{
		swprintf_s(filename, _T("./zhujue/foot/leftdun (%d).png"), i + 1);
		loadimage(&im_zj_foot_dun[i], filename);
	}
	for (int i = 0; i < zjfootrun; i++)
	{
		swprintf_s(filename, _T("./zhujue/foot/leftrun (%d).png"), i + 1);
		loadimage(&im_zj_foot_run[i], filename);
	}
	for (int i = 0; i < zjfootjump; i++)
	{
		swprintf_s(filename, _T("./zhujue/foot/leftjump (%d).png"), i + 1);
		loadimage(&im_zj_foot_jump[i], filename);
	}
	for (int i = 0; i < zjfootstand; i++)
	{
		swprintf_s(filename, _T("./zhujue/foot/leftstand.png"));
		loadimage(&im_zj_foot_stand[i], filename);
	}
	for (int i = 0; i < qiangNum; i++)
	{
		swprintf_s(filename, _T("./qiang/qiang (%d).png"), i + 1);
		loadimage(&im_qiangdiren[i], filename);
	}
	for (int i = 0; i < daoNum; i++)
	{
		swprintf_s(filename, _T("./dao/dao (%d).png"), i + 1);
		loadimage(&im_dao[i], filename);
	}
	for (int i = 0; i < sldsNum; i++)
	{
		swprintf_s(filename, _T("./shouliudan/sld (%d).png"), i + 1);
		loadimage(&im_slds[i], filename);
	}
	for (int i = 0; i < shibingdieNum; i++)
	{
		swprintf_s(filename, _T("./die/die (%d).png"), i + 1);
		loadimage(&im_shibingdie1[i], filename);
	}

	for (int i = 0; i < shibingdie2Num; i++)
	{
		swprintf_s(filename, _T("./die2/die (%d).png"), i + 1);
		loadimage(&im_shibingdie2[i], filename);
	}
	for (int i = 0; i < bossNum; i++)
	{
		swprintf_s(filename, _T("./boss/boss (%d).png"), i + 1);
		loadimage(&im_boss[i], filename);
	}
	for (int i = 0; i < bossAngerNum; i++)
	{
		swprintf_s(filename, _T("./boss/bossanger (%d).png"), i + 1);
		loadimage(&im_bossanger[i], filename);
	}
	for (int i = 0; i < bossstandNum; i++)
	{
		swprintf_s(filename, _T("./boss/bossstand (%d).png"), i + 1);
		loadimage(&im_bossstand[i], filename);
	}
	for (int i = 0; i < bossdieNum; i++)
	{
		swprintf_s(filename, _T("./boss/bossdie (%d).png"), i + 1);
		loadimage(&im_bossdie[i], filename);
	}
	for (int i = 0; i < oldmanbangNum; i++)
	{
		swprintf_s(filename, _T("./oldman/laorenbang (%d).png"), i + 1);
		loadimage(&im_oldmanbang[i], filename);
	}
	for (int i = 0; i < oldmanfreeNum; i++)
	{
		swprintf_s(filename, _T("./oldman/laorenfree (%d).png"), i + 1);
		loadimage(&im_oldmanfree[i], filename);
	}
	for (int t = 0; t < 0; t++)
	{
		qiangshoulist.push_back(qiangshou());
	}
	for (int t = 0; t < 0; t++)
	{
		rsldlist.push_back(rengshouliudanshou());
	}
	for (int t = 0; t < 3; t++)
	{
		shadailist.push_back(shadai());
	}
	for (int t = 0; t < 2; t++)
	{
		tongList.push_back(tong());
	}
	for (int t = 0; t < 0; t++)
	{
		ldps.push_back(liudanpaoshou());
	}

	for (int i = 0; i < 0; i++)
	{
		fjlist.push_back(feiji());
	}
	for (int i = 0; i < 0; i++)
	{
		dslist.push_back(daoshou());
	}
	for (int i = 0; i < 1; i++)
	{
		bossstandlist.push_back(bossstand());
	}
	for (int i = 0; i < 1; i++)
	{
		oldmanbanglist.push_back(oldmanbang());
	}
	initgraph(WIDTH, HEIGHT); // 新开一个画面
	BeginBatchDraw();		  // 开始批量绘制
}
void beforeshow() // 绘制函数
{
	putimage(0, 0, &im_fi); // 显示背景
	FlushBatchDraw();		// 批量绘制
	Sleep(1000);			// 暂停
	cleardevice();
}
bool checkIsDangZhu(int x, int y, int pos)
{
	bool flag = false;
	for (auto i = tongList.begin(); i != tongList.end(); i++)
	{
		//if (flag == false && (*i).x == x && (y > (*i).y - 60 - range_y && y < (*i).y + 10 + range_y) && pos == 1)
		if (flag == false && (*i).x + range_x >= x && (*i).x - range_x <= x && (y > (*i).y - 40 - range_y && y < (*i).y + 20 + range_y) && pos == 1)
		{
			flag = true;
		}
		//if (flag == false && (*i).x + dangrange_x == x && (y > (*i).y - 60 - range_y && y < (*i).y + 10 + range_y) && pos == 0)
		if (flag == false && (*i).x + dangrange_x + range_x >= x && (*i).x + dangrange_x - range_x <= x && (y > (*i).y - 40 - range_y && y < (*i).y + 20 + range_y) && pos == 0)
		{
			flag = true;
		}
	}
	for (auto i = shadailist.begin(); i != shadailist.end(); i++)
	{
		//if (flag == false && (*i).x == x && (y > (*i).y - 60 - range_y && y < (*i).y + 10 + range_y) && pos == 1)
		if (flag == false && (*i).x + range_x >= x && (*i).x - range_x <= x && (y > (*i).y - 20 - range_y && y < (*i).y + 20 + range_y) && pos == 1)
		{
			flag = true;
		}
		//if (flag == false && (*i).x + dangrange_x == x && (y > (*i).y - 60 - range_y && y < (*i).y + 10 + range_y) && pos == 0)
		if (flag == false && (*i).x + dangrange_x + range_x >= x && (*i).x + dangrange_x - range_x <= x && (y > (*i).y - 20 - range_y && y < (*i).y + 20 + range_y) && pos == 0)
		{
			flag = true;
		}
	}
	return flag;
}
void show() // 绘制函数
{
	gametime++;
	// if(roundnum == 0)
	putimage(0, 0, &im_bk1); // 显示背景
	// if (roundnum == 1)
	// putimage(0, 0, &im_bk2);
	// if (roundnum == 2)
	// putimage(0, 0, &im_bk3);
	for (auto i = zj.zidanlist.begin(); i != zj.zidanlist.end(); i++)
	{
		(*i).update();
		(*i).show();
	}
	for (auto i = zj.dazidanlist.begin(); i != zj.dazidanlist.end(); i++)
	{
		(*i).update();
		(*i).show();
	}
	for (auto i = qiangshoulist.begin(); i != qiangshoulist.end(); i++)
	{
		if (checkIsDangZhu((*i).x, (*i).y, (*i).pos) == false)
		(*i).update();
		(*i).show();
	}
	for (auto i = ldps.begin(); i != ldps.end(); i++)
	{
		if(checkIsDangZhu((*i).x,(*i).y,(*i).pos) == false)
		(*i).update();
		(*i).show();
	}
	for (auto i = fjlist.begin(); i != fjlist.end(); i++)
	{
		(*i).update();
		(*i).show();
	}
	for (auto i = dslist.begin(); i != dslist.end(); i++)
	{
		if (checkIsDangZhu((*i).x, (*i).y, (*i).pos) == false)
		(*i).update();
		(*i).show();
	}
	for (auto i = rsldlist.begin(); i != rsldlist.end(); i++)
	{
		if (checkIsDangZhu((*i).x, (*i).y, (*i).pos) == false)
		(*i).update();
		(*i).show();
	}
	if (is_feiji == false && (zj.vy < 0 || zj.y < groundy))
	{
		zj.y += zj.vy;
		zj.vy += my_g;
	}

	if (isBossShowed)
	{
		for (auto i = bosslist.begin(); i != bosslist.end(); i++)
		{
			(*i).show();
		}
		for (auto i = bossangerlist.begin(); i != bossangerlist.end(); i++)
		{
			(*i).show();
		}
		for (auto i = bossstandlist.begin(); i != bossstandlist.end(); i++)
		{
			(*i).show();
		}
		for (auto i = bossdielist.begin(); i != bossdielist.end(); i++)
		{
			(*i).show();
		}
	}
	
	for (auto i = die1manlist.begin(); i != die1manlist.end(); i++)
	{
		(*i).show();
	}
	for (auto i = die2manlist.begin(); i != die2manlist.end(); i++)
	{
		(*i).show();
	}
	// putToggleImagePng(100, 100, &liuDanBing[gametime % ldbNum]);
	if (is_feiji == false)
	{
		zj.draw();
	}
	// putimagePng(100, 100, &liuDanBing[gametime % ldbNum]);
	if (canIhaveFeiji == true) {
		myfj.show();
	}
	for (auto i = tongList.begin(); i != tongList.end(); i++)
	{
		(*i).show();
	}
	for (auto i = shadailist.begin(); i != shadailist.end(); i++)
	{
		(*i).show();
	}
	if (E.is_show == true)
	{
		E.show();
	}
	if (isRenzhiShowed)
	{
		for (auto i = oldmanbanglist.begin(); i != oldmanbanglist.end(); i++)
		{
			(*i).show();
		}
		for (auto i = oldmanfreelist.begin(); i != oldmanfreelist.end(); i++)
		{
			(*i).show();
		}
	}
	FlushBatchDraw(); // 批量绘制
	Sleep(100);		  // 暂停
}
bool isAllDied()
{
	if (qiangshoulist.size() == 0 && ldps.size() == 0 && fjlist.size() == 0 && dslist.size() == 0 && rsldlist.size() == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}
void addqiangshou(int n)
{
	for (int t = 0; t < n; t++)
	{
		qiangshoulist.push_back(qiangshou());
	}
}
void addrsld(int n)
{
	for (int t = 0; t < n; t++)
	{
		rsldlist.push_back(rengshouliudanshou());
	}
}
void addldps(int n)
{
	for (int t = 0; t < n; t++)
	{
		ldps.push_back(liudanpaoshou());
	}
}
void addfeiji(int n)
{
	for (int i = 0; i < n; i++)
	{
		fjlist.push_back(feiji());
	}
}
void adddaoshou(int n)
{
	for (int i = 0; i < n; i++)
	{
		dslist.push_back(daoshou());
	}
}
void round1()
{
	for (int t = 0; t < 1; t++)
	{
		qiangshoulist.push_back(qiangshou());
	}
	for (int t = 0; t < 0; t++)
	{
		rsldlist.push_back(rengshouliudanshou());
	}
	for (int t = 0; t < 1; t++)
	{
		ldps.push_back(liudanpaoshou());
	}

	for (int i = 0; i < 0; i++)
	{
		fjlist.push_back(feiji());
	}
	for (int i = 0; i < 1; i++)
	{
		dslist.push_back(daoshou());
	}
}
void round2()
{
	for (int t = 0; t < 1; t++)
	{
		qiangshoulist.push_back(qiangshou());
	}
	for (int t = 0; t < 2; t++)
	{
		rsldlist.push_back(rengshouliudanshou());
	}
	for (int t = 0; t < 0; t++)
	{
		ldps.push_back(liudanpaoshou());
	}

	for (int i = 0; i < 1; i++)
	{
		fjlist.push_back(feiji());
	}
	for (int i = 0; i < 0; i++)
	{
		dslist.push_back(daoshou());
	}
}
void round3()
{
	for (int t = 0; t < 1; t++)
	{
		qiangshoulist.push_back(qiangshou());
	}
	for (int t = 0; t < 1; t++)
	{
		rsldlist.push_back(rengshouliudanshou());
	}
	for (int t = 0; t < 1; t++)
	{
		ldps.push_back(liudanpaoshou());
	}

	for (int i = 0; i < 2; i++)
	{
		fjlist.push_back(feiji());
	}
	for (int i = 0; i < 1; i++)
	{
		dslist.push_back(daoshou());
	}
}
void addDiePerson(int x, int y)
{
	int tmp = rand() % 2;
	if (tmp == 0)
	{
		die1manlist.push_back(die1man(x, y+40));
	}
	else
	{
		die2manlist.push_back(die2man(x, y+40));
	}
}
bool checkZhangAiWu(int x, int y, int cost)
{
	bool flag = false;
	for (auto i = tongList.begin(); i != tongList.end(); i++)
	{
		if (flag == false && (*i).x - range_x < x && x < (*i).x + range_x && (y > (*i).y - range_y && y < (*i).y + range_y))
		{
			if ((*i).life)
			{
				(*i).life -= cost;
				flag = true;
			}
			else
			{
				tongList.erase(i);
			}
		}

	}
	for (auto i = shadailist.begin(); i != shadailist.end(); i++)
	{
		if (flag == false && (*i).x - range_x < x && x < (*i).x + range_x && (y > (*i).y - range_y && y < (*i).y + range_y))
		{
			flag = true;
		}
	}
	return flag;
}
void check()
{
	// check my zidan
	for (auto k = zj.dazidanlist.begin(); k != zj.dazidanlist.end(); k++)
	{
		int tmpx = (*k).x;
		int tmpy = (*k).y;
		bool t = checkZhangAiWu(tmpx, tmpy, dazidanshanghai);
		if (t == true)
		{
			zj.dazidanlist.erase(k);
		}
		else
		{
			bool flag = false;
			for (auto i = qiangshoulist.begin(); i != qiangshoulist.end() && flag == false; i++)
			{
				if ((tmpx > (*i).x - range_x && tmpx < (*i).x + range_x) && (tmpy > (*i).y - range_y && tmpy < (*i).y + range_y))
				{
					zj.dazidanlist.erase(k);
					flag = true;
					(*i).life -= dazidanshanghai;
					if ((*i).life <= 0)
					{
						addDiePerson((*i).x, (*i).y);
						qiangshoulist.erase(i);
					}
					break;
				}
			}

			for (auto i = ldps.begin(); i != ldps.end() && flag == false; i++)
			{
				if ((tmpx > (*i).x - range_x && tmpx < (*i).x + range_x) && (tmpy > (*i).y - range_y && tmpy < (*i).y + range_y))
				{
					zj.dazidanlist.erase(k);
					flag = true;
					(*i).life -= dazidanshanghai;
					if ((*i).life <= 0)
					{
						addDiePerson((*i).x, (*i).y);
						ldps.erase(i);
					}
					break;
				}
			}

			for (auto i = fjlist.begin(); i != fjlist.end() && flag == false; i++)
			{
				if ((tmpx > (*i).x - range_x && tmpx < (*i).x + range_x) && (tmpy > (*i).y - range_y && tmpy < (*i).y + range_y))
				{
					zj.dazidanlist.erase(k);
					flag = true;
					(*i).life -= dazidanshanghai;
					if ((*i).life <= 0)
					{
						fjlist.erase(i);
						//addDiePerson((*i).x, (*i).y);
					}
					break;
				}
			}

			for (auto i = dslist.begin(); i != dslist.end() && flag == false; i++)
			{
				if ((tmpx > (*i).x - range_x && tmpx < (*i).x + range_x) && (tmpy > (*i).y - range_y && tmpy < (*i).y + range_y))
				{
					zj.dazidanlist.erase(k);
					flag = true;
					(*i).life -= dazidanshanghai;
					if ((*i).life <= 0)
					{
						addDiePerson((*i).x, (*i).y);
						dslist.erase(i);
					}
					break;
				}
			}

			for (auto i = rsldlist.begin(); i != rsldlist.end() && flag == false; i++)
			{
				if ((tmpx > (*i).x - range_x && tmpx < (*i).x + range_x) && (tmpy > (*i).y - range_y && tmpy < (*i).y + range_y))
				{
					zj.dazidanlist.erase(k);
					flag = true;
					(*i).life -= dazidanshanghai;
					if ((*i).life <= 0)
					{
						addDiePerson((*i).x, (*i).y);
						rsldlist.erase(i);
					}
					break;
				}
			}
		}
	}
	for (auto k = zj.zidanlist.begin(); k != zj.zidanlist.end(); k++)
	{
		int tmpx = (*k).x;
		int tmpy = (*k).y;
		bool t = checkZhangAiWu(tmpx, tmpy, zidanshanghai);
		if (t == true)
		{
			zj.zidanlist.erase(k);
		}
		else
		{
			bool flag = false;
			for (auto i = qiangshoulist.begin(); i != qiangshoulist.end() && flag == false; i++)
			{
				if ((tmpx > (*i).x - range_x && tmpx < (*i).x + range_x) && (tmpy > (*i).y - range_y && tmpy < (*i).y + range_y))
				{
					zj.zidanlist.erase(k);
					flag = true;
					(*i).life -= zidanshanghai;
					if ((*i).life <= 0)
					{
						addDiePerson((*i).x, (*i).y);
						qiangshoulist.erase(i);
					}
					break;
				}
			}

			for (auto i = ldps.begin(); i != ldps.end() && flag == false; i++)
			{
				if ((tmpx > (*i).x - range_x && tmpx < (*i).x + range_x) && (tmpy > (*i).y - range_y && tmpy < (*i).y + range_y))
				{
					zj.zidanlist.erase(k);
					flag = true;
					(*i).life -= zidanshanghai;
					if ((*i).life <= 0)
					{
						addDiePerson((*i).x, (*i).y);
						ldps.erase(i);
					}
					break;
				}
			}

			for (auto i = fjlist.begin(); i != fjlist.end() && flag == false; i++)
			{
				if ((tmpx > (*i).x - range_x && tmpx < (*i).x + range_x) && (tmpy > (*i).y - range_y && tmpy < (*i).y + range_y))
				{
					zj.zidanlist.erase(k);
					flag = true;
					(*i).life -= zidanshanghai;
					if ((*i).life <= 0)
					{
						fjlist.erase(i);
					}
					break;
				}
			}

			for (auto i = dslist.begin(); i != dslist.end() && flag == false; i++)
			{
				if ((tmpx > (*i).x - range_x && tmpx < (*i).x + range_x) && (tmpy > (*i).y - range_y && tmpy < (*i).y + range_y))
				{
					zj.zidanlist.erase(k);
					flag = true;
					(*i).life -= zidanshanghai;
					if ((*i).life <= 0)
					{
						addDiePerson((*i).x, (*i).y);
						dslist.erase(i);
					}
					break;
				}
			}

			for (auto i = rsldlist.begin(); i != rsldlist.end() && flag == false; i++)
			{
				if ((tmpx > (*i).x - range_x && tmpx < (*i).x + range_x) && (tmpy > (*i).y - range_y && tmpy < (*i).y + range_y))
				{
					zj.zidanlist.erase(k);
					flag = true;
					(*i).life -= zidanshanghai;
					if ((*i).life <= 0)
					{
						addDiePerson((*i).x, (*i).y);
						rsldlist.erase(i);
					}
					break;
				}
			}
		}

	}
	// check xiaobingzidan
	for (auto i = qiangshoulist.begin(); i != qiangshoulist.end(); i++)
	{
		for (auto j = (*i).drzdlist.begin(); j != (*i).drzdlist.end(); j++)
		{
			if (checkZhangAiWu((*j).x, (*j).y, direnzidanshanghai) == false) {
				if ((*j).x > zj.x - range_x && (*j).x < zj.x + range_x && (*j).y > zj.y - range_y && (*j).y < zj.y + range_y)
				{
					(*i).drzdlist.erase(j);
					zj.life -= direnzidanshanghai;
				}
			}
			else
			{
				(*i).drzdlist.erase(j);
			}
		}
	}

	for (auto i = ldps.begin(); i != ldps.end(); i++)
	{
		for (auto j = (*i).drzdlist.begin(); j != (*i).drzdlist.end(); j++)
		{
			if (checkZhangAiWu((*j).x, (*j).y, direnliudanshanghai) == false) {
			if ((*j).x > zj.x - range_x && (*j).x < zj.x + range_x && (*j).y > zj.y - range_y && (*j).y < zj.y + range_y)
			{
				(*i).drzdlist.erase(j);
				zj.life -= direnliudanshanghai;
			}
			}
			else
			{
				(*i).drzdlist.erase(j);
			}
		}
	}

	for (auto i = fjlist.begin(); i != fjlist.end(); i++)
	{
		for (auto j = (*i).fjdlist.begin(); j != (*i).fjdlist.end(); j++)
		{
			if ((*j).x > zj.x - range_x && (*j).x < zj.x + range_x && (*j).y > zj.y - range_y && (*j).y < zj.y + range_y)
			{
				(*i).fjdlist.erase(j);
				zj.life -= feijidanshanghai;
			}
		}
	}
	for (auto i = dslist.begin(); i != dslist.end(); i++)
	{
		if ((*i).x > zj.x - range_x && (*i).x < zj.x + range_x && (*i).y > zj.y - range_y && (*i).y < zj.y + range_y)
		{
			if (daoshouTime <= 0)
			{
				zj.life -= daoshanghai;
				daoshouTime = DAOSHOUTIME;
			}
			else
			{
				daoshouTime--;
			}
		}
	}

	for (auto i = rsldlist.begin(); i != rsldlist.end(); i++)
	{
		for (auto j = (*i).sldlist.begin(); j != (*i).sldlist.end(); j++)
		{
			if (checkZhangAiWu((*j).x, (*j).y, shouliudanshanghai) == false) {
				if ((*j).x > zj.x - range_x && (*j).x < zj.x + range_x && (*j).y > zj.y - range_y && (*j).y < zj.y + range_y)
				{
					(*i).sldlist.erase(j);
					zj.life -= shouliudanshanghai;
				}
				else
				{
					(*i).sldlist.erase(j);
				}
			}
		}
	}
}
void bosscheck()
{
	if (isAllDied() && bossstandlist.size() > 0 && (bossstandlist.begin()->time) < 3)
	{
		int my_t = bossstandlist.begin()->time;
		my_t++;
		int mylife = bossstandlist.begin()->life;
		// bossangerlist.push_back(bossAnger(bs.x, bs.y, bs.time, bs.life));
		bossangerlist.push_back(bossAnger(my_t, mylife));
		bossstandlist.clear();
	}
	if (isAllDied() && bossstandlist.size() > 0 && (bossstandlist.begin()->time) >= 3)
	{
		isbossdied = true;
		bossdielist.push_back(bossdie());
		bossstandlist.clear();
	}
	if (isAllDied() && bossangerlist.size() > 0 && bossangerlist.begin()->imgnum >= bossAngerNum * bosstimes)
	{
		list<bossAnger>::iterator bsag = bossangerlist.begin();
		// bossstandlist.push_back(bossstand(bsag.x,bsag.y, bsag.time, bsag.life));
		int xx = bsag->x;
		int yy = bsag->y;
		int tt = bsag->time;
		int ll = bsag->life;
		bosslist.push_back(boss(xx, yy, tt, ll));
		if (bsag->time == 1)
		{
			round1();
		}
		else if (bsag->time == 2)
		{
			round2();
		}
		else if (bsag->time == 3)
		{
			round3();
		}
		bossangerlist.clear();
	}
	if (bosslist.size() > 0 && bosslist.begin()->imgnum >= bossNum * bosstimes)
	{
		list<boss>::iterator bsag = (bosslist.begin());
		int xx = bsag->x;
		int yy = bsag->y;
		int tt = bsag->time;
		int ll = bsag->life;
		bossstandlist.push_back(bossstand(xx, yy, tt, ll));
		bosslist.clear();
	}
}
void checkLetterE()
{
	if (canIhaveFeiji == true && zj.x > myfj.x - range_x && zj.x < myfj.x + range_x && is_feiji == false)
	{
		E.isshow();
		E.change(myfj.x);
	}
	else if(zj.x > oldmanbanglist.begin()->x - range_x && zj.x < oldmanbanglist.begin()->x + range_x && isAllDied() && isbossdied == true)
	{
		E.isshow();
		E.change(oldmanbanglist.begin()->x);
	}
	else
	{
		E.isnotshow();
	}
	
}
void IsPressKey()
{

	if (_kbhit()) //	If a key is pressed
	{
		char key;
		key = _getch(); //	Obtain key info
		if (is_feiji == false)
		{

			if (GetAsyncKeyState('W'))
			{
				if (zj.y >= groundy)
				{
					zj.vy = -jump_v;
					zj.changexy(zj.x, zj.y);
					int tmppos = zj.pos;
					// pos: 0 is left, 1 is right, 2 is left jump,  3 is right jump, 4 is left down, 5 is right down, 6 is left stand, 7 is right stand
					if (tmppos == 0 || tmppos == 2 || tmppos == 4 || tmppos == 6)
					{
						zj.pos = 2;
					}
					else
					{
						zj.pos = 3;
					}
				}
			}
			if (GetAsyncKeyState('S'))
			{
				zj.changexy(zj.x, groundy + 30);
				int tmppos = zj.pos;
				// pos: 0 is left, 1 is right, 2 is left jump,  3 is right jump, 4 is left down, 5 is right down, 6 is left stand, 7 is right stand
				if (tmppos == 0 || tmppos == 2 || tmppos == 4 || tmppos == 6)
				{
					zj.pos = 4;
				}
				else
				{
					zj.pos = 5;
				}
		
			}

			if (GetAsyncKeyState('A') && zj.x > 0)
			{
				if( checkIsDangZhu(zj.x,zj.y,0) == false)
				zj.changexy(zj.x - v, zj.y);
				// pos: 0 is left, 1 is right, 2 is left jump,  3 is right jump, 4 is left down, 5 is right down, 6 is left stand, 7 is right stand
				zj.pos = 0;
			}
			if (GetAsyncKeyState('D') && zj.x < WIDTH - range_x)
			{
				if (checkIsDangZhu(zj.x, zj.y, 1) == false)
				zj.changexy(zj.x + v, zj.y);
				// pos: 0 is left, 1 is right, 2 is left jump,  3 is right jump, 4 is left down, 5 is right down, 6 is left stand, 7 is right stand
				zj.pos = 1;
			}
			if (GetAsyncKeyState('1'))
			{
				// pos: 0 is left, 1 is right, 2 is left jump,  3 is right jump, 4 is left down, 5 is right down, 6 is left stand, 7 is right stand
				zj.wq = 0;
			}
			if (GetAsyncKeyState('2'))
			{
				// pos: 0 is left, 1 is right, 2 is left jump,  3 is right jump, 4 is left down, 5 is right down, 6 is left stand, 7 is right stand
				zj.wq = 1;
			}
			if (GetAsyncKeyState('E') && zj.x > oldmanbanglist.begin()->x - range_x && zj.x < oldmanbanglist.begin()->x + range_x && isAllDied() && isbossdied == true)
			{
				oldmanfreelist.push_back(oldmanfree());
				oldmanbanglist.clear();
				if (changeFlag == false)
				{
					roundnum++;
					changeFlag = true;
				}
			}
			if ( canIhaveFeiji == true && GetAsyncKeyState('E') && zj.x > myfj.x - range_x && zj.x < myfj.x + range_x)
			{
				is_feiji = true;
			}
		}
		else
		{
			if (GetAsyncKeyState('W'))
			{
				zj.y = zj.y - myfeiji_vy;
				myfj.y -= myfeiji_vy;
			}
			if (GetAsyncKeyState('S'))
			{
				zj.y = zj.y + myfeiji_vy;
				myfj.y += myfeiji_vy;
			}
			if (GetAsyncKeyState('A'))
			{
				zj.x = zj.x - myfeiji_vx;
				myfj.x -= myfeiji_vx;
				myfj.pos = 1;
				zj.pos = 0;
			}
			if (GetAsyncKeyState('D'))
			{
				zj.x = zj.x + myfeiji_vx;
				myfj.x += myfeiji_vx;
				myfj.pos = 0;
				zj.pos = 1;
			}
			if (GetAsyncKeyState('1'))
			{
				// pos: 0 is left, 1 is right, 2 is left jump,  3 is right jump, 4 is left down, 5 is right down, 6 is left stand, 7 is right stand
				zj.wq = 0;
			}
			if (GetAsyncKeyState('2'))
			{
				// pos: 0 is left, 1 is right, 2 is left jump,  3 is right jump, 4 is left down, 5 is right down, 6 is left stand, 7 is right stand
				zj.wq = 1;
			}
			if (GetAsyncKeyState('E'))
			{
				is_feiji = false;
				myfj.y = groundy;
				zj.y = groundy;
			}
		}
	}
	MOUSEMSG m;		   // 定义鼠标消息
	while (MouseHit()) // 检测当前是否有鼠标消息
	{
		m = GetMouseMsg();
		auto theta = atan2(m.y - zj.y - 20, m.x - zj.x + 10);
		if (zj.pos == 0)
		{
			zidan_vx = int(zidan_v * cos(-theta));
			zidan_vy = int(zidan_v * sin(theta));
		}
		else
		{
			zidan_vx = int(zidan_v * cos(-theta));
			zidan_vy = int(zidan_v * sin(theta));
		}
	}
	if (GetAsyncKeyState(VK_LBUTTON))
	{
		// m = GetMouseMsg();
		zj.shot = 1;
		int x = zj.pos % 2 == 0 ? 0 : 1;
		// auto theta = atan2(m.y - zj.y - 20, m.x - zj.x + 10);
		// zidan_vx = int(zidan_v * sin(theta));
		// zidan_vy = int(zidan_v * cos(theta));
		if (zj.wq == 0)
		{
			if (x == 0)
			{
				zj.zidanlist.push_back(Zidan(zj.x - 10, zj.y + 20, 0, -zidan_vx, zidan_vy));
			}
			else
			{
				zj.zidanlist.push_back(Zidan(zj.x + 10, zj.y + 20, 1, zidan_vx, zidan_vy));
			}
		}
		else
		{
			if (x == 0)
			{
				zj.dazidanlist.push_back(DaZidan(zj.x - 10, zj.y + 20, 0, -zidan_vx, zidan_vy));
			}
			else
			{
				zj.dazidanlist.push_back(DaZidan(zj.x + 10, zj.y + 20, 1, zidan_vx, zidan_vy));
			}
		}
	}
}
void xiaobingCheck()
{
	if (isAllDied() && true)
	{
		roundtruekillnum++;
		if (roundtruekillnum >= roundkillnum)
		{
			isBossShowed = true;
			isRenzhiShowed = true;
		}
		int tmp = rand() % 4;
		if (tmp == 0)
		{
			adddaoshou(rand() % 3);
		}
		else if (tmp == 1)
		{
			addqiangshou(rand() % 3);
		}
		else if (tmp == 2)
		{
			addldps(rand() % 3);
		}
		else if (tmp == 3)
		{
			addrsld(rand() % 3);
		}
	}
	else if (isAllDied() && false)
	{
		roundtruekillnum++;
		if (roundtruekillnum >= roundkillnum)
		{
			isBossShowed = true;
			isRenzhiShowed = true;
		}
		int tmp = rand() % 5;
		if (tmp == 0)
		{
			adddaoshou(rand() % 3);
		}
		else if (tmp == 1)
		{
			addqiangshou(rand() % 3);
		}
		else if (tmp == 2)
		{
			addldps(rand() % 3);
		}
		else if (tmp == 3)
		{
			addrsld(rand() % 3);
		}
		else if (tmp == 4)
		{
			addfeiji(rand() % 2);
		}
	}
}
int main()
{
	calScreenSize();
	startup(); // 初始化
	beforeshow();
	mciSendString(_T("open Market.mp3 alias bkmusic"), NULL, 0, NULL);
	mciSendString(_T("play bkmusic repeat"), NULL, 0, NULL);
	// roundtruekillnum = 0;
	////for (int i = 0; i < 1; i++)
	//{
	///	bossstandlist.push_back(bossstand());
	//}
	// while (roundnum < roundNum) {
	// isBossShowed = false;
	// isRenzhiShowed = false;
	// changeFlag = false;
	while (zj.life > 0 && finshedtime < 20) // 重复运行
	{
		if (changeFlag == true)
		{
			finshedtime++;
		}
		IsPressKey();
		check();
		if (isBossShowed == true)
			bosscheck();
		else
		{
			if (stoptime > 0)
			{
				stoptime--;
			}
			else
			{
				stoptime = stopTime;
				xiaobingCheck();
			}
		}
		checkLetterE();
		show(); // 绘制
		zj.shot = 0;
		int tmppos = zj.pos;
		if (zj.y >= groundy && is_feiji == false)
		{
			zj.changexy(zj.x, groundy);
			// pos: 0 is left, 1 is right, 2 is left jump,  3 is right jump, 4 is left down, 5 is right down, 6 is left stand, 7 is right stand
			if ((tmppos == 0 || tmppos == 2 || tmppos == 4 || tmppos == 6))
			{
				zj.pos = 6;
			}
			else
			{
				zj.pos = 7;
			}
		}
	}

	//	if (zj.life > 0) {
	//		zj.life = zj.life + 0.5 * MaxLife;
	//	if (zj.life > MaxLife)
	//		{
	//		zj.life = MaxLife;
	//	}
	//	}
	// else
	//	{
	//	break;
	// }
	//}
	// if (zj.life > 0 && roundnum >= roundNum)
	while (1)
	{
		if (zj.life > 0)
		{
			putimagePng(WIDTH * 0.4, HEIGHT * 0.4, &WIN);
			FlushBatchDraw(); // 批量绘制
			Sleep(100);		  // 暂停
		}
		else
		{
			putimage(WIDTH * 0.4, HEIGHT * 0.4, &FAIL);
			FlushBatchDraw(); // 批量绘制
			Sleep(100);		  // 暂停
		}
	}
	_getch();
	return 0;
}