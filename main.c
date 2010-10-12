
/* Simple program:  Create a blank window, wait for keypress, quit.

   Please see the SDL documentation for details on using the SDL API:
   /Developer/Documentation/SDL/docs.html
*/
   
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "SDL.h"
#include "SDL_audio.h"

SDL_TimerID timer1;

void addtimers(unsigned int timer1_time, unsigned int timer2_time);
static Uint32 game_event_push(Uint32 interval, void* param);
void mixaudio(void *unused, Uint8 *stream, int len);

#define NUM_SOUNDS 2
struct sample {
	Uint8 *data;
	Uint32 dpos;
	Uint32 dlen;
} sounds[NUM_SOUNDS];

int main(int argc, char *argv[])
{
	
	int a;
	
	char* file;
	int fullscreen = 0;
	int outputbmp = 0;
	char* outputdir;
	
	int swsurface = 0;
	int once = 0;
	int audio = 0;
	

	int width = 1024;
	int height = 768;
	int fps = 20;
	int delay = 50;
	
	unsigned int leftshift = 0;
	unsigned int rightshift = 0;
	
	unsigned long int offset = 0;
	
	offset = 0;
		
	if (argc == 1) {
		printf("Usage: %s [-f] [-o dir] [-s] [-n] [-w width] [-h height] [-p fps] [-a] [-l bits] [-r bits] [-e offset] file\n"
			   "-f : Fullscreen display\n"
			   "-o outputdir : Save each frame to outputdir\n"
			   "-s : Use a software SDL surface for those without openGL or those with issues in fullscreen mode\n"
			   "-n : Play through once (don't loop). Specify if using -o.\n"
			   "-w : Specify width\n"
			   "-h : Specify height\n"
			   "-p : Specify FPS\n"
			   "-a : Use audio\n"
			   "-l bits : Left bitshift the data (data is read in blocks of 32 bits)\n"
			   "-r bits : Right bitshift the data\n"
			   "-e offset : start at position\n"
			   , argv[0]);
		exit(0);
	}
	
	int c;
	while ((c = getopt(argc, argv, "afsno:w:h:p:l:r:e:")) != -1) {
		switch (c) {
			case 'f':
				fullscreen = 1;
				break;
			case 'o':
				outputbmp = 1;
				outputdir = optarg;
				break;
			case 's':
				swsurface = 1;
				break;
			case 'n':
				once = 1;
				break;
			case 'w':
				width = atoi(optarg);
				break;
			case 'h':
				height = atoi(optarg);
				break;
			case 'p':
				fps = atoi(optarg);
				delay = 1000/fps;
				break;
			case 'a':
				audio = 1;
				break;
			case 'l':
				leftshift = atoi(optarg);
				break;
			case 'r':
				rightshift = atoi(optarg);
				break;
			case 'e':
				offset = atol(optarg);
				break;
			default:
				file = optarg;
		}
	}
			
	file = argv[optind];
	printf("--------------------------------------\n"
		   "playfile v1.0 gm@stackunderflow.com\n"
	       "--------------------------------------\n");
	
	if (fullscreen) {
		printf("Playing fullscreen %ix%i\n",width,height);
	} else {
		printf("Playing windowed %ix%i\n",width,height);
	}
	
	if (outputbmp) {
		printf("Saving to BMP in %s\n", outputdir);
	} else {
		printf("Not saving to BMP\n");
	}
	
	if (swsurface) {
		printf("Using software Surface\n");
	} else {
		printf("Using hardware Surface\n");
	}
	
	if (once) {
		printf("Playing once\n");
	} else {
		printf("Playing in a loop\n");
	}
	
	printf("Playing at %i fps (%i ms/f)\n",fps,delay);
	
	unsigned long int bytessec = width*height*4*fps;
	unsigned long int mbytessec = bytessec / 1048576;
	
	printf("Data rate %li bytes/s (%li MB/s)\n",bytessec,mbytessec);
	
	printf("Filename to open: %s\n",file);
	
	printf("Starting at offset %lu\n",offset);
	
	if (audio) {
		printf("Using audio\n");
		int pixels = (44100)/fps;
		printf("Audio completion of each frame: %i pixels (~%i lines)\n",pixels,pixels/width);
	} else {
		printf("No audio\n");
	}

	printf("--------------------------------------\n");
	
	FILE *f;
	f = fopen(file, "r");
	
	if (f == NULL) {
		printf("Cannot open file\n");
		exit(0);
	}
	fseek(f, offset, SEEK_SET);
	
	Uint32 initflags = SDL_INIT_EVERYTHING;  /* See documentation for details */
	SDL_Surface *screen;
	Uint8  video_bpp = 32;
	Uint32 videoflags;
	SDL_AudioSpec fmt;
	SDL_AudioCVT cvt;
	
	if (swsurface) {
		videoflags = SDL_SWSURFACE;
	} else {
		videoflags = SDL_HWSURFACE;
	}
	
	if (fullscreen) {
		videoflags = videoflags | SDL_FULLSCREEN;
	}
	
	int done;
	SDL_Event event;

	if ( SDL_Init(initflags) < 0 ) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",
			SDL_GetError());
		exit(1);
	}

	screen=SDL_SetVideoMode(width,height, video_bpp, videoflags);
        if (screen == NULL) {
		SDL_Quit();
		exit(2);
	}
	
	if (audio) {
		fmt.freq = 44100;
		fmt.format = AUDIO_S16;
		fmt.channels = 2;
		fmt.samples = 512;
		fmt.callback = mixaudio;
		fmt.userdata = NULL;
		
		if ( SDL_OpenAudio(&fmt, NULL) < 0 ) {
			fprintf(stderr, "Unable to open audio: %s\n", SDL_GetError());
			exit(1);
		}
		SDL_PauseAudio(0);
	}

	Uint32 *bufp;
	Uint32 buffer[width*height];
	
	int x = 0;
	int j = 0;
	char filename[32];
	
	timer1 = SDL_AddTimer(delay, game_event_push, NULL);
	
	done = 0;
	while ( !done ) {
		/* Check for events */
		while ( SDL_PollEvent(&event) ) {
			switch (event.type) {

				case SDL_MOUSEMOTION:
					break;
				case SDL_MOUSEBUTTONDOWN:
					break;
				case SDL_KEYDOWN:
					/* Any keypress quits the app... */
				case SDL_QUIT:
					printf("Exiting at position %lu\n",ftell(f));
					SDL_CloseAudio();
					SDL_Quit();
					return 0;
				case SDL_USEREVENT:
					timer1 = SDL_AddTimer(delay, game_event_push, NULL);

					if(!(fread(&buffer, width*height*4, 1, f))) {
						if (once) {
							exit(0);
						}
						fseek(f, offset, SEEK_SET);
						puts("returned to beginning");
					}
					x = 0;
					while(x<width*height) { 
						bufp = (Uint32 *)screen->pixels + x; 
						*bufp = (buffer[x] << leftshift >> rightshift);
						x++;
					}
					SDL_Flip(screen);
					
					if (audio) {
						//SDL_BuildAudioCVT(&cvt, fmt.format, fmt.channels, fmt.freq, AUDIO_S16, 2, 44100);
						//cvt.buf = (Uint8*) buffer;
						//SDL_ConvertAudio(&cvt);
						SDL_LockAudio();
						sounds[0].data = (Uint8 *)buffer;
						sounds[0].dlen = (44100*4)/fps;
						sounds[0].dpos = 0;
						SDL_UnlockAudio();
					}
					
					if (outputbmp) {
						sprintf(filename, "%s/%.8i.bmp", outputdir, j);
						j++;
						SDL_SaveBMP(screen, filename);
					}
					
					break;
				default:
					break;
			}
		}
	}
	
	/* Clean up the SDL library */
	SDL_Quit();
	return(0);
}

static Uint32 game_event_push(Uint32 interval, void* param)
{
	SDL_Event event;
	event.type = SDL_USEREVENT;
	//event.user.data1 = 1;
	SDL_PushEvent(&event);
	return 0; //0 means stop timer
}


void mixaudio(void *unused, Uint8 *stream, int len)
{
	int i;
	Uint32 amount;
	for ( i=0; i<NUM_SOUNDS; ++i ) {
		amount = (sounds[i].dlen-sounds[i].dpos);
		if ( amount > len ) {
			amount = len;
		}
		SDL_MixAudio(stream, &sounds[i].data[sounds[i].dpos],
					 amount, SDL_MIX_MAXVOLUME);
		sounds[i].dpos += amount;
	}
}

