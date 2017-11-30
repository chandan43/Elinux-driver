#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>


static int i2c_eeprom_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	
}

static int i2c_eeprom_remove(struct i2c_client *client)
{
}

static const struct i2c_device_id i2c_eeprom_ids[]={
	{ "24c32", 0x50 },
	{ }
};

static static i2c_driver i2c_eeprom_driver = {
	.driver = {
		.name = "at24",
		.owner = THIS_MODULE,
	},
	.probe    = i2c_eeprom_probe,
	.remove   = i2c_eeprom_remove,
	.id_table = i2c_eeprom_ids,
};

/**
 * module_i2c_driver() - Helper macro for registering a modular I2C driver
 * @__i2c_driver: i2c_driver struct
 *
 * Helper macro for I2C drivers which do not do anything special in module
 * init/exit. This eliminates a lot of boilerplate. Each module may only
 * use this macro once, and calling it replaces module_init() and module_exit()
 */
module_i2c_driver(i2c_eeprom_driver);


MODULE_DESCRIPTION("Driver for at24c32 eeprom");
MODULE_AUTHOR("Chandan jha <beingchandanjha@gmail.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION(".1");
