#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>
#include "../common/common.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "audio_util.h"
#include "config.h"

#include "../common/external/include/png.h"

static int touch_fd;
static int bluetooth_fd;

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

static int score;
static int used[WORDS_LEN];
static int ra;
static int guessing=0;
static int in_image_page;

static int timer=TIME_INIT;

static char png_files[50][50]; // 假设最多有50个PNG文件，并且文件名最长为50个字符

static int png_count = 0;

static int png_now = 0;

int touch_area_game(int x, int y);

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

/*
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

*/

static void update_sizetool(){
	fb_draw_circle(TOOL_X1 + (TOOL_SIZE>>1), TOOL_Y1 + (TOOL_SIZE>>1), line_r, get_color_real(0));
	fb_update();
}

static void change_line_r(){
	fb_draw_circle(TOOL_X1 + (TOOL_SIZE>>1), TOOL_Y1 + (TOOL_SIZE>>1), line_r, WHITE);
	fb_update();
	line_r <<= 1;
	if(line_r > LINE_R_MAX)
		line_r = LINE_R_MIN;
	update_sizetool();
}



static void clear_usingtoolframe(){
	if(eraser){
		fb_draw_border(TOOL_X2, TOOL_Y2, TOOL_SIZE, TOOL_SIZE, WHITE);
		fb_update();
		return;
	}
	fb_draw_border(TOOL_X3, TOOL_Y3, TOOL_SIZE, TOOL_SIZE, WHITE);
	fb_update();
	return;
}

static void draw_usingtoolframe(){
	if(eraser){
		fb_draw_border(TOOL_X2, TOOL_Y2, TOOL_SIZE, TOOL_SIZE, BLACK);
		fb_update();
		return;
	}
	int color = get_color(0);
	fb_draw_border(TOOL_X3, TOOL_Y3, TOOL_SIZE, TOOL_SIZE, color);
	fb_update();
	return;
}

static void draw_page(char *page_image){
	fb_image *img;
	img = fb_read_png_image(page_image);
	fb_draw_image(0,0,img,0);
	fb_update();
	fb_free_image(img);	
}
/*

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

*/
static void draw_timer(){
	if(timer < 0){
		printf("timer error!\n");
		return;
	}
	char buf[12];
	sprintf(buf, "%d", timer);
	//printf("timer = %d\n", timer);
	fb_image *img;
	img = fb_read_png_image("./pictures/time-bar.png");
	fb_draw_image(TIMEBAR_X,TIMEBAR_Y,img,0);
	fb_update();
	fb_free_image(img);	
	/*
	if(role == BOTH){
		fb_draw_text(TIME_X-90, TIME_Y+20, "无限时 ", 24, COLOR_TEXT);
		fb_draw_text(TIME_X+2, TIME_Y+20, "999", 24, BLUE);
		fb_update();
		return;
	}
	*/
	/*
	fb_draw_text(TIME_X-90, TIME_Y+20, "倒计时:", 24, COLOR_TEXT);
	if(timer > 60)
		fb_draw_text(TIME_X+2, TIME_Y+20, buf, 24, BLUE);
	else if(timer > 10)
		fb_draw_text(TIME_X+2, TIME_Y+20, buf, 24, ORANGE);
	else
		fb_draw_text(TIME_X+2, TIME_Y+20, buf, 24, RED);
	fb_update();
	
	*/
	int color = CYAN;
	if(timer < (TIME_INIT >> 1)){
		color = YELLOW;
		if(timer < (TIME_INIT / 5)){
			color = RED;
		}
	}

	fb_draw_line_wide(TIME_LINE_CIRCLE_X1 + TIME_INIT - timer, TIME_LINE_CIRCLE_Y, TIME_LINE_CIRCLE_X2, TIME_LINE_CIRCLE_Y, TIME_LINE_CIRCLE_R, color);
	img = fb_read_png_image("./pictures/clock.png");
	fb_draw_image(CLOCK_X + TIME_INIT - timer, CLOCK_Y, img, 0);
	fb_update();
	fb_free_image(img);	

	return;
}

