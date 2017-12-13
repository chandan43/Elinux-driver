#include <linux/module.h>
#include <linux/init.h>

extern int lcd_exported_print(const char *,char ,char ,char);

int myinit(void){
	char data[10] = "MY DRIVER";
	printk("%s: LCD print\n",__func__);
	lcd_exported_print(data,1,0,10);
	return 0;
}

void myexit(void){
	printk("%s: Good Bye.!\n",__func__);
}

module_init(myinit);
module_exit(myexit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("chandan@jha.com");
MODULE_DESCRIPTION("Print driver is dependent on i2cLCD_drv.ko");


