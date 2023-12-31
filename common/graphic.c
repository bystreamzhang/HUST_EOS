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
	return DRAW_BUF; //DRAW_BUF is the start address of screen (2-D array, defined at beginning)
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
	//printf("you need implement fb_draw_rect()\n"); exit(0);
	//int ty = y;
	for(int i=0;i<w;i++){
		for(int j=0;j<h;j++){
		//for(int y=ty;y<ty+h;y++){
			*(buf + (y+j)*SCREEN_WIDTH + x) = color;
		}
		x++;
	}
/*---------------------------------------------------*/
	return;
}


void fb_draw_circle(int x, int y, int r, int color)
{
	int *buf = _begin_draw(x-r,y-r,r<<1,r<<1);
	int px, py;
	int dx, dy;
	for (dy = 0; dy <= r; dy++) {
		for (dx = 0; dx <= r; dx++) {
			if (dx * dx + dy * dy <= r * r) {
				px = x + dx;
				py = y + dy;
				if(px>=0 && py>=0 && px<SCREEN_WIDTH && py<SCREEN_HEIGHT)
					*(buf + py*SCREEN_WIDTH + px) = color;
				py -= (dy<<1);
		    if(px>=0 && py>=0 && px<SCREEN_WIDTH && py<SCREEN_HEIGHT)
					*(buf + py*SCREEN_WIDTH + px) = color;
				px -= (dx<<1);
				if(px>=0 && py>=0 && px<SCREEN_WIDTH && py<SCREEN_HEIGHT)
					*(buf + py*SCREEN_WIDTH + px) = color;
		    py += (dy<<1);
		    if(px>=0 && py>=0 && px<SCREEN_WIDTH && py<SCREEN_HEIGHT)
					*(buf + py*SCREEN_WIDTH + px) = color;
		    }
		}
	}
}

void fb_draw_line(int x1, int y1, int x2, int y2, int color)
{
/*---------------------------------------------------*/
	//printf("you need implement fb_draw_line()\n"); exit(0);
	if(x1 < 0) x1 = 0;
	if(y1 < 0) y1 = 0;
	if(x2 < 0) x2 = 0;
	if(y2 < 0) y2 = 0;
	if(x1 > SCREEN_WIDTH) { x1 = SCREEN_WIDTH-1;}
	if(y1 > SCREEN_HEIGHT) { y1 = SCREEN_HEIGHT-1;}
	if(x2 > SCREEN_WIDTH) { x2 = SCREEN_WIDTH-1;}
	if(y2 > SCREEN_HEIGHT) { y2 = SCREEN_HEIGHT-1;}
	
	int dx, dy, stepX, stepY;
	dx = x2 - x1;
	if(dx < 0)dx = -dx;
	dy = y2 - y1;
	if(dy < 0)dy = -dy;
	
	if(x1 > x2){	// exchange
		x1 = x1 + x2;
		x2 = x1 - x2;
		x1 = x1 - x2;
		y1 = y1 + y2;
		y2 = y1 - y2;
		y1 = y1 - y2;
	}
	
	
	if(x1 < x2) stepX = 1; else stepX = -1;
    	if(y1 < y2) stepY = 1; else stepY = -1;
	
	int err = dx - dy;
	int x = x1, y = y1;
	
	int *buf = NULL;
	if(y1 <= y2)
		buf = _begin_draw(x1,y1,dx,dy);
	else 
		buf = _begin_draw(x1,y2,dx,dy);
		
	*(buf + y1*SCREEN_WIDTH + x1) = color;
	
	while (x != x2 || y != y2) {
		int e2 = 2 * err;
		if (e2 > -dy) {
		    err -= dy;
		    x += stepX;
		}
		if (e2 < dx) {
		    err += dx;
		    y += stepY;
		}
		*(buf + y * SCREEN_WIDTH + x) = color;
    	}
/*---------------------------------------------------*/
	return;
}

