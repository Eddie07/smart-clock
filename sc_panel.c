// SPDX-License-Identifier: GPL-2.0-only
/*
 * to be filled
 *
 * Dmytro Volkov <splissken2014@gmail.com>
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/spi/spi.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/ktime.h>
#include <linux/of_gpio.h>

#include "include/project1.h"
#include "include/panel.h"


#define DEVICE_NAME		"st7735fb"


/* Define 16bit colors for st7735fb panel */

#define  WHITE_COLOR	0xffff	//white
#define  RED_COLOR	0xf800	//red
#define  GREEN_COLOR	0x07e0	//green
#define  BLUE_COLOR	0x00ff	//blue
#define  YELLOW_COLOR	0xffe0	//yellow

/*Define colors for display modes */

#define DISP_CLOCK_COLOR		BLUE_COLOR
#define DISP_ALARM_COLOR		BLUE_COLOR
#define DISP_TIMER_COLOR		BLUE_COLOR
#define DISP_NOTIF_COLOR		GREEN_COLOR
#define DISP_TEMP_COLOR			BLUE_COLOR
#define DISP_PRESS_COLOR		BLUE_COLOR
#define DISP_PEDOMETER_COLOR		BLUE_COLOR
#define DISP_OPTIONS_COLOR		BLUE_COLOR

/*Define bmp header values */
#define BMP_HEADER_IMAGE_START		0x0a
#define BMP_HEADER_SIZE			0x0e
#define BMP_HEADER_WIDTH		0x12
#define BMP_HEADER_HEIGHT		0x16
#define BMP_HEADER_BPP			0x1c


#define MAX_STRING_SIZE			(20)

#define FAHR(x) (9*x/5+32) //calcualte Fahrenheit


const char *wdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
uint8_t blink_bmask, notif_blink_bmask, alarm_blink_bmask;
static void st7735fb_update_display(void);
static void st7735fb_draw_string(char *, uint16_t, uint16_t, const struct Bitmap *, uint8_t, uint16_t);
static void draw_icon(uint8_t, uint16_t, uint16_t, uint16_t);
static void st7735fb_draw_rectangle(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye, uint16_t color, uint8_t filled);


static void display_update_work(struct work_struct *);


DECLARE_WORK(update_diplay, display_update_work);

/*-----------------------------------------------------------*/
/*	st7735 driver functions	BEGIN			     */
/*-----------------------------------------------------------*/

static int st7735_write_data(const uint8_t *data, size_t size)
{

	/* Set data mode */
	gpio_set_value(st7735fb.dc, 1);

	/* Write entire buffer */
	return spi_write(st7735fb.spi, data, size);
}


static void st7735_write_cmd(uint8_t data)
{
	int ret = 0;


	/* Set command mode */
	gpio_set_value(st7735fb.dc, 0);

	ret = spi_write(st7735fb.spi, &data, 1);
	if (ret < 0)
		pr_err("%s: write data failed\n", DEVICE_NAME);

}

static void st7735_run_cfg(void)
{
	uint8_t i = 0;
	uint8_t end_script = 0;

	do {
		switch (st7735_cfg[i].cmd) {
		case	ST7735_START:
				break;
		case ST7735_CMD:
				st7735_write_cmd(st7735_cfg[i].data);
				break;
		case ST7735_DATA:
				st7735_write_data(&st7735_cfg[i].data, 1);
				break;
		case ST7735_DELAY:
				mdelay(st7735_cfg[i].data);
				break;
		case ST7735_END:
				end_script = 1;
		}
		i++;
	} while (!end_script);
}

static void st7735_reset(void)
{
	/* Reset controller */
	gpio_set_value(st7735fb.rst, 0);
	udelay(10);
	gpio_set_value(st7735fb.rst, 1);
	mdelay(120);
}

static void display_update_work(struct work_struct *work)

