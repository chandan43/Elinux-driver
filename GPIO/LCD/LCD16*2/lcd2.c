/* Description: a kernel level Linux device driver to control a 16x2 character.
                The LCD is interfaced with a micro-controller using GPIO pins.
                See below "LCD Pin Configuration" to see GPIO pin connections

                (Tested on Linux 4.4.96)

   platform:    Beaglebone Black
   Pin Connection :
	LCD_DB6: P8_14 (GPIO pin 26)
	LCD_DB7: P8_12 (GPIO pin 44)
	LCD_DB5: P8_16 (GPIO pin 46)
	LCD_DB4: P8_18 (GPIO pin 65)
	LCD_RS:  P8_8  (GPIO pin 67)
	LCD_E:   P8_10 (GPIO pin 68)
*/


#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>
#include <linux/kobject.h>
#include <linux/delay.h> // delay

#define OMAP_GPIO_OE           0x0134
#define OMAP_GPIO_SETDATAOUT   0x0194
#define OMAP_GPIO_CLEARDATAOUT 0x0190
#define OMAP_GPIO_DATAIN       0x0138

#define CM_PER_EN 0x02
#define CM_PER 0x44E00000           // Clock Module Peripheral Registers

#define RS_COMMAND_MODE         0   // command mode to select Insruction register with RS signal
#define RS_DATA_MODE            1   // data mode to select Data register with RS signal
#define LCD_FIRST_LINE          1
#define LCD_SECOND_LINE         2

#define NUM_CHARS_PER_LINE      16  // the number of characters per line

struct gpiopin_bank {
        u32 gpio_base;
	unsigned int pinno;
        unsigned int LCD_RS_PIN_NUMBER;    //LCD_RS:  P8_8  (GPIO pin 67)
	unsigned int LCD_E_PIN_NUMBER;     //LCD_E:   P8_10 (GPIO pin 68)
	unsigned int LCD_DB4_PIN_NUMBER;   //LCD_DB4: P8_18 (GPIO pin 65)
	unsigned int LCD_DB5_PIN_NUMBER;   //LCD_DB5: P8_16 (GPIO pin 46)
	unsigned int LCD_DB6_PIN_NUMBER;   //LCD_DB6: P8_14 (GPIO pin 26)
	unsigned int LCD_DB7_PIN_NUMBER;   //LCD_DB7: P8_12 (GPIO pin 44) 
};

enum CM_PER_offset {
	CM_PER_GPIO1          = 0xAC,
	CM_PER_GPIO2	      = 0xB0,
	CM_PER_GPIO3	      =	0xB4,
};

enum GPIO_BASE_Addrs {
		GPIO0_BASE    = 0x44E07000,
		GPIO1_BASE    = 0x481AC000,
		GPIO2_BASE    = 0x481AE000,
};
static struct gpiopin_bank *gpbank;
static void *gpio0bank_base;
static void *gpio1bank_base;
static void *clock_module_base;
static struct kobject *kobj;

/*
This will insert a barrier() so we know that the CPU will execute the
write of a before the write of b. However it does not mandate that
reg2 is written with b before reg1 is written with a at the hardware
register level. 
 
*/
static void lcd_pin_setup(void *gbank_base,unsigned int pinno)
{
	unsigned int data = 0;
	data = readl_relaxed(gbank_base + OMAP_GPIO_OE);
	data = data & ~((1 << pinno));
	pr_info("lcd_init: direction of pin is set: %x\n",data);
	writel_relaxed(data, gbank_base + OMAP_GPIO_OE);
	return;

}
static void lcd_pin_setup_All(void)
{
	lcd_pin_setup(gpio1bank_base,gpbank->LCD_DB7_PIN_NUMBER);
	lcd_pin_setup(gpio1bank_base,gpbank->LCD_RS_PIN_NUMBER);
	lcd_pin_setup(gpio1bank_base,gpbank->LCD_E_PIN_NUMBER);
	
	lcd_pin_setup(gpio1bank_base,gpbank->LCD_DB5_PIN_NUMBER);
	
	lcd_pin_setup(gpio0bank_base,gpbank->LCD_DB6_PIN_NUMBER);
	lcd_pin_setup(gpio0bank_base,gpbank->LCD_DB4_PIN_NUMBER);
}
	 
