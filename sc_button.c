// SPDX-License-Identifier: GPL-2.0-only
/*
 * to be filled
 *
 * Dmytro Volkov <splissken2014@gmail.com>
 *
 */

#include <linux/module.h>
#include <linux/gpio.h>     //GPIO
#include <linux/export.h>  //export project class name
#include <linux/types.h>
#include <linux/timer.h>  //timer
#include <linux/interrupt.h> //irq trigger
#include "project1.h"

#define MODULE_NAME module_name(THIS_MODULE)
#define TRIGGER_IRQ "button_trigger"
#define DEVICE_NAME "gpio_button"

#define BUTTON_DEBOUNCE_INTERVAL 200
#define BUTTON_LONGPRESS_INTERVAL 500
#define BUTTON_2ND_LONGPRESS_INTERVAL 1000


static uint8_t button_irq;
static struct timer_list bDebounce_timer, bLongpress_timer;

static unsigned long flags = 0, counter=0, longpress_counter=0;
static uint8_t is_debounce_timer, is_longpress;
struct button my_button;



static void switch_view(void ) {
		//while 
	my_button.mode++;
	switch (my_button.mode) {
		case CLOCK: 		show_clock();					
					break;
		case TIMER:	
					show_timer();
					break;
		case TEMPERATURE:	show_temperature();
					break;
		case NO_DEV:		my_button.mode=0;
					switch_view();
					break;
		
	}
	

}


void button_workqueue_fn(struct work_struct *work)
{              
	if (!is_longpress) mod_timer(&bDebounce_timer,
			jiffies + msecs_to_jiffies(BUTTON_DEBOUNCE_INTERVAL));
	mod_timer(&bLongpress_timer,
			jiffies + msecs_to_jiffies(BUTTON_LONGPRESS_INTERVAL));
		
	longpress_counter=counter;		
		}
 

DECLARE_WORK(button_workqueue,button_workqueue_fn);



static void button_longpress_timer(struct timer_list *t)
{
	if  ((longpress_counter==counter) && gpio_get_value(button_gpio.gpio)) {
	printk("button_isr long press!!!! %lu\n", counter);
	if (is_longpress) {
		my_button.state=3;
		switch_view();
		}
		else my_button.state=2;
       
 	is_longpress++;
	mod_timer(&bLongpress_timer,
			jiffies + msecs_to_jiffies(BUTTON_2ND_LONGPRESS_INTERVAL));
		} else  is_longpress=0;

          
}

static void button_debounce_timer (struct timer_list *t)
{
	
	is_debounce_timer=0;
		if (!gpio_get_value(button_gpio.gpio)&& (!is_longpress)) 
			{ my_button.state=1;
			printk("button_isr short press!!!! %lu\n", counter);
			} else my_button.state=0;
	
}

static irqreturn_t button_press(int irq, void *data)
{
	local_irq_save(flags);
	
	counter ++;
	if (!is_debounce_timer && !is_longpress)  {
		schedule_work(&button_workqueue);
		is_debounce_timer=1;
				} 
	local_irq_restore(flags);

	return IRQ_HANDLED;
}



int  gpio_button_init(void)
{
	struct device_node *np;
	/*read GPIo value from dt */	
	np = of_find_compatible_node(NULL, NULL, "smart-clock,button");
	if (np) {
		button_gpio.gpio = be32_to_cpup(of_get_property(np, "button_gpio", NULL));
		pr_err("%s: Found Gpio %i value in DT", DEVICE_NAME, button_gpio.gpio);
		}

	if (!gpio_is_valid(button_gpio.gpio)) {
			pr_err("%s: Gpio %d is not valid", DEVICE_NAME, button_gpio.gpio);
			return -1;
		}
	/* GPIO's request*/
	if (gpio_request_one(button_gpio.gpio,button_gpio.flags, button_gpio.label  )) {
		pr_err("%s: Cannot request GPIO\n", DEVICE_NAME);
		return -1;
	}

		gpio_export(button_gpio.gpio, true);
		button_irq = gpio_to_irq(button_gpio.gpio);
			if (button_irq < 0) {
				pr_err("Unable to get irq number for GPIO %d, error %d\n",
					button_gpio.gpio, button_irq);
					gpio_free(button_gpio.gpio);
					return -1;
			}
			
		
		if( request_irq( button_irq, button_press ,IRQF_TRIGGER_RISING, TRIGGER_IRQ, DEVICE_NAME)) {
			pr_err("Register IRQ %d, error\n",
					button_gpio.gpio);
					free_irq(button_irq, DEVICE_NAME);
					gpio_free(button_gpio.gpio);
					return -1;
		}


		timer_setup(&bDebounce_timer, button_debounce_timer, 0);
		timer_setup(&bLongpress_timer, button_longpress_timer, 0);
		is_debounce_timer=0;
		is_longpress=0;
		my_button.state=0;
		//our_timer.reset=1;
		
return 0;

}


void  gpio_button_deinit(void)	{
	printk("Gpio Button - device exit\n");
	del_timer_sync(&bDebounce_timer);
	del_timer_sync(&bLongpress_timer);
	free_irq(button_irq, DEVICE_NAME);
	gpio_free(button_gpio.gpio);
	}


