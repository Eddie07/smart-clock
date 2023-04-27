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




/* Use shared buffer 1024 byte */
#define BUFFER_MAX_SIZE		1024


#define NS_PER_MSEC (1000000L)
/// Convert nanoseconds
#define NS_TO_MSEC(nsec) (div_s64(nsec, NS_PER_MSEC))


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
CLOCK = 1,
TIMER,
TEMPERATURE,
OPTIONS,
NO_DEV,
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
	uint8_t  reset;
	uint64_t nsec_stored;
};

struct clock {
	uint64_t current_sec;
	uint64_t stored_sec;
	uint64_t alarm_sec;
};

struct options {
	uint8_t is_ampm:1;
	uint8_t is_alarm_enabled:1;
};

struct bmp280 {
	struct	 i2c_client *client;
	uint32_t adc_t;
	uint32_t adc_p;
	uint16_t temperature;
	uint16_t pressure;
	uint8_t  is_temp_celsius;
};

struct ds3231 {
	struct i2c_client *client;
	uint8_t sec;
	uint8_t min;
	uint8_t hour;
	uint8_t wday;
	uint8_t mday;
	uint8_t mon;
	uint8_t year;
	uint8_t alarm_sec;
	uint8_t alarm_min;
	uint8_t alarm_hour;
};


//static void st7735fb_clock_display(void);
//static void st7735fb_timer_display(void);

/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/
extern char *fs_buffer;//[BUFFER_MAX_SIZE];
extern size_t fs_buffer_size;

extern int st7735fb_init(void);
extern void st7735fb_exit(void);
//extern struct spi_device *our_tft;
//int  st7735fb_remove(struct spi_device *);
extern int gpio_button_init(void);
extern void  gpio_button_deinit(void);
extern void init_controls(void);
extern void  deinit_controls(void);

extern void st7735fb_blank_display(void);
extern void st7735fb_rotate_display(void);
extern void st7735fb_clock_display(struct tm *, uint8_t *);
extern void st7735fb_timer_display(void);
extern void st7735fb_options_display(uint8_t *);
extern void st7735fb_temperature_display(void);
extern void st7735fb_draw_buff_display(void);
extern void ds3231_readRtcTime(void);
extern void ds3231_writeRtcTime(void);
extern void ds3231_writeOptions(void);

extern void show_timer(void);
extern void show_clock(void);
extern void show_temperature(void);
extern void show_options(void);




//extern int8_t disp_mode;
extern struct button my_button;
extern struct clock_timer our_timer;
extern struct clock clock;
extern struct options options;
extern struct bmp280 bmp280;
extern struct ds3231 ds3231;
//extern struct tm tm_now;



extern int bmp280_init(void);
extern void bmp280_deinit(void);
extern int ds3231_init(void);
extern void ds3231_deinit(void);



#endif
