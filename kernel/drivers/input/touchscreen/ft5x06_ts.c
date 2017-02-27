/* drivers/input/touchscreen/ft5x06_ts.c
 *
 * FocalTech TouchScreen driver in android 4.x.
 *
 * Copyright (c) 2010  Focal tech Ltd.
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
/*
#include <linux/i2c.h>
#include <linux/input.h>
//#include <linux/earlysuspend.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
//#include <mach/irqs.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/module.h>
//#include <linux/gpio.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/input/mt.h>
#include "ft5x06_ts.h"
*/

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/async.h>
#include <linux/irq.h>
#include <linux/workqueue.h>
#include <linux/proc_fs.h>
#include <linux/input/mt.h>
//#include <linux/regulator/act8846.h>
//#include <linux/mfd/tps65910.h>

#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include "tp_suspend.h"

#include "ft5x06_ts.h"

#ifndef CONFIG_PM
	#define CONFIG_PM
#endif


#define GSLX680_I2C_NAME_WAKE_TC   "touch_wake_tc"

extern atomic_t wake_flag_tc;


struct ts_event {
	u16 au16_x[CFG_MAX_TOUCH_POINTS];	/*x coordinate */
	u16 au16_y[CFG_MAX_TOUCH_POINTS];	/*y coordinate */
	u8 au8_touch_event[CFG_MAX_TOUCH_POINTS];	/*touch event:
					0 -- down; 1-- contact; 2 -- contact */
	u8 au8_finger_id[CFG_MAX_TOUCH_POINTS];	/*touch ID */
	u16 pressure;
	u8 touch_point;
};

struct ft5x0x_ts_data {
	int irq_pin;
	int rst_pin;
	int irq;
	int rst_val;
	unsigned int x_max;
	unsigned int y_max;
	char device_id;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct ts_event event;
	//struct ft5x0x_platform_data *pdata;
	struct work_struct 	pen_event_work;
	struct workqueue_struct *ts_workqueue;

	struct input_dev *input_tc;
	struct work_struct work_tc;
	struct workqueue_struct *wq_tc;


	#ifdef CONFIG_PM
	struct early_suspend *early_suspend;
	#endif
};


//#define FTS_CTL_IIC
#ifdef FTS_CTL_IIC
#include "focaltech_ctl.h"
#endif

