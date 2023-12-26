#include <stdio.h>
#include "../common/common.h"

#define COLOR_BACKGROUND	FB_COLOR(0xff,0xff,0xff)

int tot_f = 0;
int finger_color[20];
static int touch_fd;
static void touch_event_cb(int fd)
{
	int type,x,y,finger;
	type = touch_read(fd, &x,&y,&finger);
	switch(type){
	case TOUCH_PRESS:
		tot_f ++;
		printf("TOUCH_PRESS：x=%d,y=%d,finger=%d\n",x,y,finger);
		fb_draw_circle(x, y, 25, finger_color[(finger + tot_f) % 10]);
		if (x <= 100 && y <= 80) {
			fb_draw_rect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,COLOR_BACKGROUND);
			fb_draw_rect(0, 0, 100, 80, FB_COLOR(0xff, 0x00, 0x00));
			fb_update();
		}
		break;
	case TOUCH_MOVE:
		printf("TOUCH_MOVE：x=%d,y=%d,finger=%d\n",x,y,finger);
		fb_draw_circle(x, y, 25, finger_color[(finger + tot_f) % 10]);
		if (x <= 100 && y <= 80) {
			fb_draw_rect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,COLOR_BACKGROUND);
			fb_draw_rect(0, 0, 100, 80, FB_COLOR(0xff, 0x00, 0x00));
			fb_update();
		}
		break;
	case TOUCH_RELEASE:
		printf("TOUCH_RELEASE：x=%d,y=%d,finger=%d\n",x,y,finger);
		break;
	case TOUCH_ERROR:
		printf("close touch fd\n");
		close(fd);
		task_delete_file(fd);
		break;
	default:
		return;
	}
	fb_update();
	return;
}

void finger_color_init() {
	finger_color[0] = FB_COLOR(0x3c, 0x4e, 0x72);
	finger_color[1] = FB_COLOR(0x31, 0xc2, 0x72);
	finger_color[2] = FB_COLOR(0xc0, 0xb6, 0x9b);
	finger_color[3] = FB_COLOR(0xf2, 0x33, 0x21);
	finger_color[4] = FB_COLOR(0x58, 0xa6, 0xa6);
	finger_color[5] = FB_COLOR(0x2f, 0x2c, 0x37);
	finger_color[6] = FB_COLOR(0xda, 0x5f, 0xa2);
	finger_color[7] = FB_COLOR(0x41, 0x04, 0x14);
	finger_color[8] = FB_COLOR(0xf2, 0x6e, 0x23);
	finger_color[9] = FB_COLOR(0xae, 0xce, 0xb9);///
}

int main(int argc, char *argv[])
{
	finger_color_init();
	fb_init("/dev/fb0"); //printf("??????\n");
	fb_draw_rect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,COLOR_BACKGROUND);
	fb_draw_rect(0, 0, 100, 80, FB_COLOR(0xff, 0x00, 0x00));
	fb_update();

	
	
	//打开多点触摸设备文件, 返回文件fd
	touch_fd = touch_init("/dev/input/event2");
	//添加任务, 当touch_fd文件可读时, 会自动调用touch_event_cb函数
	task_add_file(touch_fd, touch_event_cb);
	
	task_loop(); //进入任务循环
	return 0;
}
