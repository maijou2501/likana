/**
 * rikana(http://suwa.6.ql.bz/rikana.html) for Linux.
 *
 * MIZUSHIKI 様が作成された IMEオン忘れ時打ち直しツール「りかなー」と
 * ほぼ同仕様の Linux 版りかなー.
 * IME(インプットメソッドエンジン)オンを忘れてタイプしてしまったら
 * すかさず「半角/全角」キーを2連打. 直前の文字を打ち直しします.
 *
 * @file    likana.c
 * @author  maijou2501
 * @date    2016/09/13
 * @version 1.4
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>

#define VERSION "1.4"      //!< バージョン情報
#define PUSH    1          //!< キーボード押下判定の定義
#define RELEASE 0          //!< キーボード開放判定の定義
#define DETECT_KEY_CODE  0 //!< キーボード操作判定のための定義
#define DETECT_KEY_VALUE 0 //!< マウス操作判定のための定義
#define INPUT_NUM    60    //!< キーロギングの上限数
#define HANKAKU_NUM   2    //!< "半角/全角"キー押下回数のしきい値
#define INPUT_EVENTS 64    //!< キーイベント操作の上限数
#define SLEEP_TIME    0          //!<  0 sec
#define SLEEP_TIME_NANO 20000000 //!< 20 msec

int input[INPUT_NUM] ={0}; //!< ロギングしたキーの値を格納する配列
short count   = 0;         //!< ロギングカウント
short count_h = 0;         //!< "半角/全角"キーカウント

static struct option options[] =
{
	{"help",     no_argument, NULL, 'h'},
	{"version",  no_argument, NULL, 'v'},
	{"mouse",    no_argument, NULL, 'm'},
	{"keyboard", required_argument, NULL, 'k'},
	{0, 0, 0, 0}
}; //!< 長いオプションの名前を格納する構造体配列の初期化

static struct timespec req =
{SLEEP_TIME, SLEEP_TIME_NANO}; //!< nanosleep のための構造体の宣言

/**
 * スレッドパラメータ定義
 */
typedef struct {
	char *device;   //!< character device pointer
} THREAD_ARG;

/**
 * 20 msec スリープする.
 *
 * sleep関数では1秒からの指定なので、nanosleep関数を用いて 20 msec スリープさせる.
 *
 * @param  なし
 * @return なし
 */
void mysleep()
{
	if (nanosleep(&req, NULL) == -1) {
		perror("nanosleep");
		exit(EXIT_FAILURE);
	}
}

/**
 * キーボード入力をエミュレートする.
 *
 * Linux Input Subsystem を用いている.
 *
 * @param[in] code  ロギングしたキーコード
 * @param[in] value 押下・開放の指定
 * @param[out] fd   出力先の指定
 * @return なし
 */
void write_key_event(int code, int value, int fd)
{
	// define variations
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

/**
 * マウスイベントを待ち受ける.
 *
 * スレッドを用いてキーボードイベントとは別に,マウスイベントを待ち受ける.
 * pthread スレッドに値を渡すために,構造体を使っている.
 *
 * @param[in] *arg pthread_createの第4引数のポインタ
 * @return    なし
 * @attention likana 起動中にマウスが切り離された際のハンドリングができていない.
 */
void* thread_mouse(void *arg)
{
	// define variations
	short i;
	short j;
	struct input_event events[INPUT_EVENTS];
	THREAD_ARG *thread_arg =(THREAD_ARG*)arg;

	// pthread detach
	pthread_t self_thread = pthread_self();
	if (pthread_detach(self_thread) != 0) {
		perror("detach");
		exit(EXIT_FAILURE);
	}

	// open device file of mouse
	int fd = open( (char *)thread_arg->device, O_RDONLY);
	if (fd < 0) {
		perror("mouse");
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

/**
 * 使い方の説明.
 *
 * 詳細説明は割愛します.
 * 
 * @param  なし
 * @return なし
 */
void usage()
{
	printf( "Usage: likana [option]... > /dev/input/event*\n"
			"  -k /dev/input/event*   character device of keyboard\n"
			"  -m /dev/input/event*   character device of mouse\n"
			"  -h                     display this help\n"
			"  -v                     display version\n"
		  );
}

/**
 * バージョン情報の出力.
 *
 * 詳細説明は割愛します.
 *
 * @param  なし
 * @return なし
 */
void version()
{
	printf( "likana version %s\n", VERSION);
}

/**
 * 指定されたファイルがキャラクタデバイスか判定する.
 *
 * <sys/types.h> , <sys/stat.h> の include が必要.
 *
 * @param[in] *st stat構造体のアドレス
 * @retval 0 チェックしたファイルがキャラクタデバイスだった
 * @retval 1 チェックしたファイルがキャラクタデバイスではなかった
 */
int check_stat(struct stat *st)
{
    mode_t m = st->st_mode;
    if (S_ISCHR(m)){
			return 0;
		} else {
			return 1;
		}
}
/**
 * main関数.
 *
 * 引数無しでは usage(). getopt_long() で使われなかった引数があった場合も usage().
 * 
 * @param[in] argc コマンドラインパラメータ数
 * @param[in] argv コマンドラインパラメータ
 * @retval    0     正常終了
 * @retval    0以外 異常終了
 * @attention likana 起動中にキーボードが切り離された際のハンドリングができていない.
 */
int main(int argc, char *argv[])
{
	// define variations
	short i;
	short j;
	int   index;
	char  c;
	char  *keyboard;
	struct stat st;
	struct input_event events[INPUT_EVENTS];
	pthread_t th;
	THREAD_ARG thread_arg;

	// check arguments number
	if ( argc == 1 ) {
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
				// check character device
				if (stat(optarg, &st) == -1) {
					perror("stat_keyboard");
					exit(EXIT_FAILURE);
				}
				if (check_stat(&st) != 0){
					printf( "%s is NOT a character device\n", optarg);
					exit(EXIT_FAILURE);
				}
				keyboard = optarg;
				break;

			case 'm':
				// check character device
				if (stat(optarg, &st) == -1) {
					perror("stat_mouse");
					exit(EXIT_FAILURE);
				}
				if (check_stat(&st) != 0){
					printf( "%s is NOT a character device\n", optarg);
					exit(EXIT_FAILURE);
				}

				// thread of mouse event loop
				thread_arg.device = (char *)optarg;
				pthread_create( &th, NULL, thread_mouse, &thread_arg );
				break;

			case ':':
				// no arguments
				exit(EXIT_FAILURE);

			default:
				// invalid option
				exit(EXIT_FAILURE);
		}
	}

	// check arguments number
	if ( optind != argc) {
		usage();
		exit(EXIT_FAILURE);
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

			// press hankaku/zenkaku button
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

		// open device file of keyboard
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
	return EXIT_SUCCESS;
}