/*
static void clear_line3(){
	fb_draw_rect(TEXTFRAME_X+2, pen_y+6-28, TEXTFRAME_W-4, 28, COLOR_BACKGROUND);
	fb_update();
}

static void clear_line4(){
	fb_draw_rect(TEXTFRAME_X+2, pen_y+30+6-28, TEXTFRAME_W-4, 28, COLOR_BACKGROUND);
	fb_update();
}

*/


static void update_score(){
	char str[20];
	if(role == NO_ROLE){
		printf("invalid call: update_score()\n");
		return;
	}
	sprintf(str, "%d", score-1);
	fb_draw_text(SCORE_X, SCORE_Y, str, HEAD_FONTSIZE, COLOR_BACKGROUND);
	sprintf(str, "%d", score);
	fb_draw_text(SCORE_X, SCORE_Y, str, HEAD_FONTSIZE, ORANGE);
	fb_update();
}

static void open_png_files(const char* folder_path){
	/*
	img = fb_read_png_image("./22-1703316101.png");
	fb_draw_image(IMAGE1_X1,IMAGE1_Y1,img,0);
	fb_update();
	fb_free_image(img);	
	*/

	DIR *folder = opendir(folder_path);
	if (folder == NULL) {
			perror("Unable to read directory");
			return ;
	}
	png_count = 0;
	struct dirent *entry;

	while ((entry = readdir(folder)) != NULL) {
			char *dot = strrchr(entry->d_name, '.');
			if (dot && strcmp(dot, ".png") == 0) {
					strcpy(png_files[png_count], entry->d_name);
					png_count++;
			}
			if(png_count > 50){
				printf("Too many png files.Cannot load all.\n");
				break;
			}
	}

	closedir(folder);

	// 打印获取到的PNG文件名
	printf("png file list:\n");
	for (int i = 0; i < png_count; i++) {
			printf("%d: %s\n", i, png_files[i]);
	}
}

static void draw_collection(){
	fb_image *img;
	img = fb_read_png_image(png_files[png_now]);
	fb_draw_image(IMAGE1_X1, IMAGE1_Y1, img, 0);
	fb_update();
	fb_free_image(img);	
}

static void into_image_page(){
	draw_background();
	in_image_page = 1;
	draw_page("./pictures/collection.png");
	
	open_png_files("./collections");
	png_now = 0;
	draw_collection();

}

static void into_drawer_page(){
	draw_background();
	draw_page("./pictures/board_drawer.png");
	draw_usingtoolframe();
	char str[20];
	char bstr[20];

	sprintf(str, "%d", score);
	fb_draw_text(SCORE_X, SCORE_Y, str, HEAD_FONTSIZE, ORANGE);
	fb_update();

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
	fb_draw_rect(DRAW_TOPIC_FONT_X, DRAW_TOPIC_FONT_Y - HEAD_FONTSIZE, WORDS_MAX_W, HEAD_FONTSIZE + 2, COLOR_BACKGROUND);
	fb_draw_text(DRAW_TOPIC_FONT_X, DRAW_TOPIC_FONT_Y, words[ra], HEAD_FONTSIZE, PURPLE);
	fb_update();

}

static void into_guesser_page(){
	draw_background();
	draw_page("./pictures/board_guesser.png");
	char str[20];
	
	draw_guesser_reply("./pictures/message-fighting.png");
	sprintf(str, "%d", score);
	fb_draw_text(SCORE_X, SCORE_Y, str, HEAD_FONTSIZE, ORANGE);
	fb_update();

}

static void timer_cb(int period) /*该函数1秒执行一次*/
{
	if(role != NO_ROLE && role != BOTH){
		if(timer != 0){
			timer --;
			draw_timer();
		}else{
			if(role == GUESSER){
				draw_guesser_reply("./pictures/message-timeout.png");
			}else{
				draw_drawer_reply("./pictures/message-timeout.png");
			}
			into_next_turn();
			return;
		}
	}

}


