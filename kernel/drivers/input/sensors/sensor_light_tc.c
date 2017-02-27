/* drivers/input/sensors/sensor-dev.c - handle all gsensor in this file
 *
 * Copyright (C) 2012-2015 ROCKCHIP.
 * Author: luowei <lw@rock-chips.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>
#include <linux/proc_fs.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/l3g4200d.h>
#include <linux/sensor-dev.h>
#include <linux/module.h>

#include <linux/iio/iio.h>
#include <linux/iio/machine.h>
#include <linux/iio/driver.h>
#include <linux/iio/consumer.h>

static long light_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int light_dev_open(struct inode *inode, struct file *file);
static int light_dev_release(struct inode *inode, struct file *file);


#define EM3071X_DEVICE_NAME "em3071x"
static int em3071x_open(struct inode *inode, struct file *file);
static int em3071x_release(struct inode *inode, struct file *file);
static long em3071x_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static int em3071x_open(struct inode *inode, struct file *file)
{
	return 0;
}
static int em3071x_release(struct inode *inode, struct file *file)
{return 0;}
static long em3071x_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return 0;
}


//static struct sensor_operate *sensor_ops;
struct sensor_private_data *g_sensor_tc;


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



static int light_report_value(struct input_dev *input, int data)
{
	unsigned char index = 0;
	
	if(data <= 30){
		index = 0;goto report;
	}
	else if(data <= 160){
		index = 1;goto report;
	}
	else if(data <= 225){
		index = 2;goto report;
	}
	else if(data <= 320){
		index = 3;goto report;
	}
	else if(data <= 640){
		index = 4;goto report;
	}
	else if(data <= 800){
		index = 5;goto report;
	}
	else if(data <= 900){
		index = 6;goto report;
	}
	else{
		index = 7;goto report;
	}

report:
	input_report_abs(input, ABS_MISC, index);
	input_sync(input);

	return index;
}

static int read_adc_value(struct sensor_private_data *sensor)
{
	int ret, val;
	ret = iio_read_channel_raw(sensor->chan, &val);
	if (ret < 0) {
                printk("read channel() error\n");
		return -1;
        }
	else{
		ret= fliter_val(val);
		if(g_count==16)
		{
			//printk("....valxxx= %d\n",g_val[7]);
		}
	}
	return g_val[7];
}



static int sensor_report_value(struct sensor_private_data *sensor)
{
		
	int result = 0;
	
	result = read_adc_value(sensor);
	if(result < 0)
		return result;
	
	if(g_count==16)
		light_report_value(sensor->input_dev, result);
	
	return result;
}






static int sensor_get_data_and_report(struct sensor_private_data *sensor)
{	
	int result = 0;
	
	result = sensor->ops->report(sensor);
	if(result < 0)
		goto error;

	/* set data_ready */
	//atomic_set(&sensor->data_ready, 1);
	/*wake up data_ready  work queue*/
	//wake_up(&sensor->data_ready_wq);
	
error:		
	return result;
}


static void  sensor_delaywork_func(struct work_struct *work)
{
	struct delayed_work *delaywork = container_of(work, struct delayed_work, work);
	struct sensor_private_data *sensor = container_of(delaywork, struct sensor_private_data, delaywork);
	
	mutex_lock(&sensor->sensor_mutex);	
	if (sensor_get_data_and_report(sensor) < 0) 
		printk(KERN_ERR "%s: Get data failed\n",__func__);
	
	
	schedule_delayed_work(&sensor->delaywork, msecs_to_jiffies(sensor->pdata->poll_delay_ms));
	//else
	//{
		//if((sensor->ops->trig == IRQF_TRIGGER_LOW) || (sensor->ops->trig == IRQF_TRIGGER_HIGH))
		//enable_irq(sensor->client->irq);
	//}
	mutex_unlock(&sensor->sensor_mutex);
	
}



