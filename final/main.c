#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../common/common.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "audio_util.h"

/*语音识别要求的pcm采样频率*/
#define PCM_SAMPLE_RATE 16000 /* 16KHz */

static char * send_to_vosk_server(char *file);

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

#define TIME_X	(SCREEN_WIDTH-138)
#define TIME_Y	0
#define TIME_W	38
#define TIME_H	30
#define TIME_INIT 150


#define TEXTFRAME_X	0
#define TEXTFRAME_Y	0
#define TEXTFRAME_W	260
#define TEXTFRAME_H	96

#define DRAW_X SCREEN_WIDTH/2 - 150
#define DRAW_Y SCREEN_HEIGHT/2 -80
#define DRAW_W 300
#define DRAW_H 60

#define GUESS_X SCREEN_WIDTH/2 - 150
#define GUESS_Y SCREEN_HEIGHT/2
#define GUESS_W 300
#define GUESS_H 60

#define RANDOM_X SCREEN_WIDTH/2 - 150
#define RANDOM_Y SCREEN_HEIGHT/2 + 80
#define RANDOM_W 300
#define RANDOM_H 60

#define FREE_X SCREEN_WIDTH/2 - 150
#define FREE_Y SCREEN_HEIGHT/2 + 160
#define FREE_W 300
#define FREE_H 60


#define BUTTON_R_ROLE 30
#define BUTTON_FONTSIZE_ROLE 54

#define BUTTON_W_CLEAR 180
#define BUTTON_H_CLEAR 60
#define BUTTON_R_CLEAR 30
#define BUTTON_X_CLEAR 10
#define BUTTON_Y_CLEAR 530

#define BUTTON_X2_CLEAR 190
#define BUTTON_Y2_CLEAR 590
#define BUTTON_FONTSIZE_CLEAR 54

#define TOOL_SIZE 64


#define LINE_R_INIT 8

static struct point{
	int x, y;
}old[5];

char* words[] = {"华科","大学","小心翼翼","心脏","翅膀","狗","猫","兔子","牛","牛奶","监狱","睡觉","床","被子","洗澡","电脑","熬夜","手机","手心","投降","游戏","尖叫","逃跑","拥抱","种植","玫瑰","盆栽","肥料"};
#define WORDS_LEN 28

#define NO_ROLE -1
#define DRAWER 1
#define GUESSER 2
#define BOTH 3

static int role; // -1: none	1:drawer	2:guesser	3:both
static int line_r = LINE_R_INIT;
static int color_index = 0;
static int eraser = 0;

static int line_r_op = LINE_R_INIT;
static int color_index_op = 0;
static int eraser_op = 0;

int get_color(int finger){
	int color=ORANGE;
	if(eraser)
		color = WHITE;
	else if(finger != 0)
		switch((finger+color_index)%7){
			case 0:color = ORANGE;break;
			case 1:color = YELLOW;break;
			case 2:color = GREEN;break;
			case 3:color = CYAN;break;
			case 4:color = PURPLE;break;
			case 5:color = RED;break;
			case 6:color = BLACK;break;
		}
	else 
		switch(color_index){
			case 0:color = ORANGE;break;
			case 1:color = YELLOW;break;
			case 2:color = GREEN;break;
			case 3:color = CYAN;break;
			case 4:color = PURPLE;break;
			case 5:color = RED;break;
			case 6:color = BLACK;break;
		}
	return color;
}

int get_color_real(int finger){
	// do not consider the eraser
	int color=ORANGE;
	if(finger != 0)
		switch((finger+color_index)%7){
			case 0:color = ORANGE;break;
			case 1:color = YELLOW;break;
			case 2:color = GREEN;break;
			case 3:color = CYAN;break;
			case 4:color = PURPLE;break;
			case 5:color = RED;break;
			case 6:color = BLACK;break;
		}
	else 
		switch(color_index){
			case 0:color = ORANGE;break;
			case 1:color = YELLOW;break;
			case 2:color = GREEN;break;
			case 3:color = CYAN;break;
			case 4:color = PURPLE;break;
			case 5:color = RED;break;
			case 6:color = BLACK;break;
		}
	return color;
}

int get_color_op(int finger){
	int color=ORANGE;
	if(eraser_op)
		color = WHITE;
	else if(finger != 0)
		switch((finger+color_index_op)%7){
			case 0:color = ORANGE;break;
			case 1:color = YELLOW;break;
			case 2:color = GREEN;break;
			case 3:color = CYAN;break;
			case 4:color = PURPLE;break;
			case 5:color = RED;break;
			case 6:color = BLACK;break;
		}
	else 
		switch(color_index_op){
			case 0:color = ORANGE;break;
			case 1:color = YELLOW;break;
			case 2:color = GREEN;break;
			case 3:color = CYAN;break;
			case 4:color = PURPLE;break;
			case 5:color = RED;break;
			case 6:color = BLACK;break;
		}
	return color;
}

