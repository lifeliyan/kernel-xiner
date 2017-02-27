#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/miscdevice.h>
#include <linux/platform_data/spi-rockchip.h>
#include <asm/uaccess.h>

#include "spi-rockchip-core.h"


static int rockchip_spi_trip_probe(struct spi_device *spi)
{
	
}



static int rockchip_spi_trip_remove(struct spi_device *spi)
{
	printk("%s\n",__func__);
	return 0;
}


static const struct of_device_id rockchip_spi_trip_dt_match[] = {
	{ .compatible = "rockchip,spi_trip_cs1"},
	{},
};
MODULE_DEVICE_TABLE(of, rockchip_spi_test_dt_match);

static struct spi_driver spi_rockchip_trip_driver = {
	.driver = {
		.name	= "spi_trip",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(rockchip_spi_trip_dt_match),
	},
	.probe = rockchip_spi_trip_probe,
	.remove = rockchip_spi_trip_remove,
};

static int __init spi_rockchip_trip_init(void)
{	
	int ret= 0;
	
	ret = spi_register_driver(&spi_rockchip_trip_driver);
	
	return ret;
}
module_init(spi_rockchip_trip_init);

static void __exit spi_rockchip_test_exit(void)
{
	return spi_unregister_driver(&spi_rockchip_trip_driver);
}
module_exit(spi_rockchip_trip_exit);

MODULE_AUTHOR("trip");
MODULE_DESCRIPTION("ROCKCHIP SPI trip Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:spi_trip");