struct sensor_operate light_tc_ops = {
	.name				= "light_tc",
	.type				= SENSOR_TYPE_LIGHT,	//sensor type and it should be correct
	//.id_i2c				= LIGHT_ID_CM3217,	//i2c id number
	//.read_reg			= CM3217_ADDR_DATA_LSB,	//read data
	//.read_len			= 2,			//data length
	//.id_reg				= SENSOR_UNKNOW_DATA,	//read device id from this register
	//.id_data 			= SENSOR_UNKNOW_DATA,	//device id
	//.precision			= 8,			//8 bits
	//.ctrl_reg 			= CM3217_ADDR_COM1,	//enable or disable 
	//.int_status_reg 		= SENSOR_UNKNOW_DATA,	//intterupt status register
	.range				= {1,1024},		//range
	.brightness                                        ={10,255},                          // brightness
	//.trig				= SENSOR_UNKNOW_DATA,		
	//.active				= sensor_active,	
	//.init				= sensor_init,
	.report				= sensor_report_value,
};


static int light_dev_open(struct inode *inode, struct file *file)
{
	//struct sensor_private_data *sensor = g_sensor[SENSOR_TYPE_LIGHT];
	//struct i2c_client *client = sensor->client;	
	int result = 0;	

printk("open.....................\n");
	return result;
}

static int light_dev_release(struct inode *inode, struct file *file)
{
	//struct sensor_private_data *sensor = g_sensor[SENSOR_TYPE_LIGHT];
	//struct i2c_client *client = sensor->client;	
	int result = 0;


	return result;
}



/* ioctl - I/O control */
static long light_dev_ioctl(struct file *file,
			  unsigned int cmd, unsigned long arg)
{

	struct sensor_private_data *sensor = g_sensor_tc;
	
	unsigned int *argp = (unsigned int *)arg;	
	int result = 0;
printk("ioctl....\n");
	switch(cmd)
	{
		case LIGHTSENSOR_IOCTL_GET_ENABLED:
			printk("get....\n");
			*argp = sensor->status_cur;
			break;
		case LIGHTSENSOR_IOCTL_ENABLE:		
			printk("%s:LIGHTSENSOR_IOCTL_ENABLE start\n", __func__);
			mutex_lock(&sensor->operation_mutex);    
			if(*(unsigned int *)argp)
			{
				if(sensor->status_cur == SENSOR_OFF)
				{		            		
					PREPARE_DELAYED_WORK(&sensor->delaywork, sensor_delaywork_func);
					schedule_delayed_work(&sensor->delaywork, msecs_to_jiffies(sensor->pdata->poll_delay_ms));
										
					sensor->status_cur = SENSOR_ON;
				}	
			}
			else
			{
				if(sensor->status_cur == SENSOR_ON)
				{		         
					cancel_delayed_work_sync(&sensor->delaywork);	
					
					sensor->status_cur = SENSOR_OFF;
	        		}
			}
			mutex_unlock(&sensor->operation_mutex);
	        	printk("%s:LIGHTSENSOR_IOCTL_ENABLE OK\n", __func__);
			break;
		
		default:
			break;
	}
	
	return result;

}



static const struct file_operations em3071_fops = {
    .owner = THIS_MODULE,
    .open = light_dev_open,
    .release = light_dev_release,
    .unlocked_ioctl = light_dev_ioctl,
};
static struct miscdevice em3071_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "lightsensor",
	.fops = &em3071_fops,
};





static int sensor_misc_device_register(struct sensor_private_data *sensor, int type)
{
	int result = 0;
	switch(type)
	{

		case SENSOR_TYPE_LIGHT:
			if(!sensor->ops->misc_dev)
			{	/*
				sensor->fops.owner = THIS_MODULE;
				sensor->fops.unlocked_ioctl = light_dev_ioctl;
				sensor->fops.open = light_dev_open;
				sensor->fops.release = light_dev_release;

				sensor->miscdev.minor = MISC_DYNAMIC_MINOR;
				sensor->miscdev.name = "lightsensor";
				sensor->miscdev.fops = &sensor->fops;
				*/
			}	
			else
			{
				memcpy(&sensor->miscdev, sensor->ops->misc_dev, sizeof(*sensor->ops->misc_dev));

			}
			break;

		default:
			printk("%s:unknow sensor type=%d\n",__func__,type);
			result = -1;
			goto error;
	}
	sensor->miscdev = em3071_device;				
	sensor->miscdev.parent = &sensor->pdev->dev;
printk("1..............\n");	
	result = misc_register(&sensor->miscdev);
//result = misc_register(&em3071_device);
	if (result < 0) {
		printk("fail to register misc device %s\n", sensor->miscdev.name);
		goto error;
	}
printk("2..............\n");		
	printk("%s:miscdevice: %s\n",__func__,sensor->miscdev.name);

error:	
	
	return result;

}




