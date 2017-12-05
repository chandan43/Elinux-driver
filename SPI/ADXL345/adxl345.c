#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/spi/spi.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/jiffies.h>
#include <linux/delay.h>


struct adxl345_prv {
	struct 	spi_device *spi;
	struct kobject *adxl345_kobj;
};

struct adxl345_prv *prv=NULL;


/* The adxl345 Config registers */
enum ADXL345_Reg {
	BW_RATE		= 0x2C, 	// Bandwidth rate register(0x2C) : Data rate and power mode control.
	POWER_CTL	= 0x2D,		// Power-saving features control.Power control register
	DATA_FORMAT	= 0x31, 	// Select Data format register(0x31): Data format control.
	DEVID		= 0x00,		// Device ID.
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
static int adxl345_read_value(struct spi_device *spi, u8 reg)
{
//	unsigned char *data = reg | 0x80;
	//return spi_write_then_read(spi, data, 1, data, 1);
	return spi_w8r8(spi,(reg | 0x80));
}

static int adxl345_write_value(struct spi_device *spi, u8 reg,u16 value)
{
	unsigned char buf[2];
 
	buf[0] = reg & 0x7F;
	//buf[0] = reg;
	buf[1] = value;
 
	return spi_write_then_read(spi, buf, 2, NULL, 0);
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
		data[0] = adxl345_read_value(prv->spi,DATAX0); 
		data[1] = adxl345_read_value(prv->spi,DATAX1);
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
		data[0] = adxl345_read_value(prv->spi,DATAY0); 
		data[1] = adxl345_read_value(prv->spi,DATAY1);
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
		data[0] = adxl345_read_value(prv->spi,DATAZ0); 
		data[1] = adxl345_read_value(prv->spi,DATAZ1);
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
//1.BW_RATE	: Set MODE		= 0x0A,   :00001010     //Normal mode, Output data rate = 100 Hz(0x0A)
//2.POWER_CTL	: Set AUTO_SLEEP	= 0x08,   :00001000	//Auto-sleep disable
//3.DATA_FORMAT: Set SELF_TEST		= 0x08,	  :00001000	//Self test disabled, 4-wire interface, Full resolution, range = +/-2g(0x08)

static int adxl345_probe(struct spi_device *spi)
{
	int ret,new;
	u8 set_mask,devid;
	int status;
	pr_info("%s: Device adxl345 probed......\n",__func__);	
	prv=(struct adxl345_prv *)kzalloc(sizeof(struct adxl345_prv), GFP_KERNEL);		
	if(!prv){
		pr_info("Requested memory not allocated\n");
		return -ENOMEM;
	}
	prv->spi=spi;
	devid = adxl345_read_value(spi, DEVID); 
	/* The Device ID of the ADXL345 is 0xE5, which is 229 in decimal, This should be print. */
	pr_info("DEVID: Device id is %02x\n",devid);
	/* device driver data */
	spi_set_drvdata(spi, prv);
	set_mask = (1 << 1) | (1 << 3);
	/* configure as specified */
	status = adxl345_read_value(spi, BW_RATE);
	if (status < 0) 
		dev_dbg(&spi->dev, "Can't read BW_RATE config? %d\n", status);
	
	new = status | set_mask ;
	adxl345_write_value(spi, BW_RATE , new);
	dev_dbg(&spi->dev, "BW_RATE Config %02x\n", new);
	
	/*Reset*/
	set_mask = 0;
	new = 0;
	set_mask = (1 << 3);
	/* configure as specified */
	status = adxl345_read_value(spi,POWER_CTL);
	if (status < 0) 
		dev_dbg(&spi->dev, "Can't read POWER_CTL config? %d\n", status);
	
	new = status | set_mask ;
	adxl345_write_value(spi, POWER_CTL , new);
	dev_dbg(&spi->dev, "POWER_CTL Config %02x\n", new);
	
	/*Reset*/
	set_mask=0;
	new=0;
	set_mask = (1 << 3) | (0 << 6); //Selecting 4 wire config
	/* configure as specified */
	status = adxl345_read_value(spi,DATA_FORMAT);
	if (status < 0) 
		dev_dbg(&spi->dev, "Can't read DATA_FORMAT config? %d\n", status);
	
	new = status | set_mask ;
	adxl345_write_value(spi, DATA_FORMAT, new);
	dev_dbg(&spi->dev, "DATA_FORMAT Config %02x\n", new);
	
	prv->adxl345_kobj=kobject_create_and_add("adxl345", NULL);
	if(!prv->adxl345_kobj)
		return -ENOMEM;
	ret= sysfs_create_group(prv->adxl345_kobj, &attr_group);
	if(ret)
		kobject_put(prv->adxl345_kobj);
	
	return 0;
}
static int adxl345_remove(struct spi_device *spi)
{
	pr_info("adxl345_remove\n");
	kfree(prv);
	kobject_put(prv->adxl345_kobj);
	return 0;
}
static const struct of_device_id  adxl345_of_match[]={
	{.compatible = "ADLX,adxl345", },
	{ }
};

static struct spi_driver adxl345_drv = {
	.driver = {
		.name = "adxl345",
		.of_match_table = adxl345_of_match,
	},
	.probe    = adxl345_probe,
	.remove   = adxl345_remove,
};

/**
 * module_spi_driver() - Helper macro for registering a SPI driver
 * @__spi_driver: spi_driver struct
 *
 * Helper macro for SPI drivers which do not do anything special in module
 * init/exit. This eliminates a lot of boilerplate. Each module may only
 * use this macro once, and calling it replaces module_init() and module_exit()
 */

module_spi_driver(adxl345_drv);


MODULE_DESCRIPTION("Driver for ADXL345 Digital Accelerometer");
MODULE_AUTHOR("Chandan jha <beingchandanjha@gmail.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION(".1");
