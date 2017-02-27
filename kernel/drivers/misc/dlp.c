#include <linux/miscdevice.h>
#include <linux/input.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/of_platform.h>

// zhaojun i2c porting 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <asm/io.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/input-polldev.h>
#include <linux/ioport.h>
#include <linux/gpio_keys.h>
#include <linux/miscdevice.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include "dlp.h"

static struct i2c_client * dlp_client = NULL ;

#define QVGA_PORTRAIT          0        // (240h*320v)
#define QVGA_LANDSCAPE         1        // (320h*240v)
#define QWVGA_LANDSCAPE        3        // (427h*240v)
#define VGA_PORTRAIT_2_3       4        // (430h*640v)
#define VGA_LANDSCAPE_3_2      5        // (640h*430v)
#define VGA_PORTRAIT           6        // (480h*640v)
#define VGA_LANDSCAPE          7        // (640h*480v)
#define WVGA_720_PORTRAIT      8        // (480h*720v)
#define WVGA_720_LANDSCAPE     9        // (720h*480v)
#define WVGA_752_PORTRAIT     10        // (480h*752v)
#define WVGA_752_LANDSCAPE    11        // (752h*480v)
#define WVGA_800_PORTRAIT     12        // (480h*800v)
#define WVGA_800_LANDSCAPE    13        // (800h*480v)
#define WVGA_852_PORTRAIT     14        // (480h*852v)
#define WVGA_852_LANDSCAPE    15        // (852h*480v)
#define WVGA_853_PORTRAIT     16        // (480h*853v)
#define WVGA_853_LANDSCAPE    17        // (853h*480v)
#define WVGA_854_PORTRAIT     18        // (480h*854v)
#define WVGA_854_LANDSCAPE    19        // (854h*480v)
#define WVGA_864_PORTRAIT     20        // (480h*864v)
#define WVGA_864_LANDSCAPE    21        // (864h*480v)
#define NTSC_LANDSCAPE        23        // (720h*240v)
#define PAL_LANDSCAPE         25        // (720h*288v)
#define nHD_Landscape         27        // (640h*360v)
#define VGA_DMD_OPTICAL_TEST  33        // (456h*684v)
#define WVGA_DMD_OPTICAL_TEST 35        // (608h*684v)


int i2c_master_send_henry(const struct i2c_client *client, const char *buf, int count)
{
        int ret;
        struct i2c_adapter *adap = client->adapter;
        struct i2c_msg msg;

        msg.addr = client->addr;
 //       msg.flags = client->flags & I2C_M_TEN;
        msg.flags = client->flags; 
        msg.len = count;
        msg.buf = (char *)buf;
	#ifdef CONFIG_I2C_ROCKCHIP_COMPAT
			        msg.scl_rate = 100 * 1000;
	#endif
     ret = i2c_transfer(adap, &msg, 1);

        /*
         * If everything went ok (i.e. 1 msg transmitted), return #bytes
         * transmitted, else error code.
         */
        return (ret == 1) ? count : ret;
}

void dlp_i2c_send(char addr, unsigned int  param)
{
        int ret = 0 ;
        unsigned char buf[5] = {0} ;

//0x3A  
        buf[0] = addr ;
        buf[1] = (unsigned char)((param >> 24) & 0xFF) ;
        buf[2] = (unsigned char)((param >> 16) & 0xFF) ;
        buf[3] = (unsigned char)((param >> 8) & 0xFF) ; 
        buf[4] = (unsigned char)(param & 0xFF)  ;

        ret = i2c_master_send_henry(dlp_client,buf,5);
        if(ret < 0)
                printk(KERN_ERR "i2c_master_send error \n");
        printk(KERN_ERR "henry i2c_master_send return value=%d in \n",ret);
}


void dpp2607_write_ICP8051(unsigned int cmd, unsigned int param)
{
        dlp_i2c_send(0x3A, 1);
        dlp_i2c_send(0x39, param);
        dlp_i2c_send(0x38, cmd);
        mdelay(200);
        //Delay(200ms);
}