static void draw_background(){
	fb_draw_rect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,COLOR_BACKGROUND);
	fb_update();
	return;
}

static void draw_clear_button(){
	if(role == BOTH){
		// in free mode, disable Clear button 
		printf("Clear button if disabled in free mode!\n");
		return;
	}
	fb_draw_line_wide(BUTTON_X_CLEAR + BUTTON_R_CLEAR, BUTTON_Y_CLEAR + BUTTON_R_CLEAR, BUTTON_X_CLEAR + BUTTON_W_CLEAR - BUTTON_R_CLEAR, BUTTON_Y_CLEAR + BUTTON_R_CLEAR, BUTTON_R_CLEAR, PURPLE);
	fb_draw_text(BUTTON_X_CLEAR + BUTTON_R_CLEAR-2, BUTTON_Y_CLEAR + BUTTON_FONTSIZE_CLEAR - ((BUTTON_H_CLEAR - BUTTON_FONTSIZE_CLEAR)>>1), "Clear", BUTTON_FONTSIZE_CLEAR, ORANGE);
	fb_update();
	return;
}

static void draw_speak_button(){
	fb_draw_line_wide(BUTTON_X_CLEAR + BUTTON_R_CLEAR, BUTTON_Y_CLEAR + BUTTON_R_CLEAR, BUTTON_X_CLEAR + BUTTON_W_CLEAR - BUTTON_R_CLEAR, BUTTON_Y_CLEAR + BUTTON_R_CLEAR, BUTTON_R_CLEAR, BLUE);
	fb_draw_text(BUTTON_X_CLEAR + BUTTON_R_CLEAR-2, BUTTON_Y_CLEAR + BUTTON_FONTSIZE_CLEAR - ((BUTTON_H_CLEAR - BUTTON_FONTSIZE_CLEAR)>>1), "Speak", BUTTON_FONTSIZE_CLEAR, ORANGE);
	fb_update();
	return;
}

static void draw_tools(){
	fb_image *img;
	/*
	img = fb_read_png_image("./test.png");
	fb_draw_image(100,300,img,0);
	fb_update();
	fb_free_image(img);	
	*/
	img = fb_read_png_image("./color-palette.png");
	fb_draw_image(6,BUTTON_Y_CLEAR - TOOL_SIZE,img,0);
	fb_update();
	fb_free_image(img);	
	img = fb_read_png_image("./eraser.png");
	fb_draw_image(6,BUTTON_Y_CLEAR - (TOOL_SIZE<<1),img,0);
	fb_update();
	fb_free_image(img);	
	img = fb_read_png_image("./size.png");
	fb_draw_image(6,BUTTON_Y_CLEAR - (TOOL_SIZE*3),img,0);
	fb_draw_circle(6+(TOOL_SIZE>>1), BUTTON_Y_CLEAR - (TOOL_SIZE<<1) - (TOOL_SIZE>>1), line_r, get_color_real(0));
	fb_update();
	fb_free_image(img);	
}

static void update_sizetool(){
	fb_draw_circle(6+(TOOL_SIZE>>1), BUTTON_Y_CLEAR - (TOOL_SIZE<<1) - (TOOL_SIZE>>1), line_r, get_color_real(0));
	if(line_r == 32){
			fb_image *img;
			img = fb_read_png_image("./size.png");
			fb_draw_image(6,BUTTON_Y_CLEAR - (TOOL_SIZE*3),img,0);
			fb_free_image(img);	
	}
	fb_update();
}

static void change_line_r(){
	fb_draw_circle(6+(TOOL_SIZE>>1), BUTTON_Y_CLEAR - (TOOL_SIZE<<1) - (TOOL_SIZE>>1), line_r, WHITE);
	fb_update();
	if(line_r == 32){
			fb_image *img;
			img = fb_read_png_image("./size.png");
			fb_draw_image(6,BUTTON_Y_CLEAR - (TOOL_SIZE*3),img,0);
			fb_update();
			fb_free_image(img);	
	}	
	line_r <<= 1;
	if(line_r > 32)
	line_r = 2;
	update_sizetool();
}



static void clear_usingtoolframe(){
	if(eraser){
		fb_draw_border(6, BUTTON_Y_CLEAR - (TOOL_SIZE<<1), TOOL_SIZE, TOOL_SIZE, WHITE);
		fb_update();
		return;
	}
	fb_draw_border(6, BUTTON_Y_CLEAR - TOOL_SIZE, TOOL_SIZE, TOOL_SIZE, WHITE);
	fb_update();
	return;
}

