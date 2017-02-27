/*
 * Driver for keys on GPIO lines capable of generating interrupts.
 *
 * Copyright 2005 Phil Blundell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/adc.h>
#include <linux/slab.h>
#include <linux/hrtimer.h>

#include <linux/iio/iio.h>
#include <linux/iio/machine.h>
#include <linux/iio/driver.h>
#include <linux/iio/consumer.h>

#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/miscdevice.h>

#include "ligth_tc.h"
struct light_tc_status  *g_stat;

int g_val[16];
int g_val_src[16];
int g_count;

//int g_test_val[16]={6,98,2,44,33,1,5,100,5,2,33,44,66,7,54,7};


static void sort_val(int val[], int count)
{
	int i,j,temp;
	for(i=0;i<count-1;i++)
	{
		for(j=i+1;j<count;j++)
		{
			if(val[j]<val[i])
			{
				temp = val[i];
				val[i]=val[j];
				val[j]=temp;
			}
		}
	}
}

/*
void test_val()
{
	for(g_count=0;g_count<16;g_count++)
		printk("%d\n",g_test_val[g_count]);

	printk("..\n");
	printk("...\n");g_test_val

	sort_val(g_test_val,g_count); 
	for(g_count=0;g_count<16;g_count++)
		printk("%d\n",g_test_val[g_count]);
}
*/

static int fliter_val(int val)
{
	int i;

	if(g_count<16)
	{
		g_val_src[g_count]=val;
		g_count++;
		if(g_count==16)  //数组已满，排序
		{
			for(i=0;i<g_count;i++)
				g_val[i]=g_val_src[i];

			sort_val(g_val,g_count);  //
		}
	}
	else
	{
		//先进先出
		for(i=0;i<g_count-1;i++)
		{
			g_val_src[i]=g_val_src[i+1];
		}
		g_val_src[g_count-1]=val;

		for(i=0;i<g_count;i++)
			g_val[i]=g_val_src[i];
		sort_val(g_val,16);
	}

	return g_val[7];
}



static int lsm9ds1_mag_enable(struct light_tc_status *stat)
{
	atomic_set(&stat->enabled_light, 1);
			
	hrtimer_start(&stat->hr_timer_light, stat->ktime_light,
							HRTIMER_MODE_REL);
	return 0;
}

static int lsm9ds1_mag_disable(struct light_tc_status *stat)
{
	atomic_set(&stat->enabled_light, 0);
	cancel_work_sync(&stat->work_light);
	hrtimer_cancel(&stat->hr_timer_light);

	return 0;
}


static ssize_t light_enable_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int val = (int)atomic_read(&g_stat->enabled_light);
	return sprintf(buf, "%d\n", val);
}
static ssize_t light_enable_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	unsigned long val;

	if (kstrtoul(buf, 10, &val))
		return -EINVAL;

	if (val)
		lsm9ds1_mag_enable(g_stat);
	else
		lsm9ds1_mag_disable(g_stat);

	return size;
}
static DEVICE_ATTR(light_enable, 0666, light_enable_show, light_enable_store);



static int light_open(struct inode *inode, struct file *file)
{
   
    return 0;
}

static int light_release(struct inode *inode, struct file *file)
{
   
    return 0;
}
static long light_ioctl(struct file *file, unsigned int cmd,  unsigned long arg)
{
	return 0;
}


static const struct file_operations light_fops = {
    	.owner = THIS_MODULE,
    	.open = light_open,
    	.release = light_release,
    	.unlocked_ioctl = light_ioctl,
};
static struct miscdevice light_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = LIGHT_DEVICE_MISC_NAME,
	.fops = &light_fops,
};


static void poll_function_work_light(struct work_struct *input_work_light)
{
	struct light_tc_status  *stat;
	int val, ret;

	stat = container_of((struct work_struct *)input_work_light,
			struct light_tc_status, work_light);


	ret = iio_read_channel_raw(stat->chan, &val);
        if (ret < 0) {
                printk("read channel() error\n");
        }
        else{
		//printk("....val = %d\n",val);
		ret= fliter_val(val);
		if(g_count==16)
		{
			printk("....valxxx= %d\n",g_val[7]);
		}
		
	
		hrtimer_start(&stat->hr_timer_light, stat->ktime_light, HRTIMER_MODE_REL);
	}
}


enum hrtimer_restart poll_function_read_light(struct hrtimer *timer)
{
	struct light_tc_status  *stat;


	stat = container_of((struct hrtimer *)timer,
				struct light_tc_status, hr_timer_light);

	queue_work(stat->light_wq, &stat->work_light);
	return HRTIMER_NORESTART;
}

static int  light_tc_probe(struct platform_device *pdev)
{
	int err = -1;
	

	printk("light_tc_probe..\n");
	g_stat = kmalloc(sizeof(struct light_tc_status), GFP_KERNEL);
	if (g_stat == NULL) {
		err = -ENOMEM;
		printk("failed to allocate memory for module data\n");
		goto exit_check_functionality_failed;
	}
	memset(g_stat, 0, sizeof(struct light_tc_status));

	hrtimer_init(&g_stat->hr_timer_light, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	g_stat->hr_timer_light.function = &poll_function_read_light;
	g_stat->ktime_light = ktime_set(0, MS_TO_NS(POLL_INTERVAL_DEF));

	g_stat->light_wq = create_singlethread_workqueue("light_wq");
	INIT_WORK(&g_stat->work_light, poll_function_work_light);

	err = misc_register(&light_device);
        if (err) {
        	printk("device registed failed\n");
        	goto exit_device_register_failed;
    	}

	err = device_create_file(light_device.this_device, &dev_attr_light_enable);
	if (0 != err) {
        	printk("sysfs create debug node fail\n");
        	goto exit_devices_create_group_failed;
    	}

	g_stat->chan = iio_channel_get(&pdev->dev, NULL);
	if(!g_stat->chan){
		printk("channel err\n");
		goto channel_err;
	}
	
	for(g_count=0;g_count<16;g_count++)
	{
		g_val[g_count] = 0;
		g_val_src[g_count] = 0;
	}

	g_count=0;

	//test_val();

	printk("light probe success\n");
	return 0;

channel_err:
exit_devices_create_group_failed:
	misc_deregister(&light_device);
	cancel_work_sync(&g_stat->work_light);
	hrtimer_cancel(&g_stat->hr_timer_light);	
	kfree(g_stat);
	return err;
exit_device_register_failed:
	cancel_work_sync(&g_stat->work_light);
	hrtimer_cancel(&g_stat->hr_timer_light);	
	kfree(g_stat);
	return err;
exit_check_functionality_failed:
	printk("%s: Driver Init failed\n", LIGHT_DEV_NAME);
	return err;

};


static int light_tc_remove(struct platform_device *pdev)
{

	return 0;
}



static const struct of_device_id light_tc_match[] = {
	{ .compatible = "rockchip,light_tc", .data = NULL},
	{},
};
MODULE_DEVICE_TABLE(of, light_tc_match);

static struct platform_driver light_tc_device_driver = {
	.probe		= light_tc_probe,
	.remove		= light_tc_remove,
	.driver		= {
		.name	= LIGHT_DEV_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = light_tc_match,
	}
};

module_platform_driver(light_tc_device_driver);