static void gpio_set_val(void *gbank_base,unsigned int pinno,int data)
{
	int old_data;
	old_data = readl_relaxed(gbank_base + OMAP_GPIO_DATAIN);
	old_data = old_data | (data << pinno);
	writel_relaxed(old_data, gbank_base + OMAP_GPIO_SETDATAOUT);
}

/*
 * description:		 send a 4-bit command to the HD44780 LCD controller.
 *
 * @param command	 command to be sent to the LCD controller. Only the upper 4 bits of this command is used.
*/
static void lcd_instruction(char command)
{
	int db7_data = 0;
	int db6_data = 0;
	int db5_data = 0;
	int db4_data = 0;

	usleep_range(2000, 3000);	// added delay instead of busy checking

	// Upper 4 bit data (DB7 to DB4)
	db7_data = ( (command)&(0x1 << 7) ) >> (7) ;
	db6_data = ( (command)&(0x1 << 6) ) >> (6) ;
	db5_data = ( (command)&(0x1 << 5) ) >> (5) ;
	db4_data = ( (command)&(0x1 << 4) ) >> (4) ;
	
	
	gpio_set_val(gpio1bank_base,gpbank->LCD_DB7_PIN_NUMBER,db7_data);
	gpio_set_val(gpio0bank_base,gpbank->LCD_DB6_PIN_NUMBER,db6_data);
	gpio_set_val(gpio1bank_base,gpbank->LCD_DB5_PIN_NUMBER,db5_data);
	gpio_set_val(gpio0bank_base,gpbank->LCD_DB4_PIN_NUMBER,db4_data);


	// Set to command mode
	gpio_set_val(gpio1bank_base,gpbank->LCD_RS_PIN_NUMBER, RS_COMMAND_MODE);
	usleep_range(5, 10);

	// Simulate falling edge triggered clock
	gpio_set_val(gpio1bank_base,gpbank->LCD_E_PIN_NUMBER, 1);
	usleep_range(5, 10);
	gpio_set_val(gpio1bank_base,gpbank->LCD_E_PIN_NUMBER, 0);
}