static void into_main_page(){
	//restart everything from main page
	draw_background();
	srand(time(NULL));
	role = NO_ROLE;
	guessing = 0;
	in_image_page = 0;
	timer = TIME_INIT;
	score = 0;
	line_r = LINE_R_INIT;
	line_r_op = LINE_R_INIT;
	color_index = 0;
	color_index_op = 0;
	eraser = 0;
	eraser_op = 0;
	for(int i=0;i<WORDS_LEN;i++)	used[i] = 0;
	draw_page("./pictures/main.png");

	printf("The game is ready!\n");
}

/*

static void draw_role1(){
	if(role == DRAWER){
		draw_background();	
	}else{
		fb_draw_rect(TEXTFRAME_X, TEXTFRAME_Y, TEXTFRAME_W, pen_y+30+10, COLOR_BACKGROUND);
		fb_update();
	}
			
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
*/
static void into_next_turn(){
	guessing = 0;
	timer = TIME_INIT;
	char bstr[20];
	if((role == DRAWER)) {
		role = GUESSER;
		printf("you are guesser.\n");
		sprintf(bstr, "0 1 \n");
		into_guesser_page();
		myWrite_nonblock(bluetooth_fd, bstr, 5);
		return;
	}
	if((role == GUESSER)) {
		role = DRAWER;
		printf("you are drawer.\n");
		sprintf(bstr, "0 2 \n");
		into_drawer_page();
		myWrite_nonblock(bluetooth_fd, bstr, 5);
		return;
	}
	/*
						score++;
					update_score();
					guessing = 0;
					role = NO_ROLE;
					timer = TIME_INIT;
					draw_role_buttons();
	*/
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
				//draw_role1();
				into_drawer_page();
			}	
			else if(role == GUESSER){
				printf("your role is guesser.\n");
				//draw_role2();
				into_guesser_page();
			}
			else	if(role == BOTH){
				printf("your role is drawer and guesser.\n");
				//draw_role1();
				into_drawer_page();
			}
			else	printf("warning : you received a invalid message.(case 0)\n");
			break;
		case 1:
			// clear. the receiver must be guesser, no need to draw the clear button.
			if(role == DRAWER){
				printf("warning : you received a invalid message.(case 1)\n");
				break;
			}
			clear_board();
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
			if(role == DRAWER || role == BOTH){
				/*
				clear_line3();
				fb_draw_text(TEXTFRAME_X+2, pen_y, "OP Said: ", 24, COLOR_TEXT);
				fb_draw_text(TEXTFRAME_X+2+9*11, pen_y, sword, 24, ORANGE);
				fb_update();				
				*/
				if(strcmp(sword, words[ra])==0){
					/*
					fb_draw_text(TEXTFRAME_X+2+(9+strlen(sword))*11, pen_y, ",NICE!", 24, GREEN);
					fb_update();					
					*/
					score++;
					update_score();
					draw_drawer_reply("./pictures/message-yes.png");
					into_next_turn();
					sprintf(bstr, "4 0 \n");
					myWrite_nonblock(bluetooth_fd, bstr, 5);
				}else{
					draw_drawer_reply("./pictures/message-no.png");
					sprintf(bstr, "4 1 \n");
					myWrite_nonblock(bluetooth_fd, bstr, 5);
				}
			}else printf("warning : you received a invalid message.(case 3)\n");
			break;
			/*
			else if(role == BOTH){
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
					draw_role1();
					fb_draw_text(TEXTFRAME_X+2, pen_y, "OP Said: ", 24, COLOR_TEXT);
					fb_draw_text(TEXTFRAME_X+2+9*11, pen_y, sword, 24, ORANGE);
					fb_draw_text(TEXTFRAME_X+2+(9+strlen(sword))*11, pen_y, ",NICE!", 24, GREEN);
					fb_update();
					sprintf(bstr, "4 0 \n");
					myWrite_nonblock(bluetooth_fd, bstr, 5);
				}else{
					sprintf(bstr, "4 1 \n");
					myWrite_nonblock(bluetooth_fd, bstr, 5);
				}
			*/

		case 4:
			//reply
			//sscanf(buf, "%d", &type);
			if(role == GUESSER || role == BOTH){
				if(type == 0){
					score++;
					update_score();
					draw_guesser_reply("./pictures/message-yes.png");
					into_next_turn();
					/*
						role = NO_ROLE;
					clear_line3();
					fb_draw_text(TEXTFRAME_X+2, pen_y, "Your answer is right!", 24, GREEN);
					fb_update();
					draw_role_buttons();
					*/
				}else{
					draw_guesser_reply("./pictures/message-no.png");
					//clear_line3();
					//fb_draw_text(TEXTFRAME_X+2, pen_y, "Your answer is wrong!", 24, ORANGE);
					//fb_update();
				}
			}else printf("warning : you received a invalid message.(case 4)\n");
			break;
			/*
			else if(role == BOTH){
				if(type == 0){
					score++;
					guessing = 0;
					update_score();
					//draw_textframe();
					draw_role1();
					clear_line4();
					fb_draw_text(TEXTFRAME_X+2, pen_y+30, "Your answer is right!", 24, GREEN);
					fb_update();
				}else{
					clear_line4();
					fb_draw_text(TEXTFRAME_X+2, pen_y+30, "Your answer is wrong!", 24, ORANGE);
					fb_update();
				}
			}
			*/
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
				if(eraser_op)
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
			if(role == GUESSER)
				ra = type;
			break;
	}
	
	return;
}