// a) set the current of R/G/B channel
/*      
   When setting the Current_value to 350,the power of OPD01 is 1W
   When setting the Current_value to 500,the power of OPD01 is 1.5W
   When setting the Current_value to 900,the power of OPD01 is 3W
   */
        
void dlp_set_rgb_channel(unsigned int current_value)
{
        dlp_i2c_send(0x12, current_value);
        dlp_i2c_send(0x13, current_value);
        dlp_i2c_send(0x14, current_value);
        dpp2607_write_ICP8051(0xd3, 0);
}


// b) select the sequence

/*
   The sequence_num is the number of the sequence selected.
   If you want to set OPD01 to high power (3W) when the frame rate of input
   source is 60Hz, please select the sequence 8.
   If you want to set OPD01 to middle power (1.5W) when the frame rate of input
   source is 60Hz, please select the sequence 9.
   If you want to set OPD01 to low power (1W) when the frame rate of input
   source is 60Hz, please select the sequence 10.
   If you want to set OPD01 to high power (3W) when the frame rate of input
   source is 50Hz, please select the sequence 0.
   If you want to set OPD01 to middle power (1.5W) when the frame rate of input
   source is 50Hz, please select the sequence 1.
   If you want to set OPD01 to low power (1W) when the frame rate of input
   source is 50Hz, please select the sequence 2.
*/

/* 8, 9, 10, 0, 1, 2 */
        
void dlp_select_seq(unsigned int seq_num)
{       
        dlp_i2c_send(0xa6,1);
        //Dpp2607_write_ICP8051(0xC1, sequence_num);
        dpp2607_write_ICP8051(0xC1, seq_num);
//      Write_dpp2600_i2c(0x36, 0xa6,0);  //dpp2600 在dpp2607中是否需要这个序列 
        dlp_i2c_send(0xa6,0);  //dpp2600 在dpp2607中是否需要这个序列 
}


void dlp_set_resolution(unsigned int arg)
{
        dlp_i2c_send(0xa6, 1);
        dlp_i2c_send(0x0d, 2);
        //set the pixel data format to RGB888
        dlp_i2c_send(0x0c, arg);
        //set the resolution to 864*480
        dlp_i2c_send(0x0b, 0);
        dlp_i2c_send(0x1e, 0);
        //delay10ms;
        mdelay(10);
        dlp_i2c_send(0xa6, 0);
}


// d) select the I/F interface RGB888/RGB666/BT656/MCU I/F   
void dlp_select_if(unsigned int arg)
{
        dlp_i2c_send(0xa6,1);
        dlp_i2c_send(0x82, 0);
        dlp_i2c_send(0xaf, 0x10);
        dlp_i2c_send(0x0d, 2);  // henry: select RGB888 
        //dlp_i2c_send(client, 0x0d, mode);
        // if selectiong RGB888, pls set mode to 2  
        dlp_i2c_send(0x0c, arg);
        //dlp_i2c_send(0x0c, WVGA_854_LANDSCAPE);
        dlp_i2c_send(0x0b, 0);
        dlp_i2c_send(0xC3, 0);
        dlp_i2c_send(0x82, 1);
        //delay500ms; //延时 500ms
        mdelay(500);
        dlp_i2c_send(0xa6,0);
}

        
// e)flip 
//normal
void dlp_flip_normal(void)
{
        dlp_i2c_send(0x0f, 0);
        dlp_i2c_send(0x10, 0);
}       
        
//Mirror horizontal
void dlp_flip_horizontal(void)
{
        dlp_i2c_send(0x0f, 0);
        dlp_i2c_send(0x10, 1);
}


void dlp_flip_vertical(void)
{
        dlp_i2c_send(0x0f, 1);
        dlp_i2c_send(0x10, 0);
}

   
static ssize_t ctl1_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{

        int value  = 0; 
        printk(KERN_ERR " henry invoke ctl1_show  \n");
        return sprintf(buf, "%d \n",!!value);
}
        
