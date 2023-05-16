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
#define DISP_TIME_REFRESH_TIME 200	//ms =5 fps
#define DISP_HR_TIMER_REFRESH_TIME 17	//ms =58 fps
#define DISP_ALARM_REFRESH_TIME 200     //ms 5 fps
#define DISP_TEMP_AND_PRESS_REFRESH_TIME 200 //5 fps
#define DISP_PEDOMETER_REFRESH_TIME 200 //5 fps
#define DISP_GAME_REFRESH_TIME 40 //20 fps
#define DISP_OPTIONS_REFRESH_TIME 200	//5 fps
#define ALARM_TRIGGER_REFRESH_TIME 200

#define SEC		(60)
#define SECS_PER_HOUR	(60 * 60)
#define SECS_PER_DAY	(SECS_PER_HOUR * 24)
#define ISLP(y) (((y % 400 == 0) && (y % 100 == 0)) || (y % 4 == 0))
#define GDBM(m, y) ((m == 2) ? (ISLP(y) ? 29 : 28) : ((m % 2) ? 31 : 30))


static struct hrtimer digital_timer_view;
static struct timer_list digital_clock_view, options_view, alarm_view, temp_and_press_view, pedometer_view, game_view, alarm_trigger;


/**
 * show_timer_view() - timer view
 *
 * switches view to timer mode and runs HR timer
 */
void show_timer_view(void)
{
	hrtimer_start(&digital_timer_view, ms_to_ktime(DISP_HR_TIMER_REFRESH_TIME), HRTIMER_MODE_REL);
}

void show_clock_view(void)
{
	my_button.edit_mode = 0;
	mod_timer(&digital_clock_view,
			jiffies + msecs_to_jiffies(DISP_TIME_REFRESH_TIME));
}

void show_options_view(void)
{
	/* Display the first frame before timer */
	st7735fb_options_display();
	mod_timer(&options_view,
			jiffies + msecs_to_jiffies(DISP_OPTIONS_REFRESH_TIME));
}

void show_alarm_view(void)
{
	/* Display the first frame before timer */
	st7735fb_alarm_display(clock_and_alarm.alarm_sec/3600, (clock_and_alarm.alarm_sec%3600)/60);
	mod_timer(&alarm_view,
			jiffies + msecs_to_jiffies(DISP_ALARM_REFRESH_TIME));
}

void show_temp_and_press_view(void)
{
	/* Display the first frame before timer */
	st7735fb_temp_and_press_display();
	mod_timer(&temp_and_press_view,
			jiffies + msecs_to_jiffies(DISP_TEMP_AND_PRESS_REFRESH_TIME));
}

void show_pedometer_view(void)
{
	/* Display the first frame before timer */
	st7735fb_pedometer_display();
	mod_timer(&pedometer_view,
			jiffies + msecs_to_jiffies(DISP_PEDOMETER_REFRESH_TIME));
}

void show_game_view(void)
{
	/* Prepare snake to run */
	game.len = 20;
	game.is_fruit = 0;
	game.x = 80;
	game.y = 80;
	game.game_over = 0;
	pr_err("Sizeof %d\n", sizeof(game.tail_x));
	/* Clear previuos snake tails */
	memset(game.tail_x, 0, sizeof(game.tail_x));
	memset(game.tail_y, 0, sizeof(game.tail_y));

	/* Display the first frame before timer */
	st7735fb_game_display();
	mod_timer(&game_view,
			jiffies + msecs_to_jiffies(DISP_GAME_REFRESH_TIME));
}


static enum hrtimer_restart digital_timer_view_callback(struct hrtimer *timer)

{
	/* when button longpress x1 detected erasing timer */
	if ((my_button.state == 2) && (!our_timer.is_timer_on)) {
		our_timer.nsec_stored = 0;
		our_timer.nsec = 0;
		my_button.state = 0;
				}
	/* when button press detected staring time ror pausing depending on current state */
	if ((my_button.state == 1) && (our_timer.is_timer_on == 0))  {
		our_timer.is_timer_on =  !our_timer.is_timer_on;
		my_button.state =  !my_button.state;
		our_timer.nsec_old_ktime = ktime_get_real_ns();
		if (our_timer.nsec)
			our_timer.nsec_stored = our_timer.nsec;

	} else if ((my_button.state == 1) && (our_timer.is_timer_on == 1))  {
		pr_err("Our timer stop\n");
		our_timer.is_timer_on =  !our_timer.is_timer_on;
		my_button.state =  !my_button.state;
				}

	if (our_timer.is_timer_on)
		our_timer.nsec = ktime_get_real_ns()-our_timer.nsec_old_ktime+our_timer.nsec_stored;

	if (my_button.view_mode == TIMER) {
		st7735fb_timer_display();
		hrtimer_forward_now(timer, ms_to_ktime(DISP_HR_TIMER_REFRESH_TIME));
		return HRTIMER_RESTART;
	} else {
		return HRTIMER_NORESTART;
		}
}