/*
*ft5x0x_i2c_Read-read data and write data by i2c
*@client: handle of i2c
*@writebuf: Data that will be written to the slave
*@writelen: How many bytes to write
*@readbuf: Where to store data read from slave
*@readlen: How many bytes to read
*
*Returns negative errno, else the number of messages executed
*
*
*/
int ft5x0x_i2c_Read(struct i2c_client *client, char *writebuf,
		    int writelen, char *readbuf, int readlen)
{
	int ret;

	if (writelen > 0) {
		struct i2c_msg msgs[] = {
			{
			 .addr = client->addr,
			 .flags = 0,
			 .len = writelen,
			 .buf = writebuf,
			 },
			{
			 .addr = client->addr,
			 .flags = I2C_M_RD,
			 .len = readlen,
			 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(client->adapter, msgs, 2);
		if (ret < 0)
			dev_err(&client->dev, "f%s: i2c read error.\n",
				__func__);
	} else {
		struct i2c_msg msgs[] = {
			{
			 .addr = client->addr,
			 .flags = I2C_M_RD,
			 .len = readlen,
			 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret < 0)
			dev_err(&client->dev, "%s:i2c read error.\n", __func__);
	}
	return ret;
}
/*write data by i2c*/
int ft5x0x_i2c_Write(struct i2c_client *client, char *writebuf, int writelen)
{
	int ret;

	struct i2c_msg msg[] = {
		{
		 .addr = client->addr,
		 .flags = 0,
		 .len = writelen,
		 .buf = writebuf,
		 },
	};

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret < 0)
		dev_err(&client->dev, "%s i2c write error.\n", __func__);

	return ret;
}
#if 0
/*release the point*/
static void ft5x0x_ts_release(struct ft5x0x_ts_data *data)
{
	input_mt_sync(data->input_dev);
	input_sync(data->input_dev);
}
#endif
/*Read touch point information when the interrupt  is asserted.*/
static int ft5x0x_read_Touchdata(struct ft5x0x_ts_data *data)
{
	struct ts_event *event = &data->event;
	u8 buf[POINT_READ_BUF] = { 0 };
	int ret = -1;
	int i = 0;
	u8 pointid = FT_MAX_ID;

	ret = ft5x0x_i2c_Read(data->client, buf, 1, buf, POINT_READ_BUF);
	if (ret < 0) {
		dev_err(&data->client->dev, "%s read touchdata failed.\n",
			__func__);
		return ret;
	}
	memset(event, 0, sizeof(struct ts_event));

	event->touch_point = 0;
	for (i = 0; i < CFG_MAX_TOUCH_POINTS; i++) {
		pointid = (buf[FT_TOUCH_ID_POS + FT_TOUCH_STEP * i]) >> 4;
		if (pointid >= FT_MAX_ID)
			break;
		else
			event->touch_point++;
		event->au16_x[i] =
		    (s16) (buf[FT_TOUCH_X_H_POS + FT_TOUCH_STEP * i] & 0x0F) <<
		    8 | (s16) buf[FT_TOUCH_X_L_POS + FT_TOUCH_STEP * i];
		event->au16_y[i] =
		    (s16) (buf[FT_TOUCH_Y_H_POS + FT_TOUCH_STEP * i] & 0x0F) <<
		    8 | (s16) buf[FT_TOUCH_Y_L_POS + FT_TOUCH_STEP * i];
		event->au8_touch_event[i] =
		    buf[FT_TOUCH_EVENT_POS + FT_TOUCH_STEP * i] >> 6;
		event->au8_finger_id[i] =
		    (buf[FT_TOUCH_ID_POS + FT_TOUCH_STEP * i]) >> 4;
		#if 0
		pr_info("id=%d event=%d x=%d y=%d\n", event->au8_finger_id[i],
			event->au8_touch_event[i], event->au16_x[i], event->au16_y[i]);
		#endif
	}

	event->pressure = FT_PRESS;

	return 0;
}

/*
*report the point information
*/
static void ft5x0x_report_value(struct ft5x0x_ts_data *data)
{
	struct ts_event *event = &data->event;
	int i;
	int uppoint = 0;

	/*protocol B*/	
	for (i = 0; i < event->touch_point; i++)
	{
		input_mt_slot(data->input_dev, event->au8_finger_id[i]);
		
		if (event->au8_touch_event[i]== 0 || event->au8_touch_event[i] == 2)
		{
			input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER,
				true);
			input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR,
					event->pressure);
			input_report_abs(data->input_dev, ABS_MT_POSITION_X,
					event->au16_y[i]);
			input_report_abs(data->input_dev, ABS_MT_POSITION_Y,
					1920 - event->au16_x[i]);
			//printk("x=%d    y=%d\n",event->au16_x[i],event->au16_y[i]);
		}
		else
		{
			uppoint++;
			input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER,
				false);
		}
	}
	if(event->touch_point == uppoint)
		input_report_key(data->input_dev, BTN_TOUCH, 0);
	else
		input_report_key(data->input_dev, BTN_TOUCH, event->touch_point > 0);
	input_sync(data->input_dev);


}



static void ft5x0x_delaywork_func_tc(struct work_struct *work)
{
	struct ft5x0x_ts_data *ts = container_of(work, struct ft5x0x_ts_data,work_tc);

	input_event(ts->input_tc, EV_KEY, KEY_POWER, 1);	
	input_sync(ts->input_tc);
	input_event(ts->input_tc, EV_KEY, KEY_POWER, 0);	
	input_sync(ts->input_tc);

	enable_irq(ts->irq);
}





