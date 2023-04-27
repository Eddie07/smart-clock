

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


/* Define 16bit colors for st7735fb panel */

#define  WHITE_COLOR	0xffff	//white
#define  RED_COLOR	0xf800	//red
#define  GREEN_COLOR	0x07e0	//green
#define  BLUE_COLOR	0x00ff	//blue
#define  YELLOW_COLOR	0xffe0	//yellow

/*Define colors for display modes */

#define DISP_CLOCK_COLOR	BLUE_COLOR
#define DISP_ALARM_COLOR	RED_COLOR
#define DISP_NOTIF_COLOR	GREEN_COLOR
#define DISP_TEMPERATURE_COLOR	BLUE_COLOR
#define DISP_PRESSURE_COLOR	BLUE_COLOR
#define DISP_OPTIONS_COLOR	BLUE_COLOR


const char *wdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

struct st7735fb_par par;

static void st7735fb_draw_string(char *, uint16_t, uint16_t, const struct Bitmap *, uint8_t, uint16_t);
static void draw_icon(uint8_t, uint16_t, uint16_t, uint16_t);
static void st7735fb_display_notifications(void);



static struct timer_list digital_clock;//, digital_timer;

static void display_update_work(struct work_struct *);



DECLARE_WORK(update_diplay, display_update_work);

/*-----------------------------------------------------------*/
/*	st7735 driver functions	BEGIN			     */
/*-----------------------------------------------------------*/

static int st7735_write_data(uint8_t *txbuf, size_t size)
{

	/* Set data mode */
	gpio_set_value(par.dc, 1);

	/* Write entire buffer */
	return spi_write(par.spi, txbuf, size);
}

static void st7735_write_cmd(uint8_t data)
{
	int ret = 0;


	/* Set command mode */
	gpio_set_value(par.dc, 0);

	ret = spi_write(par.spi, &data, 1);
	if (ret < 0)
		pr_err("write data failed\n");
}