int touch_area_game(int x, int y){
	if(x >= BOARD_X1 && x < BOARD_X2 && y >= BOARD_Y1 && y < BOARD_Y2){
		if(role == GUESSER)
			return TOUCH_INVALID;
		return TOUCH_BOARD;
	}
	if(x >= TOOL_X1 && x < TOOL_X1 + TOOL_SIZE && y >= TOOL_Y1 && y < TOOL_Y1 + TOOL_SIZE){
		if(role == GUESSER)
			return TOUCH_INVALID;
		return TOUCH_SIZE;
	}
	if(x >= TOOL_X2 && x < TOOL_X2 + TOOL_SIZE && y >= TOOL_Y2 && y < TOOL_Y2 + TOOL_SIZE){
		if(role == GUESSER)
			return TOUCH_INVALID;
		return TOUCH_ERASER;
	}
	if(x >= TOOL_X3 && x < TOOL_X3 + TOOL_SIZE && y >= TOOL_Y3 && y < TOOL_Y3 + TOOL_SIZE){
		if(role == GUESSER)
			return TOUCH_INVALID;
		return TOUCH_COLOR_BOARD;
	}
	if(x >= TOOL_X4 && x < TOOL_X4 + TOOL_SIZE && y >= TOOL_Y4 && y < TOOL_Y4 + TOOL_SIZE){
		if(role == GUESSER)
			return TOUCH_INVALID;
		return TOUCH_DUSTBIN;
	}
	if(x >= SPEAK_X && x < SPEAK_X + TOOL_SIZE && y >= SPEAK_Y && y < SPEAK_Y + TOOL_SIZE){
		if(role == DRAWER)
			return TOUCH_INVALID;
		return TOUCH_SPEAK;
	}
	if(x >= SAVE_X && x < SAVE_X + TOOL_SIZE && y >= SAVE_Y && y < SAVE_Y + TOOL_SIZE)
		return TOUCH_SAVE;
	if(x >= EXIT_X && x < EXIT_X + TOOL_SIZE && y >= EXIT_Y && y < EXIT_Y + TOOL_SIZE)
		return TOUCH_EXIT;
	return TOUCH_INVALID;
}

