#include <linux/fs.h>  
#include <linux/cdev.h>  
#include <asm/uaccess.h>  


#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/leds.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/pinctrl/consumer.h>
#include <linux/err.h>
#include <linux/version.h>   
#include <linux/proc_fs.h>   
#include <linux/fb.h>
#include <linux/rk_fb.h>
#include <linux/display-sys.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <dt-bindings/rkfb/rk_fb.h>
#endif

#include "leds-firefly.h"


/*   led     */
static struct cdev *trip_led_cdev;
static dev_t trip_ndev;

static int trip_major=0;
static int trip_minor=0;

static struct class *trip_led_class;




static ssize_t trip_led_read(struct file *filp, char *buf, size_t count, loff_t *f_ops)
{
	char *kbuf;
	int val=0, val_1=0; 

kbuf= kmalloc(1, GFP_KERNEL);	    //	申请内存
if(kbuf==NULL)
	return -EFAULT;
	
	gpio_direction_input(g_board_key);
	val = gpio_get_value(g_board_key);
	msleep(20);
	val_1=gpio_get_value(g_board_key);

	if((val==val)&&(val==0))		
		*kbuf=KEY_DOWN;		
	else
		*kbuf=KEY_UP;	

	if (copy_to_user(buf, kbuf, 1)) {
		kfree(kbuf);
		return -EFAULT;
	}
	
	return count;
}

static ssize_t trip_led_write(struct file *filp, char *buf, size_t count, loff_t *f_ops)
{
	return count;
}

static ssize_t trip_led_ioctl(struct file *file, unsigned int cmd, long gpio_nr)
{
	int val=0;
	//printk("trip_led_ioctl..cmd=%d   gpio_nr=%d\n",cmd,gpio_nr);

	

	switch(cmd)
	{
		case CMD_ON:
			if(gpio_nr == GPIO_0)
				gpio_direction_output(g_gpio_0, 1);
			else if(gpio_nr == GPIO_1)
				gpio_direction_output(g_gpio_1, 1);
			else if(gpio_nr == GPIO_2)
				gpio_direction_output(g_gpio_2, 1);
			else if(gpio_nr == GPIO_3)			
				gpio_direction_output(g_gpio_3, 1);			
			else if(gpio_nr == GPIO_4)
				gpio_direction_output(g_gpio_4, 1);
			else if(gpio_nr == GPIO_5)
				gpio_direction_output(g_gpio_5, 1);
			else if(gpio_nr == GPIO_6)
				gpio_direction_output(g_gpio_6, 1);
			else if(gpio_nr == GPIO_7)
				gpio_direction_output(g_gpio_7, 1);
			else
				break;
			break;

		case CMD_OFF:
			if(gpio_nr == GPIO_0)
				gpio_direction_output(g_gpio_0, 0);
			else if(gpio_nr == GPIO_1)
				gpio_direction_output(g_gpio_1, 0);
			else if(gpio_nr == GPIO_2)
				gpio_direction_output(g_gpio_2, 0);
			else if(gpio_nr == GPIO_3)
				gpio_direction_output(g_gpio_3, 0);
			else if(gpio_nr == GPIO_4)
				gpio_direction_output(g_gpio_4, 0);
			else if(gpio_nr == GPIO_5)
				gpio_direction_output(g_gpio_5, 0);
			else if(gpio_nr == GPIO_6)
				gpio_direction_output(g_gpio_6, 0);
			else if(gpio_nr == GPIO_7)
				gpio_direction_output(g_gpio_7, 0);
			else
				break;
			break;
		
		case CMD_KEY:
			gpio_direction_input(g_board_key);
			val = gpio_get_value(g_board_key);
			
			break;
		default:
			break;
	}

	return 0;
}

struct file_operations trip_led_ops = {
	.read = trip_led_read,
	.write = trip_led_write,
	.unlocked_ioctl = trip_led_ioctl,
};

/*led end*/







struct firefly_led_info led_info;

#define STATE_OFF   0
#define STATE_ON    1
#define STATE_FLASH 2
struct firefly_led_timer {
    struct timer_list timer1;
	int    state1;
	int    on_delay1;
	int    off_delay1;
	int    value1;	
#ifndef CONFIG_FIREFLY_POWER_LED
    struct timer_list timer2;
	int    state2;
	int    on_delay2;
	int    off_delay2;
	int    value2;	
#endif
};

struct firefly_led_timer led_timer;

