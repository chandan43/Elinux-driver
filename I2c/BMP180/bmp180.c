/*Programming steps :
 * Step 1: Get I2C device, VEML6070 I2C address is 0x77(119)
 * Step 2: Calibration Cofficients stored in EEPROM of the device
 * 		Read 22 bytes of data from address 0xAA(170)
 * Step 3: Select measurement control register(0xF4)
 * 		Enable temperature measurement(0x2E)
 * Step 4:  Read 2 bytes of data from register(0xF6)
 * 		temp msb, temp lsb
 * Step 5: Select measurement control register(0xf4)
 * 		Enable pressure measurement, OSS = 1(0x74)
 * Step 6: Read 3 bytes of data from register(0xF6)
 * 		pres msb1, pres msb, pres lsb
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/jiffies.h>
#include <linux/delay.h>


/*BMP180_coeff_reg */ 
static int AC1,AC2,AC3,AC4,AC5,AC6,B1,B2,MB,MC,MD;

struct bmp180_prv {
	struct i2c_client client_prv;
	struct kobject *bmp180_kobj;
};

struct bmp180_prv *prv=NULL;


/* The BMP180  registers */
enum BMP180_Reg {
	BMP180_CTR      = 0xF4, 	//measurement control register
};

enum BMP180_coeff_reg_MSB {
	AC1_MSB		= 0xAA,
	AC2_MSB 	= 0xAC,
	AC3_MSB		= 0xAE, 
	AC4_MSB 	= 0xB0,
	AC5_MSB 	= 0xB2,
	AC6_MSB	 	= 0xB4,
	B1_MSB		= 0xB6, 
	B2_MSB		= 0xB8, 
	MB_MSB		= 0xBA, 
	MC_MSB		= 0xBC, 
	MD_MSB		= 0xBE, 
};
enum BMP180_coeff_reg_LSB {
	AC1_LSB		= 0xAB,
	AC2_LSB 	= 0xAD,
	AC3_LSB		= 0xAF, 
	AC4_LSB 	= 0xB1,
	AC5_LSB 	= 0xB3,
	AC6_LSB	 	= 0xB5,
	B1_LSB		= 0xB7, 
	B2_LSB		= 0xB9, 
	MB_LSB		= 0xBB, 
	MC_LSB		= 0xBD, 
	MD_LSB		= 0xBF, 
};

enum Config_Param {
	En_Temp_M	= 0x2E,   // Enable temperature measurement(0x2E)
	En_Pres_M_0	= 0x34,   // Enable pressure measurement, OSS = 0
	En_Pres_M_1	= 0x74,   // Enable pressure measurement, OSS = 1
	En_Pres_M_2	= 0xB4,   // Enable pressure measurement, OSS = 2
	En_Pres_M_3	= 0xF4,   // Enable pressure measurement, OSS = 3
};

enum Data_reg{
	UT		= 0xF6, // TEMP Data reg     : Read 2 bytes of data from register(0xF6) 
        UP 		= 0xF6, // Pressure Data reg : Read 3 bytes of data from register(0xF6) 
};

static unsigned bmp180_timeout = 25; /*default timeout for normal I2c  devices */
/* register access */

static int power(int param1, unsigned int param2)
{
    if (param2 == 0)
        return 1;
    else if (param2%2 == 0)
        return power(param1, param2/2) * power(param1, param2/2);
    else
        return param1 * power(param1, param2/2) * power(param1, param2/2);
}
static int bmp180_read_value(struct i2c_client *client, u8 reg)
{
	return i2c_smbus_read_byte_data(client, reg);

}