static void handle_speak(){
	char bstr[132];
	pcm_info_st pcm_info;
	if(role == GUESSER)
		draw_guesser_reply("./pictures/message-recording.png");
	uint8_t *pcm_buf = audio_record(3000, &pcm_info); //录3秒音频
	if(pcm_info.sampleRate != PCM_SAMPLE_RATE) { //实际录音采用率不满足要求时 resample
		uint8_t *pcm_buf2 = pcm_s16_mono_resample(pcm_buf, &pcm_info, PCM_SAMPLE_RATE, &pcm_info);
		pcm_free_buf(pcm_buf);
		pcm_buf = pcm_buf2;
	}
	pcm_write_wav_file(pcm_buf, &pcm_info, "/tmp/test.wav");
	printf("write wav end\n");
	if(role == GUESSER)
		draw_guesser_reply("./pictures/message-waiting.png");		
	pcm_free_buf(pcm_buf);
	char *rev = send_to_vosk_server("/tmp/test.wav");
	printf("recv from server: %s\n", rev);
	sprintf(bstr, "3 %s\n", rev);
	myWrite_nonblock(bluetooth_fd, bstr, strlen(bstr));
}

static void touch_handle_game(int x, int y, int finger){
	int touch_area = touch_area_game(x,y);
	int color;
	char bstr[132];
	switch (touch_area)
	{
		case TOUCH_INVALID:
			printf("Invalid touch.\n");
			break;
		case TOUCH_BOARD:
			color = get_color(finger);
					fb_draw_circle(x, y, line_r, color);
					fb_update();
					old[finger].x = x;
					old[finger].y = y;
					sprintf(bstr, "2 0 %d %d %d\n", x, y, finger);
					myWrite_nonblock(bluetooth_fd, bstr, 18);
			break;
		case TOUCH_SIZE:
			// change size
			change_line_r();
			sprintf(bstr, "5 -2 %d\n", line_r);
			myWrite_nonblock(bluetooth_fd, bstr, 8);
			break;
		case TOUCH_ERASER:
			// eraser
			clear_usingtoolframe();
			eraser ^= 1;
			draw_usingtoolframe();
			sprintf(bstr, "5 -1\n");
			myWrite_nonblock(bluetooth_fd, bstr, 5);
			break;
		case TOUCH_COLOR_BOARD:
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
		case TOUCH_DUSTBIN:
			//clear
			clear_board();
			sprintf(bstr, "1\n");
			myWrite_nonblock(bluetooth_fd, bstr, 2);		
			break;
		case TOUCH_SPEAK:
			//speak
			handle_speak();
			
			break;
		case TOUCH_SAVE:
			printf("Save the picture.\n");
			draw_save_reply("保存中...");
			capture_screen_region(BOARD_X1, BOARD_Y1, BOARD_X2 - BOARD_X1, BOARD_Y2 - BOARD_Y1);
			draw_save_reply("已保存！");
			break;
		case TOUCH_EXIT:
			printf("Exit.\n");
			into_main_page();
			break;
		default:
			break;
	}
			
}

static void move_handle_game(int x, int y, int finger){
	if(role == GUESSER){
		//printf("You cannot draw because you are the guesser.\n");
		return;
	}
	char bstr[132];
	int color = get_color(finger);
	if(x < BOARD_X1)
		x = BOARD_X1;
	else if(x >= BOARD_X2)
		x = BOARD_X2-1;
	if(y < BOARD_Y1)
		y = BOARD_Y1;
	else if(y >= BOARD_Y2)
		y = BOARD_Y2-1;

	
	fb_draw_line_wide(old[finger].x, old[finger].y, x, y, line_r, color);
	fb_update();
	sprintf(bstr, "2 1 %d %d %d %d %d\n", old[finger].x, old[finger].y, x, y, finger);
	myWrite_nonblock(bluetooth_fd, bstr, 26);
	old[finger].x = x;
	old[finger].y = y;
	return;
}

