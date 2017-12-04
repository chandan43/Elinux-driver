#include <linux/module.h>
#include <linux/init.h>
#include <linux/spi/spi.h>
#include <linux/device.h>
#include <linux/kobject.h>
#include <linux/property.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/delay.h>

#define EN25_MAXADDRLEN  3       /* 20 bit address, up to 16 MBytes */
#define EN25_TIMEOUT     25
#define IO_LIMIT        256     /* bytes */

enum INSTw25id {
	EN25_Readid   =  0x9F,    /*read Manufacturer id  :*/
	EN25_MNid     =  0x1C,    /*Manufacturer id */
};

enum StausReg {
	EN25_ReadSR     = 0x05,   /* read status register */
	EN25_WriteSR    = 0x01,   /* write status register */
};

enum StausVal {
	EN25_Notrdy   = 0x01,   /* nRDY = write-in-progres pg.17  */
	EN25_Write_En = 0x02,   /* write enable (latched) */
};

enum ChipCmd {
	EN25_WriteEn  = 0x06,   /* latch the write enable */
	EN25_WriteDis = 0x04,   /* reset the write enablei i.e write Disable */
	EN25_Read     = 0x03,   /* read byte(s) */
	EN25_Write    = 0x02,   /* write byte(s)/sector */
	EN25_SecErase = 0x20,    /*Erase sector*/
};

struct en25t80_priv {
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

struct en25t80_priv *prv = NULL;

static ssize_t en25_flash_read(struct en25t80_priv *en25, char *buf, unsigned offset, 
				size_t count)
{
	struct spi_transfer t[2];
	struct spi_message m;
	ssize_t status;
	u8 cp[5];
	
	pr_info(" Reading data from block :: sector :: page <=> 0x%x:0x%x:0x%x \n",
		en25->block, en25->sector, en25->page);
	
	cp[0] = (u8)EN25_Read;
	cp[1] = offset >> 16;
	cp[2] = offset >> 8;
	cp[3] = offset >> 0;

	spi_message_init(&m);
	
	memset(t, 0, sizeof(t));	
	t[0].tx_buf = cp;
	t[0].len = en25->address_width/8 + 1;
	spi_message_add_tail(&t[0], &m);

	memset(buf, '\0', IO_LIMIT);
	t[1].rx_buf = buf;
	t[1].len = count;
	spi_message_add_tail(&t[1], &m);

	status = spi_sync(en25->spi, &m);
	if (status)
		pr_err("read %zd bytes at %d --> %d\n",
                          count, offset, (int) status);

	return status ? status : count ;
}

/**
 * spi_write - SPI synchronous write
 * @spi: device to which data will be written
 * @buf: data buffer
 * @len: data buffer size
 * Context: can sleep
 *
 * This function writes the buffer @buf.
 * Callable only from contexts that can sleep.
 *
 * Return: zero on success, else a negative error code.
 */
/**
 * spi_sync - blocking/synchronous SPI data transfers
 * @spi: device with which data will be exchanged
 * @message: describes the data transfers
 * Context: can sleep
 *
 * This call may only be used from a context that may sleep.  The sleep
 * is non-interruptible, and has no timeout.  Low-overhead controller
 * drivers may DMA directly into and out of the message buffers.
 *
 * Note that the SPI device's chip select is active during the message,
 * and then is normally disabled between messages.  Drivers for some
 * frequently-used devices may want to minimize costs of selecting a chip,
 * by leaving it selected in anticipation that the next message will go
 * to the same chip.  (That may increase power usage.)
 *
 * Also, the caller is guaranteeing that the memory associated with the
 * message will not be freed before this call returns.
 *
 * Return: zero on success, else a negative error code.
 */
/**
 * spi_w8r8 - SPI synchronous 8 bit write followed by 8 bit read
 * @spi: device with which data will be exchanged
 * @cmd: command to be written before data is read back
 * Context: can sleep
 *
 * Callable only from contexts that can sleep.
 *
 * Return: the (unsigned) eight bit number returned by the
 * device, or else a negative error code.
 */

static ssize_t en25_flash_write(struct en25t80_priv *en25, const char *buf, 
				loff_t off, size_t count)
{
	struct spi_transfer t;
	struct spi_message m;
	unsigned long   timeout, retries;
	ssize_t         sr, status = 0;
	u8              *tmp;
	char            cp[2];

	if(count > IO_LIMIT){
		pr_err("Data exceeds IO_LIMIT: Write Operation Terminated\n");
		return -EFBIG;
	}
	pr_info(" Writing data to block :: sector :: page <=> 0x%x:0x%x:0x%x \n",
		en25->block, en25->sector, en25->page);
	tmp=kmalloc(count + en25->address_width/8 + 1,GFP_KERNEL);
	if(!tmp){
		pr_err("Memory is not allocated for tmp\n");
		return -ENOMEM;
	}
	/*Making write enable */
	cp[0]=(u8)EN25_WriteEn;
	status = spi_write(en25->spi, cp, 1);
	if(status){
		pr_info("WREN --> %d\n", (int) status);
		return status;
	}
	/*Shifting due to address bus is 24 bits so coping to local buff only  8 bits using shifting*/
	tmp[0] = (u8)EN25_Write;
	tmp[1] = off >> 16;
	tmp[2] = off >> 8;
	tmp[3] = off >>0;
	
	memcpy(tmp+4, buf, count);

	t.tx_buf = tmp;
	t.len	 = count+4;	
	
	/* Spi msg init */
	spi_message_init(&m);	
	/* Add the message : spi_message_add_tail(struct spi_transfer *t, struct spi_message *m) */
	spi_message_add_tail(&t, &m);

	status = spi_sync(en25->spi, &m);
	if(status){
		kfree(tmp);
		return status;
	}
	/*timeout = current time + 25ms */
	timeout = jiffies + msecs_to_jiffies(EN25_TIMEOUT);
	retries = 0;
	do{
		sr=spi_w8r8(en25->spi,EN25_ReadSR); /* Check for write in progress status */
		if(sr & EN25_Notrdy){
			msleep(1);
			continue;
		}
		if(!(sr & EN25_Notrdy))	
			break;
	}while(retries++ < 3 || time_before_eq(jiffies, timeout));

	kfree(tmp);

	return count;
}

static ssize_t en25_sys_write(struct kobject *kobj, struct kobj_attribute *attr,
				 const char *buf, size_t count)
{
	struct en25t80_priv *en25 = prv;
	
	return en25_flash_write(en25, buf, en25->offset, count);

}


static ssize_t eb25_sys_read(struct kobject *kobj, struct kobj_attribute *attr,
				 char *buf)
{
	struct en25t80_priv *en25 = prv;
	size_t count = IO_LIMIT;

	return en25_flash_read(en25, buf, (int)en25->offset, count);
}

static ssize_t en25_set_offset(struct kobject *kobj, struct kobj_attribute *attr, 
				const char *buf, size_t count)
{
	struct en25t80_priv *en25 = prv;
	unsigned long tmp;
	unsigned int i, j, k;
	char cp[10], temp[10];
	
	i=j=k=0;
	for(k=0;k<2;k++){
		memset(cp, '\0', sizeof(cp));
		memset(temp, '\0', sizeof(temp));
		strcpy(temp, buf+j);
		i=0;
		do{
			if (strncmp(temp+i, ":", 1) != 0) {
				i++; j++;
			}else{
				strncpy(cp, temp, i);
				if (k == 0) {
					if (!kstrtol(cp, 16, &tmp))
						en25->block = tmp;       /*Block*/
				}else if (k == 1) {
					if (!kstrtol(cp, 16, &tmp))
						en25->sector = tmp;      /*Sector*/
				}
				i++; j++;
				break;
			}
		}while(1);
		if(k == 1 ){
			strcpy(temp, buf+j);
			if (!kstrtol(temp, 16, &tmp))
				 en25->page = tmp;        /*Page*/
		}
	}
	if (en25->block > 0xf || en25->sector > 0xf || en25->page > 0xf) {
                 pr_info("\tSpecify offset in following format\n");
                 pr_info("Blk:sec:page<=><0x0-0xf>:<0x0-0xf>:<0x0-0xf>\n");
                 return -EFAULT;
         }
	en25->offset = (en25->block*0x10000) +  (en25->sector*0x1000) +
			(en25->page*0x100);
	pr_info(" block:sector:page <=> 0x%x:0x%x:0x%x \n",
		en25->block, en25->sector, en25->page);
	
	return count;
}
static ssize_t en25_get_offset(struct kobject *kobj, 
				struct kobj_attribute *attr, char *buf)
{
	struct en25t80_priv *en25 = prv;
	pr_info(" block:sector:page <=> 0x%x:0x%x:0x%x \n",
		en25->block, en25->sector, en25->page);

