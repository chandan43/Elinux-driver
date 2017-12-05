#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/jiffies.h>
#include <linux/delay.h>


enum DS3231_reg {
	SEC		= 0x00,
	MIN		= 0x01,
	HOUR		= 0x02,
	DATE		= 0x04,
};

struct ds3231_prv {
	struct i2c_client client_prv;
	struct kobject *ds3231_kobj;
};

struct ds3231_prv *prv=NULL;

static unsigned ds3231_timeout = 25; /*default timeout for normal I2c  devices */
/* register access */

static int ds3231_read_value(struct i2c_client *client, u8 reg)
{
	return i2c_smbus_read_byte_data(client, reg);

}

static ssize_t 
ds3231_date_get(struct kobject *kobj, struct kobj_attribute *attr,char *buf)
{
	char date;
	unsigned long timeout, read_time;
	timeout = jiffies + msecs_to_jiffies(ds3231_timeout);
	do{
		read_time = jiffies;
		date = ds3231_read_value(&prv->client_prv,DATE); 
		if (date < 0 )
			pr_err("Read error\n");
		else{
			return sprintf(buf, "Date : %d\n",date);
		}
		msleep(1);

	}while (time_before(read_time, timeout));
	
	return -ETIMEDOUT;
	return 0;
}
static ssize_t 
ds3231_time_get(struct kobject *kobj, struct kobj_attribute *attr,char *buf)
{
	char time[3] ={0};
	unsigned long timeout, read_time;
	timeout = jiffies + msecs_to_jiffies(ds3231_timeout);
	do{
		read_time = jiffies;
		time[0] = ds3231_read_value(&prv->client_prv,SEC); 
		time[1] = ds3231_read_value(&prv->client_prv,MIN);
		time[2] = ds3231_read_value(&prv->client_prv,HOUR);
		if (time[0] < 0 || time[1] < 0 || time[2] < 0 )
			pr_err("Read error\n");
		else{
			return sprintf(buf, "Time : %d:%d:%d\n",time[0],time[1],time[2]);
		}
		msleep(1);

	}while (time_before(read_time, timeout));
	
	return -ETIMEDOUT;
}
static struct kobj_attribute date  = __ATTR(Date, 0444, ds3231_date_get,NULL);
static struct kobj_attribute time  = __ATTR(Time, 0444, ds3231_time_get,NULL);

static struct attribute *attrs[] = {
        &date.attr,
        &time.attr,
        NULL,
};

static struct attribute_group attr_group = {
        .attrs = attrs,
};

static int ds3231_probe(struct i2c_client *client, 
				const struct i2c_device_id *id)
{
	int ret;
	pr_info("%s: Device ds3231 probed......\n",__func__);	
	prv=(struct ds3231_prv *)kzalloc(sizeof(struct ds3231_prv), GFP_KERNEL);		
	if(!prv){
		pr_info("Requested memory not allocated\n");
		return -ENOMEM;
	}
	prv->client_prv = *client;
	prv->ds3231_kobj=kobject_create_and_add("ds3231", NULL);
	if(!prv->ds3231_kobj)
		return -ENOMEM;
	ret= sysfs_create_group(prv->ds3231_kobj, &attr_group);
	if(ret)
		kobject_put(prv->ds3231_kobj);
	
	return 0;
}
static int ds3231_remove(struct i2c_client *client)
{
	pr_info("ds3231_remove\n");
	kfree(prv);
	kobject_put(prv->ds3231_kobj);
	return 0;
}

static const struct i2c_device_id ds3231_ids[]={
	{ "ds3231", 0x68 },
	{ }
};

static struct i2c_driver ds3231_drv = {
	.driver = {
		.name = "ds3231",
		.owner = THIS_MODULE,
	},
	.probe    = ds3231_probe,
	.remove   = ds3231_remove,
	.id_table = ds3231_ids,
};

/**
 * module_i2c_driver() - Helper macro for registering a modular I2C driver
 * @__i2c_driver: i2c_driver struct
 *
 * Helper macro for I2C drivers which do not do anything special in module
 * init/exit. This eliminates a lot of boilerplate. Each module may only
 * use this macro once, and calling it replaces module_init() and module_exit()
 */
module_i2c_driver(ds3231_drv);


MODULE_DESCRIPTION("Driver for DS3231 RTC");
MODULE_AUTHOR("Chandan jha <beingchandanjha@gmail.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION(".1");
