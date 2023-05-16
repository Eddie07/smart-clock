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

/* Define screen shared values */
#define WIDTH		(160)
#define HEIGHT		(128)

/* Game */
#define SNAKE_TAIL_MAX	(1000)




/**
 * enum disp_modes  - display view order by switching
 * @CLOCK: clock view
 * @TIMER: timer view
 * @ALARM: alarm view
 * @TEMP_AND_PRESS: temperature and pressure view
 * @PEDOMETER: pedometer view
 * @GAME: game view
 * @OPTIONS: options view
 */
enum disp_modes {
	CLOCK = 1,
	TIMER,
	ALARM,
	TEMP_AND_PRESS,
	PEDOMETER,
	GAME,
	OPTIONS,
	DISP_MODES_END,
};


struct init_hw {
	uint8_t st7735_spi_probed:1;
	uint8_t bmp280_i2c_probed:1;
	uint8_t ds3231_i2c_probed:1;
	uint8_t mpu6050_i2c_probed:1;
};
extern struct init_hw init_hw;


struct button {
	uint8_t state;
	uint8_t is_longpress;
	uint8_t view_mode;
	uint8_t edit_mode;

};
extern struct button my_button;

struct clock_timer {
	uint64_t nsec;
	uint64_t nsec_old_ktime;
	uint64_t nsec_stored;
	uint8_t  is_timer_on;
};
extern struct clock_timer our_timer;

struct clock_and_alarm {
	uint64_t clock_sec;
	uint64_t clock_stored_sec;
	uint32_t alarm_sec;
	uint32_t alarm_stored_sec;
	uint8_t is_alarm;
};
extern struct clock_and_alarm clock_and_alarm;

struct temp_and_press {
	int32_t temp;
	uint32_t press;
};
extern struct temp_and_press temp_and_press;

struct game {
	int16_t x;
	int16_t y;
	int8_t dir_x;
	int8_t dir_y;
	uint16_t tail_x[SNAKE_TAIL_MAX];
	uint16_t tail_y[SNAKE_TAIL_MAX];
	size_t len;
	uint16_t fruit_x;
	uint16_t fruit_y;
	uint16_t is_fruit;
	uint8_t game_over;
	uint32_t steps_count;
};
extern struct game game;

struct options {
	uint8_t is_ampm:1;
	uint8_t is_alarm_enabled:1;
	uint8_t is_temp_celsius:1;
};
extern struct options options;

struct fs_buffer {
	char *buf;
	size_t buf_len;
};
extern struct fs_buffer fs_buffer;

/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/
extern int  st7735fb_init(void);
extern int  gpio_button_init(void);
extern void init_controls(void);
extern int  sensors_init(void);
extern void st7735fb_unload(void);
extern void gpio_button_unload(void);
extern void sensors_unload(void);
extern void controls_unload(void);

extern void st7735fb_blank_display(void);
extern void st7735fb_clear_blink_bmask(void);
extern void st7735fb_rotate_display(void);
extern void st7735fb_clock_display(struct tm *tm);
extern void st7735fb_alarm_display(uint8_t alarm_hour, uint8_t alarm_min);
extern void st7735fb_timer_display(void);
extern void st7735fb_options_display(void);
extern void st7735fb_temp_and_press_display(void);
extern void st7735fb_pedometer_display(void);
extern void st7735fb_game_display(void);
extern void st7735fb_draw_buff_display(void);
extern int st7735fb_get_buff_display(void);

/**
 * External functions for ds3231 rtc sensor
 */
extern void ds3231_writeRtcTimeAndAlarm(void);
extern void ds3231_writeOptions(void);


extern void show_clock_view(void);
extern void show_timer_view(void);
extern void show_alarm_view(void);
extern void show_temp_and_press_view(void);
extern void show_pedometer_view(void);
extern void show_game_view(void);
extern void show_options_view(void);

#endif
