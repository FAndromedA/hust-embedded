#include <stdio.h>
#include <math.h>
#include "../common/common.h"
#include"/root/lab-2022-st/common/external/include/jpeglib.h"

#define COLOR_BACKGROUND	FB_COLOR(0xff,0xff,0xff)
#define COLOR_TEXT			FB_COLOR(0x0,0x0,0x0)

static int touch_fd;
static int bluetooth_fd;

#define TIME_X	(SCREEN_WIDTH-160)
#define TIME_Y	0
#define TIME_W	100
#define TIME_H	30

#define SEND_X	(SCREEN_WIDTH-60)
#define SEND_Y	0
#define SEND_W	60
#define SEND_H	60

void write_jpeg(const char *filename, uint8_t *image_data, int width, int height, int quality);

static void draw_button() {
	fb_draw_rect(SEND_X, SEND_Y, SEND_W, SEND_H, COLOR_BACKGROUND);
	fb_draw_border(SEND_X, SEND_Y, SEND_W, SEND_H, COLOR_TEXT);
	fb_draw_text(SEND_X+2, SEND_Y+30, "send", 24, COLOR_TEXT);
	fb_draw_rect(SEND_X, SEND_Y+SEND_H, SEND_W, SEND_H, COLOR_BACKGROUND);
	fb_draw_border(SEND_X, SEND_Y+SEND_H, SEND_W, SEND_H, COLOR_TEXT);
	fb_draw_text(SEND_X+2, SEND_Y+SEND_H+30, "clear", 20, COLOR_TEXT);
	fb_draw_rect(SEND_X, SEND_Y + 2*SEND_H, SEND_W, SEND_H, COLOR_BACKGROUND);
	fb_draw_border(SEND_X, SEND_Y + 2*SEND_H, SEND_W, SEND_H, COLOR_TEXT);
	fb_draw_text(SEND_X+2, SEND_Y + 2*SEND_H+30, "rotate", 16, COLOR_TEXT);
	fb_draw_rect(SEND_X, SEND_Y + 3*SEND_H, SEND_W, SEND_H, COLOR_BACKGROUND);
	fb_draw_border(SEND_X, SEND_Y + 3*SEND_H, SEND_W, SEND_H, COLOR_TEXT);
	fb_draw_text(SEND_X+2, SEND_Y + 3*SEND_H+30, "prtsc", 20, COLOR_TEXT);
	fb_draw_rect(SEND_X, SEND_Y + 4*SEND_H, SEND_W, SEND_H, COLOR_BACKGROUND);
	fb_draw_border(SEND_X, SEND_Y + 4*SEND_H, SEND_W, SEND_H, COLOR_TEXT);
	fb_draw_text(SEND_X+2, SEND_Y + 4*SEND_H+30, "draw", 24, COLOR_TEXT);
}

static void init_UI() {
    fb_draw_rect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,COLOR_BACKGROUND);
	draw_button();
	// fb_draw_border(SEND_X, SEND_Y, SEND_W, SEND_H, COLOR_TEXT);
    // fb_draw_border(SEND_X, SEND_Y+SEND_H, SEND_W, SEND_H, COLOR_TEXT);
	// fb_draw_text(SEND_X+2, SEND_Y+30, "send", 24, COLOR_TEXT);
    // fb_draw_text(SEND_X+2, SEND_Y+SEND_H+30, "clear", 20, COLOR_TEXT);
}

int finger_color[20];
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

static int fingerExist = 0, fingerCreated = 0;
struct FingerInfo {
	int lastx, lasty;
	int active, createdAt;
}fingerInfo[10];

static int existPhoto = 0;
fb_image *img;
static int pen_y = 30;
static int img_sx = 0, img_sy = 0;
static double scaling = 1.0;
static int direction = 0;
static int drawing = 0;
const static double maxScaling = 5.0;
const static double minScaling = 0.20;

static struct FingerInfo *pref1 = NULL, *pref2 =  NULL;
static double predist;

const int radius = 15;