static void  ft5x0x_delaywork_func(struct work_struct *work)
{
	struct ft5x0x_ts_data *ft5x0x = container_of(work, struct ft5x0x_ts_data, pen_event_work);
	struct i2c_client *client = ft5x0x->client;

	//ft5x0x_process_points(ft5x0x);
	//enable_irq(client->irq);
	
	int ret = 0;
	ret = ft5x0x_read_Touchdata(ft5x0x);
	if (ret == 0)
		ft5x0x_report_value(ft5x0x);

	enable_irq(client->irq);		
}





/*The ft5x0x device will signal the host about TRIGGER_FALLING.
*Processed when the interrupt is asserted.
*/
static irqreturn_t ft5x0x_ts_interrupt(int irq, void *dev_id)
{
	struct ft5x0x_ts_data *ft5x0x_ts = dev_id;

	disable_irq_nosync(ft5x0x_ts->irq);

	if(atomic_read(&wake_flag_tc))
	{
		printk("flag\n");
		atomic_set(&wake_flag_tc, 0);
		queue_work(ft5x0x_ts->wq_tc, &ft5x0x_ts->work_tc);
	}
	else
	{
		queue_work(ft5x0x_ts->ts_workqueue, &ft5x0x_ts->pen_event_work);
	}
	return IRQ_HANDLED;
}

static int ft5x0x_ts_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	//struct ft5x0x_platform_data *pdata = (struct ft5x0x_platform_data *)client->dev.platform_data;
	struct device_node *np = client->dev.of_node;
	struct ft5x0x_ts_data *ft5x0x_ts;
	struct input_dev *input_dev;

	struct input_dev *input_device_tc;

	enum of_gpio_flags rst_flags, pwr_flags;
	unsigned long irq_flags;
	int err = 0;
	unsigned char uc_reg_value;
	unsigned char uc_reg_addr;


printk("ft5x0x_ts_probe...ttt\n");
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	ft5x0x_ts = kzalloc(sizeof(struct ft5x0x_ts_data), GFP_KERNEL);

	if (!ft5x0x_ts) {
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	i2c_set_clientdata(client, ft5x0x_ts);

ft5x0x_ts->irq_pin = of_get_named_gpio_flags(np, "touch-gpio", 0, (enum of_gpio_flags *)&irq_flags);
ft5x0x_ts->rst_pin = of_get_named_gpio_flags(np, "reset-gpio", 0, &rst_flags);
if (gpio_is_valid(ft5x0x_ts->rst_pin)) {
		ft5x0x_ts->rst_val = (rst_flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1;
		err = devm_gpio_request_one(&client->dev, ft5x0x_ts->rst_pin, (rst_flags & OF_GPIO_ACTIVE_LOW) ? GPIOF_OUT_INIT_HIGH : GPIOF_OUT_INIT_LOW, "goodix reset pin");
		if (err != 0) {
			dev_err(&client->dev, "goodix gpio_request error\n");
			return -EIO;
		}
		gpio_direction_output(ft5x0x_ts->rst_pin, 0);
		gpio_set_value(ft5x0x_ts->rst_pin,0);
		msleep(20);
		gpio_set_value(ft5x0x_ts->rst_pin,1);
	} else {
		dev_info(&client->dev, "reset pin invalid\n");
	}
ft5x0x_ts->device_id = 0;
	//ft5x0x_ts->irq = client->irq;	
ft5x0x_ts->client = client;
	//ft5x0x_ts->pdata = pdata;
ft5x0x_ts->x_max = SCREEN_MAX_X - 1;
ft5x0x_ts->y_max = SCREEN_MAX_Y - 1;

ft5x0x_ts->irq=gpio_to_irq(ft5x0x_ts->irq_pin);		//If not defined in client
client->irq = ft5x0x_ts->irq;
	//ft5x0x_ts->pdata->reset = FT5X0X_RESET_PIN;
	//ft5x0x_ts->pdata->irq = ft5x0x_ts->irq;
/*	
#ifdef CONFIG_PM
	err = gpio_request(pdata->reset, "ft5x0x reset");
	if (err < 0) {
		dev_err(&client->dev, "%s:failed to set gpio reset.\n",
			__func__);
		goto exit_request_reset;
	}
#endif
*/	

	err = request_irq(client->irq, ft5x0x_ts_interrupt,
				   IRQF_TRIGGER_FALLING, client->name,
				   ft5x0x_ts);
	if (err != 0) {
		dev_err(&client->dev, "ft5x0x_probe: request irq failed\n");
		goto exit_irq_request_failed;
	}
	disable_irq(client->irq);

	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}

	ft5x0x_ts->input_dev = input_dev;

	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(BTN_TOUCH, input_dev->keybit);
	
	input_mt_init_slots(input_dev, CFG_MAX_TOUCH_POINTS,0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR,
			     0, PRESS_MAX, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X,
			     0, ft5x0x_ts->x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y,
			     0, ft5x0x_ts->y_max, 0, 0);

	input_dev->name = FT5X0X_NAME;
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,
			"ft5x0x_ts_probe: failed to register input device: %s\n",
			dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}
	/*make sure CTP already finish startup process */
	msleep(150);