static void draw_usingtoolframe(){
	if(eraser){
		fb_draw_border(6, BUTTON_Y_CLEAR - (TOOL_SIZE<<1), TOOL_SIZE, TOOL_SIZE, BLACK);
		fb_update();
		return;
	}
	int color = get_color(0);
	fb_draw_border(6, BUTTON_Y_CLEAR - TOOL_SIZE, TOOL_SIZE, TOOL_SIZE, color);
	fb_update();
	return;
}

static void draw_role_buttons(){
	fb_draw_line_wide(DRAW_X + BUTTON_R_ROLE, DRAW_Y + BUTTON_R_ROLE, DRAW_X + DRAW_W - BUTTON_R_ROLE, DRAW_Y + BUTTON_R_ROLE, BUTTON_R_ROLE, BLUE);
	fb_draw_text(DRAW_X + BUTTON_R_ROLE, DRAW_Y + BUTTON_FONTSIZE_ROLE - ((DRAW_H - BUTTON_FONTSIZE_ROLE)>>1), "You draw", BUTTON_FONTSIZE_ROLE, ORANGE);
	
	fb_draw_line_wide(GUESS_X + BUTTON_R_ROLE, GUESS_Y + BUTTON_R_ROLE, GUESS_X + GUESS_W - BUTTON_R_ROLE, GUESS_Y + BUTTON_R_ROLE, BUTTON_R_ROLE, BLUE);
	fb_draw_text(GUESS_X + BUTTON_R_ROLE, GUESS_Y + BUTTON_FONTSIZE_ROLE - ((GUESS_H - BUTTON_FONTSIZE_ROLE)>>1), "You guess", BUTTON_FONTSIZE_ROLE, ORANGE);
	
	fb_draw_line_wide(RANDOM_X + BUTTON_R_ROLE, RANDOM_Y + BUTTON_R_ROLE, RANDOM_X + RANDOM_W - BUTTON_R_ROLE, RANDOM_Y + BUTTON_R_ROLE, BUTTON_R_ROLE, BLUE);
	fb_draw_text(RANDOM_X + BUTTON_R_ROLE, RANDOM_Y + BUTTON_FONTSIZE_ROLE - ((RANDOM_H - BUTTON_FONTSIZE_ROLE)>>1), "Rand role", BUTTON_FONTSIZE_ROLE, ORANGE);
	
	fb_draw_line_wide(FREE_X + BUTTON_R_ROLE, FREE_Y + BUTTON_R_ROLE, FREE_X + FREE_W - BUTTON_R_ROLE, FREE_Y + BUTTON_R_ROLE, BUTTON_R_ROLE, PURPLE);
	fb_draw_text(FREE_X + BUTTON_R_ROLE, FREE_Y + BUTTON_FONTSIZE_ROLE - ((FREE_H - BUTTON_FONTSIZE_ROLE)>>1), "Free mode", BUTTON_FONTSIZE_ROLE, ORANGE);
	
	fb_update();
}

#define PEN_Y_INIT 30

static int pen_y=PEN_Y_INIT;
static int score;
static int used[WORDS_LEN];
static int ra;
static int guessing=0;