static int bmp180_write_value(struct i2c_client *client, u8 reg, u16 value)
{
	return i2c_smbus_write_byte_data(client, reg, value);
}
static void get_Calibration_coeff(struct i2c_client *client)
{
	AC1 = ( bmp180_read_value(client,AC1_MSB) << 8 ) + bmp180_read_value(client,AC1_LSB);	
	AC2 = ( bmp180_read_value(client,AC2_MSB) << 8 ) + bmp180_read_value(client,AC2_LSB);	
	AC3 = ( bmp180_read_value(client,AC3_MSB) << 8 ) + bmp180_read_value(client,AC3_LSB);	
	AC4 = ( bmp180_read_value(client,AC4_MSB) << 8 ) + bmp180_read_value(client,AC4_LSB);	
	AC5 = ( bmp180_read_value(client,AC5_MSB) << 8 ) + bmp180_read_value(client,AC5_LSB);	
	AC6 = ( bmp180_read_value(client,AC6_MSB) << 8 ) + bmp180_read_value(client,AC6_LSB);	
	B1  = ( bmp180_read_value(client,B1_MSB) << 8 ) + bmp180_read_value(client,B1_LSB); 
	B2  = ( bmp180_read_value(client,B2_MSB) << 8 ) + bmp180_read_value(client,B2_LSB);
	MB  = ( bmp180_read_value(client,MB_MSB) << 8 ) + bmp180_read_value(client,MB_LSB); 
	MC  = ( bmp180_read_value(client,MC_MSB) << 8 ) + bmp180_read_value(client,MC_LSB); 
	MD  = ( bmp180_read_value(client,MD_MSB) << 8 ) + bmp180_read_value(client,MD_LSB);
}
static int get_temp_value(struct i2c_client *client )
{
	char data[2] ={0};
	long temp;
	unsigned long timeout, read_time;
	/* Enable temperature */
	bmp180_write_value(&prv->client_prv,BMP180_CTR,En_Temp_M);
	timeout = jiffies + msecs_to_jiffies(bmp180_timeout);
	do{
		read_time = jiffies;
		data[0] = bmp180_read_value(&prv->client_prv,UT); 
		data[1] = bmp180_read_value(&prv->client_prv,UT + 1);
		if (data[0] < 0 || data[1] < 0 )
			pr_err("%s: Read error\n",__func__);
		else{
			 temp = ( data[0] << 8 ) + ( data[1] )  ; /* Convert the data*/ 
			return temp;
		}
		msleep(1);
	}while (time_before(read_time, timeout));
	return -ETIMEDOUT;
}
static int get_pressure_value(struct i2c_client *client )
{
	char data[3] ={0};
	long pressure;
	unsigned long timeout, read_time;
	/* Enable pressure measurement, OSS = 1(0x74) */
	bmp180_write_value(&prv->client_prv,BMP180_CTR,En_Pres_M_1);
	timeout = jiffies + msecs_to_jiffies(bmp180_timeout);
	do{
		read_time = jiffies;
		data[0] = bmp180_read_value(client,UP); 
		data[1] = bmp180_read_value(client,UP + 1);
		data[2] = bmp180_read_value(client,UP + 2);
		if (data[0] < 0 || data[1] < 0 ||  data[2] < 0)
			pr_err("%s: Read error\n",__func__);
		else{
			 pressure= ( data[0] << 16 ) + ( data[1] << 8 ) + data[2] ; /* Convert the data*/ 
			return pressure;
		}
		msleep(1);
	}while (time_before(read_time, timeout));
	return -ETIMEDOUT;
}
static ssize_t bmp180_get_pressure(struct kobject *kobj, 
				struct kobj_attribute *attr,char *buf)
{
	long temp,pressure,pressure1,altitude,pres;
	long X1,X2,X3,B3,B4,B5,B6,B7,cTemp,fTemp;
	/* Calibration Cofficients stored in EEPROM of the device 
	 * Read 22 bytes of data from address 0xAA(170) */
	get_Calibration_coeff(&prv->client_prv);
	/* Callibration for Temperature */
	temp=get_temp_value(&prv->client_prv);
	X1 = (temp - AC6) * AC5 / 32768.0;
	X2 = (MC * 2048.0) / (X1 + MD);
	B5 = X1 + X2;
	cTemp = ((B5 + 8.0) / 16.0) / 10.0;
	fTemp = cTemp * 1.8 + 32;
	/* Calibration for Pressure */
	pres=get_pressure_value(&prv->client_prv);
	B6 = B5 - 4000;
	X1 = (B2 * (B6 * B6 / 4096.0)) / 2048.0;
	X2 = AC2 * B6 / 2048.0;
	X3 = X1 + X2;
	B3 = (((AC1 * 4 + X3) * 2) + 2) / 4.0;
	X1 = AC3 * B6 / 8192.0;
	X2 = (B1 * (B6 * B6 / 2048.0)) / 65536.0;
	X3 = ((X1 + X2) + 2) / 4.0;
	B4 = AC4 * (X3 + 32768) / 32768.0;
 	B7 = ((pres - B3) * (25000.0));
	pressure = 0.0;
	if(B7 < 2147483648LL){
		pressure = (B7 * 2) / B4;
	}else {
		pressure = (B7 / B4) * 2;
	}
	X1 = (pressure / 256.0) * (pressure / 256.0);
	X1 = (X1 * 3038.0) / 65536.0;
	X2 = ((-7357) * pressure) / 65536.0;
	pressure1 = (pressure + (X1 + X2 + 3791) / 16.0) / 100;
	// Calculate Altitude
	altitude = 44330 * (1 - power(pressure1/1013.25, 0.1903));
	return sprintf(buf, "Pressure : %ld\n",pressure1);
}