static ssize_t ctl1_store(struct kobject *kobj, struct kobj_attribute *attr, const char * buf, size_t n)
{       
	unsigned long value = simple_strtoul(buf, NULL, 10);
	switch(value)
	{
		case 0:  dlp_flip_normal() ; printk(KERN_ERR "henry flip normal \n"); break;
		case 1:  dlp_flip_horizontal(); printk(KERN_ERR "henry flip horizontal \n"); break;
		case 2:  dlp_flip_vertical() ; printk(KERN_ERR "henry flip vertical \n"); break;
		default: break;
	}
	
        printk(KERN_ERR "henry will flip  \n");
        return n;
}       

static ssize_t ctl2_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{

        int value  = 0; 
        printk(KERN_ERR " henry invoke ctl2_show  \n");
        return sprintf(buf, "%d \n",!!value);
}

static ssize_t ctl2_store(struct kobject *kobj, struct kobj_attribute *attr, const char * buf, size_t n)
{       
	unsigned long value = simple_strtoul(buf, NULL, 10);
	switch(value)
	{
		case 8: dlp_select_seq(8) ; break;
		case 9: dlp_select_seq(9) ; break;
		case 10: dlp_select_seq(10) ; break;
		case 0:  dlp_select_seq(0) ; break;
		case 1:  dlp_select_seq(1) ; break;
		case 2:  dlp_select_seq(2) ; break;
		default: break;
	}
	printk(KERN_ERR " henry invoke ctl1_store set sequence\n");
	return n;
}       


#define dlp_attr(_name) \
        static struct kobj_attribute _name##_attr = {   \
                .attr   = {                             \
                        .name = __stringify(_name),     \
                        .mode = 0664,                   \
                },                                      \
                .show   = _name##_show,                 \
                .store  = _name##_store,                \
        }

dlp_attr(ctl1);
dlp_attr(ctl2);

static struct attribute * dlp[] = {
        &ctl1_attr.attr,
        &ctl2_attr.attr,
        NULL,
};

static struct attribute_group attr_group = {
        .attrs = dlp,
};

static struct kobject *dlp_kobj = NULL ;

static int dlp_open(struct inode *inode, struct file *file) 
{
	printk(KERN_INFO "dlp_open success!!\n");
	return 0;
}

static int dlp_release(struct inode *inode, struct file *file) 
{
	printk(KERN_INFO "dlp_release success!!\n");
	return 0;
}


static long dlp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	printk(KERN_INFO "info:arg:%d\n",arg);
	switch(cmd) {
	case DLP_NORMAL:
		dlp_flip_normal();
		printk(KERN_INFO "info:dlp_flip_normal\n");
		break;

	case DLP_HORIZONTAL:
		 dlp_flip_horizontal();
		 printk(KERN_INFO "info:dlp_flip_horizontal\n");
		 break;

	case DLP_VERTICAL:	
			dlp_flip_vertical() ;
		printk(KERN_INFO "info:dlp_flip_vertical\n");
		break;
	case DLP_SEQ:
		dlp_select_seq(arg) ;
		printk(KERN_INFO "info:dlp_select_seq\n");
		break;
	case DLP_IF:
		dlp_select_if(arg);
		printk(KERN_INFO "info:dlp_select_if\n");
		break;	
	case DLP_RESOLUTION:
		dlp_set_resolution( arg);
		printk(KERN_INFO "info:dlp_set_resolution\n");
		break;
	case DLP_CHANNEL:
		dlp_set_rgb_channel(arg);
		printk(KERN_INFO "info:dlp_set_rgb_channel\n");
		break;	
	default:
		printk("invalid argument\n");
		return -EINVAL;
	}

	return 0;
}


struct file_operations dlp_fops = {
	.owner		= THIS_MODULE,
	.open		= dlp_open,
	.release	= dlp_release,
	.unlocked_ioctl		= dlp_ioctl,
};

