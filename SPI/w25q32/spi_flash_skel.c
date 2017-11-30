#include <linux/module.h>
#include <linux/init.h>
#include <linux/spi/spi.h>

static int spi_w25flash_probe(struct spi_device *spi)
{
	return 0;
}

static int spi_w25flash_remove(struct spi_device *spi)
{
	return 0;
}
static const struct of_device_id spi_w25flash_of_match[]= {
	{.compatible = "winbond,w25q32" },
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