static void draw_textframe(){
	
	if(role == NO_ROLE)return;
	pen_y = PEN_Y_INIT;
	char str[20];
	char bstr[20];
	if(role == DRAWER){
		fb_draw_text(TEXTFRAME_X+2, pen_y, "U: drawer. Score: ", 24, COLOR_TEXT);
		sprintf(str, "%d", score);
		fb_draw_text(TEXTFRAME_X+2+19*11, pen_y, str, 24, ORANGE);
		fb_update();
		pen_y += 30;
		if(!guessing){
			guessing = 1;
			ra = rand()%WORDS_LEN;
			int cnt = 0;
			while(used[ra]){
				if(cnt == WORDS_LEN){
					for(int i=0;i<WORDS_LEN;i++)	used[i] = 0;
				}
				ra = (ra+1)%WORDS_LEN;
				cnt++;
			}
			used[ra] = 1;
			sprintf(bstr, "6 %d \n", ra); 
			myWrite_nonblock(bluetooth_fd, bstr, 10);
			
		}
		fb_draw_text(TEXTFRAME_X+2, pen_y, "U draw: ", 24, COLOR_TEXT);
		fb_draw_text(TEXTFRAME_X+2+8*11, pen_y, words[ra], 24, PURPLE);
		fb_update();
		
	}else if(role == GUESSER){
		fb_draw_text(TEXTFRAME_X+2, pen_y, "U: guesser. Score: ", 24, COLOR_TEXT);
		sprintf(str, "%d", score);
		fb_draw_text(TEXTFRAME_X+2+20*11, pen_y, str, 24, ORANGE);
		fb_update();
		pen_y += 30;
		fb_draw_text(TEXTFRAME_X+2, pen_y, "U guess and speak ans.", 24, COLOR_TEXT);
		fb_update();
		
	}else if(role == BOTH){
		fb_draw_text(TEXTFRAME_X+2, pen_y, "Free mode. Score: ", 24, COLOR_TEXT);
		sprintf(str, "%d", score);
		fb_draw_text(TEXTFRAME_X+2+19*11, pen_y, str, 24, ORANGE);
		fb_update();
		pen_y += 30;
		if(!guessing){
			guessing = 1;
			ra = rand()%WORDS_LEN;
			while(used[ra])	ra = (ra+1)%WORDS_LEN;
			used[ra] = 1;
			sprintf(bstr, "6 %d \n", ra); 
			myWrite_nonblock(bluetooth_fd, bstr, 10);
			
		}
		fb_draw_text(TEXTFRAME_X+2, pen_y, "U draw: ", 24, COLOR_TEXT);
		fb_draw_text(TEXTFRAME_X+2+8*11, pen_y, words[ra], 24, PURPLE);
		fb_update();
	}else printf("invalid call: draw_textframe().\n");
	pen_y+=30;
	if(role != BOTH)
		fb_draw_border(TEXTFRAME_X, TEXTFRAME_Y, TEXTFRAME_W, pen_y+10, CYAN);
	else 
		fb_draw_border(TEXTFRAME_X, TEXTFRAME_Y, TEXTFRAME_W, pen_y+30+10, CYAN);
	fb_update();
}

static int timer=TIME_INIT;

static void draw_timer(){
	char buf[10];
	sprintf(buf, "%d", timer);
	fb_draw_rect(TIME_X, TIME_Y, TIME_W, TIME_H, COLOR_BACKGROUND);
	fb_draw_border(TIME_X, TIME_Y, TIME_W, TIME_H, COLOR_TEXT);
	if(role == BOTH){
		fb_draw_text(TIME_X-90, TIME_Y+20, "无限时 ", 24, COLOR_TEXT);
		fb_draw_text(TIME_X+2, TIME_Y+20, "999", 24, BLUE);
		fb_update();
		return;
	}
	fb_draw_text(TIME_X-90, TIME_Y+20, "倒计时:", 24, COLOR_TEXT);
	if(timer > 60)
		fb_draw_text(TIME_X+2, TIME_Y+20, buf, 24, BLUE);
	else if(timer > 10)
		fb_draw_text(TIME_X+2, TIME_Y+20, buf, 24, ORANGE);
	else
		fb_draw_text(TIME_X+2, TIME_Y+20, buf, 24, RED);
	fb_update();
	
	return;
}

static void clear_line3(){
	fb_draw_rect(TEXTFRAME_X+2, pen_y+6-28, TEXTFRAME_W-4, 28, COLOR_BACKGROUND);
	fb_update();
}

static void clear_line4(){
	fb_draw_rect(TEXTFRAME_X+2, pen_y+30+6-28, TEXTFRAME_W-4, 28, COLOR_BACKGROUND);
	fb_update();
}

static void update_score(){
	char str[20];
	if(role == NO_ROLE){
		printf("invalid call: update_score()\n");
		return;
	}
	if(role == DRAWER || role == BOTH){
		sprintf(str, "%d", score-1);
		fb_draw_text(TEXTFRAME_X+2+19*11, PEN_Y_INIT, str, 24, WHITE);
		sprintf(str, "%d", score);
		fb_draw_text(TEXTFRAME_X+2+19*11, PEN_Y_INIT, str, 24, ORANGE);
	}else{
		sprintf(str, "%d", score-1);
		fb_draw_text(TEXTFRAME_X+2+20*11, PEN_Y_INIT, str, 24, WHITE);
		sprintf(str, "%d", score);
		fb_draw_text(TEXTFRAME_X+2+20*11, PEN_Y_INIT, str, 24, ORANGE);
	}
	fb_update();
}


static void timer_cb(int period) /*该函数1秒执行一次*/
{
	if(role != NO_ROLE && role != BOTH){
		if(timer != 0)
			timer --;
		else{
			clear_line3();
			fb_draw_text(TEXTFRAME_X+2, pen_y, "Time out!", 24, RED);
			fb_update();
			guessing = 0;
			role = -1;
			timer = TIME_INIT;
			draw_role_buttons();
			return;
		}
	}
	draw_timer();
}


