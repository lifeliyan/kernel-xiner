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

#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include "tp_suspend.h"
#include "ags04.h"

struct input_dev *input_dev;
int flag=1;


int gpio_sensor;
int key1_press=0;
int key2_press=0;
int key4_press=0;
int key8_press=0;

static int ags04_iic_readbyte(struct i2c_client *client, u8 addr, u8 *pdata)
{
	int ret = 0;
	ret = ags04_iic_write(client, addr, NULL, 0);
	if (ret < 0)
	{
		printk("%s set data address fail!\n", __func__);
		return ret;
	}

	return i2c_master_recv(client, pdata, 1);
}


static int ags04_iic_read(struct i2c_client *client, u8 addr, u8 *pdata, unsigned int datalen)
{
	int ret = 0;
	int i = 0;

	if (datalen > 126)
	{
		printk("%s too big datalen = %d!\n", __func__, datalen);
		return -1;
	}
	for(i=0; i<datalen; i++){
		ret = ags04_iic_readbyte(client, addr+i, pdata+i);
		if(ret < 0)
			return ret;
	}
	return ret;
}

/*write data by i2c*/
static int ags04_iic_write(struct i2c_client *client, u8 addr, u8 *pdata, int datalen)
{
	int ret = 0;
	u8 tmp_buf[128];
	unsigned int bytelen = 0;
	if (datalen > 125)
	{
		printk("%s too big datalen = %d!\n", __func__, datalen);
		return -1;
	}

	tmp_buf[0] = addr;
	bytelen++;

	if (datalen != 0 && pdata != NULL)
	{
		memcpy(&tmp_buf[bytelen], pdata, datalen);
		bytelen += datalen;
	}

	ret = i2c_master_send(client, tmp_buf, bytelen);
	msleep(5);
	return ret;
}


static void  ags04_delaywork_func(struct work_struct *work)
{
	struct ags04_data *ags04 = container_of(work, struct ags04_data, pen_event_work);
	struct i2c_client *client = ags04->client;
	//struct ags04_data *ags04_ts;
	char readbuf;	
	int ret = 0;	
//msleep(3);
if((gpio_get_value(gpio_sensor))==0)
{
	//ags04_ts = i2c_get_clientdata(client);
	ret = ags04_iic_read(client, 0x00, &readbuf, 1);
	if(ret<=0)
		printk("read fail\n");
	//printk("read = %x\n",readbuf);

	if(readbuf==0)
	{
		if(key1_press==1)
		{
			input_event(input_dev, EV_KEY, KEY_1, 0);	
			input_sync(input_dev);
			key1_press=0;
		}
		if(key2_press==1)
		{
			input_event(input_dev, EV_KEY, KEY_2, 0);	
			input_sync(input_dev);
			key2_press=0;
		}
		if(key4_press==1)
		{
			input_event(input_dev, EV_KEY, KEY_4, 0);	
			input_sync(input_dev);
			key4_press=0;
		}
		if(key8_press==1)
		{
			input_event(input_dev, EV_KEY, KEY_8, 0);	
			input_sync(input_dev);
			key8_press=0;
		}
	}
	else
	{
		if((key1_press==0)&&(key2_press==0)&&(key4_press==0)&&(key8_press==0))
		{

			if(readbuf==0x01)
			{
				input_event(input_dev, EV_KEY, KEY_1, 1);	
				input_sync(input_dev);
				key1_press=1;
			}
			else if(readbuf==0x02)
			{

				input_event(input_dev, EV_KEY, KEY_2, 1);	
				input_sync(input_dev);
				key2_press=1;
			}
			else if(readbuf==0x04)
			{
				input_event(input_dev, EV_KEY, KEY_4, 1);	
				input_sync(input_dev);
				key4_press=1;
			}
			else if(readbuf==0x08)
			{
				input_event(input_dev, EV_KEY, KEY_8, 1);	
				input_sync(input_dev);
				key8_press=1;
			}
			else
				printk("error\n");
		}
	}
/*
	if(flag==1)
	{
		printk("flag=1\n");
		input_event(input_dev, EV_KEY, KEY_1, 1);
		input_sync(input_dev);
		flag=0;
	}
	else
	{
		printk("flag=0\n");
		input_event(input_dev, EV_KEY, KEY_1, 0);	
		input_sync(input_dev);
		flag=1;
	}
*/
}
	
	
	enable_irq(client->irq);		
}



static irqreturn_t ags04_ts_interrupt(int irq, void *dev_id)
{
	struct ags04_data *ags04_ts = dev_id;

	disable_irq_nosync(ags04_ts->irq);
	queue_work(ags04_ts->ts_workqueue, &ags04_ts->pen_event_work);

	return IRQ_HANDLED;
}




static void ags04_suspend(struct early_suspend *handler)
{
	struct ags04_data *ts = container_of(handler, struct ags04_data,
						early_suspend);

	dev_dbg(&ts->client->dev, "[ags04]ags04 suspend\n");
	disable_irq(ts->irq);
}

static void ags04_resume(struct early_suspend *handler)
{
	struct ags04_data *ts = container_of(handler, struct ags04_data,
						early_suspend);

	dev_dbg(&ts->client->dev, "[ags04]ags04 resume.\n");
	
	enable_irq(ts->irq);
}

