#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>
#include "../common/common.h"
#include "../common/external/include/png.h"

/*语音识别要求的pcm采样频率*/
#define PCM_SAMPLE_RATE 16000 /* 16KHz */


#define IP "127.0.0.1"
#define PORT 8011

#define print_err(fmt, ...) \
	printf("%d:%s " fmt, __LINE__, strerror(errno), ##__VA_ARGS__);

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

#define EXIT_X 960
#define EXIT_Y 0

/*============================Main Page============================*/

#define START_X1 110
#define START_Y1 360
#define START_X2 493
#define START_Y2 464

#define IMAGE_X1 545
#define IMAGE_Y1 360
#define IMAGE_X2 928
#define IMAGE_Y2 464


/*============================Gaming Page============================*/

#define BOARD_X1 88
#define BOARD_Y1 112
#define BOARD_X2 946
#define BOARD_Y2 463
#define BOARD_W 858
#define BOARD_H 351

#define CLOCK_X 748
#define CLOCK_Y 12
#define TIMEBAR_X 744
#define TIMEBAR_Y 7

#define TIME_LINE_CIRCLE_X1 770
#define TIME_LINE_CIRCLE_X2 915
#define TIME_LINE_CIRCLE_Y 31
#define TIME_LINE_CIRCLE_R 15

#define TIME_INIT 145

#define HEAD_FONTSIZE 40
#define DRAW_TOPIC_FONT_X 391
#define DRAW_TOPIC_FONT_Y 54

#define GUESS_REPLY_X 367
#define GUESS_REPLY_Y 21
#define DRAW_REPLY_X 120
#define DRAW_REPLY_Y 18
#define REPLY_W 142
#define REPLY_H 38

#define SAVE_REPLY_X 620
#define SAVE_REPLY_Y 591
#define SAVE_REPLY_FONT 11
#define SAVE_REPLY_W 100
#define SAVE_REPLY_H 14

#define WORDS_MAX_W 180

#define SCORE_X 678
#define SCORE_Y 54

#define TOOL_SIZE 64

#define TOOL_X1 140
#define TOOL_Y1 495

#define TOOL_X2 216
#define TOOL_Y2 493

#define TOOL_X3 292
#define TOOL_Y3 491

#define TOOL_X4 371
#define TOOL_Y4 490

#define SPEAK_X 497
#define SPEAK_Y 492

#define SAVE_X 625
#define SAVE_Y 493

#define TOUCH_INVALID -1
#define TOUCH_BOARD 0
#define TOUCH_SIZE 1
#define TOUCH_ERASER 2
#define TOUCH_COLOR_BOARD 3
#define TOUCH_DUSTBIN 4
#define TOUCH_SPEAK 5
#define TOUCH_SAVE 6
#define TOUCH_EXIT 7


#define LINE_R_INIT 8
#define LINE_R_MIN 2
#define LINE_R_MAX 16


int get_color(int finger);
int get_color_real(int finger);
int get_color_op(int finger);


static void draw_background();

static void clear_board();

//static void draw_clear_button();

//static void draw_speak_button();

//static void draw_tools();

static void draw_guesser_reply(char *message_image);
static void draw_drawer_reply(char *message_image);
static void draw_save_reply(char *message);

static void update_sizetool();

static void change_line_r();

static void clear_usingtoolframe();

static void draw_usingtoolframe();

static void draw_page(char *page_image);

//static void draw_textframe();

//static void draw_timer();

//static void clear_line3();

//static void clear_line4();

static void update_score();

static void timer_cb(int period);/*该函数1秒执行一次*/

static void into_main_page();

static void into_next_turn();

static void into_guesser_page();
static void into_drawer_page();
static void into_image_page();


//static void draw_role1();

//static void draw_role2();

static void touch_handle_game(int x, int y, int finger);

static void move_handle_game(int x, int y, int finger);

static void touch_handle_image(int x, int y, int finger);

static void handle_speak();

static void bluetooth_tty_event_cb(int fd);

static void touch_event_cb(int fd);

static int bluetooth_tty_init(const char *dev);

/*============================IMAGE Page============================*/

#define IMAGE_X 104
#define IMAGE_Y 86

#define IMAGE_TOPIC_X 216
#define IMAGE_TOPIC_Y 512
#define IMAGE_FONTSIZE 40
#define IMAGE_PAGEID_X 12
#define IMAGE_PAGEID_Y 58

static void write_png_file(const char *filename, int width, int height, unsigned char *image_data);

static void capture_screen_region(int x, int y, int w, int h);

static void open_png_files(const char *folder_path);

static void draw_collection();

static void draw_image_topic();

static void draw_image_page();

static void change_image_page(int delta);