#include <stdio.h>
#include "../common/common.h"

#define COLOR_BACKGROUND	FB_COLOR(0xff,0xff,0xff)
#define COLOR_TEXT			FB_COLOR(0x0,0x0,0x0)
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
static int bluetooth_fd;

#define TIME_X	(SCREEN_WIDTH-160)
#define TIME_Y	0
#define TIME_W	100
#define TIME_H	30

#define SEND_X	(SCREEN_WIDTH-60)
#define SEND_Y	0
#define SEND_W	60
#define SEND_H	60

#define DRAW_X (SCREEN_WIDTH/2 - SCREEN_WIDTH/10 *3/2 )
#define DRAW_Y (SCREEN_HEIGHT/2 - SCREEN_HEIGHT/12/2)
#define DRAW_W SCREEN_WIDTH/10
#define DRAW_H SCREEN_HEIGHT/12

#define GUESS_X (SCREEN_WIDTH/2 + SCREEN_WIDTH/10/2 )
#define GUESS_Y (SCREEN_HEIGHT/2 - SCREEN_HEIGHT/12/2)
#define GUESS_W SCREEN_WIDTH/10
#define GUESS_H SCREEN_HEIGHT/12

#define BUTTON_R_ROLE SCREEN_HEIGHT/12/2
#define BUTTON_FONTSIZE_ROLE SCREEN_HEIGHT/12 *9/10

#define BUTTON_W_RESET 180
#define BUTTON_H_RESET 60
#define BUTTON_R_RESET 30
#define BUTTON_X_RESET 10
#define BUTTON_Y_RESET 530

#define BUTTON_X2_RESET 190
#define BUTTON_Y2_RESET 590
#define BUTTON_FONTSIZE_RESET 54

#define LINE_R 12

static int role; // -1: none	1:drawer	2:guesser
static int score;
static void touch_event_cb(int fd)
{
	int type,x,y,finger;
	type = touch_read(fd, &x,&y,&finger);
	touch_event_cb_handle(x, y, finger, type);
	return;
	
}

static void draw_reset_button(){
	fb_draw_line_wide(BUTTON_X_RESET + BUTTON_R_RESET, BUTTON_Y_RESET + BUTTON_R_RESET, BUTTON_X_RESET + BUTTON_W_RESET - BUTTON_R_RESET, BUTTON_Y_RESET + BUTTON_R_RESET, BUTTON_R_RESET, PURPLE);
	fb_update();
	fb_draw_text(BUTTON_X_RESET + BUTTON_R_RESET, BUTTON_Y_RESET + BUTTON_FONTSIZE_RESET - ((BUTTON_H_RESET - BUTTON_FONTSIZE_RESET)>>1), "Reset", BUTTON_FONTSIZE_RESET, ORANGE);
	fb_update();
	return;
}

static void draw_score(){
	//Todo
	return;
}

static void draw_role_buttons(){

	fb_draw_line_wide(DRAW_X, DRAW_Y, DRAW_W, DRAW_H, BUTTON_R_ROLE, BLUE);
	
	fb_draw_text(DRAW_X + (DRAW_W - BUTTON_FONTSIZE_ROLE * 8)/2, DRAW_Y + BUTTON_FONTSIZE_ROLE - ((DRAW_H - BUTTON_FONTSIZE_ROLE)>>1), "You draw", BUTTON_FONTSIZE, BLACK);
	
	fb_draw_line_wide(GUESS_X, GUESS_Y, GUESS_W, GUESS_H, BUTTON_R_ROLE, BLUE);
	
	fb_draw_text(GUESS_X + (GUESS_W - BUTTON_FONTSIZE_ROLE * 9)/2, GUESS_Y + BUTTON_FONTSIZE_ROLE - ((GUESS_H - BUTTON_FONTSIZE_ROLE)>>1), "You guess", BUTTON_FONTSIZE, BLACK);
	
	fb_update();
}
static void init_game(){
	role = -1;
	score = 0;
	fb_draw_rect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,COLOR_BACKGROUND);
	
	fb_update();
	//fb_draw_border(SEND_X, SEND_Y, SEND_W, SEND_H, COLOR_TEXT);
	//fb_draw_text(SEND_X+2, SEND_Y+30, "send", 24, COLOR_TEXT);
	draw_role_buttons();
	
	printf("The game is ready!\n Please choose a role and the other will have the other role.")
}