static void init_game(){
	//restart everything
	role = NO_ROLE;
	guessing = 0;
	timer = TIME_INIT;
	score = 0;
	line_r = LINE_R_INIT;
	line_r_op = LINE_R_INIT;
	color_index = 0;
	color_index_op = 0;
	eraser = 0;
	eraser_op = 0;
	for(int i=0;i<WORDS_LEN;i++)	used[i] = 0;
	draw_background();
	draw_role_buttons();
	printf("The game is ready!\n Please choose a role and the other will have the other role.");
}


static void draw_role1(){
	draw_background();			
	if(role == DRAWER)
		draw_clear_button();
	else if(role == BOTH)
		draw_speak_button();
	else printf("invalid call: draw_role1.\n");
	draw_textframe();
	draw_tools();
	draw_usingtoolframe();
	draw_timer();
}

static void draw_role2(){
	draw_background();
	draw_speak_button();
	draw_textframe();
	draw_timer();
}

static void bluetooth_tty_event_cb(int fd)
{
	char buf[128];
	char sword[128];
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
	int x, y, finger, x2, y2, type, color=COLOR_BACKGROUND;
	sscanf(buf, "%d %d %d %d %d %d %d\n", &event_type, &type, &x, &y, &x2, &y2, &finger);
	char bstr[100];
	switch(event_type){
		case 0: 
			// choose role
			//sscanf(buf, "%d", &role);
			role = type;
			printf("your role is :%d\n",role);
			if(role == DRAWER){
				printf("your role is drawer.\n");
				draw_role1();
			}	
			else if(role == GUESSER){
				printf("your role is guesser.\n");
				draw_role2();
			}
			else	if(role == BOTH){
				printf("your role is drawer and guesser(free mode).\n");
				draw_role1();
			}
			else	printf("warning : you received a invalid message.(case 0)\n");
			break;
		case 1:
			// clear. the receiver must be guesser, no need to draw the clear button.
			if(role != GUESSER){
				printf("warning : you received a invalid message.(case 1)\n");
				break;
			}
			fb_draw_rect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,COLOR_BACKGROUND);
			draw_speak_button();
			draw_textframe();
			break;
		case 2:
			// draw what the drawer drew
			//sscanf(buf, "%d", &type);
			if(type == 0){
				//sscanf(buf, "%d%d%d", &x, &y, &finger);
				finger = x2;
				color = get_color_op(finger);
				fb_draw_circle(x, y, line_r_op, color);
				fb_update();
				
				break;
			}else if (type == 1){
				//sscanf(buf, "%d%d%d%d%d", &x, &y, &x2, &y2, &finger);
				color = get_color_op(finger);
				fb_draw_line_wide(x, y, x2, y2, line_r_op, color);
				fb_update();
				break;
			}else printf("warning : you received a invalid message.(case 2)\n");
			break;
		case 3:
			// speak
			sscanf(buf, "%d %s\n",&event_type, sword);
			printf("speak word: %s\n", sword);
			if(role == DRAWER){
				clear_line3();
				fb_draw_text(TEXTFRAME_X+2, pen_y, "OP Said: ", 24, COLOR_TEXT);
				fb_draw_text(TEXTFRAME_X+2+9*11, pen_y, sword, 24, ORANGE);
				fb_update();
				
				if(strcmp(sword, words[ra])==0){
					fb_draw_text(TEXTFRAME_X+2+(9+strlen(sword))*11, pen_y, ",NICE!", 24, GREEN);
					fb_update();
					score++;
					update_score();
					guessing = 0;
					role = -1;
					timer = TIME_INIT;
					draw_role_buttons();
					sprintf(bstr, "4 0 \n");
					myWrite_nonblock(bluetooth_fd, bstr, 5);
				}else{
					sprintf(bstr, "4 1 \n");
					myWrite_nonblock(bluetooth_fd, bstr, 5);
				}
			}else if(role == BOTH){
				clear_line3();
				fb_draw_text(TEXTFRAME_X+2, pen_y, "OP Said: ", 24, COLOR_TEXT);
				fb_draw_text(TEXTFRAME_X+2+9*11, pen_y, sword, 24, ORANGE);
				fb_update();
				
				if(strcmp(sword, words[ra])==0){
					fb_draw_text(TEXTFRAME_X+2+(9+strlen(sword))*11, pen_y, ",NICE!", 24, GREEN);
					fb_update();
					score++;
					update_score();
					guessing = 0;
					sprintf(bstr, "4 0 \n");
					myWrite_nonblock(bluetooth_fd, bstr, 5);
				}else{
					sprintf(bstr, "4 1 \n");
					myWrite_nonblock(bluetooth_fd, bstr, 5);
				}
			}else printf("warning : you received a invalid message.(case 3)\n");
			break;
		case 4:
			//reply
			//sscanf(buf, "%d", &type);
			if(role == GUESSER){
				if(type == 0){
					score++;
					update_score();
					guessing = 0;
					role = -1;
					clear_line3();
					fb_draw_text(TEXTFRAME_X+2, pen_y, "Your answer is right!", 24, GREEN);
					fb_update();
					timer = TIME_INIT;
					draw_role_buttons();
				}else{
					clear_line3();
					fb_draw_text(TEXTFRAME_X+2, pen_y, "Your answer is wrong!", 24, ORANGE);
					fb_update();
				}
			}else if(role == BOTH){
				if(type == 0){
					score++;
					update_score();
					clear_line4();
					fb_draw_text(TEXTFRAME_X+2, pen_y+30, "Your answer is right!", 24, GREEN);
					fb_update();
				}else{
					clear_line4();
					fb_draw_text(TEXTFRAME_X+2, pen_y+30, "Your answer is wrong!", 24, ORANGE);
					fb_update();
				}
			}
			else printf("warning : you received a invalid message.(case 4)\n");
			
			break;
		case 5:
			//change the op tool
			if(role == DRAWER){
				printf("warning : you received a invalid message.(case 5)\n");
				break;
			}
			if(type == -1){
				eraser_op ^= 1;
			}
			else if(type == -2){
				//change size
				line_r_op = x;
			}
			else{
				if(eraser)
					eraser_op = 0;
				color_index_op = type;
			}
			break;
		case 6:
			if(type < 0 || type > WORDS_LEN){
				printf("warning : you received a invalid message.(case 6)\n");
				break;
			}
			used[type] = 1;
			break;
	}
	
	return;
}