static void alarm_trigger_callback(struct timer_list *t)
{
	struct timespec64 curr_tm;
	struct tm tm_now;
	uint64_t time_sec;
	uint16_t timer_delay = 0;

	ktime_get_real_ts64(&curr_tm);
	time64_to_tm(curr_tm.tv_sec, 0, &tm_now);

	time_sec = tm_now.tm_hour*SECS_PER_HOUR + tm_now.tm_min*SEC;


	if ((tm_now.tm_hour == clock_and_alarm.alarm_sec/3600) &&
		(tm_now.tm_min == (clock_and_alarm.alarm_sec%3600)/60) && (!clock_and_alarm.is_alarm))
		clock_and_alarm.is_alarm = 1;

	/* Postprone next trigeer to 65 sec when alarm stopped to avoid notifications loop*/
	if ((clock_and_alarm.is_alarm) && (my_button.state)) {
		clock_and_alarm.is_alarm = 0;
		timer_delay = 0xFFFF;
	}
	mod_timer(&alarm_trigger,
			jiffies + msecs_to_jiffies(ALARM_TRIGGER_REFRESH_TIME+timer_delay));
}



static void digital_clock_view_callback(struct timer_list *t)
{

	struct timespec64 curr_tm;
	struct tm tm_now, tm_stored;

	ktime_get_real_ts64(&curr_tm);

	/* when button longpress x1 detected turning on setup mode */
	if (my_button.state == 2 && !my_button.is_longpress && !my_button.edit_mode) {
		my_button.edit_mode = 1;
		my_button.state = 0;

	}

	/* when button longpress x2 detected switching to next setup mode */
	if (my_button.state == 3 && my_button.edit_mode) {
		my_button.edit_mode++;
		my_button.state = 0;
		st7735fb_clear_blink_bmask(); //clear blinking bits
		curr_tm.tv_sec = clock_and_alarm.clock_sec;
		do_settimeofday64(&curr_tm);
		ds3231_writeRtcTimeAndAlarm();

		if (my_button.edit_mode == 6)
			my_button.edit_mode = 0;

	}

	if (!my_button.edit_mode) {
		clock_and_alarm.clock_sec = curr_tm.tv_sec;
		clock_and_alarm.clock_stored_sec = clock_and_alarm.clock_sec;
	}

	time64_to_tm(clock_and_alarm.clock_stored_sec, 0, &tm_stored);

	/* when button press detected switching values depending on current setup mode */
	if (my_button.state == 1) {
		switch (my_button.edit_mode) {
		/*adjust min */
		case 1:
			clock_and_alarm.clock_sec += SEC;
			time64_to_tm(clock_and_alarm.clock_sec, 0, &tm_now);
			if (tm_now.tm_hour != tm_stored.tm_hour)
				clock_and_alarm.clock_sec -= SECS_PER_HOUR;
			break;
		/*adjust hours */
		case 2:
			clock_and_alarm.clock_sec += SECS_PER_HOUR;
			time64_to_tm(clock_and_alarm.clock_sec, 0, &tm_now);
			if (tm_now.tm_mday != tm_stored.tm_mday)
				clock_and_alarm.clock_sec -= SECS_PER_DAY;
			break;
		/*adjust days */
		case 3:
			clock_and_alarm.clock_sec += SECS_PER_DAY;
			time64_to_tm(clock_and_alarm.clock_sec, 0, &tm_now);
			if (tm_now.tm_mon != tm_stored.tm_mon)
				clock_and_alarm.clock_sec -= GDBM(tm_stored.tm_mon+1, tm_stored.tm_year)*SECS_PER_DAY;
			break;
		/*adjust month */
		case 4:
			clock_and_alarm.clock_sec += GDBM(tm_now.tm_mon+1, tm_now.tm_year)*SECS_PER_DAY;
			time64_to_tm(clock_and_alarm.clock_sec, 0, &tm_now);
			if (tm_now.tm_year != tm_stored.tm_year)
				clock_and_alarm.clock_sec -= (ISLP(tm_stored.tm_year) ? 366*SECS_PER_DAY : 365*SECS_PER_DAY);
			break;
		/*adjust year (max 2050) */
		case 5:
			clock_and_alarm.clock_sec += (ISLP(tm_stored.tm_year) ? 366*SECS_PER_DAY : 365*SECS_PER_DAY);
			time64_to_tm(clock_and_alarm.clock_sec, 0, &tm_now);
			if (tm_now.tm_year % 100 > 50)
				clock_and_alarm.clock_sec -= ((tm_now.tm_year % 100)-50) * (ISLP(tm_stored.tm_year) ? 366*SECS_PER_DAY : 365*SECS_PER_DAY);
			break;
			};

		my_button.state = 0;
		}

	time64_to_tm(clock_and_alarm.clock_sec, 0, &tm_now);


	if (my_button.view_mode == CLOCK) {
		st7735fb_clock_display(&tm_now);
		mod_timer(&digital_clock_view,
			jiffies + msecs_to_jiffies(DISP_TIME_REFRESH_TIME));
	}

}

