#include <linux/module.h>
#include <linux/init.h>
#include <linux/spi/spi.h>
#include <linux/device.h>
#include <linux/kobject.h>
#include <linux/property.h>

#define W25_MAXADDRLEN  3       /* 24 bit address, up to 16 MBytes */
#define W25_TIMEOUT     25
#define IO_LIMIT        256     /* bytes */

enum INSTw25id {
	W25_Readid   =  0x9F,    /*read Manufacturer id  :*/
	W25_MNid     =  0xEF,    /*Manufacturer id */
};

enum StausReg {
	W25_ReadSR     = 0x05,   /* read status register */
	W25_WriteSR    = 0x01,   /* write status register */
};

enum StausVal {
	W25_NotrdySR   = 0x01,   /* nRDY = write-in-progres pg.17  */
	W25_Write_EnSR = 0x02,   /* write enable (latched) */
};

enum ChipCmd {
	W25_WriteEn  = 0x06,   /* latch the write enable */
	W25_WriteDis = 0x04,   /* reset the write enablei i.e write Disable */
	W25_Read     = 0x03,   /* read byte(s) */
	W25_Write    = 0x02,   /* write byte(s)/sector */
	W25_SecErase = 0x20,    /*Erase sector*/
};

struct w25_priv {
	struct 			spi_device *spi;
	char 			name[15];
	struct 			kobject *kobj;
	unsigned 		size;
	unsigned 		page_size;
	unsigned 		address_width;
	long int 		offset;
	unsigned                block;
	unsigned                sector;
	unsigned                page;
};

struct w25_priv *prv = NULL;

static ssize_t w25_sys_write(struct kobject *kobj, struct kobj_attribute *attr,
				 const char *buf, size_t count)
{
	return 0;
}


static ssize_t w25_sys_read(struct kobject *kobj, struct kobj_attribute *attr,
				 char *buf)
{
	return 0;
}

static ssize_t w25_set_offset(struct kobject *kobj, struct kobj_attribute *attr, 
				const char *buf, size_t count)
{
	return 0;
	
}
static ssize_t w25_get_offset(struct kobject *kobj, 
				struct kobj_attribute *attr, char *buf)
{
	return 0;
}



static struct kobj_attribute w25_rw =
		__ATTR(w25q32, 0660, w25_sys_read, w25_sys_write);
static struct kobj_attribute w25_offset =
		__ATTR(offset, 0660, w25_get_offset, w25_set_offset);

static struct attribute *attrs[] = {
	&w25_rw.attr,
	&w25_offset.attr,
	NULL	
};
static const struct attribute_group attr_group = {
	.attrs = attrs,
};
static void w25_dt_to_chip(struct device *dev)
{
	int ret;
	strcpy(prv->name, "w25q32fv");
	ret=device_property_read_u32(dev, "size", &prv->size);
	if(ret <0)
		pr_err("Error: missing \"size\" property\n");
	ret=device_property_read_u32(dev, "pagesize", &prv->page_size);
	if(ret <0)
		pr_err("Error: missing \"pagesize\" property\n");
	ret=device_property_read_u32(dev, "address-width",&prv->address_width);
	if(ret <0)
		pr_err("Error: missing \"addresswidth\" property\n");
}

/*
	devm_kzalloc() is resource-managed kzalloc(). The memory allocated with resource-managed 
	functions is associated with the device. When the device is detached from the system or the 
	driver for the device is unloaded, that memory is freed automatically. It is possible to free 
	the memory with devm_kfree() if it's no longer needed.
	static inline void *devm_kzalloc(struct device *dev, size_t size, gfp_t gfp)
*/
/**
 * spi_w8r8 - SPI synchronous 8 bit write followed by 8 bit read
 * @spi: device with which data will be exchanged
 * @cmd: command to be written before data is read back
 * Context: can sleep
 *
 * This returns the (unsigned) eight bit number returned by the
 * device, or else a negative error code.  Callable only from
 * contexts that can sleep.
 */
static int spi_w25flash_probe(struct spi_device *spi)
{
	ssize_t sr;
	int err; 	
	prv = devm_kzalloc(&spi->dev,sizeof(struct w25_priv),GFP_KERNEL);
	if(!prv)
		return -ENOMEM;
	w25_dt_to_chip(&spi->dev);
	pr_info("%s: device tree translation completed\n",__func__);
	
	prv->spi=spi;
	/* device driver data */
	spi_set_drvdata(spi, prv);
	
	sr = spi_w8r8(prv->spi, W25_Readid);	
	if(sr==W25_MNid)
		pr_info("Slave Manufracturer device Id found. \n");
	else{
		pr_info("Failed to find Manufracturer Id.");
		return sr;
	}		
	prv->kobj=kobject_create_and_add("w25q32_flash",NULL);
	if(!prv->kobj)
		return -EBUSY;
	err=sysfs_create_group(prv->kobj,&attr_group);
	if(err)
		kobject_put(prv->kobj);
	return 0;
}

static int spi_w25flash_remove(struct spi_device *spi)
{
	return 0;
}
static const struct of_device_id spi_w25flash_of_match[]= {
	{.compatible = "winbond,w25q32", },
	{ }
};


static struct spi_driver spi_w25flash_driver = {
	.driver = {
		.name           = "w25q32",
		.of_match_table = spi_w25flash_of_match,
	},
	.probe  = spi_w25flash_probe,
	.remove = spi_w25flash_remove,
};

/**
 * module_spi_driver() - Helper macro for registering a SPI driver
 * @__spi_driver: spi_driver struct
 *
 * Helper macro for SPI drivers which do not do anything special in module
 * init/exit. This eliminates a lot of boilerplate. Each module may only
 * use this macro once, and calling it replaces module_init() and module_exit()
 */

module_spi_driver(spi_w25flash_driver);



MODULE_DESCRIPTION("Driver for SPI based w25q32fv Flash memory");
MODULE_AUTHOR("Chandan jha <beingchandanjha@gmail.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION(".1");

