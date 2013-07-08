/*
 * I2C_IOExp_Lcd.c
 *
 * Created: 6/17/2013 14:29:07
 *  Author: Dale Hewgill
 
 
* Simple library to control a HD44780 based 16x2 LCD through
* Adafruit I2C based port expander 'backpack'.
Backpack connections are:
GP7:	Backlight
GP6:	DB7
GP5:	DB6
GP4:	DB5
GP3:	DB4
GP2:	E
GP1:	RS -> 0 selects configuration, 1 selects ddram or cgram
GP0:	NC
*/ 

//Includes.
#include <stdint.h>
#ifndef F_CPU
	#define F_CPU 8000000UL	      // Sets up the default speed for delay.h
#endif
#include <util/delay.h>

#include "USI_I2C_Master.h"
#include "I2C_IOExp_Lcd.h"

//Globals.
volatile uint8_t gBacklightState;

//Prototypes.
void io_expander_init(void);
void lcd_init(void);
void lcd_write(uint8_t, uint8_t, uint8_t);
void lcd_putc(uint8_t, uint8_t);
void lcd_puts(char *, uint8_t, uint8_t);
void lcd_set_backlight(uint8_t);
void toggle_e_line(uint8_t);
void lcd_blink_backlight(unsigned int);
void lcd_clear(void);

//Function definitions.
void io_expander_init(void)
{
	gBacklightState = 0;
	i2c_start(IO_EXPANDER_ADDR);	//Address the I/O expander for write.
	i2c_write(0x00);	//Start at address 0.
	i2c_write(0x00);	//All pins as outputs.
	
	i2c_write(0x00);
	i2c_write(0x00);
	i2c_write(0x00);
	i2c_write(0x00);
	
	// 	i2c_rep_start(IO_EXPANDER_ADDR);
	// 	i2c_write(IO_EXP_CONF_REG);	//Start at address 0x05 - IO expander configuration register.
	i2c_write(0x20);	//Disable sequential operation.
	i2c_stop();
}

void lcd_init(void)
{
	i2c_start(IO_EXPANDER_ADDR);	//Address the I/O expander for write.
	i2c_write(IO_EXP_IO_REG);		//Address the GPIO register.
	lcd_write(0x30,0,0);
	_delay_ms(LCD_INIT_DELAY_1);
	lcd_write(0x30,0,0);
	_delay_us(LCD_INIT_DELAY_2);
	lcd_write(0x30,0,0);	//HD44780 is now reset.
	_delay_us(LCD_INIT_DELAY_2);
	lcd_write(0x20,0,0);	//set 4bit mode.
	_delay_us(LCD_STD_CMD_DELAY);
	lcd_write(0x28,1,0);	//4bit mode, #lines=2, 5x8dot format.
	_delay_us(LCD_STD_CMD_DELAY);
	lcd_write(0x08,1,0);	//Display off, cursor off, blinking off.
	_delay_us(LCD_STD_CMD_DELAY);
	lcd_write(0x01,1,0);	//Clear display.
	_delay_ms(LCD_CLEAR_DELAY);
	lcd_write(0x06,1,0);	//Set entry mode, cursor move, no display shift.
	_delay_us(LCD_STD_CMD_DELAY);
	lcd_write(0x02,1,0);	//Return home.
	_delay_ms(LCD_HOME_DELAY);
	lcd_write(0x0c,1,0);	//Display on + no display cursor + no blinking on.
	_delay_us(LCD_STD_CMD_DELAY);
	
	i2c_stop();	//End transmission.
}

/*
Transfers a byte to the lcd through the port expander.
Inputs:
- val:	the value to be written on the data bus.
- mode:	either byte [0] or nibble [1] transfer mode.
- rs:	state of the register select line into the hd44780.
Mode refers to byte or nibble transfer mode [0 = nibble].
Assumptions:
- There is an active i2c session to the device.
- The i2c session will be terminated elsewhere.
*/
void lcd_write(uint8_t val, uint8_t nibbleMode, uint8_t rs)
{
	const uint8_t mask =	(1 << BACKLIGHT_PORT) | 
							(1 << DB7_PORT) | (1 << DB6_PORT) | (1 << DB5_PORT) | (1 << DB4_PORT) |
							(1 << RS_PORT);
	uint8_t temp;
	
	rs = (rs > 0) ? 1 : 0;

	//A "byte" transfer is really just the upper nibble.
	temp = (((val & 0xf0) >> (7 - DB7_PORT)) | (rs << RS_PORT) | (gBacklightState << BACKLIGHT_PORT)) & mask;	//upper nibble
	i2c_write(temp);
	i2c_write(temp | (1 << E_PORT));
	i2c_write(temp & ~(1 << E_PORT));

	if (nibbleMode)
	{
		temp = (((val & 0x0f) << DB4_PORT) | (rs << RS_PORT) | (gBacklightState << BACKLIGHT_PORT)) & mask;	//lower nibble
		i2c_write(temp);
		i2c_write(temp | (1 << E_PORT));
		i2c_write(temp & ~(1 << E_PORT));
	}
}

