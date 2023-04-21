// SPDX-License-Identifier: GPL-2.0-only
/*
 * to be filled
 *
 * Dmytro Volkov <splissken2014@gmail.com>
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h> // cdev_init
#include <linux/uaccess.h>  //copy_from_user
#include <linux/gpio.h>     //GPIO
#include <linux/export.h>  //export project class name
#include <linux/types.h>
#include <linux/timer.h>  //timer
#include <linux/interrupt.h> //irq trigger
#include <linux/device.h>
#include "project1.h"

#define MODULE_NAME module_name(THIS_MODULE)
#define DEVICE_NAME "gpio_button"
#define CLASS_NAME  "project1"
#define TRIGGER_IRQ "button trigger irq"
#define DEBOUNCE_INTERVAL 200
#define LONGPRESS_INTERVAL 2000

static dev_t dev;
static int is_open;

static struct cdev gpio_button_cdev;
static struct class *dev_class;
static uint8_t led_num, button_irq;
static struct timer_list bDebounce_timer, bLongpress_timer;
unsigned char fs_buffer[BUFFER_MAX_SIZE] = {0};
size_t fs_buffer_size;
static unsigned long flags = 0, counter=0, longpress_counter=0;
static uint8_t is_debounce_timer, is_longpress;


/* exporting device class for use with another project modules */
EXPORT_SYMBOL_GPL(dev_class);

static void button_longpress_timer(struct timer_list *t)
{
	if ((!is_debounce_timer) && (longpress_counter==counter) && gpio_get_value(button_gpios[0].gpio)) {
	printk("button_isr long press!!!! %d\n", counter); 
	is_longpress=1; 
	}

          
}

static void button_debounce_timer (struct timer_list *t)
{
	is_debounce_timer=0;
	if (!gpio_get_value(button_gpios[0].gpio)) printk("button_isr !!!! %d\n", counter);  

          
}

static irqreturn_t button_press(int irq, void *data)
{
	local_irq_save(flags);
        
	

	if (!is_debounce_timer)  {
		counter ++;
		mod_timer(&bDebounce_timer,
			jiffies + msecs_to_jiffies(DEBOUNCE_INTERVAL));
		mod_timer(&bLongpress_timer,
			jiffies + msecs_to_jiffies(LONGPRESS_INTERVAL));
		is_debounce_timer=1;
		is_longpress=0;
		longpress_counter=counter;		
		}
	local_irq_restore(flags);
	return IRQ_HANDLED;
}

static int my_dev_event(struct device *dev, struct kobj_uevent_env *env)
{
	add_uevent_var(env, "DEVMODE=%#o", 0666);
	return 0;
}


static struct file_operations gpio_button_fops;

 /* Driver init */

static int  __init gpio_button_driver_main(void)
{

	  /* Allocating chardev */
	if ((alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME)) < 0) {
		pr_err("%s: Cannot allocate major number\n", DEVICE_NAME);
	goto dev_unreg;
	}

	pr_err("%s: Major = %d Minor = %d\n", DEVICE_NAME, MAJOR(dev), MINOR(dev));

	/*Creating cdev structure*/
	cdev_init(&gpio_button_cdev, &gpio_button_fops);

	/*Adding chardev to the system*/

	if ((cdev_add(&gpio_button_cdev, dev, 1)) < 0) {
		pr_err("%s: Cannot add the device to the system\n", DEVICE_NAME);
	goto dev_del;
	}

	/*Creating struct class*/

	dev_class = class_create(THIS_MODULE, CLASS_NAME);
	if (dev_class == NULL) {
		pr_err("%s: Cannot create the struct class\n", DEVICE_NAME);
		goto class_del;
	}
	/* Device node access permissions*/
	dev_class->dev_uevent = my_dev_event;

	/*Creating device*/
	if (device_create(dev_class, NULL, dev, NULL, DEVICE_NAME) == NULL) {
		pr_err("%s: Cannot create the Device\n", DEVICE_NAME);
		goto dev_remove;
	}

	/* Getting data from dt and register GPIO */