int sensor_probe_plat(struct platform_device *pdev)
{
	struct sensor_private_data *sensor;
	struct sensor_platform_data *pdata;
	struct device_node *np = pdev->dev.of_node;
	
	int result = 0;
	int type = 0;

pdata = kmalloc(sizeof(struct sensor_platform_data), GFP_KERNEL);
	if (!pdata) {
		result = -ENOMEM;
		goto out_no_free_pdata;
	}
sensor = kmalloc(sizeof(struct sensor_private_data), GFP_KERNEL);
	if (!sensor) {
		result = -ENOMEM;
		goto out_no_free_sensor;
	}
sensor->chan = iio_channel_get(&pdev->dev, NULL);
	if(!sensor->chan){
		printk("channel err\n");
		goto channel_err;
	}
	
	for(g_count=0;g_count<16;g_count++)
	{
		g_val[g_count] = 0;
		g_val_src[g_count] = 0;
	}

	g_count=0;
sensor->pdev =  pdev;
g_sensor_tc = sensor;
platform_set_drvdata(pdev, sensor);	
	of_property_read_u32(np,"type",&(pdata->type));
	of_property_read_u32(np,"irq_enable",&(pdata->irq_enable));
	of_property_read_u32(np,"poll_delay_ms",&(pdata->poll_delay_ms));		
	of_property_read_u32(np,"layout",&(pdata->layout));


	switch(pdata->layout)
	{
		case 1:
			pdata->orientation[0] = 1;
			pdata->orientation[1] = 0;
			pdata->orientation[2] = 0;

			pdata->orientation[3] = 0;
			pdata->orientation[4] = 1;
			pdata->orientation[5] = 0;

			pdata->orientation[6] = 0;
			pdata->orientation[7] = 0;
			pdata->orientation[8] = 1;
			break;

		default:
			pdata->orientation[0] = 1;
			pdata->orientation[1] = 0;
			pdata->orientation[2] = 0;

			pdata->orientation[3] = 0;
			pdata->orientation[4] = 1;
			pdata->orientation[5] = 0;

			pdata->orientation[6] = 0;
			pdata->orientation[7] = 0;
			pdata->orientation[8] = 1;
			break;
	}

	
	type = pdata->type;
	
	DBG("type = %d \n",pdata->type);
	
	DBG("irq_enable = %d \n",pdata->irq_enable);

	DBG("poll_delay_ms = %d \n",pdata->poll_delay_ms);
	
	DBG("layout = %d \n",pdata->layout);
	
	DBG(" == %d,%d ,%d \t ,%d ,%d ,%d , \t ,%d, %d, %d ,==%d\n",pdata->orientation[0],pdata->orientation[1],pdata->orientation[2]
				,pdata->orientation[3],pdata->orientation[4],pdata->orientation[5]
				,pdata->orientation[6],pdata->orientation[7],pdata->orientation[8],ARRAY_SIZE(pdata->orientation));
		
		
	sensor->pdata = pdata;	
	sensor->type = type;
	//sensor->i2c_id = (struct i2c_device_id *)devid;

		
	memset(&(sensor->axis), 0, sizeof(struct sensor_axis) );
	//atomic_set(&(sensor->data_ready), 0);
	//init_waitqueue_head(&(sensor->data_ready_wq));
	mutex_init(&sensor->data_mutex);	
	mutex_init(&sensor->operation_mutex);	
	mutex_init(&sensor->sensor_mutex);
	//mutex_init(&sensor->i2c_mutex);

	/* As default, report all information */
	atomic_set(&sensor->flags.m_flag, 1);
	atomic_set(&sensor->flags.a_flag, 1);
	atomic_set(&sensor->flags.mv_flag, 1);			
	atomic_set(&sensor->flags.open_flag, 0);
	atomic_set(&sensor->flags.debug_flag, 1);
	init_waitqueue_head(&sensor->flags.open_wq);
	sensor->flags.delay = 100;

	sensor->status_cur = SENSOR_OFF;
	sensor->axis.x = 0;
	sensor->axis.y = 0;
	sensor->axis.z = 0;
	
	//sensor_ops = &light_tc_ops;
	sensor->ops = &light_tc_ops;
	sensor->input_dev = input_allocate_device();
	if (!sensor->input_dev) {
		result = -ENOMEM;
		printk("Failed to allocate input device \n");
		goto out_free_input;
	}	

	switch(type)
	{
		case SENSOR_TYPE_LIGHT:
			sensor->input_dev->name = "lightsensor-level";
			set_bit(EV_ABS, sensor->input_dev->evbit);
			input_set_abs_params(sensor->input_dev, ABS_MISC, sensor->ops->range[0], sensor->ops->range[1], 0, 0);			
			input_set_abs_params(sensor->input_dev, ABS_TOOL_WIDTH ,  sensor->ops->brightness[0],sensor->ops->brightness[1], 0, 0);
			break;
		default:
			printk("%s:unknow sensor type=%d\n",__func__,type);
			break;

	}
	sensor->input_dev->dev.parent = &pdev->dev;

	result = input_register_device(sensor->input_dev);

	if (result) {
		printk("Unable to register input device %s\n", sensor->input_dev->name);
		goto out_input_register_device_failed;
	}


	INIT_DELAYED_WORK(&sensor->delaywork, sensor_delaywork_func);
	if(sensor->pdata->poll_delay_ms < 0)
		sensor->pdata->poll_delay_ms = 30;

	//sensor->miscdev.parent = &pdev->dev;
	result = sensor_misc_device_register(sensor, type);
	if (result) {
		printk("fail to register misc device %s\n", sensor->miscdev.name);
		goto out_misc_device_register_device_failed;
	}

	printk("%s:initialized ok,sensor name:%s,type:%d\n",sensor->ops->name,type);

	return result;


channel_err:	
out_misc_device_register_device_failed:
	input_unregister_device(sensor->input_dev);	
out_input_register_device_failed:
	input_free_device(sensor->input_dev);	
out_free_input:
	kfree(sensor);
out_no_free_sensor:
	kfree(pdata);
out_no_free_pdata:
	printk("pdata failed \n");
	return result;

}


