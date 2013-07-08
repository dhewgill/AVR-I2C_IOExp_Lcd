/*
 * I2C_IOExp_Lcd.h
 *
 * Created: 6/17/2013 14:32:53
 *  Author: Dale Hewgill
 */ 


#ifndef I2C_IOEXP_LCD_H_
#define I2C_IOEXP_LCD_H_

//Defines:
#define IO_EXPANDER_ADDR 0x40
#define IO_EXP_CONF_REG 0x05
#define IO_EXP_IO_REG 0x09
#define I2C_WRITE_BIT 0x01
#define LCD_STD_CMD_DELAY 20	//microseconds
#define LCD_CLEAR_DELAY 2	//milliseconds
#define LCD_HOME_DELAY 2	//milliseconds
#define LCD_INIT_DELAY_1 4.1	//milliseconds
#define LCD_INIT_DELAY_2 100	//microseconds
#define BACKLIGHT_PORT 7
#define DB7_PORT 6
#define DB6_PORT 5
#define DB5_PORT 4
#define DB4_PORT 3
#define E_PORT 2
#define RS_PORT 1


//Provided functions:
void io_expander_init(void);
void lcd_init(void);
void lcd_write(uint8_t, uint8_t, uint8_t);
void lcd_putc(uint8_t, uint8_t);
void lcd_puts(char *, uint8_t, uint8_t);
void lcd_set_backlight(uint8_t);
void lcd_blink_backlight(unsigned int);
void lcd_clear(void);

#endif /* I2C_IOEXP_LCD_H_ */