	return strlen(buf);
}



static struct kobj_attribute en25_rw =
		__ATTR(en25q32, 0660, eb25_sys_read, en25_sys_write);
static struct kobj_attribute en25_offset =
		__ATTR(offset, 0660, en25_get_offset, en25_set_offset);

static struct attribute *attrs[] = {
	&en25_rw.attr,
	&en25_offset.attr,
	NULL	
};
static const struct attribute_group attr_group = {
	.attrs = attrs,
};
static void en25_dt_to_chip(struct device *dev)
{
	int ret;
	strcpy(prv->name, "en25t80");
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
static int en25t80_probe(struct spi_device *spi)
{
	ssize_t sr;
	int err; 	
	prv = devm_kzalloc(&spi->dev,sizeof(struct en25t80_priv),GFP_KERNEL);
	if(!prv)
		return -ENOMEM;
	en25_dt_to_chip(&spi->dev);
	pr_info("%s: device tree translation completed\n",__func__);
	
	prv->spi=spi;
	/* device driver data */
	spi_set_drvdata(spi, prv);
	
	sr = spi_w8r8(prv->spi, EN25_Readid);
	if(sr==EN25_MNid)
		pr_info("Slave Manufracturer device Id found. \n");
	else{
		pr_info("ID=%04x\n",sr);
		pr_info("Failed to find Manufracturer Id.\n");
		return sr;
	}		
	prv->kobj=kobject_create_and_add("en25t80",NULL);
	if(!prv->kobj)
		return -EBUSY;
	err=sysfs_create_group(prv->kobj,&attr_group);
	if(err)
		kobject_put(prv->kobj);
	return 0;
}

static int en25t80_remove(struct spi_device *spi)
{
	kobject_put(prv->kobj);
	return 0;
}
static const struct of_device_id en25t80_of_match[]= {
	{.compatible = "EON,EN25T80", },
	{ }
};


static struct spi_driver en25t80_driver = {
	.driver = {
		.name           = "en25t80",
		.of_match_table = en25t80_of_match,
	},
	.probe  = en25t80_probe,
	.remove = en25t80_remove,
};

/**
 * module_spi_driver() - Helper macro for registering a SPI driver
 * @__spi_driver: spi_driver struct
 *
 * Helper macro for SPI drivers which do not do anything special in module
 * init/exit. This eliminates a lot of boilerplate. Each module may only
 * use this macro once, and calling it replaces module_init() and module_exit()
 */

module_spi_driver(en25t80_driver);



MODULE_DESCRIPTION("Driver for SPI based EN25T80 Flash memory");
MODULE_AUTHOR("Chandan jha <beingchandanjha@gmail.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION(".1");