static void clear_board(){
	fb_draw_rect(BOARD_X1, BOARD_Y1, BOARD_X2 - BOARD_X1, BOARD_Y2 - BOARD_Y1, COLOR_BACKGROUND);
	fb_draw_line_wide(BOARD_X1, BOARD_Y1, BOARD_X2, BOARD_Y1, LINE_R_MAX, COLOR_BACKGROUND);
	fb_draw_line_wide(BOARD_X1, BOARD_Y1, BOARD_X1, BOARD_Y2, LINE_R_MAX, COLOR_BACKGROUND);
	fb_draw_line_wide(BOARD_X1, BOARD_Y2, BOARD_X2, BOARD_Y2, LINE_R_MAX, COLOR_BACKGROUND);
	fb_draw_line_wide(BOARD_X2, BOARD_Y1, BOARD_X2, BOARD_Y2, LINE_R_MAX, COLOR_BACKGROUND);
	fb_update();
}

static void draw_guesser_reply(char *message_image){
	fb_draw_rect(GUESS_REPLY_X, GUESS_REPLY_Y, REPLY_W, REPLY_H, COLOR_BACKGROUND);
	fb_image *img;
	img = fb_read_png_image(message_image);
	fb_draw_image(GUESS_REPLY_X, GUESS_REPLY_Y, img, 0);
	fb_update();
	fb_free_image(img);	
}

static void draw_drawer_reply(char *message_image){
	fb_draw_rect(DRAW_REPLY_X, DRAW_REPLY_Y, REPLY_W, REPLY_H, COLOR_BACKGROUND);
	fb_image *img;
	img = fb_read_png_image(message_image);
	fb_draw_image(DRAW_REPLY_X, DRAW_REPLY_Y, img, 0);
	fb_update();
	fb_free_image(img);	
}

static void draw_save_reply(char *message){
	fb_draw_rect(SAVE_REPLY_X, SAVE_REPLY_Y - SAVE_REPLY_H, SAVE_REPLY_W, SAVE_REPLY_H, COLOR_BACKGROUND);
	fb_draw_text(SAVE_REPLY_X, SAVE_REPLY_Y, message, SAVE_REPLY_FONT, PURPLE);
	fb_update();
}
static void touch_handle_image(int x, int y, int finger){
	if(x >= EXIT_X && x < EXIT_X + TOOL_SIZE && y >= EXIT_Y && y < EXIT_Y + TOOL_SIZE){
		printf("Exit.\n");
		into_main_page();
		return;
	}
}