/*
 * description:		send a 1-byte ASCII character data to the HD44780 LCD controller.
 * @param data		a 1-byte data to be sent to the LCD controller. Both the upper 4 bits and the lower 4 bits are used.
*/
static void lcd_data(char data)
{
	int db7_data = 0;
	int db6_data = 0;
	int db5_data = 0;
	int db4_data = 0;

	// Part 1.  Upper 4 bit data (from bit 7 to bit 4)
	usleep_range(2000, 3000); 	// added delay instead of busy checking

	db7_data = ( (data)&(0x1 << 7) ) >> (7) ;
	db6_data = ( (data)&(0x1 << 6) ) >> (6) ;
	db5_data = ( (data)&(0x1 << 5) ) >> (5) ;
	db4_data = ( (data)&(0x1 << 4) ) >> (4) ;
	
	gpio_set_val(gpio1bank_base,gpbank->LCD_DB7_PIN_NUMBER,db7_data);
	gpio_set_val(gpio0bank_base,gpbank->LCD_DB6_PIN_NUMBER,db6_data);
	gpio_set_val(gpio1bank_base,gpbank->LCD_DB5_PIN_NUMBER,db5_data);
	gpio_set_val(gpio0bank_base,gpbank->LCD_DB4_PIN_NUMBER,db4_data);


	// Part 1. Set to data mode
	gpio_set_val(gpio1bank_base,gpbank->LCD_RS_PIN_NUMBER, RS_DATA_MODE);
	usleep_range(5, 10);

	// Part 1. Simulate falling edge triggered clock
	gpio_set_val(gpio1bank_base,gpbank->LCD_E_PIN_NUMBER, 1);
	usleep_range(5, 10);
	gpio_set_val(gpio1bank_base,gpbank->LCD_E_PIN_NUMBER, 0);

	// Part 2. Lower 4 bit data (from bit 3 to bit 0)
	usleep_range(2000, 3000);	// added delay instead of busy checking

	db7_data = ( (data)&(0x1 << 3) ) >> (3) ;
	db6_data = ( (data)&(0x1 << 2) ) >> (2) ;
	db5_data = ( (data)&(0x1 << 1) ) >> (1) ;
	db4_data = ( (data)&(0x1)      )        ;

	gpio_set_val(gpio1bank_base,gpbank->LCD_DB7_PIN_NUMBER,db7_data);
	gpio_set_val(gpio0bank_base,gpbank->LCD_DB6_PIN_NUMBER,db6_data);
	gpio_set_val(gpio1bank_base,gpbank->LCD_DB5_PIN_NUMBER,db5_data);
	gpio_set_val(gpio0bank_base,gpbank->LCD_DB4_PIN_NUMBER,db4_data);
	
	// Part 2. Set to data mode
	gpio_set_val(gpio1bank_base,gpbank->LCD_RS_PIN_NUMBER, RS_DATA_MODE);
	usleep_range(5, 10);

	// Part 2. Simulate falling edge triggered clock
	gpio_set_val(gpio1bank_base,gpbank->LCD_E_PIN_NUMBER, 1);
	usleep_range(5, 10);
	gpio_set_val(gpio1bank_base,gpbank->LCD_E_PIN_NUMBER, 0);
}
/*
 * description: 	initialize the LCD in 4 bit mode as described on the HD44780 LCD controller document.
*/
static void lcd_initialize(void)
{
	usleep_range(41*1000, 50*1000);	// wait for more than 40 ms once the power is on

	lcd_instruction(0x30);		// Instruction 0011b (Function set)
	usleep_range(5*1000, 6*1000);	// wait for more than 4.1 ms

	lcd_instruction(0x30);		// Instruction 0011b (Function set)
	usleep_range(100,200);		// wait for more than 100 us

	lcd_instruction(0x30);		// Instruction 0011b (Function set)
	usleep_range(100,200);		// wait for more than 100 us

	lcd_instruction(0x20);		/* Instruction 0010b (Function set)
					   Set interface to be 4 bits long
					*/
	usleep_range(100,200);		// wait for more than 100 us

	lcd_instruction(0x20);		// Instruction 0010b (Function set)
	lcd_instruction(0x80);		/* Instruction NF**b
					   Set N = 1, or 2-line display
					   Set F = 0, or 5x8 dot character font
					 */
	usleep_range(41*1000,50*1000);

					/* Display off */
	lcd_instruction(0x00);		// Instruction 0000b
	lcd_instruction(0x80);		// Instruction 1000b
	usleep_range(100,200);

					/* Display clear */
	lcd_instruction(0x00);		// Instruction 0000b
	lcd_instruction(0x10);		// Instruction 0001b
	usleep_range(100,200);

					/* Entry mode set */
	lcd_instruction(0x00);		// Instruction 0000b
	lcd_instruction(0x60);		/* Instruction 01(I/D)Sb -> 0110b
					   Set I/D = 1, or increment or decrement DDRAM address by 1
					   Set S = 0, or no display shift
					*/
	usleep_range(100,200);

	/* Initialization Completed, but set up default LCD setting here */

					/* Display On/off Control */
	lcd_instruction(0x00);		// Instruction 0000b
	lcd_instruction(0xF0);		/* Instruction 1DCBb  
					   Set D= 1, or Display on
					   Set C= 1, or Cursor on
					   Set B= 1, or Blinking on
					*/
	usleep_range(100,200);
}

/*
 * description:	set the cursor to the beginning of the line specified.
 * @param line  line number should be either 1 or 2.	
*/
void lcd_setLinePosition(unsigned int line)
{
	if(line == 1){
		lcd_instruction(0x80);	// set position to LCD line 1
		lcd_instruction(0x00);
	}
	else if(line == 2){
		lcd_instruction(0xC0);
		lcd_instruction(0x00);
	}
	else{
		printk("ERR: Invalid line number. Select either 1 or 2 \n");
	}
}

