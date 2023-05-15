// SPDX-License-Identifier: GPL-2.0-only
/*
 * to be filled
 *
 * Dmytro Volkov <splissken2014@gmail.com>
 *
 */



#include <linux/timer.h>  //timer

#include <linux/kthread.h>             //kernel threads
#include <linux/sched.h>               //task_struct
#include <linux/mutex.h>

#include <linux/ktime.h>
#include "include/project1.h"


/* use the same refresh timer value for all view modes to blink notification for timer on with same frequency */
#define DISP_TIME_REFRESH_TIME 200	//5 fps
#define DISP_TIMER_REFRESH_TIME_NS 17	//58 fps
#define DISP_ALARM_REFRESH_TIME 200     //5 fps
#define DISP_TEMP_AND_PRESS_REFRESH_TIME 200 //5 fps
#define DISP_PEDOMETER_REFRESH_TIME 200 //5 fps
#define DISP_GAME_REFRESH_TIME 40 //20 fps
#define DISP_OPTIONS_REFRESH_TIME 200	//5 fps

#define SEC		(60)
#define SECS_PER_HOUR	(60 * 60)
#define SECS_PER_DAY	(SECS_PER_HOUR * 24)
#define ISLP(y) (((y % 400 == 0) && (y % 100 == 0)) || (y % 4 == 0))
#define GDBM(m, y) ((m == 2) ? (ISLP(y) ? 29 : 28) : ((m % 2) ? 31 : 30))


static struct hrtimer digital_timer_view;
static struct timer_list digital_clock_view, options_view, alarm_view, temp_and_press_view, pedometer_view, game_view, alarm_check;
struct clock clock;
struct options options;
struct clock_timer our_timer;



/************test mutex***/
/*int alarm_check(void *pv)
{
	struct timespec64 curr_tm;
	struct tm tm_now;
	uint64_t time_sec;



    while (!kthread_should_stop()) {

	ktime_get_real_ts64(&curr_tm);
	time64_to_tm(curr_tm.tv_sec, 0, &tm_now);
	//mutex_lock(&etx_mutex);
       // etx_global_variable++;


	time_sec=tm_now.tm_hour*SECS_PER_HOUR + tm_now.tm_min*SEC;
	//pr_err ("%lld %d\n", time_sec, clock.alarm_sec);
	if ((my_button.mode != ALARM) && (time_sec==clock.alarm_sec))
		{	pr_info("Alarm working\n");
			st7735fb_temp_and_press_display();
		}
	//mutex_unlock(&etx_mutex);
	msleep(1000);
    }
    return 0;
}

/***********thread**********/


void show_timer_view(void)
{
	uint32_t disp_timer;

	pr_err("Display mode %d\n", my_button.mode);
	if  (our_timer.nsec)
		our_timer.nsec_current_ktime = ktime_get_real_ns()-our_timer.nsec_old_ktime+our_timer.nsec;
	else
		our_timer.nsec_current_ktime = ktime_get_real_ns();
	hrtimer_start(&digital_timer_view, ms_to_ktime(DISP_TIMER_REFRESH_TIME_NS), HRTIMER_MODE_REL);

}

void show_clock_view(void)
{
	pr_err("Display mode %d\n",  my_button.mode);
	my_button.set_mode = 0;
	mod_timer(&digital_clock_view,
			jiffies + msecs_to_jiffies(DISP_TIME_REFRESH_TIME));
}

void show_options_view(void)
{
	pr_err("Display mode %d\n",  my_button.mode);
	/* Display the first frame before timer */
	st7735fb_options_display();
	mod_timer(&options_view,
			jiffies + msecs_to_jiffies(DISP_OPTIONS_REFRESH_TIME));
}

void show_alarm_view(void)
{
	pr_err("Display mode %d\n",  my_button.mode);
	/* Display the first frame before timer */
	st7735fb_alarm_display(clock.alarm_sec/3600, (clock.alarm_sec%3600)/60);
	mod_timer(&alarm_view,
			jiffies + msecs_to_jiffies(DISP_ALARM_REFRESH_TIME));
}

void show_temp_and_press_view(void)
{
	pr_err("Display mode %d\n",  my_button.mode);
	/* Display the first frame before timer */
	st7735fb_temp_and_press_display();
	mod_timer(&temp_and_press_view,
			jiffies + msecs_to_jiffies(DISP_TEMP_AND_PRESS_REFRESH_TIME));
}

void show_pedometer_view(void)
{
	pr_err("Display mode %d\n",  my_button.mode);
	/* Display the first frame before timer */
	st7735fb_pedometer_display();
	mod_timer(&pedometer_view,
			jiffies + msecs_to_jiffies(DISP_PEDOMETER_REFRESH_TIME));
}

