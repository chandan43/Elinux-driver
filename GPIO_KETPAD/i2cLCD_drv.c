#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/export.h>
/*LCD CMD*/
#define LCD_BL					0x08
#define LCD_EN					0x04  // Enable bit
#define LCD_RW					0x02  // Read/Write bit
#define LCD_RS					0x01  // Register select bit
#define LCD_MASK_DISABLE_EN		        0xFB  // MASK of control

/*Delay*/
#define STROBE_EN_DELAY 		1		// nanosegundos
#define CLEAR_DELAY 			1		//
#define COMMAND_INIT_DELAY 		1		// ms
#define DRV_VERSION "0.1"

/*Flag for backlight */
static u8 backlightFlag = 0x01; // Flag de in

/*LCD Commands: Follow HD44780.pdf page 23 onward*/
enum LCD_Commands {
	LCD_CMD_CLEARDISPLAY   	= 0x01,
	LCD_CMD_RETURNHOME     	= 0x02,
	LCD_CMD_ENTRYMODESET   	= 0x04,
	LCD_CMD_DISPLAYCONTROL 	= 0x08,
	LCD_CMD_CURSORSHIFT    	= 0x10,
	LCD_CMD_FUNCTIONSET    	= 0x20,
	LCD_CMD_SETCGRAMADDR   	= 0x40,
	LCD_CMD_SETDDRAMADDR   	= 0x80,
	LCD_CLEARDISPLAY       	= 0x01,
	LCD_RETURNHOME         	= 0x02,
	LCD_ENTRYMODESET       	= 0x04,
	LCD_DISPLAYCONTROL     	= 0x08,
	LCD_CURSORSHIFT        	= 0x10,
	LCD_FUNCTIONSET        	= 0x20,
	LCD_SETCGRAMADDR       	= 0x40,
	LCD_SETDDRAMADDR       	= 0x80,
	LCD_CTRL_BLINK_ON      	= 0x01,
	LCD_CTRL_BLINK_OFF     	= 0x00,
	LCD_CTRL_DISPLAY_ON    	= 0x04,
	LCD_CTRL_DISPLAY_OFF   	= 0x00,
	LCD_CTRL_CURSOR_ON     	= 0x02,
	LCD_CTRL_CURSOR_OFF    	= 0x00,
	LCD_DISPLAYON          	= 0x04,
	LCD_DISPLAYOFF         	= 0x00,
	LCD_CURSORON           	= 0x02,
	LCD_CURSOROFF          	= 0x00,
	LCD_BLINKON            	= 0x01,
	LCD_BLINKOFF           	= 0x00,
	LCD_ENTRYRIGHT         	= 0x00,
	LCD_ENTRYLEFT          	= 0x02,
	LCD_ENTRYSHIFTINCREMENT	= 0x01,
	LCD_ENTRYSHIFTDECREMENT	= 0x00,
	LCD_DISPLAYMOVE        	= 0x08,
	LCD_CURSORMOVE         	= 0x00,
	LCD_MOVERIGHT          	= 0x04,
	LCD_MOVELEFT           	= 0x00,
	LCD_8BITMODE  	       	= 0x10,
	LCD_4BITMODE  	       	= 0x00,
	LCD_2LINE     	       	= 0x08,
	LCD_1LINE     	       	= 0x00,
	LCD_5x10DOTS  	       	= 0x04,
	LCD_5x8DOTS   	       	= 0x00,
};

static u8 lcd_data 		= 0x00;
// Mutex para controle de acesso concorrente
static DEFINE_MUTEX(lcd16x2_mutex);
typedef unsigned char BYTE;

struct I2c_LCD_prv {
	struct i2c_client client_prv;
	struct kobject *I2c_LCD_kobj;
	struct mutex lock;
};

struct I2c_LCD_prv *prv=NULL;

/* register access */

static int lcd_i2c_write_byte(struct i2c_client *client, u8 *data) 
{
  	int ret;
  	ret = i2c_master_send(client, data, 1);
  	if (ret < 0)
      		dev_warn(&client->dev, "[DEBUG][%s] Write byte in i2c ['0x%02X'] failed.\n", DRV_VERSION, ((int)*data));
  	return ret;
}