static void touch_event_cb_handle(int x, int y, int finger, int type){
	char bstr[100];
	if(role == -1){
		if(type != TOUCH_PRESS )return;
		printf("type=%d,x=%d,y=%d,finger=%d\n",type,x,y,finger);
		if((x>=DRAW_X)&&(x<DRAW_X+DRAW_W)&&(y>=DRAW_Y)&&(y<DRAW_Y+DRAW_H)) {
			role = 1;
			printf("you chose to draw (role 1).\n");
			sprintf(bstr, "0 2 \n"); // first 0 means switching role, second 2 means the opposite role
			myWrite_nonblock(bluetooth_fd, bstr, 5);
			return;
		}
		if((x>=DRAW_X)&&(x<DRAW_X+DRAW_W)&&(y>=DRAW_Y)&&(y<DRAW_Y+DRAW_H)) {
			role = 2;
			printf("you chose to guess (role 2).\n");
			sprintf(bstr, "0 1 \n");
			myWrite_nonblock(bluetooth_fd, bstr, 5);
			return;
		}
	}
	switch(type){
		case TOUCH_PRESS:
			printf("type=%d,x=%d,y=%d,finger=%d\n",type,x,y,finger);
			/*
			if((x>=SEND_X)&&(x<SEND_X+SEND_W)&&(y>=SEND_Y)&&(y<SEND_Y+SEND_H)) {
				printf("bluetooth tty send hello\n");
				myWrite_nonblock(bluetooth_fd, "hello\n", 6);
			}
			break;	
			*/
			if(role == 2){
				//Todo: listen_button
				break;
			}
			if(x >= BUTTON_X - LINE_R && x < BUTTON_X2 + LINE_R && y >= BUTTON_Y - LINE_R && y < BUTTON_Y2 + LINE_R){
				//reset
				fb_draw_rect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,COLOR_BACKGROUND);
				fb_update();
				draw_reset_button();
				sprintf(bstr, "1\n");
				myWrite_nonblock(bluetooth_fd, bstr, 2);
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
			sprintf(bstr, "2 0 %d %d %d\n", x, y, finger);
			myWrite_nonblock(bluetooth_fd, bstr, 18);
			break;
			
		case TOUCH_MOVE:
			printf("TOUCH_MOVE：x=%d,y=%d,finger=%d\n",x,y,finger);
			if(role == 2){
				//
				printf("You cannot draw because you are the guesser.\n");
				break;
			}
			switch(finger){
				case 0:color = ORANGE;break;
				case 1:color = YELLOW;break;
				case 2:color = GREEN;break;
				case 3:color = CYAN;break;
				case 4:color = PURPLE;break;
			}
			if(x >= BUTTON_X - LINE_R && x < BUTTON_X2 + LINE_R && y >= BUTTON_Y - LINE_R && y < BUTTON_Y2 + LINE_R){
				if(old[finger].x < BUTTON_X){
					y = BUTTON_Y2 + LINE_R;
				} else {
					x = BUTTON_X2 + LINE_R;
				}
				old[finger].x = x;
				old[finger].y = y;
				break;
			}
			fb_draw_line_wide(old[finger].x, old[finger].y, x, y, LINE_R, color);
			fb_update();
			sprintf(bstr, "2 1 %d %d %d %d %d\n", old[finger].x, old[finger].y, x, y, finger);
			myWrite_nonblock(bluetooth_fd, bstr, 26);
			old[finger].x = x;
			old[finger].y = y;
			
			break;
		
		case TOUCH_RELEASE:
			printf("TOUCH_RELEASE：x=%d,y=%d,finger=%d\n",x,y,finger);
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

static int pen_y=30;
static void bluetooth_tty_event_cb(int fd)
{
	char buf[128];
	int n;

	n = myRead_nonblock(fd, buf, sizeof(buf)-1);
	if(n <= 0) {
		printf("close bluetooth tty fd\n");
		//task_delete_file(fd);
		//close(fd);
		exit(0);
		return;
	}

	buf[n] = '\0';
	printf("bluetooth tty receive \"%s\"\n", buf);
	int event_type;
	sscanf(buf, "%d", &event_type);
	int x, y, finger, x2, y2, draw_type, color;
	switch(event_type){
		case 0: 
			// choose role
			sscanf(buf, "%d", &role);
			if(role == 1)	printf("your role is drawer(role 1).\n");
			else if(role == 2)	printf("your role is guesser(role 2).\n");
			else	printf("warning : you received a invalid message.\n");
			break;
		case 1:
			// reset. the receiver must be guesser, no need to draw the reset button.
			fb_draw_rect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,COLOR_BACKGROUND);
			fb_update();
			break;
		case 2:
			// draw what the drawer drew
			sscanf(buf, "%d", &draw_type);
			if(draw_type == 0){
				sscanf(buf, "%d%d%d", &x, &y, &finger);
				switch(finger){
					case 0:color = ORANGE;break;
					case 1:color = YELLOW;break;
					case 2:color = GREEN;break;
					case 3:color = CYAN;break;
					case 4:color = PURPLE;break;
				}
				fb_draw_circle(x, y, LINE_R, color);
				fb_update();
				
				break;
			}else if (draw_type == 1){
				sscanf(buf, "%d%d%d%d%d", &x, &y, &x2, &y2, &finger);
				switch(finger){
					case 0:color = ORANGE;break;
					case 1:color = YELLOW;break;
					case 2:color = GREEN;break;
					case 3:color = CYAN;break;
					case 4:color = PURPLE;break;
				}
				fb_draw_line_wide(x, y, x2, y2, LINE_R, color);
				fb_update();
			}else printf("warning : you received a invalid message.\n");
		
	}
	
	//fb_draw_text(2, pen_y, buf, 24, COLOR_TEXT); fb_update();
	//pen_y += 30;
	return;
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
static void timer_cb(int period) /*该函数0.5秒执行一次*/
{
	char buf[100];
	sprintf(buf, "%d", st++);
	fb_draw_rect(TIME_X, TIME_Y, TIME_W, TIME_H, COLOR_BACKGROUND);
	fb_draw_border(TIME_X, TIME_Y, TIME_W, TIME_H, COLOR_TEXT);
	fb_draw_text(TIME_X+2, TIME_Y+20, buf, 24, COLOR_TEXT);
	fb_update();
	return;
}

int main(int argc, char *argv[])
{
	fb_init("/dev/fb0");
	font_init("./font.ttc");
	init_game();
	
	
	touch_fd = touch_init("/dev/input/event1");
	task_add_file(touch_fd, touch_event_cb);
	
	bluetooth_fd = bluetooth_tty_init("/dev/rfcomm0");
	if(bluetooth_fd == -1) return 0;
	task_add_file(bluetooth_fd, bluetooth_tty_event_cb);

	task_add_timer(500, timer_cb); /*增加0.5秒的定时器*/
	task_loop();
	return 0;
}
