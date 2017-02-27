/*
 * drivers/input/touchscreen/gslX680.c
 *
 * Copyright (c) 2012 Shanghai Basewin
 *	Guan Yuwei<guanyuwei@basewin.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
#include <linux/fs.h>  
#include <linux/cdev.h>  
#include <asm/uaccess.h>  
#include <linux/slab.h>

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
#include "dm2016.h"
#include "Dm2016.h"
#define DM2016_I2C_NAME    "dm2016"

struct dm2106 *g_dm2016;
struct i2c_client *g_client;
static struct cdev *trip_dm2016_cdev;
static dev_t trip_ndev;

static int trip_major=0;
static int trip_minor=0;

static struct class *trip_dm2016_class;


static ssize_t trip_dm2016_read(struct file *filp, char __user *buf, size_t count, loff_t *f_ops)
{
	char *kbuf;
	int err=0;
	char addr=80;
	

	//buf[0]是地址 
	if (copy_from_user(&addr, buf, 1)) {
		return  -EFAULT;
	}
	printk("aaddr=0x%x\n",addr);

kbuf= kmalloc(count, GFP_KERNEL);
if(kbuf==NULL)
	return -EFAULT;
	//读DM2016
	dm2016_iic_read(g_client, addr, kbuf, count);
	if(addr==0x90)
		msleep(40);
		
	if (copy_to_user(buf, kbuf, count)) {
		kfree(kbuf);
		err = -EFAULT;
		return err;
	}
		
	return count;
}

static ssize_t trip_dm2016_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_ops)
{
	char *kbuf;
	int i=0;
	char addr;
	kbuf = kmalloc(count+1, GFP_KERNEL);
	if(kbuf==NULL)
		return -EFAULT;	
	if (copy_from_user(kbuf, buf, count+1)) {
		kfree(kbuf);
		return  -EFAULT;
	}
	else
	{
		addr=kbuf[0];
		//printk("addr_a=%d\n",addr);
		if(addr==0x90)
		{
			dm2016_iic_write(g_client, addr, kbuf+1, count);
			msleep(40);
		}
		else
		{
			for(i=0;i<count;i++)
			{
				dm2016_iic_write(g_client, addr+i, kbuf+1, 1);
				kbuf++;	
				//printk("kbuf[i]=%d\n",kbuf[i]);
			}
		}
		

	}
	return count;
}

//static ssize_t trip_dm2016_ioctl(struct file *file, unsigned int cmd, long data)
//{
	//u8 read_buf[4]  = {0};
	//u8 write_buf = 0x6f;
/*
	dm2016_iic_read(g_client, 0x00, read_buf, 4);
printk("0x%x, 0x%x, 0x%x, 0x%x\n",read_buf[0],read_buf[1],read_buf[2],read_buf[3]);

	dm2016_iic_write(g_client, 0x00, &write_buf, 1);
printk("write......\n");
	//msleep(5);

	dm2016_iic_read(g_client, 0x00, read_buf, 4);
printk("0x%x, 0x%x, 0x%x, 0x%x\n",read_buf[0],read_buf[1],read_buf[2],read_buf[3]);
*/
	//return 0;
//}


static ssize_t trip_dm2016_ioctl(struct file *file, unsigned int cmd, char __user *buf)
{
	char *kbuf;
	int i=0;
	char addr;
	int result;
	unsigned char srcData[8];
	unsigned char decData[8];
	kbuf = kmalloc(16, GFP_KERNEL);        
	if(kbuf==NULL)
		return -EFAULT;	
	if (copy_from_user(kbuf, buf, 16)) {    //保存从用户层传过来的key,必须跟芯片里面烧录的key一样
		kfree(kbuf);
		return  -EFAULT;
	}

	for(i=0;i<8;i++)    //给srcData  decData赋予随机的初始值
	{
		srcData[i]= i;
		decData[i]= i;
	}

	if (0 != EDesEn_Crypt(decData, kbuf))   //加密
		return -1;
/*
for(i=0;i<8;i++)
	printk(KERN_ERR "decData[i]=%d\n", decData[i]);
*/
	
	dm2016_iic_write(g_client, 0x90, decData, 8);
	dm2016_iic_read(g_client, 0x90, decData, 8);


	result = 0;
	for (i=0; i<8; i++)
	{	
		if (srcData[i] != decData[i])
			result = 1;		
	}
/*

for(i=0;i<8;i++)
	printk(KERN_ERR "decData[i]=%d\n", decData[i]);
*/
/*
	else
	{
		for(i=0;i<16;i++)
			printk(KERN_ERR "kbuf[i]=%d\n",i,kbuf[i]);
	}
*/

	
/*
	dm2016_iic_read(g_client, 0x00, read_buf, 4);
printk("0x%x, 0x%x, 0x%x, 0x%x\n",read_buf[0],read_buf[1],read_buf[2],read_buf[3]);

	dm2016_iic_write(g_client, 0x00, &write_buf, 1);
printk("write......\n");
	//msleep(5);

	dm2016_iic_read(g_client, 0x00, read_buf, 4);
printk("0x%x, 0x%x, 0x%x, 0x%x\n",read_buf[0],read_buf[1],read_buf[2],read_buf[3]);
*/
	return result;
}



