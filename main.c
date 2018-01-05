#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>

#include "ximage.h"

typedef struct {
	int s,l;
} msec_timer;

#define blocks_width 6
#define blocks_height 12 

int blocks[blocks_width*blocks_height];
int timer_blocks[blocks_width*(blocks_height+1)];

ximage *blocks_image[6];

ximage *fb;

int read_block(int x, int y)
{
	if(x < 0 || x >= blocks_width || y < 0 || y >= blocks_height) return -1;
	return blocks[y * blocks_width + x];
}

void write_block(int x, int y, int d)
{
	if(x < 0 || x >= blocks_width || y < 0 || y >= blocks_height) return;
	blocks[y * blocks_width + x] = d;
}

int read_timer_block(int x, int y)
{
	if(x < 0 || x >= blocks_width || y < 0 || y >= blocks_height+1) return 0;
	return timer_blocks[y * blocks_width + x];
}

void write_timer_block(int x, int y, int d)
{
	if(x < 0 || x >= blocks_width || y < 0 || y >= blocks_height+1) return;
	timer_blocks[y * blocks_width + x] = d;
}

void apply_timer_blocks(int x, int y)
{
	int maxtim = 0;
	for(int i = 0; i < blocks_height; i++) {
		for(int j = 0; j < blocks_width; j++) {
			int t = read_timer_block(j,i);
			if(t > maxtim) maxtim = t;
		}
	}
	if(x < 0 || x >= blocks_width || y < 0 || y >= blocks_height) return;
	timer_blocks[y * blocks_width + x] = maxtim + 10;
	timer_blocks[(blocks_height) * blocks_width + 0] = maxtim + 20;
}

int check_timer_block(void)
{
	int sumtim = 0;
	for(int i = 0; i < blocks_height+1; i++) {
		for(int j = 0; j < blocks_width; j++) {
			sumtim += timer_blocks[i * blocks_width + j];
			if(timer_blocks[i * blocks_width + j] > 0) {
				timer_blocks[i * blocks_width + j]--;
				if(timer_blocks[i * blocks_width + j] == 0) write_block(j,i,0);
			}
		}
	}
	return sumtim;
}

void refresh_blocks(SDL_Surface *screen, int frames)
{
	unsigned int *p = (unsigned int *)screen->pixels;
	
	for(int i = 0; i < blocks_height; i++) {
		for(int j = 0; j < blocks_width; j++) {
			int cur = read_block(j,i);
			int tim = read_timer_block(j,i);
			
			ximage_bitblt(fb,blocks_image[cur],j*32,i*32);
			if(tim > 0 && (frames & 1)) ximage_boxfill(fb,j*32,i*32,j*32+32,i*32+32,0xffffffff);
		}
	}
}

void vanish_blocks(void)
{
	int tim = 1;
	
	for(int i = 0; i < blocks_height; i++) {
		for(int j = 0; j < blocks_width; j++) {
			int vn = 0,hn = 0;
			
			int cur = read_block(j,i);
			int skf = 0;
			
			for(int l = i; l < blocks_height; l++) if(read_block(j,l) == 0) skf = 1;
			
			if(cur == 0 || skf) continue;
			
			if(read_block(j-1,i) == cur && read_timer_block(j-1,i) == 0) hn++;
			if(read_block(j+1,i) == cur && read_timer_block(j+1,i) == 0) hn++;
			
			if(read_block(j,i-1) == cur && read_timer_block(j,i-1) == 0) vn++;
			if(read_block(j,i+1) == cur && read_timer_block(j,i+1) == 0) vn++;
			
			if(hn == 2) {
				//write_block(j-1,i,0);
				//write_block(j,i,0);
				//write_block(j+1,i,0);
				
				apply_timer_blocks(j-1,i);
				apply_timer_blocks(j,i);
				apply_timer_blocks(j+1,i);
			}
			
			if(vn == 2) {
				//write_block(j,i-1,0);
				//write_block(j,i,0);
				//write_block(j,i+1,0);
				
				apply_timer_blocks(j,i-1);
				apply_timer_blocks(j,i);
				apply_timer_blocks(j,i+1);
			}
		}
	}
}


