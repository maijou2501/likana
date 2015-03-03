/**
 *
 * likana.c
 *
 * description : rikana(http://suwa.6.ql.bz/rikana.html) for linux
 * author  : maijou2501
 * update  : 2015/03/03
 * version : 1.2
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <linux/input.h>

#define VERSION "1.2"
#define PUSH    1
#define RELEASE 0
#define DETECT_KEY_CODE  0
#define DETECT_KEY_VALUE 0
#define INPUT_NUM    60
#define HANKAKU_NUM   2
#define INPUT_EVENTS 64
#define SLEEP_TIME   0          // 0  sec
#define SLEEP_TIME_NANO 20000000 // 20 msec

// groval variations
int input[INPUT_NUM] ={0};
short count   = 0;
short count_h = 0;
static struct option options[] =
{
	{"help",     no_argument, NULL, 'h'},
	{"version",  no_argument, NULL, 'v'},
	{"mouse",    no_argument, NULL, 'm'},
	{"keyboard", required_argument, NULL, 'k'},
	{0, 0, 0, 0}
};

// for thread param
typedef struct {
	char *device;
} THREAD_ARG;

// for nanosleep
static struct timespec req = {SLEEP_TIME, SLEEP_TIME_NANO};

// sleep
void mysleep()
{
	if (nanosleep(&req, NULL) == -1) {
		perror("nanosleep");
		exit(EXIT_FAILURE);
	}
}
// write key value to device file
void write_key_event(int code, int value, int fd)
{
	// define valiations
	struct input_event key_event;
	gettimeofday(&key_event.time, NULL);
	key_event.type = EV_KEY;
	key_event.code = code;
	key_event.value = value;

	// write key value to device file
	if (write(fd, &key_event, sizeof(key_event)) == -1) {
		perror("write");
		exit(EXIT_FAILURE);
	}
}

// capture keyboard event thread
void* thread_keyboard(void *arg)
{
	// define valiations
	short i;
	short j;
	struct input_event events[INPUT_EVENTS];
	THREAD_ARG *thread_arg =(THREAD_ARG*)arg;

	// pthread detach
	pthread_t self_thread = pthread_self();
	if (pthread_detach(self_thread) != 0) {
		perror("detatch");
		exit(EXIT_FAILURE);
	}

	// open device file of keyboard
	int fd = open( (char *)thread_arg->device, O_RDONLY);
	if (fd < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	// capture keyboard event loop
	for (;;) {

		size_t read_size = read(fd, events, sizeof(events));
		for (i = 0; i < (int)(read_size / sizeof(struct input_event)); i++){
			// detect key up event
			if ( events[i].value == DETECT_KEY_VALUE && events[i].code != DETECT_KEY_CODE ){

				// 1. release "-", "a-z" keys
				if( events[i].code == KEY_MINUS\
						|| ( events[i].code >= KEY_Q && events[i].code <= KEY_P)\
						|| ( events[i].code >= KEY_A && events[i].code <= KEY_L )\
						|| ( events[i].code >= KEY_Z && events[i].code <= KEY_M ) ) {

					// check INPUT_NUM and clear counter, depend on hankaku counter
					if( count == INPUT_NUM || count_h != 0 ) {
						for ( j = 0; j < count; j++) {
							input[j]=0;
						}
						count = 0;
					}
					// key logging
					input[count] = events[i].code;
					// increment counter
					count++;
					// clear hankaku counter
					count_h = 0;

				} else if ( events[i].code == KEY_BACKSPACE ) { // 2. release backspace
					if( count != 0 ) count--;

				} else if ( events[i].code == KEY_GRAVE) { // 3. release hankaku/zenkaku key
					count_h++;

				} else { // 4. release untarget keys (clear counter)
					for ( j = 0; j < count; j++) {
						input[j]=0;
					}
					count=0;
					count_h = 0;
				}
			}
		}
	}
	// close device file of keyboard
	close(fd);
}

// capture mouse event thread
void* thread_mouse(void *arg)
{
	// define valiations
	short i;
	short j;
	struct input_event events[INPUT_EVENTS];
	THREAD_ARG *thread_arg =(THREAD_ARG*)arg;

	// pthread detach
	pthread_t self_thread = pthread_self();
	if (pthread_detach(self_thread) != 0) {
		perror("detatch");
		exit(EXIT_FAILURE);
	}

	// open device file of mouse
	int fd = open( (char *)thread_arg->device, O_RDONLY);
	if (fd < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	// capture mouse event loop
	for (;;) {
		// read device file of mouse
		size_t read_size_m = read(fd, events, sizeof(events));
		for (i = 0; i < (int)(read_size_m / sizeof(struct input_event)); i++){
			// clear counter (key up of left click)
			if ( events[i].value == DETECT_KEY_VALUE && events[i].code == BTN_LEFT){
				for (j = 0;j < count;j++) { input[j] = 0; }
				count=0;
				count_h = 0;
			}
		}
	}
	// close device file of mouse
	close(fd);
}

// print usage
void usage()
{
	printf( "Usage: linaka [option]... > /dev/input/event*\n"
			"  -k /dev/input/event*   character device of keyboard\n"
			"  -m /dev/input/event*   character device of mouse\n"
			"  -h                     display this help\n"
			"  -v                     display version\n"
		  );
}

// print version
void version()
{
	printf( "likana version %s\n", VERSION);
}


int main(int argc, char *argv[])
{
	// define valiations
	short i;
	short j;
	char c;
	int index;
	char *keyboard;
	struct input_event events[INPUT_EVENTS];
	pthread_t th;
	THREAD_ARG thread_arg;

	// check arguments number
	if ( argc == 0) {
		usage();
		exit(EXIT_FAILURE);
	}

	// check arguments
	while((c = getopt_long(argc, argv, "hvk:m:", options, &index)) != -1){

		switch(c){
			case 'h':
				usage();
				exit(EXIT_SUCCESS);

			case 'v':
				version();
				exit(EXIT_SUCCESS);

			case 'k':
				keyboard = optarg;
				// thread of keyboard event loop
				//thread_arg.device = (char *)optarg;
				//pthread_create( &th, NULL, thread_keyboard, &thread_arg );
				break;

			case 'm': 
				// thread of mouse event loop
				thread_arg.device = (char *)optarg;
				pthread_create( &th, NULL, thread_mouse, &thread_arg );
				break;

			case ':':
				// no arguments
				exit(EXIT_FAILURE);

				// case '?':
			default:
				// invalit option
				exit(EXIT_FAILURE);
		}
	}


	// open device file of keyboard
	int fd = open(keyboard, O_RDONLY);
	if (fd < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	// wait event
	for (;;) {

		// threshold counts of push hankaku/zenkaku key
		if (count_h == HANKAKU_NUM) {

			// clear hankaku/zenkaku counter
			count_h = 0;

			// if NOT sleep, fail simulate input keys...
			for (i = 0;i < count;i++) {
				write_key_event( KEY_BACKSPACE, PUSH, STDOUT_FILENO);
				mysleep();
				write_key_event( KEY_BACKSPACE, RELEASE, STDOUT_FILENO);
				mysleep();
			}

			// press hankaku/zenkaku bottun
			write_key_event( KEY_GRAVE, PUSH, STDOUT_FILENO);
			mysleep();
			write_key_event( KEY_GRAVE, RELEASE, STDOUT_FILENO);
			mysleep();

			// input key values
			for (j = 0; j < count; j++) {
				write_key_event( input[j], PUSH, STDOUT_FILENO);
				mysleep();
				write_key_event( input[j], RELEASE, STDOUT_FILENO);
				mysleep();
				input[j]=0;
			}
			count = 0;
		}

		// open device file of keybord
		size_t read_size = read(fd, events, sizeof(events));
		for (i = 0; i < (int)(read_size / sizeof(struct input_event)); i++){
			// detect key up event
			if ( events[i].value == DETECT_KEY_VALUE && events[i].code != DETECT_KEY_CODE ){

				// 1. release "-", "a-z" keys
				if( events[i].code == KEY_MINUS\
						|| ( events[i].code >= KEY_Q && events[i].code <= KEY_P)\
						|| ( events[i].code >= KEY_A && events[i].code <= KEY_L )\
						|| ( events[i].code >= KEY_Z && events[i].code <= KEY_M ) ) {

					// check INPUT_NUM and clear counter, depend on hankaku counter
					if( count == INPUT_NUM || count_h != 0 ) {
						for ( j = 0; j < count; j++) {
							input[j]=0;
						}
						count = 0;
					}
					// key logging
					input[count] = events[i].code;
					// increment counter
					count++;
					// clear hankaku counter
					count_h = 0;

				} else if ( events[i].code == KEY_BACKSPACE ) { // 2. release backspace
					if( count != 0 ) count--;

				} else if ( events[i].code == KEY_GRAVE) { // 3. release hankaku/zenkaku key
					count_h++;

				} else { // 4. release untarget keys (clear counter)
					for ( j = 0; j < count; j++) {
						input[j]=0;
					}
					count=0;
					count_h = 0;
				}
			}
		}
	}
	// close device file of mouse
	close(fd);
	return EXIT_SUCCESS;
}