void fb_draw_line_wide(int x1, int y1, int x2, int y2, int r, int color)
{
	if(x1 < 0) x1 = 0;
	if(y1 < 0) y1 = 0;
	if(x2 < 0) x2 = 0;
	if(y2 < 0) y2 = 0;
	if(x1 > SCREEN_WIDTH) { x1 = SCREEN_WIDTH-1;}
	if(y1 > SCREEN_HEIGHT) { y1 = SCREEN_HEIGHT-1;}
	if(x2 > SCREEN_WIDTH) { x2 = SCREEN_WIDTH-1;}
	if(y2 > SCREEN_HEIGHT) { y2 = SCREEN_HEIGHT-1;}
	
	int dx, dy, stepX, stepY;
	dx = x2 - x1;
	if(dx < 0)dx = -dx;
	dy = y2 - y1;
	if(dy < 0)dy = -dy;
	
	if(x1 > x2){	// exchange
		x1 = x1 + x2;
		x2 = x1 - x2;
		x1 = x1 - x2;
		y1 = y1 + y2;
		y2 = y1 - y2;
		y1 = y1 - y2;
	}
	
	
	if(x1 < x2) stepX = 1; else stepX = -1;
    	if(y1 < y2) stepY = 1; else stepY = -1;
	
	int err = dx - dy;
	int x = x1, y = y1;
	
	int *buf = NULL;
	if(y1 <= y2)
		buf = _begin_draw(x1-r,y1-r,dx+(r<<1),dy+(r<<1));
	else 
		buf = _begin_draw(x1-r,y2-r,dx+(r<<1),dy+(r<<1));
		
	*(buf + y1*SCREEN_WIDTH + x1) = color;
	
	while (x != x2 || y != y2) {
		int e2 = 2 * err;
		if (e2 > -dy) {
		    err -= dy;
		    x += stepX;
		}
		if (e2 < dx) {
		    err += dx;
		    y += stepY;
		}
		//*(buf + y * SCREEN_WIDTH + x) = color;
		// draw a circle at (x,y)
		int px, py;
		int dxc, dyc;
		for (dyc = 0; dyc <= r; dyc++) {
			for (dxc = 0; dxc <= r; dxc++) {
				if (dxc * dxc + dyc * dyc <= r * r) {
					px = x + dxc;
					py = y + dyc;
					if(px>=0 && py>=0 && px<SCREEN_WIDTH && py<SCREEN_HEIGHT)
						*(buf + py*SCREEN_WIDTH + px) = color;
					py -= (dyc<<1);
			    		if(px>=0 && py>=0 && px<SCREEN_WIDTH && py<SCREEN_HEIGHT)
						*(buf + py*SCREEN_WIDTH + px) = color;
					px -= (dxc<<1);
					if(px>=0 && py>=0 && px<SCREEN_WIDTH && py<SCREEN_HEIGHT)
						*(buf + py*SCREEN_WIDTH + px) = color;
			    		py += (dyc<<1);
			    		if(px>=0 && py>=0 && px<SCREEN_WIDTH && py<SCREEN_HEIGHT)
						*(buf + py*SCREEN_WIDTH + px) = color;
			    	}
			}
		}
    	}
/*---------------------------------------------------*/
	return;
}