{
	int ret = 0;
	uint8_t *vmem = st7735fb.screen_base;
	int i;
	uint16_t *vmem16;
	uint16_t *smem16;
	unsigned int write_nbytes;

	vmem16 = (uint16_t *)vmem;
	smem16 = (uint16_t *)st7735fb.spi_writebuf;
	for (i = 0; i < HEIGHT; i++) {
		int x;

		for (x = 0; x < WIDTH; x++)
		    smem16[x] = (vmem16[x]>>8)|(vmem16[x]<<8);
		smem16 += WIDTH*BPP/16;
		vmem16 += WIDTH*BPP/16;
	}

	/* RAM write command */
	st7735_write_cmd(ST7735_RAMWR);

	/* spi_writebuf write */
	write_nbytes = WIDTH*HEIGHT*BPP/8;
	ret = st7735_write_data(st7735fb.spi_writebuf, write_nbytes);
	if (ret < 0)
		pr_err("%s: write data failed\n", DEVICE_NAME);

}



static int st7735fb_init_display(void)
{

	if (spi_w8r8(st7735fb.spi, ST7735_NOP) < 0)
		return -ENODEV;
	st7735_reset();
	st7735_run_cfg();
	st7735fb_update_display();
	return 0;
}


/*-----------------------------------------------------------*/
/*	st7735 driver functions	END			     */
/*-----------------------------------------------------------*/

void st7735fb_blank_display(void)
{
	/* clear screen */
	memset(st7735fb.screen_base,  0, st7735fb.vmem_size);
	st7735fb_update_display();
}

void st7735fb_clear_blink_bmask(void)
{
	blink_bmask = 0xFF;
	notif_blink_bmask = 0xFF;
	alarm_blink_bmask = 0xFF;
}

void st7735fb_options_display(void)
{

	switch (my_button.edit_mode) {
	case 1:
		blink_bmask ^= 1 << 1;  // option clock 24hrs blinking
		break;
	case 2:
		blink_bmask ^= 1 << 2;  // option alarm blinking
		break;
	case 3:
		blink_bmask ^= 1 << 3;  // option celsius blinking
		break;
	default:
		blink_bmask = 0xFF;
				}

	/* clear screen */
	memset(st7735fb.screen_base, 0, st7735fb.vmem_size);

	st7735fb_draw_string("Options:", 0, 20, &font[FONT24], 14, DISP_OPTIONS_COLOR);

	/* option is clock 24 hours */
	st7735fb_draw_string("clock 24 hours", 25, 55, &font[FONT16], 9, DISP_OPTIONS_COLOR);
	/* option is alarm enabled */
	st7735fb_draw_string("enable alarm", 25, 75, &font[FONT16], 9, DISP_OPTIONS_COLOR);
	/* option is mouse enabled */
	st7735fb_draw_string("temp in celsius", 25, 95, &font[FONT16], 9, DISP_OPTIONS_COLOR);
	/* draw icon is щption enabled */
	draw_icon((options.is_ampm ? 2 : 3), 5, 55, ((blink_bmask >> 1) & 1) ? DISP_OPTIONS_COLOR : 0);
	/* draw icon is option enabled */
	draw_icon((options.is_alarm_enabled ? 3 : 2), 5, 75, ((blink_bmask >> 2) & 1) ? DISP_OPTIONS_COLOR : 0);
	/* draw icon is alarm enabled */
	draw_icon((options.is_temp_celsius ? 3 : 2), 5, 95, ((blink_bmask >> 3) & 1) ? DISP_OPTIONS_COLOR : 0);

	st7735fb_update_display();
}

