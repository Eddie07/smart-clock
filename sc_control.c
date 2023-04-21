// SPDX-License-Identifier: GPL-2.0-only
/*
 * to be filled
 *
 * Dmytro Volkov <splissken2014@gmail.com>
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/timer.h>  //timer
#include <linux/ktime.h>
#include "project1.h"

#define MODULE_NAME module_name(THIS_MODULE)

#define SMART_CLOCK_TIME_CALLBACK 100 
#define SMART_CLOCK_TIMER_CALLBACK_NS 17 



static struct hrtimer digital_timer;
static struct timer_list digital_clock;

struct clock_timer our_timer= {0,0,0,0,0};

  
void show_timer(void){
	uint32_t disp_timer;
	pr_err ("Display mode %d\n", my_button.mode);
	if  (our_timer.nsec) our_timer.nsec_current_ktime=ktime_get_real_ns()-our_timer.nsec_old_ktime+our_timer.nsec;
	else
	our_timer.nsec_current_ktime=ktime_get_real_ns();
	hrtimer_start( &digital_timer, ms_to_ktime(SMART_CLOCK_TIMER_CALLBACK_NS), HRTIMER_MODE_REL);

}

void show_clock(void){
	pr_err ("Display mode %d\n", my_button.mode);
	mod_timer(&digital_clock,
			jiffies + msecs_to_jiffies(SMART_CLOCK_TIME_CALLBACK));
}

void show_temperature (void){
	pr_err ("Display mode %d\n", my_button.mode);
	st7735fb_temperature_display(); 
}



static enum hrtimer_restart digital_timer_callback(struct hrtimer *timer)

{	
	if ((my_button.state==2)&&(!our_timer.timer_start)) {
		 	our_timer.nsec_stored=0;
			our_timer.nsec=0;
			my_button.state=0;
				}

	if ((my_button.state==1) && (our_timer.timer_start==0))  {
		our_timer.timer_start=!our_timer.timer_start;
		my_button.state=!my_button.state;
		our_timer.nsec_old_ktime=ktime_get_real_ns();
		if (our_timer.nsec)
					 our_timer.nsec_stored = our_timer.nsec;				
			
				} else if ((my_button.state==1) && (our_timer.timer_start==1))  {
					pr_err ("Our timer stop \n");
					our_timer.timer_start=!our_timer.timer_start;
					my_button.state=!my_button.state;
				} 

	if (our_timer.timer_start==1) {
		
		
		our_timer.nsec=ktime_get_real_ns()-our_timer.nsec_old_ktime+our_timer.nsec_stored;
				}
	if (our_timer.timer_start==0) {
		
		our_timer.nsec_current_ktime=ktime_get_real_ns();
				}
	
    	st7735fb_timer_display(); 
		
	if (my_button.mode==TIMER)

	{
    		hrtimer_forward_now(timer,ms_to_ktime(SMART_CLOCK_TIMER_CALLBACK_NS));
    		return HRTIMER_RESTART;
	} else {
	 	return HRTIMER_NORESTART;
		}
}


static void digital_clock_callback (struct timer_list *t) {
    	st7735fb_clock_display(); 
	if (my_button.mode==CLOCK) 
        mod_timer(&digital_clock,
			jiffies + msecs_to_jiffies(SMART_CLOCK_TIME_CALLBACK));
	
}


void init_controls(void) {

		timer_setup(&digital_clock, digital_clock_callback, 0);
    		hrtimer_init(&digital_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);	
    		digital_timer.function = &digital_timer_callback;
	}


void  deinit_controls(void)	{
	printk("Controls -deinit\n");
 	del_timer_sync(&digital_clock);
	hrtimer_cancel(&digital_timer);
	}


