#include "LiquidCrystal.h"
#include "Common.h"
#include "FreeRTOS.h"
#include "task.h"
#include "I2C.h"


bool LiquidCrystal::write(uint8_t value) 
{
    return send(value, Rs);
}

void LiquidCrystal::write_String(const char* str) 
{
    for (uint8_t i = 0; i < lcdi2c.cols; i++)
    {
        write(str[i]);
    }
}

bool LiquidCrystal::setup(uint8_t lcd_cols, uint8_t lcd_rows)
{
	lcdi2c.cols = lcd_cols;
	lcdi2c.rows = lcd_rows;
	lcdi2c.backlightval = LCD_BACKLIGHT;
	lcdi2c.displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;

	// set up number of columns and rows
	if(!begin(lcd_cols, lcd_rows))
	{
		return false;
	}

	// Create a new custom character.
	if(!createChar(1, celsius))
	{
		return false;
	}
    	
	if (!createChar(2, backslash))
	{
		return false;
	}
    	
	if (!createChar(3, vertical_bar))
	{
		return false;
	}
    	
	if (!createChar(4, right_arrow))
	{
		return false;
	}
    
	return true;
}

bool LiquidCrystal::begin(uint8_t cols, uint8_t lines) 
{
	if (lines > 1)
	{
		lcdi2c.displayfunction |= LCD_2LINE;
	}
		
	lcdi2c.numlines = lines;

			// for some 1 line displays you can select a 10 pixel high font
			/*	if ((dotsize != 0) && (lines == 1)) {
					_displayfunction |= LCD_5x10DOTS;
		}*/

	// SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
	// according to datasheet, we need at least 40ms after power rises above 2.7V
	// before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
	vTaskDelay(50 / portTICK_PERIOD_MS);

	// Now we pull both RS and R/W low to begin commands
	if(!expanderWrite(lcdi2c.backlightval))	// reset expanderand turn backlight off (Bit 8 =1)
	{
		return false;
	}
		
    //vTaskDelay(1000 / portTICK_PERIOD_MS); // Было
	vTaskDelay(1 / portTICK_PERIOD_MS); // Проверить

	//put the LCD into 4 bit mode
	// this is according to the hitachi HD44780 datasheet
	// figure 24, pg 46

	// we start in 8bit mode, try to set 4 bit mode
	if(!write4bits(0x03 << 4))
	{
		return false;
	}
			
	vTaskDelay(5 / portTICK_PERIOD_MS); // wait min 4.1ms

	// second try
	if(!write4bits(0x03 << 4))
	{
		return false;
	}
		
	vTaskDelay(5 / portTICK_PERIOD_MS); // wait min 4.1ms

	// third go!
	if(!write4bits(0x03 << 4))
	{
		return false;
	}
		
	Delay_us(150);

	// finally, set to 4-bit interface
	if(!write4bits(0x02 << 4))
	{
		return false;
	}

	// set # lines, font size, etc.
	if(!command(LCD_FUNCTIONSET | lcdi2c.displayfunction))
	{
		return false;
	}

	// turn the display on with no cursor or blinking default
	lcdi2c.displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	if (!display())
	{
		return false;
	}
    	
	// clear it off
	if(!clear())
	{
		return false;
	}
		
	// Initialize to default text direction (for roman languages)
	lcdi2c.displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;

	// set the entry mode
	if(!command(LCD_ENTRYMODESET | lcdi2c.displaymode))
	{
		return false;
	}
    	
	if (!home())
	{
		return false;
	}
		
	return true;
}

bool LiquidCrystal::clear() 
{
	bool result = command(LCD_CLEARDISPLAY); // clear display, set cursor position to zero
	vTaskDelay(3 / portTICK_PERIOD_MS);  // this command takes a long time!
	return result;
}

bool LiquidCrystal::home() 
{
	bool result = command(LCD_RETURNHOME);  // set cursor position to zero
	vTaskDelay(3 / portTICK_PERIOD_MS);  // this command takes a long time!
	return result;
}