void st7735fb_alarm_display(uint8_t alarm_hour, uint8_t alarm_min)
{
	static char time[MAX_STRING_SIZE];


	switch (my_button.edit_mode) {
	case 1:
		blink_bmask ^= 1 << 1;  // min
		break;
	case 2:
		blink_bmask ^= 1 << 2;  // hour
		break;
	default:
		blink_bmask = 0xFF;
			}

	/* clear screen */
	memset(st7735fb.screen_base, 0, st7735fb.vmem_size);

	st7735fb_draw_string("Alarm:", 0, 20, &font[FONT24], 14, DISP_ALARM_COLOR);

	if (options.is_ampm) {
		strcpy(time, (alarm_hour > 12 ? "PM":"AM"));
		st7735fb_draw_string(time, 110, 55, &font[FONT24], 22, DISP_ALARM_COLOR);
		/* draw tm_hour */
		sprintf(time, "%2d", (alarm_hour != 12) ? (alarm_hour%12) : 12);
	} else
		sprintf(time, "%02d", alarm_hour);

	st7735fb_draw_string(time, 0, 40, &font[FONT48], 22, ((blink_bmask >> 2) & 1) ? DISP_ALARM_COLOR : 0);
	/* draw tm_min */
	sprintf(time, "%02d", alarm_min);
	st7735fb_draw_string(time, 55, 40, &font[FONT48], 22, ((blink_bmask >> 1) & 1) ? DISP_ALARM_COLOR : 0);

	/* draw time separators  */
	sprintf(time, ":  ");
	st7735fb_draw_string(time, 38, 38, &font[FONT48], 18, DISP_ALARM_COLOR);

	st7735fb_update_display();


}

void st7735fb_clock_display(struct tm *tm)
{
	static char time[MAX_STRING_SIZE];


	switch (my_button.edit_mode) {
	case 1:
		blink_bmask ^= 1 << 1;  // min blinking
		break;
	case 2:
		blink_bmask ^= 1 << 2;  // hour blinking
		break;
	case 3:
		blink_bmask ^= 1 << 3;  // day blinking
		break;
	case 4:
		blink_bmask ^= 1 << 4;  // month blinking
		break;
	case 5:
		blink_bmask ^= 1 << 5;  // year blinking
		break;
	default:
		blink_bmask = 0xFF;
			}

	/* clear screen */
	memset(st7735fb.screen_base, 0, st7735fb.vmem_size);

	/* draw tm_mday */
	sprintf(time, "%s", wdays[tm->tm_wday]);
	st7735fb_draw_string(time, 0, 15, &font[FONT24], 12, DISP_CLOCK_COLOR);
	/* draw m_day */
	sprintf(time, "%02d", tm->tm_mday);
	st7735fb_draw_string(time, 50, 15, &font[FONT24], 12, ((blink_bmask >> 3) & 1) ? DISP_CLOCK_COLOR : 0);
	/* draw tm_mon */
	sprintf(time, "%02d", tm->tm_mon+1);
	st7735fb_draw_string(time, 85, 15, &font[FONT24], 12, ((blink_bmask >> 4) & 1) ? DISP_CLOCK_COLOR : 0);
	/* draw tm_year */
	sprintf(time, "%02d", (uint16_t) tm->tm_year % 100);
	st7735fb_draw_string(time, 120, 15, &font[FONT24], 12, ((blink_bmask >> 5) & 1) ? DISP_CLOCK_COLOR : 0);
	/* draw date dividers  */
	sprintf(time, "/  /");
	st7735fb_draw_string(time, 75, 15, &font[FONT24], 11, DISP_CLOCK_COLOR);

	if (!options.is_ampm) {
		/* draw tm_hour */
		sprintf(time, "%02d", tm->tm_hour);
		st7735fb_draw_string(time, 0, 40, &font[FONT48], 22, ((blink_bmask >> 2) & 1) ? DISP_CLOCK_COLOR : 0);
		/* draw tm_min */
		sprintf(time, "%02d", tm->tm_min);
		st7735fb_draw_string(time, 55, 40, &font[FONT48], 22, ((blink_bmask >> 1) & 1) ? DISP_CLOCK_COLOR : 0);
		/* draw tm_sec */
		sprintf(time, "%02d", tm->tm_sec % 100);
		st7735fb_draw_string(time, 110, 40, &font[FONT48], 22, DISP_CLOCK_COLOR);
		/* draw time separators  */
		sprintf(time, ":  :");
		st7735fb_draw_string(time, 38, 38, &font[FONT48], 18, DISP_CLOCK_COLOR);
	} else {
		sprintf(time, "%2d", (tm->tm_hour != 12) ? (tm->tm_hour%12) : 12);
		st7735fb_draw_string(time, 0, 50, &font[FONT32], 14, ((blink_bmask >> 2) & 1) ? DISP_CLOCK_COLOR : 0);
		/* draw tm_min */
		sprintf(time, "%02d", tm->tm_min);
		st7735fb_draw_string(time, 37, 50, &font[FONT32], 14, ((blink_bmask >> 1) & 1) ? DISP_CLOCK_COLOR : 0);
		/* draw tm_sec */
		sprintf(time, "%02d", tm->tm_sec % 100);
		st7735fb_draw_string(time, 75, 50, &font[FONT32], 14, DISP_CLOCK_COLOR);
		/* draw time separators  */
		sprintf(time, ":  :");
		st7735fb_draw_string(time, 25, 50, &font[FONT32], 13, DISP_CLOCK_COLOR);
		/* draw am/pm  */
		strcpy(time, (tm->tm_hour > 12 ? "PM":"AM"));
		st7735fb_draw_string(time, 110, 60, &font[FONT24], 12, DISP_CLOCK_COLOR);
			}

	st7735fb_update_display();
}