static int sensor_remove_plat(struct platform_device *pdev)
{
	struct sensor_private_data *sensor = platform_get_drvdata(pdev);
	cancel_delayed_work_sync(&sensor->delaywork);
	misc_deregister(&sensor->miscdev);
	input_unregister_device(sensor->input_dev);	
	input_free_device(sensor->input_dev);	
	kfree(sensor);
#ifdef CONFIG_HAS_EARLYSUSPEND
	if((sensor->ops->suspend) && (sensor->ops->resume))
		unregister_early_suspend(&sensor->early_suspend);
#endif  
	return 0;
}


static const struct of_device_id sensor_dt_ids_plat[] = {
	{ .compatible = "rockchip,light_tc", .data = NULL},
	{},
};


static struct platform_driver sensor_driver_plat = {
	.probe		= sensor_probe_plat,
	.remove		= sensor_remove_plat,
	//.id_table   = sensor_id,
	.driver		= {
		.name	= "sensors",
		.owner	= THIS_MODULE,
		.of_match_table	= sensor_dt_ids_plat,
	},
};


static int __init sensor_init(void)
{
	printk("xxsensor_init\n");
	int res = platform_driver_register(&sensor_driver_plat);
/*	
	struct proc_dir_entry *sensor_proc_entry;	
	pr_info("%s: Probe name %s\n", __func__, sensor_driver.driver.name);
	if (res)
		pr_err("%s failed\n", __func__);
	
	sensor_proc_entry = proc_create("driver/sensor_dbg", 0660, NULL, &sensor_proc_fops); 
*/
	//printk("%s\n", SENSOR_VERSION_AND_TIME);
	return res;
}

static void __exit sensor_exit(void)
{
	printk("%s\n", __func__);
	platform_driver_unregister(&sensor_driver_plat);
}


late_initcall(sensor_init);
module_exit(sensor_exit);

MODULE_AUTHOR("ROCKCHIP Corporation:lw@rock-chips.com");
MODULE_DESCRIPTION("User space character device interface for sensors");
MODULE_LICENSE("GPL");