static void touch_event_cb(int fd)
{
	int type,x,y,finger;
	type = touch_read(fd, &x,&y,&finger);
	//printf("%d\n", type);
	switch(type){
	case TOUCH_PRESS:
		fingerExist ++;
		//printf("type=%d,x=%d,y=%d,finger=%d\n",type,x,y,finger);
		if((x>=SEND_X)&&(x<SEND_X+SEND_W)&&(y>=SEND_Y)&&(y<SEND_Y+SEND_H)) {
			printf("bluetooth tty send hello\n");
			myWrite_nonblock(bluetooth_fd, "hello\n", 6);
		}
        else if((x>=SEND_X)&&(x<SEND_X+SEND_W)&&(y>=SEND_Y+SEND_H)&&(y<SEND_Y+2*SEND_H)) {
			printf("press button clear\n");
			existPhoto = 0; 
			img_sx = img_sy = 0;
			fb_draw_rect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,COLOR_BACKGROUND);
            pen_y = 0;
            init_UI();
        }
		else if((x>=SEND_X)&&(x<SEND_X+SEND_W)&&(y>=SEND_Y+2*SEND_H)&&(y<SEND_Y+3*SEND_H)) {
			printf("press button rotate\n");
			direction = (direction + 1) % 4;
		}
		else if((x>=SEND_X)&&(x<SEND_X+SEND_W)&&(y>=SEND_Y+3*SEND_H)&&(y<SEND_Y+4*SEND_H)){
			write_jpeg("drawjpg.jpg", (JSAMPLE *)_begin_draw(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT), SCREEN_WIDTH, SCREEN_HEIGHT, 90);
		}
		else if((x>=SEND_X)&&(x<SEND_X+SEND_W)&&(y>=SEND_Y+4*SEND_H)&&(y<SEND_Y+5*SEND_H)){
			drawing ^= 1;
		}
		else {
			fingerCreated ++;
			fingerInfo[finger].lastx = x;
			fingerInfo[finger].lasty = y;
			fingerInfo[finger].active = 1;
			fingerInfo[finger].createdAt = fingerCreated;
		}
		break;
	case TOUCH_MOVE:
		if (drawing) {
			int f_color = finger_color[(finger + fingerInfo[finger].createdAt) % 10];
			fb_draw_circle(x, y, radius, f_color);

			if ((x - fingerInfo[finger].lastx) * (x - fingerInfo[finger].lastx) + (y - fingerInfo[finger].lasty) * (y - fingerInfo[finger].lasty) > 0) {
				fb_draw_circle((3 * x + fingerInfo[finger].lastx)/4, (3 * y + fingerInfo[finger].lasty)/4, radius, f_color);
				fb_draw_circle((x + 3 * fingerInfo[finger].lastx)/4, (y + 3 * fingerInfo[finger].lasty)/4, radius, f_color);
			}
			if ((x - fingerInfo[finger].lastx) * (x - fingerInfo[finger].lastx) + (y - fingerInfo[finger].lasty) * (y - fingerInfo[finger].lasty) > 0) {
				fb_draw_circle((x + fingerInfo[finger].lastx)/2, (y + fingerInfo[finger].lasty)/2, radius, f_color);
			}
			if ((x - fingerInfo[finger].lastx) * (x - fingerInfo[finger].lastx) + (y - fingerInfo[finger].lasty) * (y - fingerInfo[finger].lasty) > 0) {
				fb_draw_circle((5 * x + 3 * fingerInfo[finger].lastx)/8, (5 * y + 3 * fingerInfo[finger].lasty)/8, radius, f_color);
				fb_draw_circle((3 * x + 5 * fingerInfo[finger].lastx)/8, (3 * y + 5 * fingerInfo[finger].lasty)/8, radius, f_color);
				fb_draw_circle((7 * x + fingerInfo[finger].lastx)/8, (7 * y + fingerInfo[finger].lasty)/8, radius, f_color);
				fb_draw_circle((x + 7 * fingerInfo[finger].lastx)/8, (y + 7 * fingerInfo[finger].lasty)/8, radius, f_color);
			}
			fingerInfo[finger].lastx = x;
			fingerInfo[finger].lasty = y;
		}
		else if (fingerExist == 1) { // 只有一根手指是拖动
			img_sx += x - fingerInfo[finger].lastx;
			img_sy += y - fingerInfo[finger].lasty;
			fingerInfo[finger].lastx = x;
			fingerInfo[finger].lasty = y;
		}
		else if (fingerExist >= 2) { // 大于两根手指则根据创建时间取前两根，来缩放
			//printf("type=%d,x=%d,y=%d,finger=%d,fingerExist=%d\n",type,x,y,finger,fingerExist);
			fingerInfo[finger].lastx = x;
			fingerInfo[finger].lasty = y;
			struct FingerInfo *f1 = NULL, *f2 = NULL, *temp = NULL;
			for(int i = 0;i < 10;++ i) {
				if (fingerInfo[i].active) {
					if (f1 == NULL) {
						f1 = fingerInfo + i;
					}
					else {
						if (f2 == NULL) 
							f2 = fingerInfo + i;
						else if (f2->createdAt > fingerInfo[i].createdAt) {
							f2 = fingerInfo + i;
						}
						if (f1->createdAt > f2->createdAt) {
							temp = f1;
							f1 = f2;
							f2 = temp;
						}
					}
				}
			}
			double nowDist = sqrt((f1->lastx - f2->lastx) * (f1->lastx - f2->lastx)
				 				+(f1->lasty - f2->lasty) * (f1->lasty - f2->lasty));
			if (f1 != pref1 || f2 != pref2) {
				predist = nowDist;
				pref1 = f1; 
				pref2 = f2;
			}
			else {
				double preS = scaling;
				scaling *= nowDist / predist;
				scaling = fmin(scaling, maxScaling);
				scaling = fmax(scaling, minScaling);
				// if (nowDist != predist) {
				// 	printf("%d %d %d %d\n", f1->lastx, f1->lasty, f2->lastx, f2->lasty);
				// 	printf("%lf %lf ", nowDist, predist);
				// 	printf("%.2lf\n", scaling);
				// }
				img_sx = f1->lastx + (img_sx - f1->lastx) * scaling / preS;
				img_sy = f1->lasty + (img_sy - f1->lasty) * scaling / preS; 
				predist = nowDist;
				
			}
		}
		break;
	case TOUCH_RELEASE:
		fingerExist --;
		fingerInfo[finger].lastx = fingerInfo[finger].lasty = 0;
		fingerInfo[finger].active = 0;
		if (fingerExist < 2) {
			pref1 = NULL; pref2 = NULL;
		}
		break;
	case TOUCH_ERROR:
		printf("close touch fd\n");
		task_delete_file(fd);
		close(fd);
		break;
	default:
		return;
	}
	fb_update();
	return;
}