/*
 * description: 	print a string data on the LCD
 * 			(If the line number is 1 and the string is too long to be fit in the first line, the LCD
 * 			will continue to print the string on the second line)
 *
 * @param lineNumber	the line number of the LCD where the string is be printed. It should be either 1 or 2.
 * 			Otherwise, it is readjusted to 1.
 *
 * detail:		I implemented the code to only allow a certain number of characters to be written on the LCD.
 * 			As each character is written, the DDRAM address in the LCD controller is incrmented.
 * 			When the string is too long to be fit in the LCD, the DDRAM can be set back to 0 and the existing
 * 			data on the LCD are overwritten by the new data. This causes the LCD to be very unstable and also
 * 			lose data.		
*/
static void lcd_print(char * msg, unsigned int lineNumber)
{
	unsigned int counter = 0;
	unsigned int lineNum = lineNumber;

	if(msg == NULL){
		printk( KERN_DEBUG "ERR: Empty data for lcd_print \n");
		return;
	}

	if( (lineNum != 1) && (lineNum != 2) ) { 
		printk( KERN_DEBUG "ERR: Invalid line number readjusted to 1 \n");
		lineNum = 1;
	}

	if( lineNum == 1 )
	{
		lcd_setLinePosition( LCD_FIRST_LINE );

		while( *(msg) != '\0' )
		{
			if(counter >=  NUM_CHARS_PER_LINE )
			{
				lineNum = 2;	// continue writing on the next line if the string is too long
				counter = 0;
				break;		
			}

			lcd_data(*msg);
			msg++;
			counter++;
		}
	}

	if( lineNum == 2)
	{
		lcd_setLinePosition( LCD_SECOND_LINE);
		
		while( *(msg) != '\0' )
		{
			if(counter >=  NUM_CHARS_PER_LINE )
			{
				 break;
			}

			lcd_data(*msg);
			msg++;
			counter++;
		}
	}
}

/*
 * description:	clear the display on the LCD	
*/
static void lcd_clearDisplay(void)
{
	lcd_instruction( 0x00 ); // upper 4 bits of command
	lcd_instruction( 0x10 ); // lower 4 bits of command

	printk(KERN_INFO "klcd Driver: display clear\n");
}
/*
 * description:	turn off the LCD display. It is called upon module exit	
*/
static void lcd_display_off(void)
{
	lcd_instruction(0x00);		// Instruction 0000b
	lcd_instruction(0x80);		/* Instruction 1DCBb  
					   Set D= 0, or Display off

					   Set C= 0, or Cursor off
					   Set B= 0, or Blinking off
					*/
	printk(KERN_INFO "klcd Driver: lcd_display_off\n");
}


static ssize_t lcd_write(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret;
	char d[1023];
	pr_info("%s: invoked from sysfs...\n",__func__);
	ret=sscanf(buf,"%s",d);
	if(ret < 0)
		return -EINVAL;
	lcd_print(d, LCD_FIRST_LINE);
	return count;
}
static struct kobj_attribute lcd_attr =__ATTR(extled, 0220, NULL, lcd_write);
/**
 * of_get_named_gpio() - Get a GPIO number to use with GPIO API
 * @np:		device node to get GPIO from
 * @propname:	Name of property containing gpio specifier(s)
 * @index:	index of the GPIO
 *
 * Returns GPIO number to use with Linux generic GPIO API, or one of the errno
 * value on the error condition.
 */
/*struct device_node *of_parse_phandle(const struct device_node *np, 
                                            const char *phandle_name,
                                            int index);
*/
/**
 * of_property_read_u32_u32 - Find and read a property.
 *
 * @np:         device node from which the property value is to be read.
 * @propname:   name of the property to be searched.
 * @out_values: pointer to return value, modified only if return value is 0.
 *
 *
 * Search for a property in a device node and read 32-bit value(s) from
 * it. Returns 0 on success, -EINVAL if the property does not exist,
 * -ENODATA if property does not have a value, and -EOVERFLOW if the
 * property data isn't large enough.
 *
 * The out_values is modified only if a valid u32 value can be decoded.
 */
static int get_gpio_pinno(struct platform_device *pdev,unsigned int offset)
{
	unsigned int pinno;
	pinno = of_get_named_gpio(pdev->dev.of_node, "gpios", offset);	
	if(pinno > 0 )
		return pinno % 32;
	return -1;
}
static int get_gpiobank_address_maping(void)
{
	gpio0bank_base = ioremap(GPIO0_BASE, 4095);
	if(!gpio0bank_base)
		return -1;
	pr_info("GPIO0 Base address as seen by kernel: %p\n", gpio0bank_base);
	
	gpio1bank_base = ioremap(GPIO1_BASE, 4095);
	if(!gpio1bank_base)
		return -1;
	pr_info("GPIO1 Base address as seen by kernel: %p\n", gpio1bank_base);

	return 0;
}

