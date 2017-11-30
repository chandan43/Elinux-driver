#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>

/*platform_get_resource(struct platform_device *,unsigned int, unsigned int);
 * #@param1: The first parameter tells the function which device we are interested in, so it can extract the info we need.
 * #@param2:  he second parameter depends on what kind of resource you are handling. 
 * If it is memory( or anything that can be mapped as memory :-)) then it's IORESOURCE_MEM. 
 * #@param3: The last parameter says which resource of that type is desired, with zero indicating the first one.
 */
/*void __iomem *devm_ioremap_resource(struct device *dev, struct resource *res);*/
/**
 * devm_ioremap_resource() - check, request region, and ioremap resource
 * @dev: generic device to handle the resource for
 * @res: resource to be handled
 *
 * Checks that a resource is a valid memory region, requests the memory
 * region and ioremaps it. All operations are managed and will be undone
 * on driver detach.
 *
 * Returns a pointer to the remapped memory or an ERR_PTR() encoded error code
 * on failure. Usage example:
 *
 *	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
 *	base = devm_ioremap_resource(&pdev->dev, res);
 *	if (IS_ERR(base))
 *		return PTR_ERR(base);
 */

static int virtualdev_probe(struct platform_device *pdev)
{
	void __iomem *base;
	struct resource *res;
	pr_info("%s: Invoked.! \n",__func__);
	pr_info("Driver binded: %s\n",pdev->name);
	res=platform_get_resource(pdev,IORESOURCE_MEM,0);  /*Get information on the structure of the device resource */
	base=devm_ioremap_resource(&pdev->dev,res);
	if (IS_ERR(base)){
		pr_err("Failed to remap device registers\n");
		return PTR_ERR(base);
	}
	pr_info("device start %x : end : %x\n", res->start, res->end);
	pr_info("device name %s\n", res->name);
	pr_info("address of device as seen by kernel: %p\n", base);
	
	return 0;
}
static int virtualdev_remove(struct platform_device *pdev)
{
	pr_info("%s: Rmmod performed\n", __func__);
	return 0;
}

static const struct of_device_id virtual_of_match_table[] = {
	{.compatible = "ti,techved-vDev"},
	{},
};
MODULE_DEVICE_TABLE(of,virtual_of_match_table);

static  struct platform_driver virtual_driver={
	.driver = {
		.name          = "virtual_drv",
		.owner         = THIS_MODULE,
		.of_match_table= virtual_of_match_table,                     /* @of_match_table: The open firmware table.*/
	},
	.probe  =virtualdev_probe,
	.remove =virtualdev_remove,
		
};
static int __init virtual_init(void)
{
	pr_info("My first driver for BBB board:  My virtual node related driver successfully inserted\n");	
	return platform_driver_register(&virtual_driver);
}

static void __exit virtual_exit(void)
{
	platform_driver_unregister(&virtual_driver);
	pr_info("%s: My virtual node related driver successfully removed\n",__func__);	
}

module_init(virtual_init);
module_exit(virtual_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chandan Jha <beingchandanjha@gmail.com>");
MODULE_DESCRIPTION("platform driver for off-board led device");
MODULE_VERSION(".1");
