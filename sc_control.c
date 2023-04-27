// SPDX-License-Identifier: GPL-2.0-only
/*
 * to be filled
 *
 * Dmytro Volkov <splissken2014@gmail.com>
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/timer.h>  //timer

#include <linux/kthread.h>             //kernel threads
#include <linux/sched.h>               //task_struct
#include <linux/mutex.h>

#include <linux/ktime.h>
#include "include/project1.h"

#define MODULE_NAME module_name(THIS_MODULE)

#define DISP_TIME_CALLBACK 200
#define DISP_TIMER_CALLBACK_NS 17
#define DISP_OPTIONS_CALLBACK 200

#define SEC		(60)
#define SECS_PER_HOUR	(60 * 60)
#define SECS_PER_DAY	(SECS_PER_HOUR * 24)
#define ISLP(y) ((y % 400 == 0) || (y % 100 != 0) && (y % 4 == 0))


/************test mutex***/
struct mutex etx_mutex;
static struct task_struct *etx_thread1;
unsigned long etx_global_variable;
/***********thread**********/

static struct hrtimer digital_timer;
static struct timer_list digital_clock, options_view;
struct clock clock;
struct options options;
struct clock_timer our_timer = {0, 0, 0, 0, 0};

uint8_t clock_blink_bmask, options_blink_bmask;

/************test mutex***/
int thread_function1(void *pv)
{

    while (!kthread_should_stop()) {
	mutex_lock(&etx_mutex);
       // etx_global_variable++;
	pr_info("In EmbeTronicX Thread Function1 %lu\n", etx_global_variable);
	st7735fb_temperature_display();
	mutex_unlock(&etx_mutex);
	msleep(1000);
    }
    return 0;
}

/***********thread**********/

static int daysGetByMonth(uint8_t month, uint8_t year) 
{

uint8_t ret;

if (month == 2)
	ret = (ISLP(year) ? 29 : 28);
	else if (month % 2)
		ret = 31;
	else
		ret = 30;
	return ret;

}

void show_timer(void)
{
	uint32_t disp_timer;

	pr_err ("%s: Display mode %d\n", MODULE_NAME, my_button.mode);
	if  (our_timer.nsec)
		our_timer.nsec_current_ktime = ktime_get_real_ns()-our_timer.nsec_old_ktime+our_timer.nsec;
	else
	our_timer.nsec_current_ktime = ktime_get_real_ns();
	hrtimer_start(&digital_timer, ms_to_ktime(DISP_TIMER_CALLBACK_NS), HRTIMER_MODE_REL);

}

void show_clock(void)
{
	pr_err ("%s: Display mode %d\n", MODULE_NAME, my_button.mode);
	my_button.set_mode = 0;
	mod_timer(&digital_clock,
			jiffies + msecs_to_jiffies(DISP_TIME_CALLBACK));
}

void show_options (void)
{
	pr_err ("%s: Display mode %d\n", MODULE_NAME, my_button.mode);
	mod_timer(&options_view,
			jiffies + msecs_to_jiffies(DISP_OPTIONS_CALLBACK));
}

void show_temperature (void)
{
	pr_err ("%s: Display mode %d\n", MODULE_NAME, my_button.mode);
	st7735fb_temperature_display();
}



static enum hrtimer_restart digital_timer_callback(struct hrtimer *timer)

{
	if ((my_button.state == 2) && (!our_timer.timer_start)) {
		 	our_timer.nsec_stored = 0;
			our_timer.nsec = 0;
			my_button.state = 0;
				}

	if ((my_button.state == 1) && (our_timer.timer_start == 0))  {
		our_timer.timer_start =  !our_timer.timer_start;
		my_button.state =  !my_button.state;
		our_timer.nsec_old_ktime = ktime_get_real_ns();
		if (our_timer.nsec)
					 our_timer.nsec_stored = our_timer.nsec;

				} else if ((my_button.state == 1) && (our_timer.timer_start == 1))  {
					pr_err("Our timer stop\n");
					our_timer.timer_start =  !our_timer.timer_start;
					my_button.state =  !my_button.state;
				}

	if (our_timer.timer_start == 1) {


		our_timer.nsec = ktime_get_real_ns()-our_timer.nsec_old_ktime+our_timer.nsec_stored;
				}
	if (our_timer.timer_start == 0) {

		our_timer.nsec_current_ktime = ktime_get_real_ns();
				}


	if (my_button.mode == TIMER)

	{
		st7735fb_timer_display();
    		hrtimer_forward_now(timer, ms_to_ktime(DISP_TIMER_CALLBACK_NS));
		return HRTIMER_RESTART;
	} else {
		return HRTIMER_NORESTART;
		}
}