#include <string.h>
#define transfer_TEXT 0
#define transfer_JPG 1
static int transfer_mode = transfer_TEXT; // 0 when transfering text 1 when transfering image(jpg)
static int transfer_levels = 0; // 嵌套了几层，当第1层的结束时才是文件的结束
FILE* jpegFile = NULL;


// static int hexCharToInt(char c) {
//     if (c >= '0' && c <= '9') 
//         return c - '0';
//     if (c >= 'a' && c <= 'f') 
//         return c - 'a' + 10;
//     if (c >= 'A' && c <= 'F')
//         return c - 'A' + 10;
//     return -1;
// }

// static unsigned char hexesToByte(unsigned char c1, unsigned char c2) {
//     int high = hexCharToInt(c1);
//     int low = hexCharToInt(c2);
// 	printf("%d %d %c %c\n", high, low, c1, c2);
//     if (high == -1 || low == -1) {
//         printf("invalid hex");
//         exit(0);
//     }
//     return (unsigned char)((high << 4) | low);
// }

static void convertHexToBinaryAndWriteToFile(char* hexBuff, size_t hexLen,FILE* outFile) {
    // size_t binaryBytes = hexLen; // 256 = 16 ^ 2
    // unsigned char* binaryData = (unsigned char *)malloc(binaryBytes);

    // for (size_t i = 0;i < binaryBytes; ++ i) {
    //     binaryData[i] = hexBuff[i];//hexesToByte(hexBuff[i * 2], hexBuff[i * 2 + 1]);
    // }

    // fwrite(binaryData, 1, binaryBytes, outFile);
    // free(binaryData);
	fwrite(hexBuff, 1, hexLen, outFile);
}