/*
Put a character to the lcd [and hopefully display it].
Set the RS line high to select data register in hd44780.
Pos is the line position [0x00-0x27:line1, 0x40-0x67:line2]
If pos = 255 then just write to the current cursor position.
Assumptions:
- 4-bit transfer mode.
- There is an active i2c session to the device.
- The i2c session will be terminated elsewhere.
*/
void lcd_putc(uint8_t val, uint8_t pos)
{
	if (pos < 0xff)
	{
		lcd_write((0x80|pos),1,0);	//Set the cursor position.
		_delay_us(LCD_STD_CMD_DELAY);
	}
	lcd_write(val,1,1);
	_delay_us(LCD_STD_CMD_DELAY);
}

/*
Put a real string [null terminated] or char buffer data to the lcd [and hopefully display it].
Set the RS line high to select data register in hd44780.
theStr is the string or buffer to print out.
strSz is the number of characters to print.
	- a value of 0 indicates that a null-terminated string is being printed.
Pos is the starting display position [0x00-0x27:line1, 0x40-0x67:line2]
If pos = 255 then just write to the current cursor position.
Assumptions:
- An input string is null terminated!  There is no other bounds checking!
- 4-bit transfer mode.
- There is an active i2c session to the device.
- The i2c session will be terminated elsewhere.
*/
void lcd_puts(char *theStr, uint8_t pos, uint8_t strSz)
{
	if (pos < 0xff)
	{
		lcd_write((0x80|pos),1,0);	//Set the initial cursor position.
		_delay_us(LCD_STD_CMD_DELAY);
	}
	if (strSz)
	{
		//Write a buffer...
		while(strSz-- > 0)
		{
			lcd_write((uint8_t) *theStr++, 1, 1);
			_delay_us(LCD_STD_CMD_DELAY);
		}
	}
	else
	{
		//Write a null-terminated string...
		while (*theStr != '\0')
		{
			lcd_write((uint8_t) *theStr++, 1, 1);
			_delay_us(LCD_STD_CMD_DELAY);
		}
	}
}

/*
Turn the backlight pin on or off.
0 = off, 1 = on.
Can't use in the middle of another transaction!
*/
void lcd_set_backlight(uint8_t state)
{
	gBacklightState = (state > 0) ? 1 : 0;
	i2c_start(IO_EXPANDER_ADDR);	//Address the I/O expander for write.
	i2c_write(IO_EXP_IO_REG);	//Address the GPIO register.
	i2c_write(gBacklightState << BACKLIGHT_PORT);
	i2c_stop();	//End transmission.
}

//Toggle the E line into the HD44780 to clock a command in or out.
//writeMask is the current state of all the other data lines.
//Assumes that an i2c session is already started and is still active.
//No delays needed because it takes ~22.5us to write a byte+(n)ack to the IO Expander at 400KHz i2c.
void toggle_e_line(uint8_t writeMask)
{
	i2c_write(writeMask | (1 << E_PORT));
	i2c_write(writeMask & ~(1 << E_PORT));
}

void lcd_blink_backlight(unsigned int numTimes)
{
	while (numTimes--)
	{
		lcd_set_backlight(1);
		_delay_ms(250);
		lcd_set_backlight(0);
		_delay_ms(250);
	}
}

void lcd_clear(void)
{
	i2c_start(IO_EXPANDER_ADDR);	//Address the I/O expander for write.
	i2c_write(IO_EXP_IO_REG);		//Address the GPIO register.
	lcd_write(0x01,1,0);			//Clear display.
	i2c_stop();						//End transmission.
	_delay_ms(LCD_CLEAR_DELAY);		//Delay for this command.
}