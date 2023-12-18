#include <stdio.h>
#include "../common/common.h"

#define COLOR_BACKGROUND	FB_COLOR(0xff,0xff,0xff)
#define RED	FB_COLOR(255,0,0)
#define ORANGE	FB_COLOR(255,165,0)
#define YELLOW	FB_COLOR(255,255,0)
#define GREEN	FB_COLOR(0,255,0)
#define CYAN	FB_COLOR(0,127,255)
#define BLUE	FB_COLOR(0,0,255)
#define PURPLE	FB_COLOR(139,0,255)
#define WHITE   FB_COLOR(255,255,255)
#define BLACK   FB_COLOR(0,0,0)

static int touch_fd;
static struct point{
	int x, y;
}old[5];
#define BUTTON_W 180
#define BUTTON_H 60
#define BUTTON_R 30
#define BUTTON_X 10
#define BUTTON_Y 530

#define BUTTON_X2 190
#define BUTTON_Y2 590
#define BUTTON_FONTSIZE 54

#define LINE_R 12

static void touch_event_cb(int fd)
{
	int type,x,y,finger;
	type = touch_read(fd, &x,&y,&finger);
	int color = COLOR_BACKGROUND;
	switch(type){
	case TOUCH_PRESS:
		printf("TOUCH_PRESS：x=%d,y=%d,finger=%d\n",x,y,finger);
		if(x >= BUTTON_X - LINE_R && x < BUTTON_X2 + LINE_R && y >= BUTTON_Y - LINE_R && y < BUTTON_Y2 + LINE_R){
			//reset
			fb_draw_rect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,COLOR_BACKGROUND);
	fb_update();
			fb_draw_line_wide(BUTTON_X + BUTTON_R, BUTTON_Y + BUTTON_R, BUTTON_X + BUTTON_W - BUTTON_R, BUTTON_Y + BUTTON_R, BUTTON_R, PURPLE);
			fb_update();
			fb_update();
			fb_draw_text(BUTTON_X + BUTTON_R, BUTTON_Y + BUTTON_FONTSIZE - ((BUTTON_H - BUTTON_FONTSIZE)>>1), "Reset", BUTTON_FONTSIZE, ORANGE);
			fb_update();
			break;
		}
		switch(finger){
			case 0:color = ORANGE;break;
			case 1:color = YELLOW;break;
			case 2:color = GREEN;break;
			case 3:color = CYAN;break;
			case 4:color = PURPLE;break;
		}
		fb_draw_circle(x, y, LINE_R, color);
		fb_update();
		old[finger].x = x;
		old[finger].y = y;
		break;
	case TOUCH_MOVE:
		printf("TOUCH_MOVE：x=%d,y=%d,finger=%d\n",x,y,finger);
		switch(finger){
			case 0:color = ORANGE;break;
			case 1:color = YELLOW;break;
			case 2:color = GREEN;break;
			case 3:color = CYAN;break;
			case 4:color = PURPLE;break;
		}
		if(x >= BUTTON_X - LINE_R && x < BUTTON_X2 + LINE_R && y >= BUTTON_Y - LINE_R && y < BUTTON_Y2 + LINE_R){
			if(old[finger].x < BUTTON_X2 + LINE_R){
				y = BUTTON_Y - LINE_R;
			} else {
				x = BUTTON_X2 + LINE_R;
			}
		}
		fb_draw_line_wide(old[finger].x, old[finger].y, x, y, LINE_R, color);
		fb_update();
		old[finger].x = x;
		old[finger].y = y;
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

int main(int argc, char *argv[])
{
	fb_init("/dev/fb0");
	fb_draw_rect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,COLOR_BACKGROUND);
	fb_update();
	
	// draw button
	font_init("./font.ttc"); //must init font
	
	fb_draw_line_wide(BUTTON_X + BUTTON_R, BUTTON_Y + BUTTON_R, BUTTON_X + BUTTON_W - BUTTON_R, BUTTON_Y + BUTTON_R, BUTTON_R, PURPLE);
	fb_update();
	//
	fb_draw_text(BUTTON_X + BUTTON_R-2, BUTTON_Y + BUTTON_FONTSIZE - ((BUTTON_H - BUTTON_FONTSIZE)>>1), "Reset", BUTTON_FONTSIZE, ORANGE);
	fb_update();
	
	//打开多点触摸设备文件, 返回文件fd./dev/input/可能是event0/1/2/3， 可以使用
	//cat /dev/input/event(0/1/2/3)进行辅助判断,设备正确按压触摸屏会有乱码打印。
	touch_fd = touch_init("/dev/input/event1");
	//添加任务, 当touch_fd文件可读时, 会自动调用touch_event_cb函数
	task_add_file(touch_fd, touch_event_cb);
	task_loop(); //进入任务循环
	return 0;
}