myTime start_time, end_time;

static void bluetooth_tty_event_cb(int fd)
{
	char buf[1024];
	char ststr[3], edstr[3];
	ststr[0] = 0xff; ststr[1] = 0xd8; ststr[2] = '\0';
	edstr[0] = 0xff; edstr[1] = 0xd9; edstr[2] = '\0';
	int n;

	n = myRead_nonblock(fd, buf, sizeof(buf)-1);
	if(n <= 0) {
		printf("close bluetooth tty fd\n");
		// task_delete_file(fd);
		// close(fd);
		exit(0);
		return;
	}
	// printf("%d\n", transfer_mode);
	if (strstr(buf, ststr) != NULL) {
		if (!transfer_levels) {
			start_time = task_get_time();
			existPhoto = 0; // 修改时图片不完整
			img_sx = img_sy = 0;
			scaling = 1.0;
			direction = 0; // 初始化图片移动缩放旋转
			transfer_mode = transfer_JPG;
			if (jpegFile != NULL) {
				fclose(jpegFile);
			}
			jpegFile = fopen("output.jpg", "wb");
			if (!jpegFile) {
				perror("Error opening output file");
				exit(0);
			}
			printf("start transfering jpg\n");
			img_sx = img_sy = 0;
		}
		transfer_levels ++; // 难点处理嵌套jpg(以及查出这个错误) (()())
		//printf("%d\n", transfer_levels);
	}
	if (transfer_mode == transfer_TEXT) {
		
		if (transfer_mode == transfer_TEXT) {
			buf[n] = '\0';
			// printf("bluetooth tty receive \"%s\"\n", buf);
			// for(int i = 0;i < n;++ i) printf("%2x ", buf[i]);
			fb_draw_text(2, pen_y, buf, 24, COLOR_TEXT); fb_update();
			pen_y += 30;
			return;
		}
	}

	if (transfer_mode == transfer_JPG) {
        convertHexToBinaryAndWriteToFile(buf, n, jpegFile);
		//printf("\n------------------------------------------------------------\n");
        for (int i = 0;i < n;++ i) {
			if (buf[i] == '\0') buf[i] = 1;
			//printf("%2x ", buf[i]);
		} 
		if (n != 1024) buf[n] = '\0';
		// because the function strstr() will stop when meeting the 0 but we do transfering 0 in hex data 
		// so after writing into file we could let 0 to 2 so that we can find if there is edstr inside
		end_time = task_get_time();
		//printf("%d\n", end_time - start_time);
		char * posc;
		if ((posc = strstr(buf, edstr)) != NULL || (end_time - start_time > 20 * 1000)) { 
            transfer_levels --; 
			
			//printf("%d %d %2x %2x\n", transfer_levels, (posc-buf), *posc, *(posc+1));
			
			if (!transfer_levels || (end_time - start_time > 20 * 1000)) {// 可能会遇到嵌套的jpg，所以需要当最底层的结束符出现时才算整个文件的结束。
				transfer_mode = transfer_TEXT;
				transfer_levels = 0;
				fclose(jpegFile);
				jpegFile = NULL;

				//fb_image *img;
				img = fb_read_jpeg_image("./output.jpg");
				printf("loading image on x:%d, y:%d\n", img_sx, img_sy);
				//fb_draw_image(img_sx, img_sy, img, 0);
				fb_draw_jpg(img_sx, img_sy, img, 1.0, direction);
				draw_button();
				fb_update();
				//fb_free_image(img);
				
				printf("finish transfering jpg\n");
				existPhoto = 1;
				fflush(stdout);
			}
			
        }
		// else {printf("\n1111111111111111111111111111111111111111111111111111111111\n");
		// 	for(int i = 0;i < n;++ i) printf("%2x ", buf[i]);
		// 	printf("\n");
		// }printf("\n22222222222222222222222222222222222222222222222222222222222\n");
	}

}