void show_game_view(void)
{
	pr_err("Display mode %d\n",  my_button.mode);
	/* Display the first frame before timer */
	game.len = 20;
	game.is_fruit = 0;
	game.x = 80;
	game.y = 80;
	game.game_over = 0;
	st7735fb_game_display();
	mod_timer(&game_view,
			jiffies + msecs_to_jiffies(DISP_GAME_REFRESH_TIME));
}




static enum hrtimer_restart digital_timer_view_callback(struct hrtimer *timer)

{
	/* when button longpress x1 detected erasing timer */
	if ((my_button.state == 2) && (!our_timer.timer_start)) {
		our_timer.nsec_stored = 0;
		our_timer.nsec = 0;
		my_button.state = 0;
				}
	/* when button press detected staring time ror pausing depending on current state */
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

	if (our_timer.timer_start)
		our_timer.nsec = ktime_get_real_ns()-our_timer.nsec_old_ktime+our_timer.nsec_stored;
	else
		our_timer.nsec_current_ktime = ktime_get_real_ns();

	if (my_button.mode == TIMER) {
		st7735fb_timer_display();
		hrtimer_forward_now(timer, ms_to_ktime(DISP_TIMER_REFRESH_TIME_NS));
		return HRTIMER_RESTART;
	} else {
		return HRTIMER_NORESTART;
		}
}


static void digital_clock_view_callback(struct timer_list *t)
{

	struct timespec64 curr_tm;
	struct tm tm_now, tm_stored;

	ktime_get_real_ts64(&curr_tm);

	/* when button longpress x1 detected turning on setup mode */
	if (my_button.state == 2 && !my_button.is_longpress && !my_button.set_mode) {
		my_button.set_mode = 1;
		my_button.state = 0;
		//clock.current_sec=curr_tm.tv_sec;
		//clock.stored_date=clock.current_sec-tm_now.tm_hour*3600+tm_now.tm_min*60+tm_now.tm_sec;

	}

	/* when button longpress x2 detected switching to next setup mode */
	if (my_button.state == 3 && my_button.set_mode) {
		my_button.set_mode++;
		my_button.state = 0;
		st7735fb_clear_blink_bmask(); //clear blinking bits
		curr_tm.tv_sec = clock.current_sec;
		do_settimeofday64(&curr_tm);
		ds3231_writeRtcTimeAndAlarm();

		if (my_button.set_mode == 6)
			my_button.set_mode = 0;

	}

	if (!my_button.set_mode) {
		clock.current_sec = curr_tm.tv_sec;
		clock.stored_sec = clock.current_sec;
	}

	time64_to_tm(clock.stored_sec, 0, &tm_stored);

	/* when button press detected switching values depending on current setup mode */
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
						clock.current_sec -= GDBM(tm_stored.tm_mon+1, tm_stored.tm_year)*SECS_PER_DAY;
				break;
			/*adjust month */
			case 4:	clock.current_sec += GDBM(tm_now.tm_mon+1, tm_now.tm_year)*SECS_PER_DAY;
				time64_to_tm(clock.current_sec, 0, &tm_now);
				if (tm_now.tm_year != tm_stored.tm_year)
						clock.current_sec -= (ISLP(tm_stored.tm_year) ? 366*SECS_PER_DAY : 365*SECS_PER_DAY);
				break;
			/*adjust year (max 2050) */
			case 5:	clock.current_sec += (ISLP(tm_stored.tm_year) ? 366*SECS_PER_DAY : 365*SECS_PER_DAY);
				time64_to_tm(clock.current_sec, 0, &tm_now);
				if (tm_now.tm_year % 100 > 50)
						clock.current_sec -= ((tm_now.tm_year % 100)-50) * (ISLP(tm_stored.tm_year) ? 366*SECS_PER_DAY : 365*SECS_PER_DAY);
				break;
			};

		my_button.state = 0;
		}

	time64_to_tm(clock.current_sec, 0, &tm_now);


	if (my_button.mode == CLOCK) {
		st7735fb_clock_display(&tm_now);
		mod_timer(&digital_clock_view,
			jiffies + msecs_to_jiffies(DISP_TIME_REFRESH_TIME));
	}

}

