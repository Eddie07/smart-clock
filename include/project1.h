/* SPDX-License-Identifier: GPL-2.0-only
 *
 * Leds GPIO's driver for RGB leds header
 *
 *
 * Dmytro Volkov <splissken2014@gmail.com>
 *
 */

#ifndef __PROJECT1_H_INCLUDED
#define __PROJECT1_H_INCLUDED

#include <linux/gpio.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/delay.h>

#define MODULE_NAME module_name(THIS_MODULE)

#ifdef pr_fmt
#undef pr_fmt
#endif

#define pr_fmt(fmt) "%s: " fmt, KBUILD_MODNAME


/* Use shared buffer 1024 byte */
//#define BUFFER_MAX_SIZE		1024


#define NS_PER_MSEC (1000000L)
/// Convert nanoseconds
#define NS_TO_MSEC(nsec) (div_s64(nsec, NS_PER_MSEC))

#define WIDTH		160
#define HEIGHT		128

/*set default GPIO value to avoid troubles. But later replace from DT*/

/*
struct gpio {
	unsigned	gpio;
	unsigned long	flags;
	const char	*label;
};
*/

static struct gpio button_gpio = { 17, GPIOF_DIR_IN,  "Switch button"};


enum disp_modes {
CLOCK=1,
TIMER,
ALARM,
TEMP_AND_PRESS,
PEDOMETER,
GAME,
OPTIONS,
END,
};


struct init_hw {
	uint8_t st7735_spi_probed:1;
	uint8_t bmp280_i2c_probed:1;
	uint8_t ds3231_i2c_probed:1;
	uint8_t mpu6050_i2c_probed:1;
};


struct button {
	uint8_t mode;
	uint8_t state;
	uint8_t set_mode;
	uint8_t is_longpress;
};

struct clock_timer {
	uint64_t nsec;
	uint64_t nsec_current_ktime;
	uint64_t nsec_old_ktime;
	uint8_t  timer_start;
	uint64_t nsec_stored;
};

struct clock {
	uint64_t current_sec;
	uint64_t stored_sec;
	uint32_t alarm_sec;
	uint32_t stored_alarm_sec;
};

struct temp_and_press {
	uint32_t temp;
	uint32_t press;
};

struct game {
	int16_t x;
	int16_t y;
	uint16_t *tail_x;
	uint16_t *tail_y;
	size_t len;
	uint16_t fruit_x;
	uint16_t fruit_y;
	uint16_t is_fruit;
	int8_t dir_x;
	int8_t dir_y;
	uint8_t score;
	uint8_t game_over;
	uint32_t steps_count;
};

struct options {
	uint8_t is_ampm:1;
	uint8_t is_alarm_enabled:1;
	uint8_t is_temp_celsius:1;
};

struct fs_buffer {
	char * buf;
	size_t buf_len;
};

/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/
extern int  st7735fb_init(void);
extern void st7735fb_exit(void);
extern int  gpio_button_init(void);
extern void gpio_button_deinit(void);
extern void init_controls(void);
extern void deinit_controls(void);
extern int  sensors_init(void);
extern void sensors_deinit(void);

extern void st7735fb_blank_display(void);
extern void st7735fb_clear_blink_bmask(void);
extern void st7735fb_rotate_display(void);
extern void st7735fb_clock_display(struct tm *);
extern void st7735fb_alarm_display(uint8_t, uint8_t);
extern void st7735fb_timer_display(void);
extern void st7735fb_options_display(void);
extern void st7735fb_temp_and_press_display(void);
extern void st7735fb_pedometer_display(void);
extern void st7735fb_game_display(void);
extern void st7735fb_draw_buff_display(void);
extern void ds3231_writeRtcTimeAndAlarm(void);
extern void ds3231_writeOptions(void);


extern void show_clock_view(void);
extern void show_timer_view(void);
extern void show_alarm_view(void);
extern void show_temp_and_press_view(void);
extern void show_pedometer_view(void);
extern void show_game_view(void);
extern void show_options_view(void);

extern struct fs_buffer fs_buffer;
extern struct button my_button;
extern struct clock_timer our_timer;
extern struct clock clock;
extern struct options options;
extern struct temp_and_press temp_and_press; 
extern struct init_hw init_hw;
extern struct game game;
extern struct big big;



#endif