void fb_draw_image(int x, int y, fb_image *image, int color)
{
	if(image == NULL) return;

	int ix = 0; //image x
	int iy = 0; //image y
	int w = image->pixel_w; //draw width
	int h = image->pixel_h; //draw height

	if(x<0) {w+=x; ix-=x; x=0;}
	if(y<0) {h+=y; iy-=y; y=0;}
	
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

	int alpha;
	int ww;
	
	if(image->color_type == FB_COLOR_RGB_8880) /*lab3: jpg*/
	{
		//printf("you need implement fb_draw_image() FB_COLOR_RGB_8880\n"); exit(0);
		src = image->content + iy*image->line_byte + ix*4;
		for (int j = 0; j < h; j++) {
			memcpy(dst, src, w * 4 * sizeof(char)); 
			dst +=  SCREEN_WIDTH*4;
			src +=  image->line_byte;
		}
	
		return;
	}
	else if(image->color_type == FB_COLOR_RGBA_8888) /*lab3: png*/
	{
		//printf("you need implement fb_draw_image() FB_COLOR_RGBA_8888\n"); exit(0);
		src = image->content + iy*image->line_byte + ix*4; 
		int src_pixel, dst_pixel;
		for (int j = 0; j < h; j++) {
		    for (int i = 0; i < w*4; i+=4) {
			
			// 读取像素颜色值
			src_pixel = *((int*)(src + i));
			dst_pixel = *((int*)(dst + i));
			alpha = (src_pixel >> 24) & 0xFF;  // 确保 alpha 的范围在 0 到 255 之间
			if(alpha == 0)
				continue;
				
			int src_channel = src_pixel & 0xFF;
			int dst_channel = dst_pixel & 0xFF;
			
			//b
			if(alpha == 255)
				*(dst + i) = src_channel;
			else
				*(dst + i) += (alpha * (src_channel - dst_channel))>>8;
			src_pixel >>= 8;
			dst_pixel >>= 8;
			src_channel = src_pixel & 0xFF;
			dst_channel = dst_pixel & 0xFF;
			
			//g
			if(alpha == 255)
				*(dst + i + 1) = src_channel;
			else
				*(dst + i + 1) += (alpha * (src_channel - dst_channel))>>8;
			src_pixel >>= 8;
			dst_pixel >>= 8;
			src_channel = src_pixel & 0xFF;
			dst_channel = dst_pixel & 0xFF;
			
			//r
			if(alpha == 255)
				*(dst + i + 2) = src_channel;
			else
				*(dst + i + 2) += (alpha * (src_channel - dst_channel))>>8;
				
		    }
			dst +=  SCREEN_WIDTH*4;
			src +=  image->line_byte;
		}
		return;
	}
	else if(image->color_type == FB_COLOR_ALPHA_8) /*lab3: font*/
	{
		//printf("you need implement fb_draw_image() FB_COLOR_ALPHA_8\n"); exit(0);
		src = image->content + iy*image->line_byte + ix; 
		int src_pixel, dst_pixel;
		for (int j = 0; j < h; j++) {
			for (int i = 0; i < w; i++) {
				alpha = (*((int*)(src + i))) & 0xFF;
				
		    		//*(dst + i*4) = color & 0xFF;
		    		//*(dst + i*4 + 1) = (color >> 8) & 0xFF;
		    		//*(dst + i*4 + 2) = (color >> 16) & 0xFF;
		    		//*(dst + i*4 + 3) = & 0xFF;
		    		//src_channel = (src_pixel >> 8) & 0xFF;
				//dst_channel = (dst_pixel >> 8) & 0xFF;
		    		if(alpha == 0)
					continue;
					
				src_pixel = color;
				dst_pixel = *((int*)(dst + i*4));
				int src_channel = src_pixel & 0xFF;
				int dst_channel = dst_pixel & 0xFF;
				
				if(alpha == 255)
					*(dst + i*4) = src_channel;
				else
					*(dst + i*4) += (alpha * (src_channel - dst_channel))>>8;
				src_pixel >>= 8;
				dst_pixel >>= 8;
				src_channel = src_pixel & 0xFF;
				dst_channel = dst_pixel & 0xFF;
				
				if(alpha == 255)
					*(dst + i*4 + 1) = src_channel;
				else
					*(dst + i*4 + 1) += (alpha * (src_channel - dst_channel))>>8;
				src_pixel >>= 8;
				dst_pixel >>= 8;
				src_channel = src_pixel & 0xFF;
				dst_channel = dst_pixel & 0xFF;
				
				if(alpha == 255)
					*(dst + i*4 + 2) = src_channel;
				else
					*(dst + i*4 + 2) += (alpha * (src_channel - dst_channel))>>8;
			}
			dst +=  SCREEN_WIDTH*4;
			src +=  image->line_byte;
		}
		return;
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


unsigned char * get_screen_region(int x, int y, int w, int h){
	unsigned char *img = (unsigned char *)malloc(sizeof(unsigned char) * w * 4 * h);
	if (img == NULL) {
    fprintf(stderr, "Error: Memory allocation failed in get_screen_region\n");
    return NULL;
}
	unsigned char *img_start = img;
	int *buft = DRAW_BUF;
	buft += y*SCREEN_WIDTH + x;
	while(h-- > 0){
		memcpy(img, buft, w*4);
		img += w * 4;
		buft += SCREEN_WIDTH;
	}
	return img_start;
}
