struct dm2106 {
	struct i2c_client *client;
	struct cdev *cdev;
	struct work_struct work;
	struct workqueue_struct *wq;
	int irq;
	int irq_pin;
	int rst_pin;
	int rst_val;
};
static int dm2016_iic_write(struct i2c_client *client, u8 addr, u8 *pdata, int datalen);
static int dm2016_iic_readbyte(struct i2c_client *client, u8 addr, u8 *pdata);
static int dm2016_iic_read(struct i2c_client *client, u8 addr, u8 *pdata, unsigned int datalen);