static void st7735_run_cfg(void)
{
	uint8_t i = 0;
	uint8_t end_script = 0;
	uint8_t arg;


	do {
		switch (st7735_cfg[i].cmd)
		{
		case ST7735_START:
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
	gpio_set_value(par.rst, 0);
	udelay(10);
	gpio_set_value(par.rst, 1);
	mdelay(120);
}





static void display_update_work(struct work_struct *work)

{
	int ret = 0;
	uint8_t *vmem = par.screen_base;
	int i;
	uint16_t *vmem16;
	uint16_t *smem16;
	unsigned int vmem_start;
	unsigned int write_nbytes;


	vmem16 = (uint16_t *)vmem;
	smem16 = (uint16_t *)par.spi_writebuf;
	for (i = 0; i < HEIGHT; i++) {
		int x;
#ifdef __LITTLE_ENDIAN
		for (x = 0; x < WIDTH; x++)
		    smem16[x] = swab16(vmem16[x]);
#else
		memcpy(smem16, vmem16, WIDTH*2);
#endif
		smem16 += WIDTH*BPP/16;
		vmem16 += WIDTH*BPP/16;
	}


	/* Internal RAM write command */
	st7735_write_cmd(ST7735_RAMWR);

	/* Blast spi_writebuf to ST7735 internal display RAM target window */
	write_nbytes = WIDTH*HEIGHT*BPP/8;
	ret = st7735_write_data(par.spi_writebuf, write_nbytes);
	if (ret < 0)
		pr_err("write data failed\n");

}


static void st7735fb_update_display (void)
{

	/*add to display alarm if enabled*/
	st7735fb_display_notifications();

	/*add to display timer if enabled*/


	schedule_work(&update_diplay);
}

static int st7735fb_init_display(void)
{
	st7735_reset();
	st7735_run_cfg();
	st7735fb_update_display();
	return 0;
}


void st7735fb_blank_display(void)
{

	memset(par.screen_base,  0, par.vmem_size);
	st7735fb_update_display();

}

/*-----------------------------------------------------------*/
/*	st7735 driver functions	END			     */
/*-----------------------------------------------------------*/



void st7735fb_draw_buff_display(void)
{
	uint16_t *vmem16 = (uint16_t *)par.screen_base;
	int y = 0, x = 0, i = 0x7b;
		for (x = 0; x < WIDTH; x++)
			for (y = 0; y < HEIGHT; y++) {
			{

        			vmem16[(y)*WIDTH+(x)] = (fs_buffer[i++]<<11)|(fs_buffer[i++]<<5)|fs_buffer[i++];
				i++;

		}
	}

	st7735fb_update_display();

}

void st7735fb_options_display(uint8_t *options_blink_bmask)

{

	switch (my_button.set_mode) {
			case 1:	*options_blink_bmask ^= 1 << 1;  // option clock 24hrs
				break;
			default: *options_blink_bmask = 0xFF;
				}

	st7735fb_draw_string("Options:", 0, 0, &font[1], 14, DISP_OPTIONS_COLOR);

	st7735fb_draw_string("clock 24 hours", 25, 40, &font[0], 8, DISP_OPTIONS_COLOR);
	/* draw icon is clock 24 hours */
	draw_icon((options.is_ampm ? 2 : 3), 5, 40, ((*options_blink_bmask >> 1) & 1) ? DISP_OPTIONS_COLOR : 0);
	st7735fb_update_display();
}

void st7735fb_clock_display(struct tm *tm, uint8_t *clock_blink_bmask)

{
	char time[20];


	switch (my_button.set_mode) {
			case 1:	*clock_blink_bmask ^= 1 << 1;  // min
				break;
			case 2: *clock_blink_bmask ^= 1 << 2;  // hour
				break;
			case 3: *clock_blink_bmask ^= 1 << 3;  // day
				break;
			case 4:	*clock_blink_bmask ^= 1 << 4;  // month
				break;
			case 5:	*clock_blink_bmask ^= 1 << 5;  // year
				break;
			default: *clock_blink_bmask = 0xFF;
			}



	/* clear screen */
	memset(par.screen_base, 0, par.vmem_size);

	/* draw tm_mday */

	sprintf(time, "%s", wdays[tm->tm_wday]);
	st7735fb_draw_string(time, 0, 15, &font[1], 12, DISP_CLOCK_COLOR);
	/* draw m_day */
	sprintf(time, "%02d", tm->tm_mday);
	st7735fb_draw_string(time, 50, 15, &font[1], 12, ((*clock_blink_bmask >> 3) & 1) ? DISP_CLOCK_COLOR : 0);
	/* draw tm_mon */
	sprintf(time, "%02d", tm->tm_mon+1);
	st7735fb_draw_string(time, 85, 15, &font[1], 12, ((*clock_blink_bmask >> 4) & 1) ? DISP_CLOCK_COLOR : 0);
	/* draw tm_year */
	sprintf(time, "%02d", (uint16_t) tm->tm_year % 100);
	st7735fb_draw_string(time, 120, 15, &font[1], 12, ((*clock_blink_bmask >> 5) & 1) ? DISP_CLOCK_COLOR : 0);
	/* draw date dividers  */
	sprintf(time, "/  /");
	st7735fb_draw_string(time, 75, 15, &font[1], 11, DISP_CLOCK_COLOR);

	if (!options.is_ampm) {
		/* draw tm_hour */
		sprintf(time, "%02d", tm->tm_hour);
		st7735fb_draw_string(time, 0, 40, &font[3], 22, ((*clock_blink_bmask >> 2) & 1) ? DISP_CLOCK_COLOR : 0);
		/* draw tm_min */
		sprintf(time, "%02d", tm->tm_min);
		st7735fb_draw_string(time, 55, 40, &font[3], 22, ((*clock_blink_bmask >> 1) & 1) ? DISP_CLOCK_COLOR : 0);
		/* draw tm_sec */
		sprintf(time, "%02d", tm->tm_sec % 100);
		st7735fb_draw_string(time, 110, 40, &font[3], 22, DISP_CLOCK_COLOR);
		/* draw time separators  */
		sprintf(time, ":  :");
		st7735fb_draw_string(time, 38, 38, &font[3], 18, DISP_CLOCK_COLOR);
	} else {
		sprintf (time, "%2d", (tm->tm_hour != 12) ? (tm->tm_hour%12) : 12);
		st7735fb_draw_string(time, 0, 50, &font[2], 14, ((*clock_blink_bmask >> 2) & 1) ? DISP_CLOCK_COLOR : 0);
		/* draw tm_min */
		sprintf(time, "%02d", tm->tm_min);
		st7735fb_draw_string(time, 37, 50, &font[2], 14, ((*clock_blink_bmask >> 1) & 1) ? DISP_CLOCK_COLOR : 0);
		/* draw tm_sec */
		sprintf(time, "%02d", tm->tm_sec % 100);
		st7735fb_draw_string(time, 75, 50, &font[2], 14, DISP_CLOCK_COLOR);
		/* draw time separators  */
		sprintf(time, ":  :");
		st7735fb_draw_string(time, 25, 50, &font[2], 13, DISP_CLOCK_COLOR);
		/* draw am/pm  */
		strcpy(time, (tm->tm_hour > 12 ? "PM":"AM"));
		st7735fb_draw_string(time, 110, 60, &font[1], 12, DISP_CLOCK_COLOR);
			}


	st7735fb_update_display();


}


//static void digital_timer_timer (struct timer_list *t) {

void st7735fb_timer_display(void)

{


        char timer[20] = {0};


	sprintf(timer, "%lld", NS_TO_MSEC(our_timer.nsec));

	/* clear screen */
	memset(par.screen_base, 0, par.vmem_size);

	st7735fb_draw_string(timer, 0, 20, &font[3], 20, BLUE_COLOR);

	st7735fb_update_display();


}

void st7735fb_temperature_display(void)

{


        char temperature[20] = {0};


	memset(par.screen_base, 0, par.vmem_size);
	//sprintf (temperature, "%d", bmp280.temperature);
	sprintf(temperature, " ");

	st7735fb_draw_string(temperature, 0, 20, &font[3], 20, BLUE_COLOR);

	st7735fb_update_display();


}


static void st7735fb_display_notifications(void)
{


	draw_icon(2, 20, 0, DISP_NOTIF_COLOR);
	if ((our_timer.nsec) && (my_button.mode != TIMER))
			draw_icon(1, 30, 0, DISP_NOTIF_COLOR);

}

static void draw_char(char letter, uint16_t x, uint16_t y, const struct Bitmap *font, uint16_t color)
{
	uint16_t *vmem16 = (uint16_t *)par.screen_base;
	uint8_t xc, yc;

	uint16_t char_offset = (letter - ' ') * font->height *
	 (font->width / 8 + (font->width % 8 ? 1 : 0));

	uint8_t *ptr = &font->table[char_offset];
	for (yc = 0; yc < font->height; yc++)
	{
		for (xc = 0; xc < font->width; xc++)
		{
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
	uint16_t *vmem16 = (uint16_t *)par.screen_base;
	uint8_t xc, yc;

	uint16_t icon_offset = icon * icons.height *
	 (icons.width / 8 + (icons.width % 8 ? 1 : 0));

	uint8_t *ptr = &icons.table[icon_offset];
	for (yc = 0; yc < icons.height; yc++)
	{
		for (xc = 0; xc < icons.width; xc++)
		{
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



static int st7735fb_probe(struct spi_device *spi)
{
	struct device_node *np = spi->dev.of_node;
	uint8_t status = 0;
	uint8_t *vmem = NULL;
	uint8_t *spi_writebuf = NULL;
	int retval = -EINVAL;


	pr_err("probe started\n");

	spi->mode = SPI_MODE_0;
	status = spi_setup(spi);

	if (status < 0) {
		dev_err(&spi->dev, "%s: SPI setup error %d\n",
			__func__, status);
		return status;
	} else dev_err(&spi->dev, "SPI ok\n");

	par.vmem_size = WIDTH*HEIGHT*BPP/8;

	vmem = kzalloc(par.vmem_size, GFP_KERNEL);
	if (!vmem) {
		retval = -ENOMEM;
		goto alloc_fail;
		}
	par.screen_base = (uint8_t __force __iomem *)vmem;
	pr_err("Total vm initialized %d\n", par.vmem_size);

	/* Allocate spi write buffer */
	spi_writebuf = kzalloc(par.vmem_size, GFP_KERNEL);
	if (!spi_writebuf) {
		retval = -ENOMEM;
		goto alloc_fail;
		}

	par.spi = spi;
	par.spi_writebuf = spi_writebuf;
	par.dc = of_get_named_gpio(np, "dc-gpios", 0);
	par.rst = of_get_named_gpio(np, "reset-gpios", 0);
	/* Request GPIOs and initialize to default values */
	gpio_request_one(par.rst, GPIOF_OUT_INIT_HIGH,
		"ST7735 Reset Pin");
	gpio_request_one(par.dc, GPIOF_OUT_INIT_LOW,
		"ST7735 Data/Command Pin");


	retval = st7735fb_init_display();

	return 0;

alloc_fail:
	if (spi_writebuf)
		kfree(spi_writebuf);
	if (vmem)
		kfree(vmem);

	return retval;
};



static const struct spi_device_id st7735fb_ids[] = {
	{ "st7735fb", 0 },
	{ },
};

MODULE_DEVICE_TABLE(spi, st7735fb_ids);

static const struct of_device_id st7735fb_of_match_table[] = {
	{
		.compatible = "smart-clock,st7735fb",
	},
	{},
};
MODULE_DEVICE_TABLE(of, st7735fb_of_match_table);

int  st7735fb_remove(struct spi_device *spi)
{

	printk(KERN_ALERT "ST7735FB - deregistering\n");
	msleep(100);
	kfree(par.screen_base);
	kfree(par.spi_writebuf);
	gpio_free(par.dc);
	gpio_free(par.rst);
	return 0;
}


struct spi_driver st7735fb_driver = {
	.driver = {
		.name   = "st7735fb",
		.owner  = THIS_MODULE,
		.of_match_table = st7735fb_of_match_table,
	},
	.id_table = st7735fb_ids,
	.probe  = st7735fb_probe,
	.remove = st7735fb_remove,
};


int st7735fb_init(void)
{
	printk("ST7735FB - init\n");
	return spi_register_driver(&st7735fb_driver);
}

void st7735fb_exit(void)
{
	printk("ST7735FB - device exit\n");
	spi_unregister_driver(&st7735fb_driver);

}