INIT_WORK(&ft5x0x_ts->pen_event_work, ft5x0x_delaywork_func);
ft5x0x_ts->ts_workqueue = create_singlethread_workqueue("ft5x0x_ts");
if (!ft5x0x_ts->ts_workqueue) {
	err = -ESRCH;
	printk("ts_workqueue  fail..\n");
	goto exit_input_register_device_failed;
}

#ifdef FTS_CTL_IIC
		if (ft_rw_iic_drv_init(client) < 0)
			dev_err(&client->dev, "%s:[FTS] create fts control iic driver failed\n",
					__func__);
#endif

	/*get some register information */
	uc_reg_addr = FT5x0x_REG_FW_VER;
	ft5x0x_i2c_Read(client, &uc_reg_addr, 1, &uc_reg_value, 1);
	dev_dbg(&client->dev, "[FTS] Firmware version = 0x%x\n", uc_reg_value);
	

	uc_reg_addr = FT5x0x_REG_POINT_RATE;
	ft5x0x_i2c_Read(client, &uc_reg_addr, 1, &uc_reg_value, 1);
	dev_dbg(&client->dev, "[FTS] report rate is %dHz.\n",
		uc_reg_value * 10);

	uc_reg_addr = FT5X0X_REG_THGROUP;
	ft5x0x_i2c_Read(client, &uc_reg_addr, 1, &uc_reg_value, 1);
	dev_dbg(&client->dev, "[FTS] touch threshold is %d.\n",
		uc_reg_value * 4);


/*****************************************************************************************/
	input_device_tc = input_allocate_device();
	if (!input_device_tc) {
		err = -ENOMEM;
		goto error_alloc_dev_tc;
	}
	ft5x0x_ts->input_tc = input_device_tc;

	input_set_capability(input_device_tc, EV_KEY, KEY_POWER);
	input_device_tc->name = GSLX680_I2C_NAME_WAKE_TC;
	input_device_tc->id.bustype = BUS_HOST;
	input_set_drvdata(input_device_tc, ft5x0x_ts);
	
	ft5x0x_ts->wq_tc = create_singlethread_workqueue("kworkqueue_ts_tc");
	if (!ft5x0x_ts->wq_tc) {
		dev_err(&client->dev, "Could not create workqueue_tc\n");
		goto error_wq_create_tc;
	}
	flush_workqueue(ft5x0x_ts->wq_tc);

	INIT_WORK(&ft5x0x_ts->work_tc, ft5x0x_delaywork_func_tc);

	err = input_register_device(input_device_tc);
	if (err)
		goto error_unreg_device_tc;

	enable_irq(client->irq);
	return 0;
/******************************************************************************************/
error_unreg_device_tc:
	destroy_workqueue(ft5x0x_ts->wq_tc);
error_wq_create_tc:
	input_free_device(input_device_tc);