static ssize_t bmp180_get_altitude(struct kobject *kobj, 
					struct kobj_attribute *attr,char *buf)
{
	long temp,pressure,pressure1,altitude,pres;
	long X1,X2,X3,B3,B4,B5,B6,B7,cTemp,fTemp;
	/* Calibration Cofficients stored in EEPROM of the device 
	 * Read 22 bytes of data from address 0xAA(170) */
	get_Calibration_coeff(&prv->client_prv);
	/* Callibration for Temperature */
	temp=get_temp_value(&prv->client_prv);
	X1 = (temp - AC6) * AC5 / 32768.0;
	X2 = (MC * 2048.0) / (X1 + MD);
	B5 = X1 + X2;
	cTemp = ((B5 + 8.0) / 16.0) / 10.0;
	fTemp = cTemp * 1.8 + 32;
	/* Calibration for Pressure */
	pres=get_pressure_value(&prv->client_prv);
	B6 = B5 - 4000;
	X1 = (B2 * (B6 * B6 / 4096.0)) / 2048.0;
	X2 = AC2 * B6 / 2048.0;
	X3 = X1 + X2;
	B3 = (((AC1 * 4 + X3) * 2) + 2) / 4.0;
	X1 = AC3 * B6 / 8192.0;
	X2 = (B1 * (B6 * B6 / 2048.0)) / 65536.0;
	X3 = ((X1 + X2) + 2) / 4.0;
	B4 = AC4 * (X3 + 32768) / 32768.0;
 	B7 = ((pres - B3) * (25000.0));
	pressure = 0.0;
	if(B7 < 2147483648LL){
		pressure = (B7 * 2) / B4;
	}else {
		pressure = (B7 / B4) * 2;
	}
	X1 = (pressure / 256.0) * (pressure / 256.0);
	X1 = (X1 * 3038.0) / 65536.0;
	X2 = ((-7357) * pressure) / 65536.0;
	pressure1 = (pressure + (X1 + X2 + 3791) / 16.0) / 100;
	// Calculate Altitude
	altitude = 44330 * (1 - power(pressure1/1013.25, 0.1903));
	return sprintf(buf, "Pressure : %ld\n",altitude);
}
static ssize_t bmp180_get_cTemp(struct kobject *kobj, 
				struct kobj_attribute *attr,char *buf)
{
	long temp;
	long X1,X2,B5,cTemp,fTemp;
	/* Calibration Cofficients stored in EEPROM of the device 
	 * Read 22 bytes of data from address 0xAA(170) */
	get_Calibration_coeff(&prv->client_prv);
	/* Callibration for Temperature */
	temp=get_temp_value(&prv->client_prv);
	X1 = (temp - AC6) * AC5 / 32768.0;
	X2 = (MC * 2048.0) / (X1 + MD);
	B5 = X1 + X2;
	cTemp = ((B5 + 8.0) / 16.0) / 10.0;
	fTemp = cTemp * 1.8 + 32;
	return sprintf(buf, "Temperature in Celsius : %ld\n",cTemp);
}
static ssize_t bmp180_get_fTemp(struct kobject *kobj, 
				struct kobj_attribute *attr,char *buf)
{
	long temp;
	long X1,X2,B5,cTemp,fTemp;
	/* Calibration Cofficients stored in EEPROM of the devicei*/ 
	 /* Read 22 bytes of data from address 0xAA(170) */
	get_Calibration_coeff(&prv->client_prv);
	/* Callibration for Temperature */
	temp=get_temp_value(&prv->client_prv);
	X1 = (temp - AC6) * AC5 / 32768.0;
	X2 = (MC * 2048.0) / (X1 + MD);
	B5 = X1 + X2;
	cTemp = ((B5 + 8.0) / 16.0) / 10.0;
	fTemp = cTemp * 1.8 + 32;
	return sprintf(buf, "Temperature in  Fahrenheit : %ld\n",fTemp);
}
static struct kobj_attribute pressure  = __ATTR(Pressure, 0444, bmp180_get_pressure,NULL);
static struct kobj_attribute altitude  = __ATTR(Altitude, 0444, bmp180_get_altitude,NULL);
static struct kobj_attribute cTemp     = __ATTR(Temp_in_Cel, 0444, bmp180_get_cTemp,NULL);
static struct kobj_attribute fTemp     = __ATTR(Temp_in_Fah, 0444, bmp180_get_fTemp,NULL);