static void digital_clock_callback (struct timer_list *t)
{

	struct timespec64 curr_tm;
	struct tm tm_now, tm_stored;

	ktime_get_real_ts64(&curr_tm);

	if (my_button.state == 2 && !my_button.is_longpress && !my_button.set_mode) {
		my_button.set_mode = 1;
		my_button.state = 0;		
		//clock.current_sec=curr_tm.tv_sec;
		//clock.stored_date=clock.current_sec-tm_now.tm_hour*3600+tm_now.tm_min*60+tm_now.tm_sec;

	}
	if (my_button.state == 3 && my_button.set_mode) {
		my_button.set_mode++;
		my_button.state = 0;
		clock_blink_bmask = 0xFF;
		curr_tm.tv_sec = clock.current_sec;
		do_settimeofday64(&curr_tm);
		ds3231_writeRtcTime();

		if (my_button.set_mode == 6)
			my_button.set_mode = 0;


	}

	if (!my_button.set_mode) {
			clock.current_sec = curr_tm.tv_sec;
			clock.stored_sec = clock.current_sec;
			}

	time64_to_tm(clock.stored_sec, 0, &tm_stored);


	if (my_button.state == 1) {
		switch (my_button.set_mode) {
			/*adjust min */
			case 1:	clock.current_sec += SEC;
				time64_to_tm(clock.current_sec, 0, &tm_now);
				if (tm_now.tm_hour != tm_stored.tm_hour) 
						clock.current_sec -= SECS_PER_HOUR;
				break;
			/*adjust hours */
			case 2:	clock.current_sec += SECS_PER_HOUR;
				time64_to_tm(clock.current_sec, 0, &tm_now);
				if (tm_now.tm_mday != tm_stored.tm_mday) 
						clock.current_sec -= SECS_PER_DAY;
				break;
			/*adjust days */
			case 3:	clock.current_sec += SECS_PER_DAY;
				time64_to_tm(clock.current_sec, 0, &tm_now);
				if (tm_now.tm_mon != tm_stored.tm_mon)
						clock.current_sec -= daysGetByMonth(tm_stored.tm_mon+1, tm_stored.tm_year)*SECS_PER_DAY;
				break;
			/*adjust month */
			case 4:	clock.current_sec += daysGetByMonth(tm_now.tm_mon+1, tm_now.tm_year)*SECS_PER_DAY;
				time64_to_tm(clock.current_sec, 0, &tm_now);
				if (tm_now.tm_year != tm_stored.tm_year) 
						clock.current_sec -= (ISLP(tm_stored.tm_year) ? 366*SECS_PER_DAY : 365*SECS_PER_DAY); 
				break;
			/*adjust year (max 2050) */
			case 5:	clock.current_sec += (ISLP(tm_stored.tm_year) ? 366*SECS_PER_DAY : 365*SECS_PER_DAY);
				time64_to_tm(clock.current_sec, 0, &tm_now);
				pr_err("tm_now.tm_year %d\n", tm_now.tm_year);
				if (tm_now.tm_year % 100 > 50)
						clock.current_sec -= ((tm_now.tm_year % 100)-50) * (ISLP(tm_stored.tm_year) ? 366*SECS_PER_DAY : 365*SECS_PER_DAY);
				break;
			};

		my_button.state = 0;
		}

	time64_to_tm(clock.current_sec, 0, &tm_now);


	if (my_button.mode == CLOCK) { 
	st7735fb_clock_display(&tm_now, &clock_blink_bmask);
	mod_timer(&digital_clock,
			jiffies + msecs_to_jiffies(DISP_TIME_CALLBACK));
	}

}


static void options_callback (struct timer_list *t)
{

	if (my_button.state == 2 && !my_button.is_longpress && !my_button.set_mode) {
		my_button.set_mode = 1;
		my_button.state = 0;
	}

	if (my_button.state == 3 && my_button.set_mode) {
		my_button.set_mode++;
		my_button.state = 0;
		options_blink_bmask = 0xFF;
		ds3231_writeOptions();
		if (my_button.set_mode == 2)
			my_button.set_mode = 0;
	}

	if (my_button.state == 1) {
		switch (my_button.set_mode) {
			case 1: options.is_ampm =  !options.is_ampm;
				break;
						}
		my_button.state = 0;
		}

	if (my_button.mode == OPTIONS) { 
	st7735fb_options_display(&options_blink_bmask);
	mod_timer(&options_view,
			jiffies + msecs_to_jiffies(DISP_OPTIONS_CALLBACK));
	}
}

void init_controls(void)
{
		/*test mutex*/
		//mutex_init(&etx_mutex);
		//etx_thread1 = kthread_run(thread_function1,NULL,"eTx Thread1");

		/*setup display clock mode timer */
		timer_setup(&digital_clock, digital_clock_callback, 0);

		/*setup display HR timer for clock timer mode */
		hrtimer_init(&digital_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		digital_timer.function = &digital_timer_callback;

		/*setup display options mode timer */
		timer_setup(&options_view, options_callback, 0);
	}


void  deinit_controls(void)	
{
	printk("Controls -deinit\n");

	//kthread_stop(etx_thread1);

	del_timer_sync(&digital_clock);
	hrtimer_cancel(&digital_timer);
	del_timer_sync(&options_view);
	}