static int lcd_i2c_read_byte(struct i2c_client *client) 
{
 	unsigned char i2c_data[1];
  	int ret = 0;
  	ret = i2c_master_recv(client, i2c_data, 1);
  	if (ret < 0) {
    		dev_warn(&client->dev, "[DEBUG][%s] LCD 16x4 i2c read data failed\n", DRV_VERSION);
    		return ret;
  	}
  	return (i2c_data[0]);
}
/*Enable the lcd*/
static void lcd_en_strobe(struct i2c_client *client, int msdelay) 
{
  	int ret = 0;
  	lcd_data = lcd_i2c_read_byte(client);
  	lcd_data = (lcd_data | LCD_EN | (backlightFlag == 1 ? LCD_BL : 0x00));  
  	ret = lcd_i2c_write_byte(client, &lcd_data);
  	ndelay(msdelay);	
  	lcd_data &= LCD_MASK_DISABLE_EN | (backlightFlag == 1 ? LCD_BL : 0x00); 
  	ret = lcd_i2c_write_byte(client, &lcd_data);
}

static int lcd_send_cmd(struct i2c_client *client, u8 cmd, int msdelay) 
{
 	BYTE d;
  	int ret = 0;
  	mutex_lock(&lcd16x2_mutex);
  	lcd_data = cmd;
  	d = (cmd & 0xF0) | (backlightFlag == 1 ? LCD_BL : 0x00);
  	lcd_i2c_write_byte(client, &d);
  	lcd_en_strobe(client, STROBE_EN_DELAY);
  	d = (cmd << 4) | (backlightFlag == 1 ? LCD_BL : 0x00);
  	lcd_i2c_write_byte(client, &d);
  	lcd_en_strobe(client, STROBE_EN_DELAY);
  	msleep(msdelay);
  	mutex_unlock(&lcd16x2_mutex);
  	if (ret < 0)
    		dev_warn(&client->dev, "[DEBUG][%s] command '%c' failed.\n", DRV_VERSION, cmd);
  	return ret;
}

static int lcd_send_data(struct i2c_client *client, u8 data) 
{
  	BYTE d;
  	int ret = 0;
  	mutex_lock(&lcd16x2_mutex);
  	lcd_data = data;
  	d = (data & 0xF0) | LCD_RS | (backlightFlag == 1 ? LCD_BL : 0x00);
  	lcd_i2c_write_byte(client, &d);
  	lcd_en_strobe(client, STROBE_EN_DELAY);
  	d = (data << 4) | LCD_RS | (backlightFlag == 1 ? LCD_BL : 0x00);
  	lcd_i2c_write_byte(client, &d);
 	lcd_en_strobe(client, STROBE_EN_DELAY);
  	mutex_unlock(&lcd16x2_mutex);
  	if (ret < 0)
    		dev_warn(&client->dev, "[DEBUG][%s] data '%c' failed.\n", DRV_VERSION, data);
  	return ret;
}

static void _setDDRAMAddress(struct i2c_client *client, int line, int col)
{
  	//we write to the Data Display RAM (DDRAM)
  	if(line == 1)
    		lcd_send_cmd(client, LCD_SETDDRAMADDR | (0x00 + col), CLEAR_DELAY);
  	else
    		lcd_send_cmd(client, LCD_SETDDRAMADDR | (0x40 + col), CLEAR_DELAY);
}
static void lcd_puts(struct i2c_client *client, const char *string, 
			u8 line,u8 col, u8 count)
{
  	u8 i;
  	_setDDRAMAddress(client, line, col);
  	for(i = 0; i < count-1; i++){
      		lcd_send_data(client, string[i]);
  	}
}

static int lcd_init(struct i2c_client *client){
  	BYTE msg[15] = {"WELCOME .!"};
  	int displayshift 	= (LCD_CURSORMOVE | LCD_MOVERIGHT);
  	int displaymode 	= (LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT);
  	int displaycontrol 	= (LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF);

  	lcd_send_cmd(client, 0x02, COMMAND_INIT_DELAY);
 	lcd_send_cmd(client, LCD_FUNCTIONSET | LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS , COMMAND_INIT_DELAY);
  	lcd_send_cmd(client, LCD_DISPLAYCONTROL | displaycontrol , COMMAND_INIT_DELAY);
  	lcd_send_cmd(client, LCD_ENTRYMODESET | displaymode , COMMAND_INIT_DELAY);
  	lcd_send_cmd(client, LCD_CLEARDISPLAY , COMMAND_INIT_DELAY);
  	lcd_send_cmd(client, LCD_CURSORSHIFT | displayshift , COMMAND_INIT_DELAY);
  	//lcd_send_cmd(client, LCD_RETURNHOME , COMMAND_INIT_DELAY);
  	lcd_send_cmd(client, LCD_RETURNHOME , COMMAND_INIT_DELAY);

  	lcd_puts(client, msg, 1, 0, 11);
  	return 0;
}
static void lcd_clear(struct i2c_client *client){
  lcd_send_cmd(client, LCD_CLEARDISPLAY , CLEAR_DELAY);
}