/*
static void led1_timer_fuc(unsigned long _data)
{
    if(led_timer.state1 == STATE_FLASH) {
        gpio_direction_output(led_info.work_gpio,  led_timer.value1);
        if(led_timer.value1 == led_info.work_enable_value) {
            mod_timer(&led_timer.timer1,jiffies + msecs_to_jiffies(led_timer.on_delay1));
        } else {
            mod_timer(&led_timer.timer1,jiffies + msecs_to_jiffies(led_timer.off_delay1));
        }
        led_timer.value1 = !(led_timer.value1);
    }
}
*/
/*
#ifndef CONFIG_FIREFLY_POWER_LED
static void led2_timer_fuc(unsigned long _data)
{
    if(led_timer.state2 == STATE_FLASH) {
        gpio_direction_output(led_info.power_gpio, led_timer.value2);
        if(led_timer.value2 == led_info.power_enable_value) {
            mod_timer(&led_timer.timer2,jiffies + msecs_to_jiffies(led_timer.on_delay2));
        } else {
            mod_timer(&led_timer.timer2,jiffies + msecs_to_jiffies(led_timer.off_delay2));
        }
        led_timer.value2 = !(led_timer.value2);
    }
}
#endif
*/

#define TIME_STRING_LEN 5
#define TIME_MAX 10000
#define TIME_MIN 0 
#define TIME_BACE 1 // bace 1ms
#define TIMER_ERROR (-1)

#define LED_STRING_LEN 5

int get_timers(char *cmd, int *timer) 
{
    int len1= 0,len2= 0, i = 0, next = 0;
    
    while(cmd[i] != 0) {
        if(cmd[i] >= '0' && cmd[i] <= '9')
        {
            if(next != 0) {
                len2++;
            } else {
                len1++;
            }
        } else if (cmd[i] = ' ') {
            next = i + 1;
        } else {
            return -1;
        }
        i++ ;
    }
    
    if(len1 > 0 && len2 > 0) {
        if(len1 > TIME_STRING_LEN) len1 = TIME_STRING_LEN; // limit timer
        timer[0] = 0;
        for(i = 0 ; i < len1; i++) {
            timer[0] *= 10;
            timer[0] += (cmd[i] - '0');
            if(timer[0] > TIME_MAX) timer[0] = TIME_MAX;
        }
        if(len2 > TIME_STRING_LEN) len2 = TIME_STRING_LEN; // limit timer
        timer[1] = 0;
        for(i = next ; i <  (next + len2); i++) {
            timer[1] *= 10;
            timer[1] += (cmd[i] - '0');
            if(timer[1] > TIME_MAX) timer[1] = TIME_MAX;
        }
        return 0;
    }
    return -1;
}

void firefly_leds_ctrl(char *cmd) 
{
    int timer[2], ret;
    if(strncmp(cmd,"LED1 ",LED_STRING_LEN) == 0) {
        ret = get_timers(&cmd[LED_STRING_LEN],timer);
        if(ret != TIMER_ERROR) {
            if(timer[0] == TIME_MIN) {
                led_timer.state1 = STATE_OFF;
                gpio_direction_output(led_info.work_gpio, !(led_info.work_enable_value));
            } else if(timer[1] == TIME_MIN) {
                led_timer.state1 = STATE_ON;
                gpio_direction_output(led_info.work_gpio, led_info.work_enable_value);
            } else {
                led_timer.state1 = STATE_FLASH;
                led_timer.on_delay1 = timer[0] * TIME_BACE;
                led_timer.off_delay1 = timer[1] * TIME_BACE;
                if(led_timer.value1 != 1 && led_timer.value1 != 0) {
                    led_timer.value1 = led_info.work_enable_value;
                }
                if(led_timer.value1 == led_info.work_enable_value) {
                    mod_timer(&led_timer.timer1,jiffies + msecs_to_jiffies(led_timer.on_delay1));
                } else {
                    mod_timer(&led_timer.timer1,jiffies + msecs_to_jiffies(led_timer.off_delay1));
                }
            }
        }
    }
#ifndef CONFIG_FIREFLY_POWER_LED
    else if(strncmp(cmd,"LED2 ",LED_STRING_LEN) == 0) {
        ret = get_timers(&cmd[LED_STRING_LEN],timer);
        if(ret != TIMER_ERROR) {
            if(timer[0] == TIME_MIN) {
                led_timer.state2 = STATE_OFF;
                gpio_direction_output(led_info.power_gpio, !(led_info.power_enable_value));
            } else if(timer[1] == TIME_MIN) {
                led_timer.state2 = STATE_ON;
                gpio_direction_output(led_info.power_gpio, led_info.power_enable_value);
            } else if(timer != TIMER_ERROR) {
                led_timer.state2 = STATE_FLASH;
                led_timer.on_delay2 = timer[0] * TIME_BACE;
                led_timer.off_delay2 = timer[1] * TIME_BACE;
                if(led_timer.value2 != 1 && led_timer.value2 != 0) {
                    led_timer.value2 = led_info.power_enable_value;
                }
                if(led_timer.value2 == led_info.power_enable_value) {
                    mod_timer(&led_timer.timer2,jiffies + msecs_to_jiffies(led_timer.on_delay2));
                } else {
                    mod_timer(&led_timer.timer2,jiffies + msecs_to_jiffies(led_timer.off_delay2));
                }
            }
       }
    }
#endif
}