void LiquidCrystal::setCursor(uint8_t col, uint8_t row) 
{
	int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
	
	if (row > lcdi2c.numlines) 
	{
		row = lcdi2c.numlines - 1;    // we count rows starting w/0
	}
	command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

bool LiquidCrystal::noDisplay() 
{
	lcdi2c.displaycontrol &= ~LCD_DISPLAYON;
	return command(LCD_DISPLAYCONTROL | lcdi2c.displaycontrol);
}

bool LiquidCrystal::display() 
{
	lcdi2c.displaycontrol |= LCD_DISPLAYON;
	if (!command(LCD_DISPLAYCONTROL | lcdi2c.displaycontrol))
		return false;
		
	return true;
}

void LiquidCrystal::noCursor() 
{
	lcdi2c.displaycontrol &= ~LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | lcdi2c.displaycontrol);
}

void LiquidCrystal::cursor() 
{
	lcdi2c.displaycontrol |= LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | lcdi2c.displaycontrol);
}

void LiquidCrystal::noBlink() 
{
	lcdi2c.displaycontrol &= ~LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | lcdi2c.displaycontrol);
}

void LiquidCrystal::blink() 
{
	lcdi2c.displaycontrol |= LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | lcdi2c.displaycontrol);
}

void LiquidCrystal::scrollDisplayLeft() 
{
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

void LiquidCrystal::scrollDisplayRight() 
{
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

void LiquidCrystal::leftToRight() 
{
	lcdi2c.displaymode |= LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | lcdi2c.displaymode);
}

void LiquidCrystal::rightToLeft() 
{
	lcdi2c.displaymode &= ~LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | lcdi2c.displaymode);
}

void LiquidCrystal::autoscroll() 
{
	lcdi2c.displaymode |= LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | lcdi2c.displaymode);
}

void LiquidCrystal::noAutoscroll() 
{
	lcdi2c.displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | lcdi2c.displaymode);
}

bool LiquidCrystal::createChar(uint8_t location, uint8_t charmap[]) 
{
	location &= 0x7; // we only have 8 locations 0-7
	command(LCD_SETCGRAMADDR | (location << 3));
	int i;
	for (i = 0; i < 8; i++) 
	{
		if (!write(charmap[i]))
		{
			return false;
		}
	}
	return true;
}

void LiquidCrystal::noBacklight() 
{
	lcdi2c.backlightval = LCD_NOBACKLIGHT;
	expanderWrite(0);
}

void LiquidCrystal::backlight() 
{
	lcdi2c.backlightval = LCD_BACKLIGHT;
	expanderWrite(0);
}

bool LiquidCrystal::command(uint8_t value) 
{
	return send(value, 0);
}

bool LiquidCrystal::send(uint8_t value, uint8_t mode) 
{
	uint8_t highnib = value & 0xF0;
	uint8_t lownib = (value << 4) & 0xF0;
		
	if (!write4bits((highnib) | mode))
	{
		return false;
	}
		
	if (!write4bits((lownib) | mode))
	{
		return false;
	}
		
	return true;
}

bool LiquidCrystal::write4bits(uint8_t value) 
{
	if (!expanderWrite(value))
	{
		return false;
	}
			
	if (!pulseEnable(value))
	{
		return false;
	}
		
	return true;
}

bool LiquidCrystal::expanderWrite(uint8_t data) 
{
	return _i2c.LCD_expanderWrite(data | lcdi2c.backlightval);
}

bool LiquidCrystal::pulseEnable(uint8_t data) 
{
	if (!expanderWrite(data | En))	// En high.
	{
		return false;
	}
		
	// Enable pulse must be >450ns.
	Delay_us(1);

	if (!expanderWrite(data & ~En))	// En low.
	{
		return false;
	}
		
	// commands need > 37us to settle.
	Delay_us(50);

	return true;
}

void LiquidCrystal::load_custom_character(uint8_t char_num, uint8_t *rows) 
{
	createChar(char_num, rows);
}