void st7735fb_timer_display(void)
{
	static char timer[MAX_STRING_SIZE];

	sprintf(timer, "%lld", NS_TO_MSEC(our_timer.nsec));

	/* clear screen */
	memset(st7735fb.screen_base, 0, st7735fb.vmem_size);

	st7735fb_draw_string(timer, 0, 20, &font[FONT48], 20, BLUE_COLOR);

	st7735fb_update_display();
}

void st7735fb_temp_and_press_display(void)
{
	static char value[MAX_STRING_SIZE];

	/* clear screen */
	memset(st7735fb.screen_base, 0, st7735fb.vmem_size);

	st7735fb_draw_string("Temperature:", 0, 20, &font[FONT16], 10, DISP_TEMP_COLOR);

	/* Display temperature in C or in Farenheight  */
	if (options.is_temp_celsius)
		sprintf(value, "%d,%d C", temp_and_press.temp/100, temp_and_press.temp%100);
	else
		sprintf(value, "%d F", FAHR(temp_and_press.temp/100));

	st7735fb_draw_string(value, 20, 35, &font[FONT24], 12, DISP_TEMP_COLOR);
	st7735fb_draw_string("Pressure:", 0, 60, &font[FONT16], 10, DISP_PRESS_COLOR);
	sprintf(value, "%d,%d hPa", temp_and_press.press/256/100, temp_and_press.press/256%100);
	st7735fb_draw_string(value, 20, 75, &font[FONT24], 12, DISP_PRESS_COLOR);

	st7735fb_update_display();
}

void st7735fb_pedometer_display(void)
{
	static char value[MAX_STRING_SIZE];

	/* clear screen */
	memset(st7735fb.screen_base, 0, st7735fb.vmem_size);

	st7735fb_draw_string("Pedometer:", 0, 20, &font[FONT16], 10, DISP_PEDOMETER_COLOR);
	sprintf(value, "%d", game.steps_count);
	st7735fb_draw_string(value, 20, 35, &font[FONT24], 12, DISP_PEDOMETER_COLOR);

	st7735fb_update_display();
}