#define USER_PATH "driver/firefly-leds" 
static struct proc_dir_entry *firefly_led_entry; 

#define INFO_PROC "LED Control: power/work on/off"


int proc_write_information(struct file *file, const char *buffer, unsigned long count, void *data) 
{ 
    #define cmd_len 16
    char cmd[cmd_len];
    int len = count,i;
    
    memset(cmd,0,cmd_len);
    if(len >= cmd_len) len = cmd_len - 1;
    memcpy(cmd,buffer,len);
    
    for(i = 0; i < len; i++) {  // 杩囨护闈濧SCII绗﹀彿
        if(cmd[i] < 0x20 || cmd[i] >= 0x7F)  cmd[i] = 0;
    }

    firefly_leds_ctrl(cmd);
    
    return count;
} 

static int firefly_led_proc_show(struct seq_file *seq, void *v)
{
#ifndef CONFIG_FIREFLY_POWER_LED
    char *led_string = "LED1 LED2";
#else
    char *led_string = "LED1";
#endif
	seq_printf(seq,"%s \n",led_string);

	return  0;
}

static int firefly_led_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, firefly_led_proc_show, NULL);
}

static const struct file_operations firefly_led_proc_fops = {
	.owner		= THIS_MODULE, 
	.open		= firefly_led_proc_open,
	.read		= seq_read,
	.write		= proc_write_information,
}; 