static void alarm_view_callback(struct timer_list *t)
{
	/* when button longpress x1 detected turning on setup mode */
	if (my_button.state == 2 && !my_button.is_longpress && !my_button.edit_mode) {
		my_button.edit_mode = 1;
		my_button.state = 0;
	}

	/* when button longpress x2 detected switching to next setup mode */
	if (my_button.state == 3 && my_button.edit_mode) {
		my_button.edit_mode++;
		my_button.state = 0;
		st7735fb_clear_blink_bmask(); //clear blinking bits
		ds3231_writeRtcTimeAndAlarm();
		if (my_button.edit_mode == 3)
			my_button.edit_mode = 0;
	}
	/* when button press detected switching values depending on current setup mode */
	if (my_button.state == 1) {
		switch (my_button.edit_mode) {
		/*adjust min */
		case 1:
			clock_and_alarm.alarm_sec += SEC;
			if (clock_and_alarm.alarm_stored_sec/SECS_PER_HOUR != clock_and_alarm.alarm_sec/SECS_PER_HOUR)
				clock_and_alarm.alarm_sec -= SECS_PER_HOUR;
			break;
		/*adjust hours */
		case 2:
			clock_and_alarm.alarm_sec += SECS_PER_HOUR;
			if (clock_and_alarm.alarm_stored_sec/SECS_PER_DAY != clock_and_alarm.alarm_sec/SECS_PER_DAY)
				clock_and_alarm.alarm_sec -= SECS_PER_DAY;
			break;
						}
		my_button.state = 0;
		}
	if (!my_button.edit_mode)
		clock_and_alarm.alarm_stored_sec = clock_and_alarm.alarm_sec;


	if (my_button.view_mode == ALARM) {
		st7735fb_alarm_display(clock_and_alarm.alarm_sec/3600, (clock_and_alarm.alarm_sec%3600)/60);
		mod_timer(&alarm_view,
			jiffies + msecs_to_jiffies(DISP_TIME_REFRESH_TIME));
	}
}

static void options_view_callback(struct timer_list *t)
{
	/* when button longpress x1 detected turning on setup mode */
	if (my_button.state == 2 && !my_button.is_longpress && !my_button.edit_mode) {
		my_button.edit_mode = 1;
		my_button.state = 0;
	}

	/* when button longpress x2 detected switching to next setup mode */
	if (my_button.state == 3 && my_button.edit_mode) {
		my_button.edit_mode++;
		my_button.state = 0;
		st7735fb_clear_blink_bmask();
		ds3231_writeOptions();
		if (my_button.edit_mode == 4)
			my_button.edit_mode = 0;
	}
	/* when button press detected switching values depending on current setup mode */
	if (my_button.state == 1) {
		switch (my_button.edit_mode) {
		case 1:
			options.is_ampm = !options.is_ampm;
			break;
		case 2:
			options.is_alarm_enabled = !options.is_alarm_enabled;
			break;
		case 3:
			options.is_temp_celsius = !options.is_temp_celsius;
			break;
		}
		my_button.state = 0;
		}

	if (my_button.view_mode == OPTIONS) {
		st7735fb_options_display();
		mod_timer(&options_view,
			jiffies + msecs_to_jiffies(DISP_OPTIONS_REFRESH_TIME));
	}
}

static void temp_and_press_view_callback(struct timer_list *t)
{

	if (my_button.view_mode == TEMP_AND_PRESS) {
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
	if (my_button.view_mode == PEDOMETER) {
		st7735fb_pedometer_display();
		mod_timer(&pedometer_view,
			jiffies + msecs_to_jiffies(DISP_PEDOMETER_REFRESH_TIME));
	}
}

static void game_view_callback(struct timer_list *t)
{

	if (my_button.view_mode == GAME) {
		if (!game.game_over)
			st7735fb_game_display();
		else if (my_button.state == 1)
			show_game_view();

		mod_timer(&game_view,
			jiffies + msecs_to_jiffies(DISP_GAME_REFRESH_TIME));
	}


void init_controls(void)
{

	/*setup view clock mode refresh timer */
	timer_setup(&digital_clock_view, digital_clock_view_callback, 0);

	/*setup view HR timer for clock timer refresh mode */
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

	/*setup view options mode refresh timer */
	timer_setup(&options_view, options_view_callback, 0);

	/*setup and run alarm clock trigger timer */
	timer_setup(&alarm_trigger, alarm_trigger_callback, 0);
	mod_timer(&alarm_trigger,
			jiffies + msecs_to_jiffies(ALARM_TRIGGER_REFRESH_TIME));

}


void  controls_unload(void)
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