static void touch_event_cb(int fd)
{
	int type,x,y,finger;
	type = touch_read(fd, &x,&y,&finger);
	char bstr[132];
	if(in_image_page){
		touch_handle_image(x, y, finger);
		return;
	}
	if(role == NO_ROLE){
		if(type != TOUCH_PRESS )return;
		if((x>=START_X1)&&(x<START_X2)&&(y>=START_Y1)&&(y<START_Y2)) {
			role = rand()%2+1;
				if((role == DRAWER)) {
				role = DRAWER;
				printf("you are drawer.\n");
				sprintf(bstr, "0 2 \n"); // first 0 means switching role, second 2 means the opposite role
				into_drawer_page();
				myWrite_nonblock(bluetooth_fd, bstr, 5);
				return;
			}
			if((role == GUESSER)) {
				role = GUESSER;
				printf("you are guesser.\n");
				sprintf(bstr, "0 1 \n");
				into_guesser_page();
				myWrite_nonblock(bluetooth_fd, bstr, 5);
				return;
			}
		}else if((x>=IMAGE_X1)&&(x<IMAGE_X2)&&(y>=IMAGE_Y1)&&(y<IMAGE_Y2)){
			into_image_page();
			return;
		}
		return;
	}
	switch(type){
		case TOUCH_PRESS:
			//printf("type=%d,x=%d,y=%d,finger=%d\n",type,x,y,finger);
			touch_handle_game(x, y, finger);
			break;
			
		case TOUCH_MOVE:
			//printf("TOUCH_MOVE：x=%d,y=%d,finger=%d\n",x,y,finger);
			move_handle_game(x, y, finger);
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
	
	audio_record_init(NULL, PCM_SAMPLE_RATE, 1, 16); //单声道，S16采样

	into_main_page();

	printf("If your input device is not working, you may exit and enter ' ./lab6 -e event0 ' if your event is event0, for example.\n(The default event is event1.)\n");
	int s = 0, e = 0;
	if(argv[1] != NULL){
		if(argv[1][0] == '-'){
			if(argv[1][1] == 'e' || argv[1][2] == 'e'){
				e = 1;
			}
			if(argv[1][1] == 's' || argv[1][2] == 's'){
				s = 1;
			}
		}
	}
	if(argv[2] == NULL || e == 0)
		touch_fd = touch_init("/dev/input/event1");
	else if(e == 1){
		char event[30];
		sprintf(event, "/dev/input/%s", argv[2]);
		touch_fd = touch_init(event);
	}

	printf("If you do not want to use bluetooth and just want to view the pictures, you may add -s.For example, enter './lab6 -es event0'\n");
	task_add_file(touch_fd, touch_event_cb);
	
	if(s == 0){
		bluetooth_fd = bluetooth_tty_init("/dev/rfcomm0");

		if(bluetooth_fd == -1) return 0;
		task_add_file(bluetooth_fd, bluetooth_tty_event_cb);
	}else{
		printf("(In this mode, you should only view the pictures)\n");
	}

	task_add_timer(1000, timer_cb); /*增加1秒的定时器*/
	task_loop();
	return 0;
}
/*===============================================================*/

static void write_png_file(const char* filename, int width, int height, unsigned char* image_data) {
		// 修改数据排列顺序，将BGR转换为RGBA
		unsigned char* fixed_image_data = (unsigned char*)malloc(width * height * 4);
		if (fixed_image_data == NULL) {
			fprintf(stderr, "Error: Memory allocation for fixed_image_data failed in write_png_file\n");
			return ;
		}
		for (int y = 0; y < height*4; y+=4) {
				for (int x = 0; x < width*4; x+=4) {
						fixed_image_data[y * width + x + 0] = image_data[y * width + x + 2];
						fixed_image_data[y * width + x + 1] = image_data[y * width + x + 1];
						fixed_image_data[y * width + x + 2] = image_data[y * width + x + 0];
						fixed_image_data[y * width + x + 3] = image_data[y * width + x + 3];
				}
		}
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Error: Could not open file %s for writing\n", filename);
        return;
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fprintf(stderr, "Error: Could not create PNG write struct\n");
        fclose(fp);
        return;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        fprintf(stderr, "Error: Could not create PNG info struct\n");
        fclose(fp);
        return;
    }
		if (setjmp(png_jmpbuf(png_ptr))) {
        fclose(fp);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fprintf(stderr, "Error during PNG creation\n");
        return;
    }
		png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    // Explicitly set the byte order to RGBA
    png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);
		png_bytep *row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
		if (fixed_image_data == NULL) {
			fprintf(stderr, "Error: Memory allocation for row_pointers failed in write_png_file\n");
			return ;
		}
    for (int y = 0; y < height; y++) {
        row_pointers[y] = (png_bytep)(fixed_image_data + y * width * 4);
    }

    png_set_rows(png_ptr, info_ptr, row_pointers);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    png_free_data(png_ptr, info_ptr, PNG_FREE_ROWS, 0);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    free(row_pointers);
		free(fixed_image_data);
		fclose(fp);
}



static void capture_screen_region(int x, int y, int w, int h) {
	unsigned char *img = get_screen_region(x,y,w,h);
	// 将捕获的图像数据进行处理
	char filename[128];
	sprintf(filename, "./collections/%d-%ld.png",ra,time(NULL));
	write_png_file(filename, w, h, img);
	free(img);
}


/*===============================================================*/	


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