static int firefly_led_probe(struct platform_device *pdev)
{
    int err;
    int ret = -1;
    int gpio, rc,flag;
    unsigned long ddata;
	struct device_node *led_node = pdev->dev.of_node;

    led_info.pdev = pdev;
/*
#ifndef CONFIG_FIREFLY_POWER_LED
printk("led-power...\n");
	gpio = of_get_named_gpio_flags(led_node,"led-power", 0,&flag);
	if (!gpio_is_valid(gpio)){
		printk("invalid led-power: %d\n",gpio);
		return -1;
	} 
    ret = gpio_request(gpio, "led_power");
	if (ret != 0) {
		gpio_free(gpio);
		ret = -EIO;
		goto failed_1;
	}
	led_info.power_gpio = gpio;
	led_info.power_enable_value = (flag == OF_GPIO_ACTIVE_LOW)? 0:1;
	//gpio_direction_output(led_info.power_gpio, !(led_info.power_enable_value));
	gpio_direction_output(led_info.power_gpio, 0);
#endif	
	gpio = of_get_named_gpio_flags(led_node,"led-work", 0,&flag);
	if (!gpio_is_valid(gpio)){
		printk("invalid led-work: %d\n",gpio);
		return -1;
	} 
    ret = gpio_request(gpio, "led-work");
	if (ret != 0) {
		gpio_free(gpio);
		ret = -EIO;
		goto failed_1;
	}
	led_info.work_gpio = gpio;
	led_info.work_enable_value = (flag == OF_GPIO_ACTIVE_LOW)? 0:1;
	//gpio_direction_output(led_info.work_gpio, !(led_info.work_enable_value));
	gpio_direction_output(led_info.work_gpio, 0);
*/


	
	gpio = of_get_named_gpio_flags(led_node,"led-0", 0,&flag);
	if (!gpio_is_valid(gpio)){
		printk("invalid led-0: %d\n",gpio);
		return -1;
	} 
    	ret = gpio_request(gpio, "led-0");
	if (ret != 0) {
		gpio_free(gpio);
		ret = -EIO;
		goto failed_1;
	}
	led_info.led_WifiHostWake = gpio;
	//led_info.power_enable_value = (flag == OF_GPIO_ACTIVE_LOW)? 0:1;
	//gpio_direction_output(led_info.power_gpio, !(led_info.power_enable_value));
	gpio_direction_output(led_info.led_WifiHostWake, 0);
	
	gpio = of_get_named_gpio_flags(led_node,"led-1", 0,&flag);
	if (!gpio_is_valid(gpio)){
		printk("invalid led-1: %d\n",gpio);
		return -1;
	} 
    	ret = gpio_request(gpio, "led-1");
	if (ret != 0) {
		gpio_free(gpio);
		ret = -EIO;
		goto failed_1;
	}
	led_info.led_Gpio7A5D = gpio;
	gpio_direction_output(led_info.led_Gpio7A5D, 0);

	gpio = of_get_named_gpio_flags(led_node,"led-2", 0,&flag);
	if (!gpio_is_valid(gpio)){
		printk("invalid led-2: %d\n",gpio);
		return -1;
	} 
    	ret = gpio_request(gpio, "led-2");
	if (ret != 0) {
		gpio_free(gpio);
		ret = -EIO;
		goto failed_1;
	}
	led_info.led_CompInt = gpio;
	gpio_direction_output(led_info.led_CompInt, 0);


	gpio = of_get_named_gpio_flags(led_node,"led-3", 0,&flag);
	if (!gpio_is_valid(gpio)){
		printk("invalid led-3: %d\n",gpio);
		return -1;
	} 
    	ret = gpio_request(gpio, "led-3");
	if (ret != 0) {
		gpio_free(gpio);
		ret = -EIO;
		goto failed_1;
	}
	led_info.led_GryInt = gpio;
	gpio_direction_output(led_info.led_GryInt, 0);

	gpio = of_get_named_gpio_flags(led_node,"led-4", 0,&flag);
	if (!gpio_is_valid(gpio)){
		printk("invalid led-4: %d\n",gpio);
		return -1;
	} 
    	ret = gpio_request(gpio, "led-4");
	if (ret != 0) {
		gpio_free(gpio);
		ret = -EIO;
		goto failed_1;
	}
	led_info.led_LightInt = gpio;
	gpio_direction_output(led_info.led_LightInt, 0);

	gpio = of_get_named_gpio_flags(led_node,"led-5", 0,&flag);
	if (!gpio_is_valid(gpio)){
		printk("invalid led-5: %d\n",gpio);
		return -1;
	} 
    	ret = gpio_request(gpio, "led-5");
	if (ret != 0) {
		gpio_free(gpio);
		ret = -EIO;
		goto failed_1;
	}
	led_info.led_key = gpio;
	gpio_direction_output(led_info.led_key, 0);


	gpio = of_get_named_gpio_flags(led_node,"led-power", 0,&flag);
	if (!gpio_is_valid(gpio)){
		printk("invalid led-power: %d\n",gpio);
		return -1;
	} 
    	ret = gpio_request(gpio, "led-power");
	if (ret != 0) {
		gpio_free(gpio);
		ret = -EIO;
		goto failed_1;
	}
	led_info.led_power = gpio;
	gpio_direction_output(led_info.led_power, 0);

printk("485 gpio.....\n");
	gpio = of_get_named_gpio_flags(led_node,"gpio-485", 0,&flag);
	if (!gpio_is_valid(gpio)){
		printk("invalid gpio-485: %d\n",gpio);
		return -1;
	} 
    	ret = gpio_request(gpio, "gpio-485");
	if (ret != 0) {
		gpio_free(gpio);
		ret = -EIO;
		goto failed_1;
	}
	led_info.gpio_485 = gpio;
	gpio_direction_output(led_info.gpio_485, 0);


	gpio = of_get_named_gpio_flags(led_node,"board-key", 0,&flag);
	if (!gpio_is_valid(gpio)){
		printk("invalid board-key: %d\n",gpio);
		return -1;
	} 
    	ret = gpio_request(gpio, "board-key");
	if (ret != 0) {
		gpio_free(gpio);
		ret = -EIO;
		goto failed_1;
	}
	led_info.board_key = gpio;
	gpio_direction_input(led_info.board_key);   //按键设置成输入




/*此处增加一个usb网卡dm9620的复位操作*/
printk("led-dm9620xx\n");
gpio = of_get_named_gpio_flags(led_node,"led-dm9620", 0,&flag);
	if (!gpio_is_valid(gpio)){
		printk("invalid led-dm9620: %d\n",gpio);
		return -1;
	} 
    ret = gpio_request(gpio, "led-dm9620");
	if (ret != 0) {
		gpio_free(gpio);
		ret = -EIO;
		goto failed_1;
	}
	led_info.led_dm9620 = gpio;
	led_info.dm9620_enable_value = (flag == OF_GPIO_ACTIVE_LOW)? 0:1;
	//gpio_direction_output(led_info.work_gpio, !(led_info.work_enable_value));
	gpio_direction_output(led_info.led_dm9620, 1);
	msleep(20);
	gpio_direction_output(led_info.led_dm9620, 0);
/****************************************************************************************/



/*
//此处增加一个外部功放mute脚的操作
	printk("muet-ext\n");
	gpio = of_get_named_gpio_flags(led_node,"mute-ext", 0,&flag);
	if (!gpio_is_valid(gpio)){
		printk("invalid mute-ext: %d\n",gpio);
		return -1;
	} 
        ret = gpio_request(gpio, "mute-ext");
	if (ret != 0) {
		gpio_free(gpio);
		ret = -EIO;
		goto failed_1;
	}
	gpio_direction_output(gpio, 1);	
*/


    // Create a test entry under USER_ROOT_DIR   
    firefly_led_entry = proc_create(USER_PATH, 0666, NULL, &firefly_led_proc_fops);   
    if (NULL == firefly_led_entry)   
    {   
        goto failed_1;   
    }
 /*  
    setup_timer(&led_timer.timer1, led1_timer_fuc, NULL);
#ifndef CONFIG_FIREFLY_POWER_LED
    setup_timer(&led_timer.timer2, led2_timer_fuc, NULL);
#endif
  */  



/* led tirp............*/
g_gpio_0=led_info.led_WifiHostWake;
g_gpio_1=led_info.led_Gpio7A5D;   //此处取得所有gpio，以方便ioctl控制
g_gpio_2=led_info.led_CompInt;
g_gpio_3=led_info.led_GryInt;
g_gpio_4=led_info.led_LightInt;
g_gpio_5=led_info.led_key;
g_gpio_6=led_info.led_power;

g_gpio_7=led_info.gpio_485;

g_board_key=led_info.board_key;

	printk("Initializing trip_led TTT.\n");
	err = alloc_chrdev_region(&trip_ndev, 0, 1, "trip_led");
	if(err < 0)
	{
		printk("alloc trip_ndev fail.\n");
		return err;	
	}
	trip_major = MAJOR(trip_ndev);
	trip_minor = MINOR(trip_ndev);
	printk("trip_major = %d,  trip_minor = %d\n", MAJOR(trip_ndev), MINOR(trip_ndev));

	trip_led_cdev = cdev_alloc();
	cdev_init(trip_led_cdev, &trip_led_ops);
	trip_led_cdev->owner = THIS_MODULE;
	err = cdev_add(trip_led_cdev, trip_ndev, 1);
	if(err<0)
	{
		printk("cdev_add trip_led_cdev fail.\n");
		return err;
	}
	trip_led_class = class_create(THIS_MODULE, "trip_led_class");
	device_create(trip_led_class, NULL, trip_ndev, NULL, "trip_led");
/* led tirp   end.........*/
	
	

	
	printk("%s %d\n",__FUNCTION__,__LINE__);
	
	return 0;  //return Ok

failed_1:
	return ret;
}


