#include "common.h"
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <math.h>

static int LCD_FB_FD;
static int *LCD_FB_BUF = NULL;
static int DRAW_BUF[SCREEN_WIDTH*SCREEN_HEIGHT];

static struct area {
	int x1, x2, y1, y2;
} update_area = {0,0,0,0};

#define AREA_SET_EMPTY(pa) do {\
	(pa)->x1 = SCREEN_WIDTH;\
	(pa)->x2 = 0;\
	(pa)->y1 = SCREEN_HEIGHT;\
	(pa)->y2 = 0;\
} while(0)

inline void swap(int *x, int *y) {
	int tmp = *x;
	*x = *y;
	*y = tmp;
}

void fb_init(char *dev)
{
	int fd;
	struct fb_fix_screeninfo fb_fix;
	struct fb_var_screeninfo fb_var;

	if(LCD_FB_BUF != NULL) return; /*already done*/

	//进入终端图形模式
	fd = open("/dev/tty0",O_RDWR,0);
	ioctl(fd, KDSETMODE, KD_GRAPHICS);
	close(fd);

	//First: Open the device
	if((fd = open(dev, O_RDWR)) < 0){
		printf("Unable to open framebuffer %s, errno = %d\n", dev, errno);
		return;
	}
	if(ioctl(fd, FBIOGET_FSCREENINFO, &fb_fix) < 0){
		printf("Unable to FBIOGET_FSCREENINFO %s\n", dev);
		return;
	}
	if(ioctl(fd, FBIOGET_VSCREENINFO, &fb_var) < 0){
		printf("Unable to FBIOGET_VSCREENINFO %s\n", dev);
		return;
	}

	printf("framebuffer info: bits_per_pixel=%u,size=(%d,%d),virtual_pos_size=(%d,%d)(%d,%d),line_length=%u,smem_len=%u\n",
		fb_var.bits_per_pixel, fb_var.xres, fb_var.yres, fb_var.xoffset, fb_var.yoffset,
		fb_var.xres_virtual, fb_var.yres_virtual, fb_fix.line_length, fb_fix.smem_len);

	//Second: mmap
	void *addr = mmap(NULL, fb_fix.smem_len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if(addr == (void *)-1){
		printf("failed to mmap memory for framebuffer.\n");
		return;
	}

	if((fb_var.xoffset != 0) ||(fb_var.yoffset != 0))
	{
		fb_var.xoffset = 0;
		fb_var.yoffset = 0;
		if(ioctl(fd, FBIOPAN_DISPLAY, &fb_var) < 0) {
			printf("FBIOPAN_DISPLAY framebuffer failed\n");
		}
	}

	LCD_FB_FD = fd;
	LCD_FB_BUF = addr;

	//set empty
	AREA_SET_EMPTY(&update_area);
	return;
}

static void _copy_area(int *dst, int *src, struct area *pa)
{
	int x, y, w, h;
	x = pa->x1; w = pa->x2-x;
	y = pa->y1; h = pa->y2-y;
	src += y*SCREEN_WIDTH + x;
	dst += y*SCREEN_WIDTH + x;
	while(h-- > 0){
		memcpy(dst, src, w*4);
		src += SCREEN_WIDTH;
		dst += SCREEN_WIDTH;
	}
}

static int _check_area(struct area *pa)
{
	if(pa->x2 == 0) return 0; //is empty

	if(pa->x1 < 0) pa->x1 = 0;
	if(pa->x2 > SCREEN_WIDTH) pa->x2 = SCREEN_WIDTH;
	if(pa->y1 < 0) pa->y1 = 0;
	if(pa->y2 > SCREEN_HEIGHT) pa->y2 = SCREEN_HEIGHT;

	if((pa->x2 > pa->x1) && (pa->y2 > pa->y1))
		return 1; //no empty

	//set empty
	AREA_SET_EMPTY(pa);
	return 0;
}

void fb_update(void)
{
	if(_check_area(&update_area) == 0) return; //is empty
	_copy_area(LCD_FB_BUF, DRAW_BUF, &update_area);
	AREA_SET_EMPTY(&update_area); //set empty
	return;
}

/*======================================================================*/

static void * _begin_draw(int x, int y, int w, int h)
{
	int x2 = x+w;
	int y2 = y+h;
	if(update_area.x1 > x) update_area.x1 = x;
	if(update_area.y1 > y) update_area.y1 = y;
	if(update_area.x2 < x2) update_area.x2 = x2;
	if(update_area.y2 < y2) update_area.y2 = y2;
	return DRAW_BUF;
}

void fb_draw_pixel(int x, int y, int color)
{
	if(x<0 || y<0 || x>=SCREEN_WIDTH || y>=SCREEN_HEIGHT) return;
	int *buf = _begin_draw(x,y,1,1);
/*---------------------------------------------------*/
	*(buf + y*SCREEN_WIDTH + x) = color;
/*---------------------------------------------------*/
	return;
}

void fb_draw_rect(int x, int y, int w, int h, int color)
{
	if(x < 0) { w += x; x = 0;}
	if(x+w > SCREEN_WIDTH) { w = SCREEN_WIDTH-x;}
	if(y < 0) { h += y; y = 0;}
	if(y+h >SCREEN_HEIGHT) { h = SCREEN_HEIGHT-y;}
	if(w<=0 || h<=0) return;
	int *buf = _begin_draw(x,y,w,h);
/*---------------------------------------------------*/
	// printf("you need implement fb_draw_rect()\n"); exit(0);
	//buf += y  * SCREEN_WIDTH;
	for(register int i = 0;i < h;++ i) {
		for(register int j = 0;j < w;++ j) {
			*(buf + (i + y)*SCREEN_WIDTH + (j + x)) = color;
		}
		// memset(buf + x, color, w);
		// buf += SCREEN_WIDTH;
	}
/*---------------------------------------------------*/
	return;
}

void fb_draw_line(int x1, int y1, int x2, int y2, int color)
{
	int *buf = _begin_draw(x1, y1, x2 - x1, y2 - y1);
/*---------------------------------------------------*/
	//printf("you need implement fb_draw_line()\n"); exit(0);
	if (fabs(x2 - x1) >= fabs(y2 - y1)) { // 在y方向上变化不明显
		if (x1 > x2) {
			swap(&x1, &x2);
			swap(&y1, &y2);
		}
		if (x2 == x1) return;
		double k = 1.0 * (y2 - y1) / (x2 - x1);
		
		double y_now = k * (fmax(0, x1) - x1) + y1;
		for(register int i = fmax(0, x1);i <= fmin(x2, SCREEN_WIDTH-1) && y_now < SCREEN_HEIGHT;++ i) {
			//printf("!%d %d? %d\n", i, (int)round(y_now), x2);
		//	fb_draw_pixel(i, (int)round(y_now), color);
			if (y_now >= 0)
				*(buf + (i + (int)round(y_now) * SCREEN_WIDTH)) = color;
			y_now += k;
		} 
	}
	else { // 在x方向上变化不明显
		if (y1 > y2) {
			swap(&y1, &y2);
			swap(&x1, &x2);
		}
		double k = 1.0 * (x2 - x1) / (y2 - y1);
		double x_now = k * (fmax(0, y1) - y1) + x1;
		for(register int i = fmax(0, y1);i <= fmin(y2, SCREEN_HEIGHT-1) && x_now < SCREEN_WIDTH;++ i) {
			//printf("?%d %lf\n", i, x_now);
		//	fb_draw_pixel((int)round(x_now), i, color);
			if (x_now >= 0)
				*(buf + ((int)round(x_now) + i * SCREEN_WIDTH)) = color;
			x_now += k;
		}
	}
/*---------------------------------------------------*/
	return;
}

inline void _8888_TO_0888(char *dst, char *src, int alpha) {
	//char alpha = *(src + 3), R1 = *(src + 2), G1 = *(src + 1), B1 = *src;
	if (alpha == -1) alpha = src[3];
	//else printf("%d\n", src[0]);
	switch(alpha) {
		case 0:break;
		case 255:
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			break;
		default:
			dst[0] += (((src[0] - dst[0]) * alpha) >> 8);
			dst[1] += (((src[1] - dst[1]) * alpha) >> 8);
			dst[2] += (((src[2] - dst[2]) * alpha) >> 8);
	}
	/*
	switch(alpha) {
		case 0: break;
		case 255:
			p[0] = B1;
			p[1] = G1;
			p[2] = R1;
			break;
		default:
			p[0] += (((B1 - p[0]) * alpha) >> 8);
			p[1] += (((G1 - p[1]) * alpha) >> 8);
			p[2] += (((R1 - p[2]) * alpha) >> 8);
	}
	*/
}

void fb_draw_image(int x, int y, fb_image *image, int color)
{
	if(image == NULL) return;

	int ix = 0; //image x
	int iy = 0; //image y
	int w = image->pixel_w; //draw width
	int h = image->pixel_h; //draw height
	int offsetx = 0;
	int offsety = 0;

	if(x<0) {offsetx = x; w+=x; ix-=x; x=0;}
	if(y<0) {offsety = y; h+=y; iy-=y; y=0;}

	if(x+w > SCREEN_WIDTH) {
		w = SCREEN_WIDTH - x;
	}
	if(y+h > SCREEN_HEIGHT) {
		h = SCREEN_HEIGHT - y;
	}
	if((w <= 0)||(h <= 0)) return;

	int *buf = _begin_draw(x,y,w,h);
/*---------------------------------------------------------------*/
	char *dst = (char *)(buf + y*SCREEN_WIDTH + x);
	char *src; //不同的图像颜色格式定位不同
/*---------------------------------------------------------------*/

	//int alpha;
	int ww = image->line_byte;
	// printf("%d %d\n", w, h);
	if(image->color_type == FB_COLOR_RGB_8880) /*lab3: jpg*/
	{
		//printf("you need implement fb_draw_image() FB_COLOR_RGB_8880\n"); exit(0);
		src = image->content + ww * (-offsety);
		for(register int j = 0;j < h;++ j) { // h = image_h + offsety(仅当y<0时)
			//int h_bias1 = (y + j) * SCREEN_WIDTH, h_bias2 = j * (ww >> 2);
			memcpy(dst, src + (ww - (offsetx << 2)), w << 2);
			// for(register int i = 0;i < w;++ i) {
			// 	*(buf + (x + i) + h_bias1) = *((int *)image->content + i + h_bias2);
			// }
			dst += SCREEN_WIDTH << 2;
			src += ww;
		}
		return;
	}
	else if(image->color_type == FB_COLOR_RGBA_8888) /*lab3: png*/
	{
		//printf("you need implement fb_draw_image() FB_COLOR_RGBA_8888\n"); exit(0);
		for(register int j = 0;j < h;++ j) {
			src = (image->content + j * ww);
			dst = (char *)(buf + (y + j) * SCREEN_WIDTH + x);
			for(register int i = 0;i < w;++ i) {
				_8888_TO_0888(dst, src, -1);
				src += 4;
				dst += 4;
			}
		}
		return;
	}
	else if(image->color_type == FB_COLOR_ALPHA_8) /*lab3: font*/
	{
		//printf("you need implement fb_draw_image() FB_COLOR_ALPHA_8\n"); exit(0);
		for (register int j = 0;j < h;++ j) {
			src = (image->content + j * ww);
			dst = (char *)(buf + (y + j) * SCREEN_WIDTH + x);
			for(register int i = 0;i < w;++ i) {
				int tmp = color;
				_8888_TO_0888(dst, (char *)&tmp, *src);
				src ++;
				dst += 4;
			}
		}
		return;
	}
/*---------------------------------------------------------------*/
	return;
}

inline int max(int x, int y) {
	return x > y ? x : y;
}

inline int min(int x, int y) {
	return x < y ? x : y;
}

void fb_draw_jpg(int x, int y, fb_image *image, double scaling, int direction)
{
	if(image == NULL) return;
	// int ix = 0; //image x
	// int iy = 0; //image y
	int w = (int)image->pixel_w * scaling; //draw width
	int h = (int)image->pixel_h * scaling; //draw height
	if (direction == _Vertical_90 || direction == _Vertical_n90) {
		int temp = w; w = h; h = temp;
	}
	int offsetx = 0; // <= 0
	int offsety = 0; // <= 0

	if(x<0) {offsetx = x; w+=x; /*ix-=x;*/ x=0;}
	if(y<0) {offsety = y; h+=y; /*iy-=y;*/ y=0;}

	if(x+w > SCREEN_WIDTH) {
		w = SCREEN_WIDTH - x;
	}
	if(y+h > SCREEN_HEIGHT) {
		h = SCREEN_HEIGHT - y;
	}
	if((w <= 0)||(h <= 0)) return;

	int *buf = _begin_draw(x,y,w,h);
	char *dst = (char *)(buf + y*SCREEN_WIDTH + x);
/*---------------------------------------------------------------*/
	int ww = image->line_byte;
	
	if(image->color_type == FB_COLOR_RGB_8880) /*lab3: jpg*/
	{
		if (direction == _Horizontal) {
			for(register int j = 0;j < h;++ j) {
				double projected_j = 1.0 * (j - offsety) / scaling;
				int downj = (int)projected_j;
				int upj = min(downj + 1, image->pixel_h - 1);
				char *src1 = image->content + ww * downj;
				char *src2 = image->content + ww * upj;

				double projected_i = 1.0 * (0 - offsetx) / scaling;
				double delta_i = 1.0 / scaling;
				for(register int i = 0;i < w;++ i) {// ww - 1 to ww - w ;--i
					
					int downi = (int)projected_i;
					int upi = min(downi + 1, image->pixel_w - 1);
					//printf("%d %d %.2lf %.2lf %d %d %d %d\n", j, i, projected_j, projected_i, downi, downj, upi, upj);

					*(dst + (i << 2)) = (char)((int)*(src1 + (downi << 2)) * (projected_i - downi) * (projected_j - downj) + 
							(int)*(src1 + (upi << 2)) * (upi - projected_i) * (projected_j - downj) + 
							(int)*(src2 + (downi << 2)) * (projected_i - downi) * (upj - projected_j) + 
							(int)*(src2 + (upi << 2)) * (upi - projected_i) * (upj - projected_j) + 0.5);
					//printf("%c", (char)((int)(*(src1 + (downi << 2))) * (projected_i - downi) * (projected_j - downj) ));
					*(dst + (i << 2) + 1) = (char)((int)*(src1 + (downi << 2) + 1) * (projected_i - downi) * (projected_j - downj) + 
							(int)*(src1 + (upi << 2) + 1) * (upi - projected_i) * (projected_j - downj) + 
							(int)*(src2 + (downi << 2) + 1) * (projected_i - downi) * (upj - projected_j) + 
							(int)*(src2 + (upi << 2) + 1) * (upi - projected_i) * (upj - projected_j) + 0.5);

					*(dst + (i << 2) + 2) = (char)((int)*(src1 + (downi << 2) + 2) * (projected_i - downi) * (projected_j - downj) + 
							(int)*(src1 + (upi << 2) + 2) * (upi - projected_i) * (projected_j - downj) + 
							(int)*(src2 + (downi << 2) + 2) * (projected_i - downi) * (upj - projected_j) + 
							(int)*(src2 + (upi << 2) + 2) * (upi - projected_i) * (upj - projected_j) + 0.5);
					projected_i += delta_i;
				}
				dst += SCREEN_WIDTH << 2;
			}
		}
		if (direction == _Vertical_90) {
			for(register int j = 0;j < w;++ j) {
				double projected_j = 1.0 * (j - offsetx) / scaling;
				int downj = (int)projected_j;
				int upj = min(downj + 1, image->pixel_h - 1);
				char *src1 = image->content + ww * downj;
				char *src2 = image->content + ww * upj;

				double projected_i = 1.0 * ((image->pixel_w - 1) * scaling + offsety ) / scaling;
				double delta_i = 1.0 / scaling;

				for(register int i = 0;i < h;++ i) {
					
					int downi = max((int)projected_i, 0);
					int upi = min(downi + 1, image->pixel_w - 1);
					//printf("%d %d %.2lf %.2lf %d %d %d %d\n", j, i, projected_j, projected_i, downi, downj, upi, upj);

					*(dst + i * (SCREEN_WIDTH << 2)) = (char)((int)*(src1 + (downi << 2)) * (projected_i - downi) * (projected_j - downj) + 
							(int)*(src1 + (upi << 2)) * (upi - projected_i) * (projected_j - downj) + 
							(int)*(src2 + (downi << 2)) * (projected_i - downi) * (upj - projected_j) + 
							(int)*(src2 + (upi << 2)) * (upi - projected_i) * (upj - projected_j) + 0.5);
					//printf("%c", (char)((int)(*(src1 + (downi << 2))) * (projected_i - downi) * (projected_j - downj) ));
					*(dst + i * (SCREEN_WIDTH << 2) + 1) = (char)((int)*(src1 + (downi << 2) + 1) * (projected_i - downi) * (projected_j - downj) + 
							(int)*(src1 + (upi << 2) + 1) * (upi - projected_i) * (projected_j - downj) + 
							(int)*(src2 + (downi << 2) + 1) * (projected_i - downi) * (upj - projected_j) + 
							(int)*(src2 + (upi << 2) + 1) * (upi - projected_i) * (upj - projected_j) + 0.5);

					*(dst + i * (SCREEN_WIDTH << 2) + 2) = (char)((int)*(src1 + (downi << 2) + 2) * (projected_i - downi) * (projected_j - downj) + 
							(int)*(src1 + (upi << 2) + 2) * (upi - projected_i) * (projected_j - downj) + 
							(int)*(src2 + (downi << 2) + 2) * (projected_i - downi) * (upj - projected_j) + 
							(int)*(src2 + (upi << 2) + 2) * (upi - projected_i) * (upj - projected_j) + 0.5);
					projected_i -= delta_i;
				}
				dst += 4;
				//dst += SCREEN_WIDTH << 2;
			}
		}
		if (direction == _Vertical_n90) {
			for(register int j = 0;j < w;++ j) {
				double projected_j = 1.0 * ((image->pixel_h - 1) * scaling - j + offsetx) / scaling;
				int downj = max((int)projected_j, 0);
				int upj = min(downj + 1, image->pixel_h - 1);
				char *src1 = image->content + ww * downj;
				char *src2 = image->content + ww * upj;

				double projected_i = 1.0 * (0 - offsety) / scaling;
				double delta_i = 1.0 / scaling;

				for(register int i = 0;i < h;++ i) {
					
					int downi = (int)projected_i;
					int upi = min(downi + 1, image->pixel_w - 1);
					//printf("%d %d %.2lf %.2lf %d %d %d %d\n", j, i, projected_j, projected_i, downi, downj, upi, upj);

					*(dst + i * (SCREEN_WIDTH << 2)) = (char)((int)*(src1 + (downi << 2)) * (projected_i - downi) * (projected_j - downj) + 
							(int)*(src1 + (upi << 2)) * (upi - projected_i) * (projected_j - downj) + 
							(int)*(src2 + (downi << 2)) * (projected_i - downi) * (upj - projected_j) + 
							(int)*(src2 + (upi << 2)) * (upi - projected_i) * (upj - projected_j) + 0.5);
					//printf("%c", (char)((int)(*(src1 + (downi << 2))) * (projected_i - downi) * (projected_j - downj) ));
					*(dst + i * (SCREEN_WIDTH << 2) + 1) = (char)((int)*(src1 + (downi << 2) + 1) * (projected_i - downi) * (projected_j - downj) + 
							(int)*(src1 + (upi << 2) + 1) * (upi - projected_i) * (projected_j - downj) + 
							(int)*(src2 + (downi << 2) + 1) * (projected_i - downi) * (upj - projected_j) + 
							(int)*(src2 + (upi << 2) + 1) * (upi - projected_i) * (upj - projected_j) + 0.5);

					*(dst + i * (SCREEN_WIDTH << 2) + 2) = (char)((int)*(src1 + (downi << 2) + 2) * (projected_i - downi) * (projected_j - downj) + 
							(int)*(src1 + (upi << 2) + 2) * (upi - projected_i) * (projected_j - downj) + 
							(int)*(src2 + (downi << 2) + 2) * (projected_i - downi) * (upj - projected_j) + 
							(int)*(src2 + (upi << 2) + 2) * (upi - projected_i) * (upj - projected_j) + 0.5);
					projected_i += delta_i;
				}
				dst += 4;
				//dst += SCREEN_WIDTH << 2;
			}
		}
		if (direction == _Horizontal_neg) {
			for(register int j = 0;j < h;++ j) {
				double projected_j = 1.0 * ((image->pixel_h - 1) * scaling - j + offsety) / scaling;
				int downj = max((int)projected_j, 0);
				int upj = min(downj + 1, image->pixel_h - 1);
				char *src1 = image->content + ww * downj;
				char *src2 = image->content + ww * upj;

				double projected_i = 1.0 * ((image->pixel_w - 1) * scaling + offsetx ) / scaling;
				double delta_i = 1.0 / scaling;

				for(register int i = 0;i < w;++ i) {
					
					int downi = max((int)projected_i, 0);
					int upi = min(downi + 1, image->pixel_w - 1);
					//printf("%d %d %.2lf %.2lf %d %d %d %d\n", j, i, projected_j, projected_i, downi, downj, upi, upj);

					*(dst + (i << 2)) = (char)((int)*(src1 + (downi << 2)) * (projected_i - downi) * (projected_j - downj) + 
							(int)*(src1 + (upi << 2)) * (upi - projected_i) * (projected_j - downj) + 
							(int)*(src2 + (downi << 2)) * (projected_i - downi) * (upj - projected_j) + 
							(int)*(src2 + (upi << 2)) * (upi - projected_i) * (upj - projected_j) + 0.5);
					//printf("%c", (char)((int)(*(src1 + (downi << 2))) * (projected_i - downi) * (projected_j - downj) ));
					*(dst + (i << 2) + 1) = (char)((int)*(src1 + (downi << 2) + 1) * (projected_i - downi) * (projected_j - downj) + 
							(int)*(src1 + (upi << 2) + 1) * (upi - projected_i) * (projected_j - downj) + 
							(int)*(src2 + (downi << 2) + 1) * (projected_i - downi) * (upj - projected_j) + 
							(int)*(src2 + (upi << 2) + 1) * (upi - projected_i) * (upj - projected_j) + 0.5);

					*(dst + (i << 2) + 2) = (char)((int)*(src1 + (downi << 2) + 2) * (projected_i - downi) * (projected_j - downj) + 
							(int)*(src1 + (upi << 2) + 2) * (upi - projected_i) * (projected_j - downj) + 
							(int)*(src2 + (downi << 2) + 2) * (projected_i - downi) * (upj - projected_j) + 
							(int)*(src2 + (upi << 2) + 2) * (upi - projected_i) * (upj - projected_j) + 0.5);
					projected_i -= delta_i;
				}
				dst += SCREEN_WIDTH << 2;
			}
		}
	}
/*---------------------------------------------------------------*/
	return;
}

void fb_draw_border(int x, int y, int w, int h, int color)
{
	if(w<=0 || h<=0) return;
	fb_draw_rect(x, y, w, 1, color);
	if(h > 1) {
		fb_draw_rect(x, y+h-1, w, 1, color);
		fb_draw_rect(x, y+1, 1, h-2, color);
		if(w > 1) fb_draw_rect(x+w-1, y+1, 1, h-2, color);
	}
}

/** draw a text string **/
void fb_draw_text(int x, int y, char *text, int font_size, int color)
{
	fb_image *img;
	fb_font_info info;
	int i=0;
	int len = strlen(text);
	while(i < len)
	{
		img = fb_read_font_image(text+i, font_size, &info);
		if(img == NULL) break;
		fb_draw_image(x+info.left, y-info.top, img, color);
		fb_free_image(img);

		x += info.advance_x;
		i += info.bytes;
	}
	return;
}

void fb_draw_circle(int x, int y, int radius, int color)
{
	int *buf = _begin_draw(max(0, x - radius), max(0, y - radius), 2*radius, 2*radius);

	printf("x:%d y:%d d:%d r:%d\n",max(0, x - radius), max(0, y - radius), 2*radius, radius);

	int r2 = radius * radius;
	buf += y * SCREEN_WIDTH;
	for (int i = 0, i2 = 0; i < radius; ++ i) {
		int len = round(sqrt(r2 - i2));
		int stx = max(0, x - len);
		int edx = min(SCREEN_WIDTH - 1, x + len);
		if (y + i < SCREEN_HEIGHT) {
			printf("%d %d %d %d\n", y + i, stx, edx, color);
			// memset(buf + (y + i) * SCREEN_WIDTH + stx, color, (edx - stx + 1));
			for(int j = stx;j < edx;++ j) {
				fb_draw_pixel(j,y+i,color);
				// *(buf + j + (y + i) * SCREEN_WIDTH) = color;
			}
		}
		if (y - i >= 0 && i) {
			// memset(buf + (y - i) * SCREEN_WIDTH + stx, color, (edx - stx + 1));
			printf("%d %d %d %d\n", y - i, stx, edx, color);
			for(int j = stx;j < edx;++ j) {
				fb_draw_pixel(j,y-i,color);
				// *(buf + j + (y - i) * SCREEN_WIDTH) = color;
			}
		}
		i2 += (i << 1) | 1; // += 2 * i + 1;
	}
}