void st7735fb_game_display(void)
{
	static char disp_string[MAX_STRING_SIZE];
	int i;

	memset(st7735fb.screen_base, 0, st7735fb.vmem_size);


	if (!game.is_fruit) {
		get_random_bytes(&game.fruit_x, sizeof(game.fruit_x));
		get_random_bytes(&game.fruit_y, sizeof(game.fruit_y));
		game.fruit_x = game.fruit_x%(WIDTH-15)+10;
		game.fruit_y = game.fruit_x%(HEIGHT-15)+10;
			if ((game.fruit_x > 20) && (game.fruit_y > 20))
				game.is_fruit = 1;

	} else
		draw_icon(8, game.fruit_x-8, game.fruit_y-8, RED_COLOR);

	for (i = 0; i < game.len; i++)
		draw_icon(9, game.tail_x[i]-8, game.tail_y[i]-8, YELLOW_COLOR);

	for (i = 5; i < game.len; i++)
		if ((game.tail_x[i] < game.x+3) &&
				(game.tail_x[i] > game.x-3) &&
				(game.tail_y[i] > game.y-3) &&
				(game.tail_y[i] < game.y+3)) {
			st7735fb_draw_string("GAME OVER", 30, 50, &font[FONT8], 10, RED_COLOR);
			game.game_over = 1;
			my_button.state = 0;
		}
	for (i = game.len; i > 0; i--) {
		game.tail_x[i] = game.tail_x[i-1];
		game.tail_y[i] = game.tail_y[i-1];
		}
	game.tail_x[0] = (uint16_t)game.x;
	game.tail_y[0] = (uint16_t)game.y;

	if (((game.x) < (game.fruit_x+10)) &&
				(game.x > (game.fruit_x-10)) &&
				(game.y > (game.fruit_y-10)) &&
				(game.y < (game.fruit_y+10)) && game.is_fruit)	{
		game.is_fruit = 0;
		game.len += 20;
	}

	if (game.dir_x > 0)
		draw_icon(7, game.x-8, game.y-8, GREEN_COLOR);
	if (game.dir_x < 0)
		draw_icon(5, game.x-8, game.y-8, GREEN_COLOR);
	if (game.dir_y > 0)
		draw_icon(4, game.x-8, game.y-8, GREEN_COLOR);
	if (game.dir_y < 0)
		draw_icon(6, game.x-8, game.y-8, GREEN_COLOR);


	st7735fb_draw_rectangle(0, 0, WIDTH-1, HEIGHT-1, WHITE_COLOR, 0);
	st7735fb_draw_rectangle(0, 0, WIDTH-1, 15, WHITE_COLOR, 0);
	sprintf(disp_string, "%06d", game.len-20);
	st7735fb_draw_string(disp_string, 5, 5, &font[FONT8], 8, WHITE_COLOR);
	st7735fb_update_display();

}





static void draw_char(char letter, uint16_t x, uint16_t y, const struct Bitmap *font, uint16_t color)
{
	uint16_t *vmem16 = (uint16_t *)st7735fb.screen_base;
	uint8_t xc, yc;

	uint16_t char_offset = (letter - ' ') * font->height *
	 (font->width / 8 + (font->width % 8 ? 1 : 0));

	uint8_t *ptr = &font->table[char_offset];

	for (yc = 0; yc < font->height; yc++) {
		for (xc = 0; xc < font->width; xc++) {
			if (*ptr & (0x80 >> (xc % 8)))
				vmem16[(y+yc)*WIDTH+(x+xc)] = color;
			if (xc % 8 == 7)
				ptr++;
		}
		if (font->width % 8 != 0)
			ptr++;
	}

}

static void draw_icon(uint8_t icon, uint16_t x, uint16_t y, uint16_t color)
{
	uint16_t *vmem16 = (uint16_t *)st7735fb.screen_base;
	uint8_t xc, yc;

	uint16_t icon_offset = icon * icons.height *
	 (icons.width / 8 + (icons.width % 8 ? 1 : 0));

	uint8_t *ptr = &icons.table[icon_offset];

	for (yc = 0; yc < icons.height; yc++) {
		for (xc = 0; xc < icons.width; xc++) {
			if (*ptr & (0x80 >> (xc % 8)))
				vmem16[(y+yc)*WIDTH+(x+xc)] = color;
			if (xc % 8 == 7)
				ptr++;
		}
		if (icons.width % 8 != 0)
			ptr++;
	}
}


static void st7735fb_draw_string(char *word, uint16_t x, uint16_t y, const struct Bitmap *font, uint8_t x_offset, uint16_t color)
{
	uint8_t i = 0;

	while (word[i] != 0) {
		draw_char(word[i], x, y, font, color);
		x += x_offset;
		i++;
		}
}

