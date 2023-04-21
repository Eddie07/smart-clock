

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

#include "project1.h"
#include "panel.h"


//struct st7735fb_par par;
//struct bmp280 bmp280;

const char* days[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};


static void st7735fb_draw_string(char *, uint16_t, uint16_t, const struct BitmapFont *, uint8_t , const uint16_t color ); 

 

static struct timer_list digital_clock;//, digital_timer;
 
static void display_update_work(struct work_struct *);
  

/*Creating work by Static Method */
DECLARE_WORK(update_diplay,display_update_work);


static int st7735_write_data(uint8_t *txbuf, size_t size)
{
	
	/* Set data mode */
	gpio_set_value(par.dc, 1);

	/* Write entire buffer */
	return spi_write(par.spi, txbuf, size);
}

static void st7735_write_cmd( uint8_t data)
{
	int ret = 0;


	/* Set command mode */
	gpio_set_value(par.dc, 0);

	ret = spi_write(par.spi, &data, 1);
	if (ret < 0)
		pr_err("write data failed \n");
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
			st7735_write_data(&st7735_cfg[i].data,1);
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
	for (i=0; i<HEIGHT; i++) {
		int x;
#ifdef __LITTLE_ENDIAN
		for (x=0; x<WIDTH; x++)
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
		pr_err("write data failed \n");

}


static void st7735fb_update_display (void) {

	schedule_work(&update_diplay);
}


void st7735fb_blank_display(void)
{

	memset(par.screen_base,  0, par.vmem_size);
	st7735fb_update_display();

}

void st7735fb_rotate_display(void)
{	
	
	st7735fb_update_display();

}

static int st7735fb_init_display(void)
{
	st7735_reset();
	st7735_run_cfg();		
	st7735fb_update_display();
	return 0;
}


 
void st7735fb_clock_display(void) 

{

	struct timespec64 curr_tm;
	struct tm tm_now;
        char time[20];
	int i,x,y,step;

	

	//ktime_get_ts64(&curr_tm);
	ktime_get_real_ts64(&curr_tm);
	time64_to_tm(curr_tm.tv_sec, 0, &tm_now);
	pr_err("%llu \n", curr_tm.tv_sec);




	
	//pr_err("%s \n", time);
	/* clear screen */
	memset(par.screen_base,0, par.vmem_size);
	
	/* draw tm_mday, tm_mon, tm_year */ 

	sprintf (time, "%s %02d/%02d/%02ld", days[tm_now.tm_wday], tm_now.tm_mday, tm_now.tm_mon+1, tm_now.tm_year % 100);
	st7735fb_draw_string(time, 0, 15, &font[0], 12, BLUE_COLOR );

	/* draw time*/
	i=0;step=0;x=0;
	sprintf (time, "%02d:%02d:%02d", tm_now.tm_hour,tm_now.tm_min,tm_now.tm_sec);
	st7735fb_draw_string(time, 0, 40, &font[1], 22, BLUE_COLOR );

	


	st7735fb_update_display();


}

//static void digital_timer_timer (struct timer_list *t) {

void st7735fb_timer_display(void) 

{

	uint64_t timer_value_start, timer_value_now;

	
        char timer[20]={0};

	int i=0,x=0,y=20,step=0;


	sprintf (timer, "%lld", NS_TO_MSEC(our_timer.nsec));

	/* clear screen */
	memset(par.screen_base,0, par.vmem_size);
	
	st7735fb_draw_string(timer, 0, 20, &font[1], 20, BLUE_COLOR );
	
	st7735fb_update_display();


}

void st7735fb_temperature_display(void) 

{
	



	
        char temperature[20]={0};

	int i=0,x=0,y=20,step=0;


	memset(par.screen_base,0, par.vmem_size);
	sprintf (temperature, "%d", bmp280.temperature);

	st7735fb_draw_string(temperature, 0, 20, &font[1], 20, BLUE_COLOR ); 
	
	st7735fb_update_display();


}



static void draw_char(char letter, uint16_t x, uint16_t y, const struct BitmapFont *font, uint16_t color )
{
	uint16_t *vmem16 = (uint16_t *)par.screen_base;
	uint8_t xc,yc;	
           
 
	uint16_t char_offset = (letter - ' ') * font->height * 
         (font->width / 8 + (font->width % 8 ? 1 : 0));

	uint8_t *ptr = &font->table[char_offset];	
    	for (yc = 0; yc < font->height; yc++)
      	{
     		for (xc = 0; xc < font->width; xc++)
       		{
       			 if (*ptr & (0x80 >>(xc % 8)))
        			vmem16[(y+yc)*WIDTH+(x+xc)]=color;
        		 if (xc % 8 == 7) ptr++;
        	}
      		if (font->width % 8 != 0) ptr++;
      	}

}

static void st7735fb_draw_string(char *word, uint16_t x, uint16_t y, const struct BitmapFont *font, uint8_t x_offset, uint16_t color ) 

{
	uint8_t i=0;
	while (word[i]!=0) {
		draw_char(word[i], x,y ,font,color);
		if (word[i+1]==':' || word[i]==':') x+=x_offset-6;
			else x+=x_offset;
		i++;
		}
}



static int st7735fb_probe (struct spi_device *spi)
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
	} else dev_err(&spi->dev, "SPI ok \n");

	par.vmem_size = WIDTH*HEIGHT*BPP/8;

	//vmem = vzalloc(par.vmem_size);
	vmem =kzalloc(par.vmem_size, GFP_KERNEL);
	if (!vmem) {
		retval = -ENOMEM;
		goto alloc_fail;
		}
	par.screen_base = (uint8_t __force __iomem *)vmem;
	pr_err("Total vm initialized %d\n",par.vmem_size );

	/* Allocate spi write buffer */
	//spi_writebuf = vzalloc(par.vmem_size);
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
	//timer_setup(&digital_clock, digital_clock_callback, 0);
	//timer_setup(&digital_timer, digital_timer_timer, 0);
   

	
	
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
	msleep (100);
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