void fall_blocks(void)
{
	for(int i = blocks_height-1; i >= 0; i--) {
		for(int j = 0; j < blocks_width; j++) {
			int cur = read_block(j,i);
			
			if(cur == 0) continue;
			
			if(read_block(j,i+1) == 0) {
				write_block(j,i+1,cur);
				write_block(j,i,0);
			} 
		}
	}
}

void upslide_blocks(void)
{
	for(int i = 0; i < blocks_height; i++) {
		for(int j = 0; j < blocks_width; j++) {
			int nxt = read_block(j,i+1);
			nxt = nxt < 0 ? 0 : nxt;
			
			int nxt2 = read_timer_block(j,i+1);
			
			write_block(j,i,nxt);
			write_timer_block(j,i,nxt2);
		}
	}
	
	int i = blocks_height - 1;
	
	for(int j = 0; j < blocks_width; j++) {
		while(1) {
			int cur = (rand()%5) + 1;
			int vn = 0,hn = 0;
			if(read_block(j-1,i) == cur) hn++;
			if(read_block(j+1,i) == cur) hn++;
			
			if(read_block(j,i-1) == cur) vn++;
			if(read_block(j,i+1) == cur) vn++;
			
			//if(vn == 0 && hn == 0) {
				write_block(j,i,cur);
				break;
			//}
		}
	}
}

void msectimer_start(msec_timer *t, int i)
{
	t->s = SDL_GetTicks();
	t->l = i;
}

int msectimer_check(msec_timer *t)
{
	if(SDL_GetTicks() - t->s > t->l) {
		t->s = SDL_GetTicks();
		return 1;
	}
	return 0;
}

void process_message(void)
{
	SDL_Event event;
	if(SDL_PollEvent(&event)) {
		if(event.type == SDL_QUIT) {
			SDL_Quit();
			exit(0);
		}
	}
}

int main(int argc, char *argv[])
{
	for(int i = 0; i < blocks_height; i++) {
		for(int j = 0; j < blocks_width; j++) {
			write_block(j,i,0);
			timer_blocks[i * blocks_width + j] = 0;
		}
	}
	
	for(int i = 0; i < 4; i++) {
		upslide_blocks();
	}
	
	const int colors[] = {
		0x000000,
		0x00ff00,
		0x00ffff,
		0xff0000,
		0xff00ff,
		0xffff00,
	};
	
	{
		unsigned int *p = (unsigned int *)malloc(32*32*4);
		memset(p,0,32*32*4);
		blocks_image[0] = ximage_create(32,32,32,p);
	}
	
	for(int n = 1; n <= 5; n++) {
		char sz[512];
		sprintf(sz,"resource/block_%d.png",n);
		blocks_image[n] = ximage_load(sz);
	}
	
	SDL_Init(SDL_INIT_VIDEO);
	
	SDL_Surface *screen = SDL_SetVideoMode(640, 480, 32, SDL_SWSURFACE);
	
	ximage_init();
	
	fb = ximage_create(640, 480, screen->pitch/4, screen->pixels);
	
	msec_timer upslide, fps1sec;
	
	ximage_fsize(FSIZ_12);
	
	msectimer_start(&upslide, 3000);
	msectimer_start(&fps1sec, 1000);
	
	uint32_t next_frame = SDL_GetTicks() * 3 + 100;
	
	char info_str[256];
	
	strcpy(info_str,"N/A FPS");
	
	int frames = 0;
	int dframes=0;
	
	while(1) {
		process_message();
		
		vanish_blocks();
		
		if(check_timer_block() == 0) {
			fall_blocks();
			if(msectimer_check(&upslide)) upslide_blocks();
		}
		
		uint32_t now = SDL_GetTicks() * 3;
		if(now < next_frame) {
			memset(screen->pixels,0,640*480*4);
			refresh_blocks(screen, frames);
			ximage_textout(fb,0,0,0xffffff,0,info_str);
			SDL_UpdateRect(screen,0,0,0,0);
			dframes++;
			now = SDL_GetTicks() * 3;
			if(now < next_frame) {
				SDL_Delay((next_frame-now)/3);
			}
		}
		next_frame += 100;
		
		frames++;
		
		if(msectimer_check(&fps1sec)) {
			sprintf(info_str,"%d FPS",dframes);
			dframes = 0;
		}
	}
	
	return 0;
}