error_alloc_dev_tc:
	printk("touch to wake register input fail\n");

	enable_irq(client->irq);
	return 0;

	

exit_input_register_device_failed:
	input_free_device(input_dev);

exit_input_dev_alloc_failed:
	free_irq(client->irq, ft5x0x_ts);
#ifdef CONFIG_PM
exit_request_reset:
	gpio_free(ft5x0x_ts->rst_pin);
#endif

exit_irq_request_failed:
	i2c_set_clientdata(client, NULL);
	kfree(ft5x0x_ts);

exit_alloc_data_failed:
exit_check_functionality_failed:
	return err;
}

#ifdef CONFIG_PM
static void ft5x0x_ts_suspend(struct early_suspend *handler)
{
	struct ft5x0x_ts_data *ts = container_of(handler, struct ft5x0x_ts_data,
						early_suspend);

	dev_dbg(&ts->client->dev, "[FTS]ft5x0x suspend\n");
	disable_irq(ts->irq);
}

static void ft5x0x_ts_resume(struct early_suspend *handler)
{
	struct ft5x0x_ts_data *ts = container_of(handler, struct ft5x0x_ts_data,
						early_suspend);

	dev_dbg(&ts->client->dev, "[FTS]ft5x0x resume.\n");
	gpio_set_value(ts->rst_pin, 0);
	msleep(20);
	gpio_set_value(ts->rst_pin, 1);
	enable_irq(ts->irq);
}
#else
#define ft5x0x_ts_suspend	NULL
#define ft5x0x_ts_resume		NULL
#endif

static int  ft5x0x_ts_remove(struct i2c_client *client)
{
	struct ft5x0x_ts_data *ft5x0x_ts;
	ft5x0x_ts = i2c_get_clientdata(client);
	input_unregister_device(ft5x0x_ts->input_dev);
	#ifdef CONFIG_PM
	gpio_free(ft5x0x_ts->rst_pin);
	#endif

#ifdef FTS_CTL_IIC
	ft_rw_iic_drv_exit();
#endif

	free_irq(client->irq, ft5x0x_ts);

	kfree(ft5x0x_ts);
	i2c_set_clientdata(client, NULL);
	return 0;
}

static const struct i2c_device_id ft5x0x_ts_id[] = {
	{FT5X0X_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, ft5x0x_ts_id);
static struct of_device_id goodix_ts_dt_ids[] = {
	{ .compatible = "tchip,ft5x06" },
	{ }
};


static struct i2c_driver ft5x0x_ts_driver = {
/*
	.probe = ft5x0x_ts_probe,
	.remove = __devexit_p(ft5x0x_ts_remove),
	.id_table = ft5x0x_ts_id,
	.suspend = ft5x0x_ts_suspend,
	.resume = ft5x0x_ts_resume,
	.driver = {
		   .name = FT5X0X_NAME,
		   .owner = THIS_MODULE,
		   },
*/

	.driver = {
		.name = FT5X0X_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(goodix_ts_dt_ids),
	},
	.suspend	= ft5x0x_ts_suspend,
	.resume	= ft5x0x_ts_resume,
	.probe		= ft5x0x_ts_probe,
	.remove		= ft5x0x_ts_remove,
	.id_table	= ft5x0x_ts_id,
};

static int __init ft5x0x_ts_init(void)
{
	int ret;
	ret = i2c_add_driver(&ft5x0x_ts_driver);
	if (ret) {
		printk(KERN_WARNING "Adding ft5x0x driver failed "
		       "(errno = %d)\n", ret);
	} else {
		pr_info("Successfully added driver %s\n",
			ft5x0x_ts_driver.driver.name);
	}
	return ret;
}

static void __exit ft5x0x_ts_exit(void)
{
	i2c_del_driver(&ft5x0x_ts_driver);
}

module_init(ft5x0x_ts_init);
module_exit(ft5x0x_ts_exit);

MODULE_AUTHOR("<luowj>");
MODULE_DESCRIPTION("FocalTech TouchScreen driver");
MODULE_LICENSE("GPL");