static int bluetooth_tty_init(const char *dev)
{
	int fd = open(dev, O_RDWR|O_NOCTTY|O_NONBLOCK); /*非阻塞模式*/
	if(fd < 0){
		printf("bluetooth_tty_init open %s error(%d): %s\n", dev, errno, strerror(errno));
		return -1;
	}
	return fd;
}

static int st=0;
static void timer_cb(int period) /*该函数0.1秒执行一次*/
{
	char buf_st[100], buf_scale[30];
	sprintf(buf_st, "%d", st++);
	sprintf(buf_scale, "缩放%d%%", (int)(scaling * 100));
	if (existPhoto) {
		static int prex = 0, prey = 0, preDire = 0;
		static double preScaling = 1.0;
		if (prex != img_sx || prey != img_sy || fabs(preScaling - scaling) > 0.01 || preDire != direction) { // 避免没有改变时重复画
			fb_draw_rect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,COLOR_BACKGROUND);
			//fb_image *img;
			//img = fb_read_jpeg_image("./output.jpg"); 避免大图片每次重新打开的耗时
			//printf("loading image on x:%d, y:%d, scaling:%.2lf\n", img_sx, img_sy, scaling);
			//fb_draw_image(img_sx, img_sy, img, 0);
			fb_draw_jpg(img_sx, img_sy, img, scaling, direction);
			
			//exit(0);
			//fb_free_image(img);
			prex = img_sx; prey = img_sy;
			preScaling = scaling;
			preDire = direction;

			fb_draw_rect(TIME_X, TIME_Y + TIME_H, TIME_W, TIME_H, COLOR_BACKGROUND);
			fb_draw_border(TIME_X, TIME_Y + TIME_H, TIME_W, TIME_H, COLOR_TEXT);
			fb_draw_text(TIME_X+2, TIME_Y + TIME_H +20, buf_scale, 24, COLOR_TEXT);
			draw_button();
			
		}
	}
	fb_draw_rect(TIME_X, TIME_Y, TIME_W, TIME_H, COLOR_BACKGROUND);
	fb_draw_border(TIME_X, TIME_Y, TIME_W, TIME_H, COLOR_TEXT);
	fb_draw_text(TIME_X+2, TIME_Y+20, buf_st, 24, COLOR_TEXT);

	fb_update();
	return;
}

int main(int argc, char *argv[])
{
	finger_color_init();
	fb_init("/dev/fb0");
	font_init("./font.ttc");
	init_UI();
	fb_update();

	touch_fd = touch_init("/dev/input/event2");
	task_add_file(touch_fd, touch_event_cb);

	bluetooth_fd = bluetooth_tty_init("/dev/rfcomm0");
	if(bluetooth_fd == -1) return 0;
	task_add_file(bluetooth_fd, bluetooth_tty_event_cb);

	task_add_timer(100, timer_cb); /*增加0.1秒的定时器*/
	task_loop();
	
	return 0;
}


void write_jpeg(const char *filename, JSAMPLE *image_data, int width, int height, int quality) {
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    FILE *outfile;
    if ((outfile = fopen(filename, "wb")) == NULL) {
        fprintf(stderr, "Can't open %s for writing.\n", filename);
        exit(EXIT_FAILURE);
    }
    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 4;  // BGRX image
    cinfo.in_color_space = JCS_EXT_BGRX; // Specify extended BGRX color space

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    JSAMPROW row_pointer[1];
    int row_stride = width * cinfo.input_components;

    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = &image_data[cinfo.next_scanline * row_stride];
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);

    fclose(outfile);
    jpeg_destroy_compress(&cinfo);
}