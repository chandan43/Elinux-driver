#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/i2c.h>


struct adxl345_prv {
	struct i2c_client client_prv;
	struct kobject *adxl345_kobj;
};

struct adxl345_prv *prv=NULL;


static ssize_t 
x_read(struct kobject *kobj, struct kobj_attribute *attr,char *buf)
{
	return 0;
}
static ssize_t 
y_read(struct kobject *kobj, struct kobj_attribute *attr,char *buf)
{
	return 0;
}
static ssize_t 
z_read(struct kobject *kobj, struct kobj_attribute *attr,char *buf)
{
	return 0;
}
static struct kobj_attribute xaxis_read  = __ATTR(Xaxis, 0444, x_read,NULL);
static struct kobj_attribute yaxis_read  = __ATTR(Yaxis, 0444, y_read,NULL);
static struct kobj_attribute zaxis_read  = __ATTR(Zaxis, 0444, z_read,NULL);

static struct attribute *attrs[] = {
        &xaxis_read.attr,
        &yaxis_read.attr,
        &zaxis_read.attr,
        NULL,
};

static struct attribute_group attr_group = {
        .attrs = attrs,
};

static int adxl345_probe(struct i2c_client *client, 
				const struct i2c_device_id *id)
{
	int ret;
	pr_info("%s: Device adxl345 probed......\n",__func__);	
	prv=(struct adxl345_prv *)kzalloc(sizeof(struct adxl345_prv), GFP_KERNEL);		
	if(!prv){
		pr_info("Requested memory not allocated\n");
		return -ENOMEM;
	}
	prv->client_prv = *client;
	prv->adxl345_kobj=kobject_create_and_add("adxl345", NULL);
	if(!prv->adxl345_kobj)
		return -ENOMEM;
	ret= sysfs_create_group(prv->adxl345_kobj, &attr_group);
	if(ret)
		kobject_put(prv->adxl345_kobj);
	
	return 0;
}
static int adxl345_remove(struct i2c_client *client)
{
	pr_info("adxl345_remove\n");
	kfree(prv);
	kobject_put(prv->adxl345_kobj);
	return 0;
}

static const struct i2c_device_id adxl345_ids[]={
	{ "adxl345", 0x53 },
	{ }
};

static struct i2c_driver adxl345_drv = {
	.driver = {
		.name = "adxl345",
		.owner = THIS_MODULE,
	},
	.probe    = adxl345_probe,
	.remove   = adxl345_remove,
	.id_table = adxl345_ids,
};

/**
 * module_i2c_driver() - Helper macro for registering a modular I2C driver
 * @__i2c_driver: i2c_driver struct
 *
 * Helper macro for I2C drivers which do not do anything special in module
 * init/exit. This eliminates a lot of boilerplate. Each module may only
 * use this macro once, and calling it replaces module_init() and module_exit()
 */
module_i2c_driver(adxl345_drv);


MODULE_DESCRIPTION("Driver for ADXL345 Digital Accelerometer");
MODULE_AUTHOR("Chandan jha <beingchandanjha@gmail.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION(".1");
