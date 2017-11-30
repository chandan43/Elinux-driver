#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/jiffies.h>
#include <linux/delay.h>


struct adxl345_prv {
	struct i2c_client client_prv;
	struct kobject *adxl345_kobj;
};

struct adxl345_prv *prv=NULL;


/* The adxl345 Config registers */
enum ADXL345_Reg {
	BW_RATE		= 0x2C, 	//Bandwidth rate register(0x2C) : Data rate and power mode control.
	POWER_CTL	= 0x2D,		//Power-saving features control.Power control register
	DATA_FORMAT	= 0x31, 	//Select Data format register(0x31): Data format control.
};

enum Axis_reg {
	DATAX0		= 0x32,
	DATAX1		= 0x33,
	DATAY0		= 0x34,
	DATAY1		= 0x35,
	DATAZ0		= 0x36,
	DATAZ1		= 0x37,
};

static unsigned adxl345_timeout = 25; /*default timeout for normal I2c  devices */
/* register access */

/* All registers are word-sized, except for the configuration register.
   LM75 uses a high-byte first convention, which is exactly opposite to
   the SMBus standard. */
static int adxl345_read_value(struct i2c_client *client, u8 reg)
{
	return i2c_smbus_read_byte_data(client, reg);

}

static int adxl345_write_value(struct i2c_client *client, u8 reg, u16 value)
{
	return i2c_smbus_write_byte_data(client, reg, value);
}


static ssize_t 
x_read(struct kobject *kobj, struct kobj_attribute *attr,char *buf)
{
	char data[2] ={0};
	int  xAccl;
	unsigned long timeout, read_time;

	timeout = jiffies + msecs_to_jiffies(adxl345_timeout);
	do{
		read_time = jiffies;
		data[0] = adxl345_read_value(&prv->client_prv,DATAX0); 
		data[1] = adxl345_read_value(&prv->client_prv,DATAX1);
		if (data[0] < 0 || data[1] < 0 )
			pr_err("LSB %d, MSB %d, LSB err %d,MSB err %d\n",
						DATAX0,DATAX0, data[0],data[1]);
		else{
			xAccl = ((data[1] & 0x03) * 256 + (data[0] & 0xFF));
			if(xAccl > 511){
			 	xAccl -= 1024;
			}
			return sprintf(buf, "Acceleration in X-Axis : %d\n",xAccl);
		}
		msleep(1);

	}while (time_before(read_time, timeout));
	
	return -ETIMEDOUT;
}
static ssize_t 
y_read(struct kobject *kobj, struct kobj_attribute *attr,char *buf)
{
	char data[2] ={0};
	int  yAccl;
	unsigned long timeout, read_time;

	timeout = jiffies + msecs_to_jiffies(adxl345_timeout);
	do{
		read_time = jiffies;
		data[0] = adxl345_read_value(&prv->client_prv,DATAY0); 
		data[1] = adxl345_read_value(&prv->client_prv,DATAY1);
		if (data[0] < 0 || data[1] < 0 )
			pr_err("LSB %d, MSB %d, LSB err %d,MSB err %d\n",
						DATAX0,DATAX0, data[0],data[1]);
		else{
			yAccl = ((data[1] & 0x03) * 256 + (data[0] & 0xFF));
			if(yAccl > 511){
				 yAccl -= 1024;
			}
			return sprintf(buf, "Acceleration in Y-Axis : %d\n",yAccl);
		}
		
		msleep(1);
	}while (time_before(read_time, timeout));
	
	return -ETIMEDOUT;
}
static ssize_t 
z_read(struct kobject *kobj, struct kobj_attribute *attr,char *buf)
{
	char data[2] ={0};
	int  zAccl;
	unsigned long timeout, read_time;

	timeout = jiffies + msecs_to_jiffies(adxl345_timeout);
	do{
		read_time = jiffies;
		data[0] = adxl345_read_value(&prv->client_prv,DATAZ0); 
		data[1] = adxl345_read_value(&prv->client_prv,DATAZ1);
		if (data[0] < 0 || data[1] < 0 )
			pr_err("LSB %d, MSB %d, LSB err %d,MSB err %d\n",
						DATAX0,DATAX0, data[0],data[1]);
		else{
			zAccl = ((data[1] & 0x03) * 256 + (data[0] & 0xFF));
			if(zAccl > 511){
				 zAccl -= 1024;
			}
			return sprintf(buf, "Acceleration in Z-Axis : %d\n",zAccl);
		}
		msleep(1);
	}while (time_before(read_time, timeout));
	
	return -ETIMEDOUT;
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
//1.BW_RATE	: Set MODE		= 0x0A,   :00001010      //Normal mode, Output data rate = 100 Hz(0x0A)
//2.POWER_CTL	: Set AUTO_SLEEP	= 0x08,   :00001000	//Auto-sleep disable
//3.DATA_FORMAT: Set SELF_TEST		= 0x08,	  :00001000	//Self test disabled, 4-wire interface, Full resolution, range = +/-2g(0x08)

static int adxl345_probe(struct i2c_client *client, 
				const struct i2c_device_id *id)
{
	int ret,new;
	u8 set_mask;
	int status;
	pr_info("%s: Device adxl345 probed......\n",__func__);	
	prv=(struct adxl345_prv *)kzalloc(sizeof(struct adxl345_prv), GFP_KERNEL);		
	if(!prv){
		pr_info("Requested memory not allocated\n");
		return -ENOMEM;
	}
	prv->client_prv = *client;
	
	set_mask = (1 << 1) | (1 << 3);
	/* configure as specified */
	status = adxl345_read_value(client,BW_RATE);
	if (status < 0) 
		dev_dbg(&client->dev, "Can't read BW_RATE config? %d\n", status);
	
	new = status | set_mask ;
	adxl345_write_value(client, BW_RATE , new);
	dev_dbg(&client->dev, "BW_RATE Config %02x\n", new);
	
	/*Reset*/
	set_mask = 0;
	new = 0;
	set_mask = (1 << 3);
	/* configure as specified */
	status = adxl345_read_value(client,POWER_CTL);
	if (status < 0) 
		dev_dbg(&client->dev, "Can't read POWER_CTL config? %d\n", status);
	
	new = status | set_mask ;
	adxl345_write_value(client, POWER_CTL , new);
	dev_dbg(&client->dev, "POWER_CTL Config %02x\n", new);
	
	/*Reset*/
	set_mask=0;
	new=0;
	set_mask = (1 << 3);
	/* configure as specified */
	status = adxl345_read_value(client,DATA_FORMAT);
	if (status < 0) 
		dev_dbg(&client->dev, "Can't read DATA_FORMAT config? %d\n", status);
	
	new = status | set_mask ;
	adxl345_write_value(client, DATA_FORMAT, new);
	dev_dbg(&client->dev, "DATA_FORMAT Config %02x\n", new);
	
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