struct file_operations trip_dm2016_ops = {
	.read = trip_dm2016_read,
	.write = trip_dm2016_write,
	.unlocked_ioctl = trip_dm2016_ioctl,
};


static int dm2016_iic_write(struct i2c_client *client, u8 addr, u8 *pdata, int datalen)
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

static int dm2016_iic_readbyte(struct i2c_client *client, u8 addr, u8 *pdata)
{
	int ret = 0;
	ret = dm2016_iic_write(client, addr, NULL, 0);
	if (ret < 0)
	{
		printk("%s set data address fail!\n", __func__);
		return ret;
	}

	return i2c_master_recv(client, pdata, 1);
}
static int dm2016_iic_read(struct i2c_client *client, u8 addr, u8 *pdata, unsigned int datalen)
{
	int ret = 0;
	int i = 0;

	if (datalen > 126)
	{
		printk("%s too big datalen = %d!\n", __func__, datalen);
		return -1;
	}
	for(i=0; i<datalen; i++){
		ret = dm2016_iic_readbyte(client, addr+i, pdata+i);
		if(ret < 0)
			return ret;
	}
	return ret;
}


static int  dm2016_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	//struct dm2106 *dm2016;
	int err;
	u8 read_buf[4]  = {0};
	u8 write_buf = 0xcc;
	printk("dm2016_pro_rr\n");
	printk("client->addr = 0x%x\n",client->addr);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "I2C functionality not supported\n");
		return -ENODEV;
	}
/*	
	//dm2016->client = client;
	//dm2016->cdev = trip_dm2016_cdev;
	
	dm2016_iic_read(client, 0x00, read_buf, 4);
printk("0x%x, 0x%x, 0x%x, 0x%x\n",read_buf[0],read_buf[1],read_buf[2],read_buf[3]);

	dm2016_iic_write(client, 0x00, &write_buf, 1);
printk("write......\n");
	//msleep(5);

	dm2016_iic_read(client, 0x00, read_buf, 4);
printk("0x%x, 0x%x, 0x%x, 0x%x\n",read_buf[0],read_buf[1],read_buf[2],read_buf[3]);
	
*/

g_client = kzalloc(sizeof(*g_client), GFP_KERNEL);
g_client = client;
	printk("Initializing trip_dm2016_cdev.\n");
	err = alloc_chrdev_region(&trip_ndev, 0, 1, "trip_dm2016");
	if(err < 0)
	{
		printk("alloc trip_ndev fail.\n");
		return err;	
	}
	trip_major = MAJOR(trip_ndev);
	trip_minor = MINOR(trip_ndev);
	printk("trip_major = %d,  trip_minor = %d\n", MAJOR(trip_ndev), MINOR(trip_ndev));

	trip_dm2016_cdev = cdev_alloc();
	cdev_init(trip_dm2016_cdev, &trip_dm2016_ops);
	trip_dm2016_cdev->owner = THIS_MODULE;
	err = cdev_add(trip_dm2016_cdev, trip_ndev, 1);
	if(err<0)
	{
		printk("cdev_add trip_led_cdev fail.\n");
		return err;
	}
	trip_dm2016_class = class_create(THIS_MODULE, "trip_dm2016_class");
	device_create(trip_dm2016_class, NULL, trip_ndev, NULL, "trip_dm2016");
/*	
write_buf=0xdd;

dm2016_iic_write(g_client, 0x00, &write_buf, 1);
printk("write....1..\n");
	//msleep(5);
dm2016_iic_read(g_client, 0x00, read_buf, 4);
printk("0x%x, 0x%x, 0x%x, 0x%x\n",read_buf[0],read_buf[1],read_buf[2],read_buf[3]);
*/
	return 0;	

}


static int dm2016_suspend(struct i2c_client *client, pm_message_t mesg)
{
	return 0;
}

static int dm2016_resume(struct i2c_client *client)
{
	return 0;
}

static int  dm2016_remove(struct i2c_client *client)
{
	return 0;
}


static const struct i2c_device_id dm2016_id[] = {
	{DM2016_I2C_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, dm2016_id);

static struct of_device_id dm2016_dt_ids[] = {
	{ .compatible = "rockchip,dm2016_trip" },
	{ }
};

static struct i2c_driver dm2016_driver = {
	.driver = {
		.name = DM2016_I2C_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(dm2016_dt_ids),
	},
	.suspend	= dm2016_suspend,
	.resume	= dm2016_resume,
	.probe		= dm2016_probe,
	.remove		= dm2016_remove,
	.id_table	= dm2016_id,
};


static int __init dm2016_init(void)
{
    	int ret;
	
	ret = i2c_add_driver(&dm2016_driver);
	printk("==dm2016_init== ret=%d\n",ret);
	return ret;
}
static void __exit dm2016_exit(void)
{
	printk("==dm2016_exit==\n");

trip_ndev=MKDEV(trip_major, trip_minor);
device_destroy(trip_dm2016_class, trip_ndev);
class_destroy(trip_dm2016_class);
cdev_del(trip_dm2016_cdev);
unregister_chrdev_region(trip_ndev,1);


	i2c_del_driver(&dm2016_driver);
	return;
}


module_init(dm2016_init);
module_exit(dm2016_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("dm2016 driver");
MODULE_AUTHOR("tom");
MODULE_ALIAS("platform:dm2016");