static void alarm_view_callback(struct timer_list *t)
{
	/* when button longpress x1 detected turning on setup mode */
	if (my_button.state == 2 && !my_button.is_longpress && !my_button.set_mode) {
		my_button.set_mode = 1;
		my_button.state = 0;
	}

	/* when button longpress x2 detected switching to next setup mode */
	if (my_button.state == 3 && my_button.set_mode) {
		my_button.set_mode++;
		my_button.state = 0;
		st7735fb_clear_blink_bmask(); //clear blinking bits
		ds3231_writeRtcTimeAndAlarm();
		if (my_button.set_mode == 3)
			my_button.set_mode = 0;
	}
	/* when button press detected switching values depending on current setup mode */
	if (my_button.state == 1) {
		switch (my_button.set_mode) {
			/*adjust min */
			case 1: clock.alarm_sec += SEC;
				if (clock.stored_alarm_sec/SECS_PER_HOUR != clock.alarm_sec/SECS_PER_HOUR)
					clock.alarm_sec -= SECS_PER_HOUR;
				break;
			/*adjust hours */
			case 2:	clock.alarm_sec += SECS_PER_HOUR;
				if (clock.stored_alarm_sec/SECS_PER_DAY != clock.alarm_sec/SECS_PER_DAY)
					clock.alarm_sec -= SECS_PER_DAY;
				break;
						}
		my_button.state = 0;
		}
	if (!my_button.set_mode) {
		clock.stored_alarm_sec = clock.alarm_sec;
		clock.stored_sec = clock.current_sec;
	}

	if (my_button.mode == ALARM) {
		st7735fb_alarm_display(clock.alarm_sec/3600, (clock.alarm_sec%3600)/60);
		mod_timer(&alarm_view,
			jiffies + msecs_to_jiffies(DISP_TIME_REFRESH_TIME));
	}
}

static void options_view_callback(struct timer_list *t)
{
	/* when button longpress x1 detected turning on setup mode */
	if (my_button.state == 2 && !my_button.is_longpress && !my_button.set_mode) {
		my_button.set_mode = 1;
		my_button.state = 0;
	}

	/* when button longpress x2 detected switching to next setup mode */
	if (my_button.state == 3 && my_button.set_mode) {
		my_button.set_mode++;
		my_button.state = 0;
		st7735fb_clear_blink_bmask();
		ds3231_writeOptions();
		if (my_button.set_mode == 4)
			my_button.set_mode = 0;
	}
	/* when button press detected switching values depending on current setup mode */
	if (my_button.state == 1) {
		switch (my_button.set_mode) {
			case 1: options.is_ampm = !options.is_ampm;
				break;
			case 2: options.is_alarm_enabled = !options.is_alarm_enabled;
				break;
			case 3: options.is_temp_celsius = !options.is_temp_celsius;
				break;
		}
		my_button.state = 0;
		}

	if (my_button.mode == OPTIONS) {
		st7735fb_options_display();
		mod_timer(&options_view,
			jiffies + msecs_to_jiffies(DISP_OPTIONS_REFRESH_TIME));
	}
}

static void temp_and_press_view_callback(struct timer_list *t)
{

	if (my_button.mode == TEMP_AND_PRESS) {
		st7735fb_temp_and_press_display();
		mod_timer(&temp_and_press_view,
			jiffies + msecs_to_jiffies(DISP_TEMP_AND_PRESS_REFRESH_TIME));
	}
}

static void pedometer_view_callback(struct timer_list *t)
{

	/* Reset pedometer when button is pressed */
	if (my_button.state == 1) {
		game.steps_count = 0;
		my_button.state = 0;
		}
	if (my_button.mode == PEDOMETER) {
		st7735fb_pedometer_display();
		mod_timer(&pedometer_view,
			jiffies + msecs_to_jiffies(DISP_PEDOMETER_REFRESH_TIME));
	}
}

static void game_view_callback(struct timer_list *t)
{

	if ((my_button.mode == GAME))  {
		if (!game.game_over)
				st7735fb_game_display();
		else if (my_button.state == 1)
				show_game_view();

		mod_timer(&game_view,
			jiffies + msecs_to_jiffies(DISP_GAME_REFRESH_TIME));
	} 

}


void init_controls(void)
{

		/*setup display clock mode refresh timer */
		timer_setup(&digital_clock_view, digital_clock_view_callback, 0);

		/*setup display HR timer for clock timer refresh mode */
		hrtimer_init(&digital_timer_view, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		digital_timer_view.function = &digital_timer_view_callback;

		/*setup alarm view mode refresh timer */
		timer_setup(&alarm_view, alarm_view_callback, 0);

		/*setup temperature and pressure view mode refresh timer */
		timer_setup(&temp_and_press_view, temp_and_press_view_callback, 0);

		/*setup temperature and pressure view mode refresh timer */
		timer_setup(&pedometer_view, pedometer_view_callback, 0);

		/*setup game view mode refresh timer */
		timer_setup(&game_view, game_view_callback, 0);

		/*setup display options mode refresh timer */
		timer_setup(&options_view, options_view_callback, 0);

	}


void  deinit_controls(void)
{
	del_timer_sync(&alarm_trigger);
	del_timer_sync(&game_view);
	del_timer_sync(&pedometer_view);
	del_timer_sync(&temp_and_press_view);
	del_timer_sync(&digital_clock_view);
	hrtimer_cancel(&digital_timer_view);
	del_timer_sync(&options_view);
	del_timer_sync(&alarm_view);
	msleep(100);
	pr_err("controls unloaded\n");
	}