static struct miscdevice dlp_dev ={
    .minor = DLP_MAJOR,
    .name  = "dlp",
    .fops  = &dlp_fops,
};
static int i2c_driver_dlp_probe(struct i2c_client * client, const struct i2c_device_id* id)
{

		printk(KERN_INFO " ENTER i2c_driver_dlp_probe \n");
		int ret = 0;
		int i ;
		int io1, io2, io3, io4, io5, io6;
		enum of_gpio_flags flag1, flag2, flag3, flag4, flag5, flag6;
        dlp_client = client;
		struct device_node *np = client->dev.of_node;
		
		printk(KERN_ERR "henry will exec  of_get_named_gpio_flags  \n");
		
		io1 = of_get_named_gpio_flags(np,"dlp-gpios1",0,&flag1);
		
		printk(KERN_ERR "henry get gpio id successful io1=%d \n",io1);
		
		io2 = of_get_named_gpio_flags(np,"dlp-gpios2",0,&flag2)  ;
		printk(KERN_ERR "henry get gpio id successful io2=%d \n",io2);
		io3 = of_get_named_gpio_flags(np,"dlp-gpios3",0,&flag3)  ;
		printk(KERN_ERR "henry get gpio id successful io3=%d \n",io3);
		io4 = of_get_named_gpio_flags(np,"dlp-gpios4",0,&flag4)  ;
		printk(KERN_ERR "henry get gpio id successful io4=%d \n",io4);
//		io5 = of_get_named_gpio_flags(np,"dlp-gpios5",0,&flag5)  ;
//		printk(KERN_ERR "henry get gpio id successful io5=%d \n",io5);

//		io6 = of_get_named_gpio_flags(np,"dlp-gpios6",0,&flag6)  ;
		
//		printk(KERN_ERR "henry get gpio id successful io6=%d \n",io6);
		if(!gpio_is_valid(io1)){ 
			printk("invalid led-gpios: %d\n",io1) ; 
			return -1 ;
		}
		if(!gpio_is_valid(io2)){ 
			printk("invalid led-gpios: %d\n",io2) ; 
			return -1 ;
		}
		if(!gpio_is_valid(io3)){ 
			printk("invalid led-gpios: %d\n",io3) ; 
			return -1 ;
		}
		if(!gpio_is_valid(io4)){ 
			printk("invalid led-gpios: %d\n",io4) ; 
			return -1 ;
		}
		if(!gpio_is_valid(io5)){ 
			printk("invalid led-gpios: %d\n",io5) ; 
			return -1 ;
		}
#if 0
		if(!gpio_is_valid(io6)){ 
			printk("invalid led-gpios: %d\n",io6) ; 
			return -1 ;
		}

		printk(KERN_ERR "henry will request_qpio io1 \n");
		if(gpio_request(io1,"dlp_gpio1")){
			printk(KERN_ERR "io1 request failed!\n");
			return ret ;
		} 
		printk(KERN_ERR "henry will set direction \n");
		ret = gpio_direction_output(io1,1);
		printk(KERN_ERR "henry gpio_direction_output io1 return %d \n",ret);
		printk(KERN_ERR "henry will set io1 value 1 \n");
		gpio_set_value(io1,1) ;
		//gpio_set_value(io1,0) ;


		printk(KERN_ERR "henry will request_qpio io2 \n");
		if(gpio_request(io2,"dlp_gpio2")){
			printk(KERN_ERR "io2 request failed!\n");
			return ret ;
		} 
		printk(KERN_ERR "henry will set direction \n");
		ret = gpio_direction_output(io2,1);
		printk(KERN_ERR "henry gpio_direction_output io2 return %d \n",ret);
		printk(KERN_ERR "henry will set value 1 \n");
		gpio_set_value(io2,1) ;
		//gpio_set_value(io2,0) ;


		printk(KERN_ERR "henry will request_qpio io3 \n");
		if(gpio_request(io3,"dlp_gpio3")){
			printk(KERN_ERR "io3 request failed!\n");
			return ret ;
		} 
		printk(KERN_ERR "henry will set direction \n");
		ret = gpio_direction_output(io3,1);
		printk(KERN_ERR "henry gpio_direction_output io3 return %d \n",ret);
		printk(KERN_ERR "henry will set value 1 \n");
		gpio_set_value(io3,1) ;
		//gpio_set_value(io3,0) ;

		if(gpio_request(io4,"dlp_gpio4")){
			printk(KERN_ERR "io4 request failed!\n");
			return ret ;
		} 
		printk(KERN_ERR "henry will set direction \n");
		ret = gpio_direction_output(io4,1);
		printk(KERN_ERR "henry gpio_direction_output io4 return %d \n",ret);
		printk(KERN_ERR "henry will set value 1 \n");
		gpio_set_value(io4,1) ;
		//gpio_set_value(io4,0) ;
#endif	
#if 0 
		printk(KERN_ERR "henry will request_qpio io5\n");
		if(gpio_request(io5,"dlp_gpio5")){
			printk(KERN_ERR "io5 request failed!\n");
			return ret ;
		} 
		printk(KERN_ERR "henry will set direction \n");
		ret = gpio_direction_output(io5,1);
		printk(KERN_ERR "henry gpio_direction_output io5 return %d \n",ret);
		printk(KERN_ERR "henry will set value 1 \n");
		gpio_set_value(io5,1) ;
		//gpio_set_value(io5,0) ;
		
		printk(KERN_ERR "henry will request_qpio io6\n");
		if(gpio_request(io6,"dlp_gpio6")){
			printk(KERN_ERR "io6 request failed!\n");
			return ret ;
		} 
		printk(KERN_ERR "henry will set direction \n");
		ret = gpio_direction_input(io6);
		printk(KERN_ERR "henry gpio_direction_output io6 return %d \n",ret);
		printk(KERN_ERR "henry will set value 1 \n");
	
		printk(KERN_ERR "henry initial dlp gpio finished \n");
#endif
		/*misc node */
		ret = misc_register(&dlp_dev);
		if(ret)
		{
			printk(KERN_ERR "cannot register miscdev (%d)\n", ret);
			goto err_misc_register;
		}


		/*sys node*/
        if ( (dlp_kobj = kobject_create_and_add("dlp", NULL)) == NULL ) {  //create top node
                printk("[%s]: subsystem_register failed\n", __func__);
                ret = -ENOMEM;
        }
                
        if( ret = sysfs_create_group(dlp_kobj, &attr_group) ) {
                printk("[%s] sysfs_create_group failed\n", __func__);
        }
		return 0;

		err_misc_register:
			misc_deregister(&dlp_dev);

       return ret;
}   