static int get_clockmodule_addr_mapping(void)
{
	clock_module_base = ioremap(CM_PER,1023);
	if(!clock_module_base)
		return -1;
	pr_info("Clock module base address as seen by kernel: %p\n", clock_module_base);

	/*Enabling GPIO1 and GPIO2 bank : by default disable*/
	writel_relaxed(CM_PER_EN,clock_module_base + CM_PER_GPIO1);
	writel_relaxed(CM_PER_EN,clock_module_base + CM_PER_GPIO1);
	writel_relaxed(CM_PER_EN,clock_module_base + CM_PER_GPIO1);
	return 0;
}
static int lcd_probe(struct platform_device *pdev)
{
	int ret;

	pr_info("%s: Invoked.!\n",__func__);

	gpbank = (struct gpiopin_bank *)kmalloc(sizeof(struct gpiopin_bank), GFP_KERNEL);
	/* Acess GPIO PIN number*/
	gpbank->pinno = of_get_named_gpio(pdev->dev.of_node, "gpios", 0);
	gpbank->LCD_DB6_PIN_NUMBER = get_gpio_pinno(pdev, 0);
	gpbank->LCD_DB7_PIN_NUMBER = get_gpio_pinno(pdev, 1);
	gpbank->LCD_DB5_PIN_NUMBER = get_gpio_pinno(pdev, 2);
	gpbank->LCD_DB4_PIN_NUMBER = get_gpio_pinno(pdev, 3);
	gpbank->LCD_RS_PIN_NUMBER  = get_gpio_pinno(pdev, 4);
	gpbank->LCD_E_PIN_NUMBER   = get_gpio_pinno(pdev, 5);

	pr_info("GPIO Pin:  DB4 : %u, DB5 : %u, DB6 : %u, DB7 : %u, E: %u ,RS: %u\n",
			gpbank->LCD_DB4_PIN_NUMBER,gpbank->LCD_DB5_PIN_NUMBER,
			gpbank->LCD_DB6_PIN_NUMBER,gpbank->LCD_DB7_PIN_NUMBER,
			gpbank->LCD_E_PIN_NUMBER,gpbank->LCD_RS_PIN_NUMBER);

	
	get_gpiobank_address_maping();
	get_clockmodule_addr_mapping();	

	kobj = kobject_create_and_add("led", NULL);
	if(!kobj)
                return -ENOMEM;

        ret = sysfs_create_file(kobj, &lcd_attr.attr);
        if(ret)
                kobject_put(kobj);
	// setup GPIO pins
	lcd_pin_setup_All();

	//initialize LCD once
	lcd_initialize();

	return 0;
}
static void gpio_unmapping(void)
{
	iounmap(gpio0bank_base);
	iounmap(gpio1bank_base);
	iounmap(clock_module_base);
}
static int lcd_remove(struct platform_device *pdev)
{
	lcd_display_off();
	gpio_unmapping();
        kobject_put(kobj);
	kfree(gpbank);
	pr_info("%s: Rmmod performed\n", __func__);
	return 0;
}

static const struct of_device_id lcd_of_match_table[] = {
	{.compatible = "gpio-lcd"},
	{},
};
MODULE_DEVICE_TABLE(of,lcd_of_match_table);

static  struct platform_driver lcd_driver={
	.driver = {
		.name          = "lcd",
		.owner         = THIS_MODULE,
		.of_match_table= lcd_of_match_table,                     /* @of_match_table: The open firmware table.*/
	},
	.probe  =lcd_probe,
	.remove =lcd_remove,
		
};
static int __init lcd_init(void)
{
	pr_info("My first LED driver on BBB board\n");	
	return platform_driver_register(&lcd_driver);
}

static void __exit lcd_exit(void)
{
	platform_driver_unregister(&lcd_driver);
	pr_info("%s: My gpio node related driver successfully removed\n",__func__);	
}

module_init(lcd_init);
module_exit(lcd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chandan Jha <beingchandanjha@gmail.com>");
MODULE_DESCRIPTION("platform driver for 16x2 character LCD");
MODULE_VERSION(".1");