static void touch_event_cb(int fd)
{
	int type,x,y,finger;
	type = touch_read(fd, &x,&y,&finger);
	int color = COLOR_BACKGROUND;
	char bstr[132];
	if(role == NO_ROLE){
		if(type != TOUCH_PRESS )return;
		//printf("type=%d,x=%d,y=%d,finger=%d\n",type,x,y,finger);
		if((x>=RANDOM_X)&&(x<RANDOM_X+RANDOM_W)&&(y>=RANDOM_Y)&&(y<RANDOM_Y+RANDOM_H)) {
			role = rand()%2+1;
		}
		if((role == DRAWER) || ( (x>=DRAW_X)&&(x<DRAW_X+DRAW_W)&&(y>=DRAW_Y)&&(y<DRAW_Y+DRAW_H) )) {
			role = DRAWER;
			printf("you chose to draw (you are role 1).\n");
			sprintf(bstr, "0 2 \n"); // first 0 means switching role, second 2 means the opposite role
			draw_role1();
			
			myWrite_nonblock(bluetooth_fd, bstr, 5);
			return;
		}
		if((role == GUESSER) || ( (x>=GUESS_X)&&(x<GUESS_X+GUESS_W)&&(y>=GUESS_Y)&&(y<GUESS_Y+GUESS_H) )) {
			role = GUESSER;
			printf("you chose to guess (you are role 2).\n");
			sprintf(bstr, "0 1 \n");
			draw_role2();
			
			myWrite_nonblock(bluetooth_fd, bstr, 5);
			return;
		}
		if( (x>=FREE_X)&&(x<FREE_X+FREE_W)&&(y>=FREE_Y)&&(y<FREE_Y+FREE_H) ) {
			role = BOTH;
			printf("you chose the free mode.\n");
			sprintf(bstr, "0 3 \n");
			draw_role1();
			
			myWrite_nonblock(bluetooth_fd, bstr, 5);
			return;
		}
		return;
	}
	switch(type){
		case TOUCH_PRESS:
			//printf("type=%d,x=%d,y=%d,finger=%d\n",type,x,y,finger);
			
			if(x >= BUTTON_X_CLEAR - line_r && x < BUTTON_X2_CLEAR + line_r && y >= BUTTON_Y_CLEAR && y < BUTTON_Y2_CLEAR + line_r){
				if(role == DRAWER){
					//clear
					draw_role1();
					
					sprintf(bstr, "1\n");
					myWrite_nonblock(bluetooth_fd, bstr, 2);				
				}else {
					//speak
					pcm_info_st pcm_info;
					if(role == GUESSER){
						clear_line3();
						fb_draw_text(TEXTFRAME_X+2, pen_y, "Recording...", 24, PURPLE);
					}else {
						clear_line4();
						fb_draw_text(TEXTFRAME_X+2, pen_y+30, "Recording...", 24, PURPLE);
					}
					fb_update();

					uint8_t *pcm_buf = audio_record(3000, &pcm_info); //录3秒音频

					if(pcm_info.sampleRate != PCM_SAMPLE_RATE) { //实际录音采用率不满足要求时 resample
						uint8_t *pcm_buf2 = pcm_s16_mono_resample(pcm_buf, &pcm_info, PCM_SAMPLE_RATE, &pcm_info);
						pcm_free_buf(pcm_buf);
						pcm_buf = pcm_buf2;
					}

					pcm_write_wav_file(pcm_buf, &pcm_info, "/tmp/test.wav");
					printf("write wav end\n");

					if(role == GUESSER){
						clear_line3();
						fb_draw_text(TEXTFRAME_X+2, pen_y, "Waiting for reply...", 24, PURPLE);
					}else {
						clear_line4();
						fb_draw_text(TEXTFRAME_X+2, pen_y+30, "Waiting for reply...", 24, PURPLE);
					}
					fb_update();
					
					pcm_free_buf(pcm_buf);

					char *rev = send_to_vosk_server("/tmp/test.wav");
					printf("recv from server: %s\n", rev);
					sprintf(bstr, "3 %s\n", rev);
					myWrite_nonblock(bluetooth_fd, bstr, strlen(bstr));
					
				}

				break;
			}
			
			if(role == GUESSER){
				//
				printf("You cannot draw because you are the guesser.\n");
				break;
			}
			
			if(x >= 0 && x < TOOL_SIZE + line_r && y >= BUTTON_Y_CLEAR - TOOL_SIZE && y < BUTTON_Y_CLEAR){
				// color-palette
				clear_usingtoolframe();
				if(eraser)
					eraser = 0;
				else
					color_index = (color_index + 1)%7;
				draw_usingtoolframe();
				update_sizetool();
				sprintf(bstr, "5 %d\n", color_index);
				myWrite_nonblock(bluetooth_fd, bstr, 5);
				break;
			}
			if(x >= 0 && x < TOOL_SIZE + line_r && y >= BUTTON_Y_CLEAR - (TOOL_SIZE<<1)&& y < BUTTON_Y_CLEAR - TOOL_SIZE){
				// eraser
				clear_usingtoolframe();
				eraser ^= 1;
				draw_usingtoolframe();

				sprintf(bstr, "5 -1\n");
				myWrite_nonblock(bluetooth_fd, bstr, 5);
				break;
			}
			if(x >= 0 && x < TOOL_SIZE + line_r && y >= BUTTON_Y_CLEAR - (TOOL_SIZE*3) - line_r && y < BUTTON_Y_CLEAR - (TOOL_SIZE<<1)){
				// change size
				change_line_r();

				sprintf(bstr, "5 -2 %d\n", line_r);
				myWrite_nonblock(bluetooth_fd, bstr, 8);
				break;
			}
			if(role == DRAWER){
				if(x >= 0 && x < TEXTFRAME_X + TEXTFRAME_W + line_r && y >= 0 && y < TEXTFRAME_Y + pen_y + 6 + line_r)
					break;
			}else{
				if(x >= 0 && x < TEXTFRAME_X + TEXTFRAME_W + line_r && y >= 0 && y < TEXTFRAME_Y + pen_y + 30 + 6 + line_r)
					break;
			}
			
			color = get_color(finger);
			fb_draw_circle(x, y, line_r, color);
			fb_update();
			old[finger].x = x;
			old[finger].y = y;
			sprintf(bstr, "2 0 %d %d %d\n", x, y, finger);
			myWrite_nonblock(bluetooth_fd, bstr, 18);
			break;
			
		case TOUCH_MOVE:
			//printf("TOUCH_MOVE：x=%d,y=%d,finger=%d\n",x,y,finger);
			if(role == GUESSER){
				//printf("You cannot draw because you are the guesser.\n");
				break;
			}
			color = get_color(finger);
			if((role == DRAWER) && (x >= 0 && x < TEXTFRAME_X + TEXTFRAME_W + line_r && y >= 0 && y < TEXTFRAME_Y + pen_y + 6 + line_r)){
				if(old[finger].x < TEXTFRAME_X + TEXTFRAME_W + line_r){
					y = TEXTFRAME_Y + pen_y + 6 + line_r;
				} else {
					x = TEXTFRAME_X + TEXTFRAME_W + line_r;
				}
			}else
			if((role == BOTH) && (x >= 0 && x < TEXTFRAME_X + TEXTFRAME_W + line_r && y >= 0 && y < TEXTFRAME_Y + pen_y + 30 + 6 + line_r)){
				if(old[finger].x < TEXTFRAME_X + TEXTFRAME_W + line_r){
					y = TEXTFRAME_Y + pen_y + 30 + 6 + line_r;
				} else {
					x = TEXTFRAME_X + TEXTFRAME_W + line_r;
				}
			}else
			if(x >= 0 && x < TOOL_SIZE + line_r && y >= BUTTON_Y_CLEAR - (TOOL_SIZE*3) - line_r && y < BUTTON_Y_CLEAR - (TOOL_SIZE<<1)){
				if(old[finger].x < TOOL_SIZE + line_r){
					y = BUTTON_Y_CLEAR - (TOOL_SIZE*3) - line_r;
				} else {
					x = TOOL_SIZE + line_r;
				}
			}else
			if(x >= 0 && x < TOOL_SIZE + line_r && y >= BUTTON_Y_CLEAR - (TOOL_SIZE<<1) && y < BUTTON_Y_CLEAR - TOOL_SIZE){
				if(old[finger].x < TOOL_SIZE + line_r){
					y = BUTTON_Y_CLEAR - (TOOL_SIZE*3) - line_r;
				} else {
					x = TOOL_SIZE + line_r;
				}
			}else
			if(x >= 0 && x < TOOL_SIZE + line_r && y >= BUTTON_Y_CLEAR - TOOL_SIZE && y < BUTTON_Y_CLEAR){
			
				if(old[finger].x < TOOL_SIZE + line_r){
					y = BUTTON_Y_CLEAR - (TOOL_SIZE*3) - line_r;
				} else {
					x = TOOL_SIZE + line_r;
				}
			}else
			if(x >= BUTTON_X_CLEAR - line_r && x < BUTTON_X2_CLEAR + line_r && y >= BUTTON_Y_CLEAR - line_r && y < BUTTON_Y2_CLEAR + line_r){
				if(old[finger].x < BUTTON_X2_CLEAR + line_r){
					y = BUTTON_Y_CLEAR - line_r;
				} else {
					x = BUTTON_X2_CLEAR + line_r;
				}
			}
			
			fb_draw_line_wide(old[finger].x, old[finger].y, x, y, line_r, color);
			fb_update();
			sprintf(bstr, "2 1 %d %d %d %d %d\n", old[finger].x, old[finger].y, x, y, finger);
			myWrite_nonblock(bluetooth_fd, bstr, 26);
			old[finger].x = x;
			old[finger].y = y;
			
			break;
		
		case TOUCH_RELEASE:
			//printf("TOUCH_RELEASE：x=%d,y=%d,finger=%d\n",x,y,finger);
			break;
		
		case TOUCH_ERROR:
			printf("close touch fd\n");
			task_delete_file(fd);
			close(fd);
			break;
		default:
			return;
	}
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


int main(int argc, char *argv[])
{
	fb_init("/dev/fb0");
	font_init("./font.ttc");
	srand(time(NULL));
	audio_record_init(NULL, PCM_SAMPLE_RATE, 1, 16); //单声道，S16采样
	init_game();
	
	
	touch_fd = touch_init("/dev/input/event1");
	task_add_file(touch_fd, touch_event_cb);
	
	bluetooth_fd = bluetooth_tty_init("/dev/rfcomm0");
	if(bluetooth_fd == -1) return 0;
	task_add_file(bluetooth_fd, bluetooth_tty_event_cb);

	task_add_timer(1000, timer_cb); /*增加1秒的定时器*/
	task_loop();
	return 0;
}


/*===============================================================*/	

#define IP "127.0.0.1"
#define PORT 8011

#define print_err(fmt, ...) \
	printf("%d:%s " fmt, __LINE__, strerror(errno), ##__VA_ARGS__);

static char * send_to_vosk_server(char *file)
{
	static char ret_buf[128]; //识别结果

	if((file == NULL)||(file[0] != '/')) {
		print_err("file %s error\n", file);
		return NULL;
	}

	int skfd = -1, ret = -1;
	skfd = socket(AF_INET, SOCK_STREAM, 0);
	if(skfd < 0) {
		print_err("socket failed\n");
		return NULL;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = inet_addr(IP);
	ret = connect(skfd,(struct sockaddr*)&addr, sizeof(addr));
	if(ret < 0) {
		print_err("connect failed\n");
		close(skfd);
		return NULL;
	}

	printf("send wav file name: %s\n", file);
	ret = send(skfd, file, strlen(file)+1, 0);
	if(ret < 0) {
		print_err("send failed\n");
		close(skfd);
		return NULL;
	}

	ret = recv(skfd, ret_buf, sizeof(ret_buf)-1, 0);
	if(ret < 0) {
		print_err("recv failed\n");
		close(skfd);
		return NULL;
	}
	ret_buf[ret] = '\0';
	return ret_buf;
}