static int i2c_driver_dlp_remove(struct i2c_client * client, const struct i2c_device_id* id)
{ 
		printk(KERN_ERR "enter in i2c_driver_dlp_remove\n");
       	return 0; 
}
        
static const struct i2c_device_id dlp_ts_id[] = {
        {"edu_dlp", 0},
        {}
};
MODULE_DEVICE_TABLE(i2c, dlp_ts_id);
static struct of_device_id dlp_ts_dt_ids[] = {
        { .compatible = "dlp,edu" },
        { }
};

static struct i2c_driver dlp_ts_driver = {
        .driver = {
                .name = "edu_dlp",
                .owner = THIS_MODULE,
                .of_match_table = of_match_ptr(dlp_ts_dt_ids),
        },
        .probe          = i2c_driver_dlp_probe,
        .remove         = i2c_driver_dlp_remove,
        .id_table       = dlp_ts_id,
};

          
static int __init dlps_init(void)
{
	printk(KERN_INFO "henry enter i2c_driver_dlp_init \n");
	
		   i2c_add_driver(&dlp_ts_driver);
		   return 0;
}


static void __exit dlps_exit(void)
{
	printk(KERN_INFO "dlps_exit");
	i2c_del_driver(&dlp_ts_driver);
	
}


module_init(dlps_init);
module_exit(dlps_exit);

MODULE_DESCRIPTION("dlp gpio Driver") ;
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:dlp gpio drivers") ;