static void st7735fb_draw_rectangle(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye, uint16_t color, uint8_t filled)
{
	uint16_t *vmem16 = (uint16_t *)st7735fb.screen_base;
	uint16_t x = 0, y = 0;

	for (x = 0; x < WIDTH; x++)
		for (y = 0; y < HEIGHT; y++) {
			if (((x == xs) || (x == xe)) && ((y >= ys) && (y <= ye)) || ((y == ys) || (y == ye)) && ((x >= xs) && (x <= xe)))
				vmem16[(y)*WIDTH+(x)] = color;
			if (filled)
				if (((x >= xs) && (x <= xe)) && ((y >= ys) && (y <= ye)))
					vmem16[(y)*WIDTH+(x)] = color;
			}
}


void st7735fb_draw_buff_display(void)
{
	int y, x, xs = 0, ys = 0, bmp_x, bmp_y, header;
	uint16_t *vmem16 = (uint16_t *)st7735fb.screen_base;

	/* Clear display buffer */
	memset(st7735fb.screen_base, 0, st7735fb.vmem_size);

	bmp_x = fs_buffer.buf[BMP_HEADER_WIDTH];
	bmp_y = fs_buffer.buf[BMP_HEADER_HEIGHT];
	header = fs_buffer.buf[BMP_HEADER_IMAGE_START]+1;
	if ((bmp_x > WIDTH) || (bmp_y > HEIGHT)) {
		st7735fb_draw_string("Not supported size", 5, 50, &font[FONT16], 8, RED_COLOR);
		goto err;
	}

	if (bmp_x < WIDTH)
		xs = (WIDTH-bmp_x)/2;
	else
		bmp_x = WIDTH;

	if (bmp_y < HEIGHT)
		ys = (HEIGHT-bmp_y)/2;
	else
		bmp_y = HEIGHT;

	while (y < bmp_y) {
		while (x < bmp_x) {
			if (fs_buffer.buf[BMP_HEADER_BPP] == 0x10)
				vmem16[(y+ys)*WIDTH+(x+xs)] = (fs_buffer.buf[header++] << 8) |
													(fs_buffer.buf[header++]);
			if (fs_buffer.buf[BMP_HEADER_BPP] == 0x18)
				vmem16[(y+ys)*WIDTH+(x+xs)] = (fs_buffer.buf[header++] << 16) |
													(fs_buffer.buf[header++] << 8) |
													(fs_buffer.buf[header++]);
			if (fs_buffer.buf[BMP_HEADER_BPP] == 0x20)
				vmem16[(y+ys)*WIDTH+(x+xs)] = (fs_buffer.buf[header++] << 24) |
													(fs_buffer.buf[header++] << 16) |
													(fs_buffer.buf[header++] << 8) |
													(fs_buffer.buf[header++]);
				x++;
				}
			 x = 0;
			 y++;
	}
err:
	st7735fb_update_display();
}

}

void st7735fb_draw_obj(uint16_t xs, uint16_t ys, uint16_t size)
{
	uint16_t *vmem16 = (uint16_t *)st7735fb.screen_base;
	uint16_t x = 0, y = 0;

	for (x = 0; x < WIDTH; x++)
		for (y = 0; y < HEIGHT; y++) {
			if ((x >= xs) && (x <= xs+size) && (y >= ys) && (y <= ys+size))
				vmem16[(y)*WIDTH+(x)] = BLUE_COLOR;
			}

}

static void st7735fb_notifications_overlay(void)
{

	if ((options.is_alarm_enabled) && (my_button.view_mode != GAME))
		draw_icon(0, 0, 0, DISP_NOTIF_COLOR);
	if ((our_timer.nsec) && (my_button.view_mode != TIMER) && (my_button.view_mode != GAME)) {
		notif_blink_bmask ^= 1 << 1;
		draw_icon(1, 25, 0, ((notif_blink_bmask >> 1) & 1) ? DISP_NOTIF_COLOR : 0);
	}

}

static void st7735fb_alarm_overlay(void)
{

	if ((clock_and_alarm.is_alarm) && (my_button.view_mode != ALARM)) {
		alarm_blink_bmask ^= 1 << 1;
		if  ((alarm_blink_bmask >> 1) & 1)
			st7735fb_draw_string("ALARM!!!", 0, 50, &font[FONT32], 20, DISP_NOTIF_COLOR);
	}

}