*/
	if (!gpio_is_valid(button_gpios[0].gpio)) {
			pr_err("%s: Gpio %i is not valid", DEVICE_NAME, button_gpios[0].gpio);
			goto dev_remove;
		}
	/* Multiple GPIO's request*/
	if (gpio_request_array(button_gpios, 1)) {
		pr_err("%s: Cannot request GPIOs\n", DEVICE_NAME);
		goto remove_gpio;
	}

	/* UnHiding GPIO's controls from gpio sysfs */
	//for (gpio_num = 0; led_num < 1 ; gpio_num++)
		gpio_export(button_gpios[0].gpio, true);
		button_irq = gpio_to_irq(button_gpios[0].gpio);
			if (button_irq < 0) {
				pr_err("Unable to get irq number for GPIO %d, error %d\n",
					button_gpios[0].gpio, button_irq);
				goto remove_gpio;
			}
			
		
		if( request_irq( button_irq, button_press ,IRQF_TRIGGER_RISING, TRIGGER_IRQ, DEVICE_NAME)) {
			pr_err("Register IRQ %d, error\n",
					button_gpios[0].gpio);
				goto free_irq;
		}

		timer_setup(&bDebounce_timer, button_debounce_timer, 0);
		timer_setup(&bLongpress_timer, button_longpress_timer, 0);
		is_debounce_timer=0;
		is_longpress=0;


				
				
	

return 0;

free_irq:
	free_irq(button_irq, DEVICE_NAME);
remove_gpio:
	gpio_free_array(button_gpios, 1);
dev_remove:
	device_destroy(dev_class, dev);
class_del:
	class_destroy(dev_class);
dev_del:
	cdev_del(&gpio_button_cdev);
dev_unreg:
	unregister_chrdev_region(dev, 1);

return -1;

}

/* Driver exit */

static void  __exit gpio_button_driver_exit(void)
{
	free_irq(button_irq, DEVICE_NAME);
	gpio_free_array(button_gpios,1 );
	device_destroy(dev_class, dev);
	class_destroy(dev_class);
	cdev_del(&gpio_button_cdev);
	unregister_chrdev_region(dev, 1);
	pr_err("%s: Quitting driver OK", DEVICE_NAME);

}


/* devsysfs open */
static int device_file_open(struct inode *inode, struct file *file)
{
	if (is_open) {
		pr_err("%s: Can't open. devsysfs is busy", MODULE_NAME);
		return -EBUSY;
	}
	is_open = 1;
	return 0;
}
/* devsysfs release */
static int device_file_release(struct inode *inode, struct file *file)
{
	is_open = 0;
	return 0;
}

/* devsysfs write */
static ssize_t device_file_read(struct file *filp, char __user *buffer, size_t count, loff_t *offset)
{
	ssize_t ret;

	if (*offset >= fs_buffer_size) {
		/* we have finished to read, return 0 */
		ret  = 0;
	} else {
		/* fill the buffer, return the buffer size */
		copy_to_user(buffer, fs_buffer, fs_buffer_size);
		*offset += fs_buffer_size;
		ret = fs_buffer_size;
	}
	return ret;
}
/* devsysfs read */
static ssize_t device_file_write(struct file *filp, const char __user *buffer, size_t count, loff_t *offset)
{

	fs_buffer_size = count; //store buffer size
	memset(fs_buffer, 0, BUFFER_MAX_SIZE); //clear buffer from old garbage

	/* check for write buffer overflow > 1024 */
	if (fs_buffer_size > BUFFER_MAX_SIZE)
		fs_buffer_size = BUFFER_MAX_SIZE;

	if (copy_from_user(fs_buffer, buffer, fs_buffer_size)) {
		pr_err("%s: procfs write failed\n", MODULE_NAME);
		return -EFAULT;
	}
	pr_err("%s: Driver write = %s\n", MODULE_NAME, fs_buffer);

	return count;
}


static struct file_operations gpio_button_fops = {
	.owner  = THIS_MODULE,
	.read	= device_file_read,
	.write  = device_file_write,
	.open   = device_file_open,
	.release  = device_file_release,
};


module_init(gpio_button_driver_main);
module_exit(gpio_button_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eddie07 <splissken2014@gmail.com>");
MODULE_DESCRIPTION("GPIO button driver");
MODULE_VERSION("0.1");