static struct attribute *attrs[] = {
        &pressure.attr,
        &altitude.attr,
        &cTemp.attr,
        &fTemp.attr,
        NULL,
};

static struct attribute_group attr_group = {
        .attrs = attrs,
};


static int bmp180_probe(struct i2c_client *client, 
				const struct i2c_device_id *id)
{
	int ret;
	pr_info("%s: Device bmp180 probed......\n",__func__);	
	prv=(struct bmp180_prv *)kzalloc(sizeof(struct bmp180_prv), GFP_KERNEL);		
	if(!prv){
		pr_info("Requested memory not allocated\n");
		return -ENOMEM;
	}
	prv->client_prv = *client;
	
	prv->bmp180_kobj=kobject_create_and_add("bmp180", NULL);
	if(!prv->bmp180_kobj)
		return -ENOMEM;

	ret= sysfs_create_group(prv->bmp180_kobj, &attr_group);
	if(ret)
		kobject_put(prv->bmp180_kobj);
	
	return 0;
}
static int bmp180_remove(struct i2c_client *client)
{
	pr_info("bmp180_remove\n");
	kfree(prv);
	kobject_put(prv->bmp180_kobj);
	return 0;
}

static const struct i2c_device_id bmp180_ids[]={
	{ "bmp180", 0x77 },
	{ }
};

static struct i2c_driver bmp180_drv = {
	.driver = {
		.name = "bmp180",
		.owner = THIS_MODULE,
	},
	.probe    = bmp180_probe,
	.remove   = bmp180_remove,
	.id_table = bmp180_ids,
};

/**
 * module_i2c_driver() - Helper macro for registering a modular I2C driver
 * @__i2c_driver: i2c_driver struct
 *
 * Helper macro for I2C drivers which do not do anything special in module
 * init/exit. This eliminates a lot of boilerplate. Each module may only
 * use this macro once, and calling it replaces module_init() and module_exit()
 */
module_i2c_driver(bmp180_drv);


MODULE_DESCRIPTION("Driver for BMP180 I2c pressure sensor");
MODULE_AUTHOR("Chandan jha <beingchandanjha@gmail.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION(".1");