static void st7735fb_update_display(void)
{

	/*add notification overlay*/
	st7735fb_notifications_overlay();
	/*add  alarm overlay*/
	st7735fb_alarm_overlay();

	schedule_work(&update_diplay);
}

static int st7735fb_probe(struct spi_device *spi)
{
	struct device_node *np = spi->dev.of_node;
	uint8_t status = 0;
	uint8_t *vmem = NULL;
	uint8_t *spi_writebuf = NULL;
	int retval = -EINVAL;


	pr_err("%s: probe started\n", DEVICE_NAME);


	spi->mode = SPI_MODE_0;
	status = spi_setup(spi);

	if (status < 0) {
		dev_err(&spi->dev, "%s: SPI setup error %d\n",
			__func__, status);
		return status;
	dev_err(&spi->dev, "SPI ok\n");

	st7735fb.spi = spi;
	st7735fb.dc = of_get_named_gpio(np, "dc-gpios", 0);
	st7735fb.rst = of_get_named_gpio(np, "reset-gpios", 0);

	/* Request GPIOs and initialize to default values */
	if (gpio_request_one(st7735fb.rst, GPIOF_OUT_INIT_HIGH, "ST7735 Reset Pin")) {
		pr_err("%s: Cannot request GPIO\n", DEVICE_NAME);
		retval = -EBUSY;
		goto gpio_fail;
	}
	if (gpio_request_one(st7735fb.dc, GPIOF_OUT_INIT_LOW, "ST7735 Data/Command Pin")) {
		pr_err("%s: Cannot request GPIO\n", DEVICE_NAME);
		retval = -EBUSY;
		goto gpio_fail;
	}

	st7735fb.vmem_size = MEM_SIZE;

	vmem = kzalloc(st7735fb.vmem_size, GFP_KERNEL);
	if (!vmem) {
		retval = -ENOMEM;
		goto alloc_fail;
		}
	st7735fb.screen_base = (uint8_t __force __iomem *)vmem;
	pr_err("%s: Total vm initialized %d\n", DEVICE_NAME, st7735fb.vmem_size);

	/* Allocate spi write buffer */
	spi_writebuf = kzalloc(st7735fb.vmem_size, GFP_KERNEL);
	if (!spi_writebuf) {
		retval = -ENOMEM;
		goto alloc_fail;
		}
	st7735fb.spi_writebuf = spi_writebuf;


	retval = st7735fb_init_display();

	/*reporting probe to init */
	init_hw.st7735_spi_probed = 1;

	return retval;

alloc_fail:
	if (spi_writebuf = !NULL)
		kfree(spi_writebuf);
	if (vmem = !NULL)
		kfree(vmem);
gpio_fail:
	gpio_free(st7735fb.dc);
	gpio_free(st7735fb.rst);
	return retval;
};

int  st7735fb_remove(struct spi_device *spi)
{
	pr_err("%s: deregistering\n", DEVICE_NAME);
	msleep(100);
	kfree(st7735fb.screen_base);
	kfree(st7735fb.spi_writebuf);
	gpio_free(st7735fb.dc);
	gpio_free(st7735fb.rst);
	pr_err("%s: unloaded\n", DEVICE_NAME);
	return 0;
}

static const struct of_device_id st7735fb_of_match_table[] = {
	{
		.compatible = "smart-clock,st7735fb",
	},
	{},
};
MODULE_DEVICE_TABLE(of, st7735fb_of_match_table);




struct spi_driver st7735fb_driver = {
	.driver = {
		.name   = "st7735fb",
		.owner  = THIS_MODULE,
		.of_match_table = st7735fb_of_match_table,
	},
	.probe  = st7735fb_probe,
	.remove = st7735fb_remove,
};


int st7735fb_init(void)
{
	pr_err("%s: init\n", DEVICE_NAME);
	return spi_register_driver(&st7735fb_driver);
}

void st7735fb_unload(void)
{
	pr_err("%s: device exit\n", DEVICE_NAME);
	spi_unregister_driver(&st7735fb_driver);

}






