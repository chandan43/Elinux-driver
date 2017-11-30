#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/i2c.h>
#include <linux/jiffies.h>
#include <linux/delay.h>

struct lm75_prv {
	struct i2c_client client_prv;
	struct kobject *lm75_kobj;
        char *ptr;
	u8 orig_conf;
};

struct lm75_prv *prv=NULL;

/* The LM75 registers */
enum lm75_Reg {
	LM75_REG_CONF	= 0x01,
	LM75_REG_TEMP	= 0x00,
};

static unsigned lm75_timeout = 25; /*default timeout for normal I2c  devices */
/* register access */

/* All registers are word-sized, except for the configuration register.
   LM75 uses a high-byte first convention, which is exactly opposite to
   the SMBus standard. */
static int lm75_read_value(struct i2c_client *client, u8 reg)
{
	int value;
	if (reg == LM75_REG_CONF)
		return i2c_smbus_read_byte_data(client, reg);

	value = i2c_smbus_read_word_data(client, reg);
	return (value < 0) ? value : swab16(value);
}

static int lm75_write_value(struct i2c_client *client, u8 reg, u16 value)
{
	if (reg == LM75_REG_CONF)
		return i2c_smbus_write_byte_data(client, reg, value);
	else
		return i2c_smbus_write_word_data(client, reg, swab16(value));
}

/* TEMP: 0.001C/bit (-55C to +125C)
 *    REG: (0.5C/bit, two's complement) << 7 
*/
static inline int LM75_TEMP_FROM_REG(u16 reg)
{
	/* use integer division instead of equivalent right shift to
	 * 	   guarantee arithmetic shift and preserve the sign */
	return ((s16)reg / 128) * 5/10;
}

#if 0
static size_t lm75_temp_read(struct i2c_client *client, char *buf)
{
	struct i2c_msg msg[2]; /*array of msg buf*/
	u8 msgbuf[1];
	unsigned long timeout, read_time;
	int status;
	memset(msg, 0, sizeof(msg));
	
	msgbuf[0] = LM75_REG_TEMP;
	/*writing part i.e perform writing 1 byte and device addr*/
	//msg[0].addr = client->addr;
	msg[0].addr = 0x48;

	msg[0].flags = 0;
	msg[0].buf = msgbuf;

	msg[0].len = 2;
	
	//msg[1].addr = client->addr;
	msg[1].addr = 0x48;
	msg[1].flags = 1; /* Read */
	msg[1].buf = buf; 
	msg[1].len = 2;
	
	timeout = jiffies + msecs_to_jiffies(lm75_timeout);
	do {
		read_time = jiffies;

		status = i2c_transfer(client->adapter, msg, 2);
		if (status == 2)
			status = 16;
		if (status == 16) {
			return 0;
		}
		msleep(1);
	} while (time_before(read_time, timeout));

	return -ETIMEDOUT;

}
#endif
static size_t 
lm75_get_temp(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	size_t status;
	unsigned long timeout, read_time;
#if 0
	ret = lm75_temp_read(&prv->client_prv, buf);
	if (ret < 0) {
		pr_info("error in reading\n");
		return ret;
	}
#endif
	timeout = jiffies + msecs_to_jiffies(lm75_timeout);
	do{
		read_time = jiffies;
		status = lm75_read_value(&prv->client_prv, LM75_REG_TEMP);
		if (status < 0)
			pr_err("reg %d, err %d\n",
						LM75_REG_TEMP, status);
		else{
			return sprintf(buf, "%d\n",
					LM75_TEMP_FROM_REG(status));
		}
		msleep(1);
	}while (time_before(read_time, timeout));
	return -ETIMEDOUT;
	
}
static struct kobj_attribute lm75_temp = __ATTR(temp,0444,lm75_get_temp,NULL);

static struct attribute *attrs[] = {
        &lm75_temp.attr,
        NULL,
};

static struct attribute_group attr_group = {
        .attrs = attrs,
};

static int 
lm75_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret, new;
	int status;
	u8 set_mask, clr_mask;
	pr_info("%s: Device lm75 probed.!!\n",__func__);	
	prv=(struct lm75_prv *)kzalloc(sizeof(struct lm75_prv), GFP_KERNEL);		
	if(!prv){
		pr_info("Requested memory not allocated\n");
		return -ENOMEM;
	}
	prv->client_prv = *client;
	
	set_mask = 0;
	clr_mask = (1 << 0)			/* continuous conversions */
		| (1 << 6) | (1 << 5); 		/* 9-bit mode */
	/* configure as specified */
	status = lm75_read_value(client, LM75_REG_CONF);
	if (status < 0) 
		dev_dbg(&client->dev, "Can't read config? %d\n", status);
	
	prv->orig_conf = status;
	new = status & ~clr_mask;
	new |= set_mask;
	if (status != new)
		lm75_write_value(client, LM75_REG_CONF, new);
	dev_dbg(&client->dev, "Config %02x\n", new);
	

	prv->lm75_kobj=kobject_create_and_add("lm75", NULL);
	if(!prv->lm75_kobj)
		return -ENOMEM;
	ret= sysfs_create_group(prv->lm75_kobj, &attr_group);
	if(ret)
		kobject_put(prv->lm75_kobj);
	return 0;
}

static int lm75_remove(struct i2c_client *client)
{
	pr_info("lm75_remove\n");
	kfree(prv); 
	kobject_put(prv->lm75_kobj);
	return 0;
}
static const struct i2c_device_id lm75_ids[]={
	{ "lm75", 0x48 },
	{ }
};

static struct i2c_driver lm75_driver = {
	.driver = {
		.name = "lm75",
		.owner = THIS_MODULE,
	},
	.probe    = lm75_probe,
	.remove   = lm75_remove,
	.id_table = lm75_ids,

};

/**
 * module_i2c_driver() - Helper macro for registering a modular I2C driver
 * @__i2c_driver: i2c_driver struct
 *
 * Helper macro for I2C drivers which do not do anything special in module
 * init/exit. This eliminates a lot of boilerplate. Each module may only
 * use this macro once, and calling it replaces module_init() and module_exit()
 */
module_i2c_driver(lm75_driver);


MODULE_DESCRIPTION("Driver lm75B I2c temperature sensor");
MODULE_AUTHOR("Chandan jha <beingchandanjha@gmail.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION(".1");