static int firefly_led_remove(struct platform_device *pdev)
{ 
    remove_proc_entry(USER_PATH, NULL); 
    return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id of_rk_firefly_led_match[] = {
	{ .compatible = "firefly,led" },
	{ /* Sentinel */ }
};
#endif

static struct platform_driver firefly_led_driver = {
	.probe		= firefly_led_probe,
	.remove		= firefly_led_remove,
	.driver		= {
		.name	= "firefly-led",
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table	= of_rk_firefly_led_match,
#endif
	},

};


static int __init firefly_led_init(void)
{
    printk(KERN_INFO "Enter %s\n", __FUNCTION__);
    return platform_driver_register(&firefly_led_driver);
}

static void __exit firefly_led_exit(void)
{
	trip_ndev=MKDEV(trip_major, trip_minor);
	device_destroy(trip_led_class, trip_ndev);
	class_destroy(trip_led_class);
	cdev_del(trip_led_cdev);
	unregister_chrdev_region(trip_ndev,1);


	platform_driver_unregister(&firefly_led_driver);

    printk(KERN_INFO "Enter %s\n", __FUNCTION__);
}

subsys_initcall(firefly_led_init);
module_exit(firefly_led_exit);

MODULE_AUTHOR("liaohm <teefirefly@gmail.com>");
MODULE_DESCRIPTION("Firefly LED driver");
MODULE_LICENSE("GPL");