int lcd_exported_print(const char *string,u8 line,u8 col, u8 count)
{
	lcd_clear(&prv->client_prv);
	lcd_puts(&prv->client_prv,string, 1, 0, count);
	return 0;
}


static ssize_t I2c_LCD_clear_data(struct kobject *kobj, struct kobj_attribute *attr, 
			const char *buf, size_t count)
{
	int ret;
        int d;
        ret=sscanf(buf,"%d\n",&d);
        if(ret < 0 || d > 0 || d < 0)
                return -EINVAL;
        if(!d)
             lcd_clear(&prv->client_prv);

        return count;
}
static ssize_t I2c_LCD_print_data(struct kobject *kobj, struct kobj_attribute *attr, 
			const char *buf, size_t count)
{
	int ret;
	char d[256];
	ret=sscanf(buf,"%s",d);
	if(ret < 0)
		return -EINVAL;
	lcd_puts(&prv->client_prv, d, 1, 0, count);
	return count;
}

static ssize_t I2c_LCD_light_off(struct kobject *kobj, struct kobj_attribute *attr, 
					const char *buf, size_t count)
{
	int ret,d;
	BYTE data; 
        ret = sscanf(buf,"%d\n",&d);
	if(ret < 0 || d < 0 || d > 1)
                return -EINVAL;
        if(d)
             backlightFlag = 0x01;
	else
	     backlightFlag = 0x00;

  	data = (backlightFlag == 1 ? LCD_BL : 0x00);
  	
	mutex_lock(&lcd16x2_mutex);
  	i2c_master_send(&prv->client_prv, &data, 1);
  	mutex_unlock(&lcd16x2_mutex);
        return count;
}

static struct kobj_attribute lcd_print_data  = __ATTR(Print_Date, 0220,NULL,I2c_LCD_print_data);
static struct kobj_attribute lcd_clear_data  = __ATTR(Clear_Date, 0220,NULL,I2c_LCD_clear_data);
static struct kobj_attribute lcd_light_off   = __ATTR(LIght_off, 0220,NULL,I2c_LCD_light_off);

static struct attribute *attrs[] = {
        &lcd_print_data.attr,
        &lcd_clear_data.attr,
        &lcd_light_off.attr,
        NULL,
};

static struct attribute_group attr_group = {
        .attrs = attrs,
};

static int I2c_LCD_probe(struct i2c_client *client, 
				const struct i2c_device_id *id)
{
	int ret;
	pr_info("%s: Driver for 16x2 lcd is probed !......\n",__func__);	
	prv=(struct I2c_LCD_prv *)kzalloc(sizeof(struct I2c_LCD_prv), GFP_KERNEL);		
	if(!prv){
		pr_info("Requested memory not allocated\n");
		return -ENOMEM;
	}
	prv->client_prv = *client;
	prv->I2c_LCD_kobj=kobject_create_and_add("LCD_I2c", NULL);
	if(!prv->I2c_LCD_kobj)
		return -ENOMEM;
	ret= sysfs_create_group(prv->I2c_LCD_kobj, &attr_group);
	if(ret)
		kobject_put(prv->I2c_LCD_kobj);
	lcd_init(client);	
	return 0;
}
static int I2c_LCD_remove(struct i2c_client *client)
{
	pr_info("I2c_LCD_remove\n");
	lcd_clear(&prv->client_prv);
	kfree(prv);
	kobject_put(prv->I2c_LCD_kobj);
	return 0;
}

static const struct i2c_device_id I2c_LCD_ids[]={
	{ "PCF8574T", 0x27 },
	{ }
};

static struct i2c_driver I2c_LCD_drv = {
	.driver = {
		.name = "I2c_LCD",
		.owner = THIS_MODULE,
	},
	.probe    = I2c_LCD_probe,
	.remove   = I2c_LCD_remove,
	.id_table = I2c_LCD_ids,
};

/**
 * module_i2c_driver() - Helper macro for registering a modular I2C driver
 * @__i2c_driver: i2c_driver struct
 *
 * Helper macro for I2C drivers which do not do anything special in module
 * init/exit. This eliminates a lot of boilerplate. Each module may only
 * use this macro once, and calling it replaces module_init() and module_exit()
 */
module_i2c_driver(I2c_LCD_drv);

EXPORT_SYMBOL(lcd_exported_print);

MODULE_DESCRIPTION("Driver for 16x2 lcd display via I2C protocol");
MODULE_AUTHOR("Chandan jha <beingchandanjha@gmail.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION(".1");