static int  ags04_remove(struct i2c_client *client)
{
	struct ags04_data *ags04_ts;
	ags04_ts = i2c_get_clientdata(client);
	input_unregister_device(ags04_ts->input_dev);
	

	free_irq(client->irq, ags04_ts);

	kfree(ags04_ts);
	i2c_set_clientdata(client, NULL);
	return 0;
}


static int ags04_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct device_node *np = client->dev.of_node;
	struct ags04_data *ags04_ts;
	//struct input_dev *input_dev;
int test_ret;
	char i;
	unsigned long irq_flags;
	int err = 0;
	char writebuf[2]={0};
	int  writelen=1;
	


printk("ags04_probe...aa\n");
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	ags04_ts = kzalloc(sizeof(struct ags04_data), GFP_KERNEL);
	if (!ags04_ts) {
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	i2c_set_clientdata(client, ags04_ts);
	ags04_ts->irq_pin = of_get_named_gpio_flags(np, "sensor-int", 0, (enum of_gpio_flags *)&irq_flags);
	ags04_ts->device_id = 0;	
	ags04_ts->client = client;
	
	gpio_sensor=ags04_ts->irq_pin;
	gpio_direction_input(gpio_sensor);
	ags04_ts->irq=gpio_to_irq(ags04_ts->irq_pin);		//If not defined in client
	client->irq = ags04_ts->irq;	
	err = request_irq(client->irq, ags04_ts_interrupt,
				   IRQF_TRIGGER_FALLING, client->name,
				   ags04_ts);
	if (err != 0) {
		dev_err(&client->dev, "ags04_probe: request irq failed\n");
		goto exit_irq_request_failed;
	}
	disable_irq(client->irq);


	input_dev = input_allocate_device();
	if (!input_dev) {
		err = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}
	ags04_ts->input_dev = input_dev;	
	//set_bit(EV_KEY, input_dev->evbit);
	//input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY);
	//for(i = 0; i < MAX_KEY_CNT; i++)
		//set_bit(KeyCode[i], input_dev->keybit);
for(i = 0; i < MAX_KEY_CNT; i++)
	input_set_capability(input_dev, EV_KEY, KeyCode[i]);	

	input_dev->name = AGS04_NAME;
	input_dev->phys = "ags04/input1";

	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0001;
	input_dev->id.version = 0x0100;
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,
			"ags04_probe: failed to register input device: %s\n",
			dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}
	/*make sure CTP already finish startup process */
	msleep(150);


	INIT_WORK(&ags04_ts->pen_event_work, ags04_delaywork_func);
	ags04_ts->ts_workqueue = create_singlethread_workqueue("ags04_ts");
	if (!ags04_ts->ts_workqueue) {
		err = -ESRCH;
		printk("ts_workqueue  fail..\n");
		goto exit_input_register_device_failed;
	}


//²âÊÔ¹â»ú
char readbuf;
printk("test....\n");
test_ret = ags04_iic_read(client, 0x04, &readbuf, 1);
if(test_ret<=0)
	printk("read fail\n");
else
	printk("test_ret = %d\n",test_ret);
//////////////////////////////////////////////////////////




	writebuf[1]=0;
	err = ags04_iic_write(client, 0x02, writebuf,1);
	if(err<=0)
	{
		printk("iic write fail\n");
		goto exit_input_register_device_failed;
	}

	writebuf[1]=0x06;
	err = ags04_iic_write(client, 0x16, writebuf,1);
	if(err<=0)
	{
		printk("iic write fail\n");
		goto exit_input_register_device_failed;
	}
	
	enable_irq(client->irq);
	printk("ags04_probe sucessfull\n");
	return 0;

exit_input_register_device_failed:
	input_free_device(input_dev);

exit_input_dev_alloc_failed:
	free_irq(client->irq, ags04_ts);

exit_irq_request_failed:
	i2c_set_clientdata(client, NULL);
	kfree(ags04_ts);

exit_alloc_data_failed:
exit_check_functionality_failed:
	return err;
}



static const struct i2c_device_id ags04_id[] = {
	{AGS04_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, ags04_id);
static struct of_device_id ags04_dt_ids[] = {
	{ .compatible = "trip,ags04" },
	{ }
};


static struct i2c_driver ags04_driver = {

	.driver = {
		.name = AGS04_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(ags04_dt_ids),
	},
	.suspend	= ags04_suspend,
	.resume	 	= ags04_resume,
	.probe		= ags04_probe,
	.remove		= ags04_remove,
	.id_table	= ags04_id,
};


static int __init ags04_init(void)
{
	int ret;
printk("ags04_int..\n");
	ret = i2c_add_driver(&ags04_driver);
	if (ret) {
		printk(KERN_WARNING "Adding ags04 driver failed "
		       "(errno = %d)\n", ret);
	} else {
		pr_info("Successfully added driver %s\n",
			ags04_driver.driver.name);
	}
	return ret;
}

static void __exit ags04_exit(void)
{
	i2c_del_driver(&ags04_driver);
}

module_init(ags04_init);
module_exit(ags04_exit);

MODULE_AUTHOR("<dh>");
MODULE_DESCRIPTION("ags04 driver");
MODULE_LICENSE("GPL");
