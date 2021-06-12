#include "LiquidCrystal.h"
#include "Common.h"
#include "FreeRTOS.h"
#include "task.h"
#include "I2C.h"


bool LiquidCrystal::Write(uint8_t value) 
{
    return Send(value, Rs);
}

void LiquidCrystal::WriteString(const char* str) 
{
    for (uint8_t i = 0; i < m_lcdi2c.cols; i++)
    {
        Write(str[i]);
    }
}

bool LiquidCrystal::Setup(uint8_t lcd_cols, uint8_t lcd_rows)
{
	m_lcdi2c.cols = lcd_cols;
	m_lcdi2c.rows = lcd_rows;
	m_lcdi2c.backlightval = LCD_BACKLIGHT;
	m_lcdi2c.displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;

	// set up number of columns and rows
	if(!Begin(lcd_cols, lcd_rows))
	{
		return false;
	}

	// Create a new custom character.
	if(!CreateChar(1, kCelsius))
	{
		return false;
	}
    	
	if (!CreateChar(2, kBackslash))
	{
		return false;
	}
    	
	if (!CreateChar(3, kVerticalBar))
	{
		return false;
	}
    	
	if (!CreateChar(4, kRightArrow))
	{
		return false;
	}
    
	return true;
}

bool LiquidCrystal::Begin(uint8_t cols, uint8_t lines) 
{
	if (lines > 1)
	{
		m_lcdi2c.displayfunction |= LCD_2LINE;
	}
		
	m_lcdi2c.numlines = lines;

			// for some 1 line displays you can select a 10 pixel high font
			/*	if ((dotsize != 0) && (lines == 1)) {
					_displayfunction |= LCD_5x10DOTS;
		}*/

	// SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
	// according to datasheet, we need at least 40ms after power rises above 2.7V
	// before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
	vTaskDelay(50 / portTICK_PERIOD_MS);

	// Now we pull both RS and R/W low to begin commands
	if(!ExpanderWrite(m_lcdi2c.backlightval))	// reset expanderand turn backlight off (Bit 8 =1)
	{
		return false;
	}
		
    //vTaskDelay(1000 / portTICK_PERIOD_MS); // Было
	vTaskDelay(1 / portTICK_PERIOD_MS); // Проверить

	//put the LCD into 4 bit mode
	// this is according to the hitachi HD44780 datasheet
	// figure 24, pg 46

	// we start in 8bit mode, try to set 4 bit mode
	if(!Write4bits(0x03 << 4))
	{
		return false;
	}
			
	vTaskDelay(5 / portTICK_PERIOD_MS); // wait min 4.1ms

	// second try
	if(!Write4bits(0x03 << 4))
	{
		return false;
	}
		
	vTaskDelay(5 / portTICK_PERIOD_MS); // wait min 4.1ms

	// third go!
	if(!Write4bits(0x03 << 4))
	{
		return false;
	}
		
	DELAY_US(150);

	// finally, set to 4-bit interface
	if(!Write4bits(0x02 << 4))
	{
		return false;
	}

	// set # lines, font size, etc.
	if(!Command(LCD_FUNCTIONSET | m_lcdi2c.displayfunction))
	{
		return false;
	}

	// turn the display on with no cursor or blinking default
	m_lcdi2c.displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	if (!Display())
	{
		return false;
	}
    	
	// clear it off
	if(!Clear())
	{
		return false;
	}
		
	// Initialize to default text direction (for roman languages)
	m_lcdi2c.displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;

	// set the entry mode
	if(!Command(LCD_ENTRYMODESET | m_lcdi2c.displaymode))
	{
		return false;
	}
    	
	if (!Home())
	{
		return false;
	}
		
	return true;
}

bool LiquidCrystal::Clear() 
{
	bool result = Command(LCD_CLEARDISPLAY); // clear display, set cursor position to zero
	vTaskDelay(3 / portTICK_PERIOD_MS);  // this command takes a long time!
	return result;
}

bool LiquidCrystal::Home() 
{
	bool result = Command(LCD_RETURNHOME);  // set cursor position to zero
	vTaskDelay(3 / portTICK_PERIOD_MS);  // this command takes a long time!
	return result;
}

void LiquidCrystal::SetCursor(uint8_t col, uint8_t row) 
{
	int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
	
	if (row > m_lcdi2c.numlines) 
	{
		row = m_lcdi2c.numlines - 1;    // we count rows starting w/0
	}
	Command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

bool LiquidCrystal::NoDisplay() 
{
	m_lcdi2c.displaycontrol &= ~LCD_DISPLAYON;
	return Command(LCD_DISPLAYCONTROL | m_lcdi2c.displaycontrol);
}

bool LiquidCrystal::Display() 
{
	m_lcdi2c.displaycontrol |= LCD_DISPLAYON;
	if (!Command(LCD_DISPLAYCONTROL | m_lcdi2c.displaycontrol))
		return false;
		
	return true;
}

void LiquidCrystal::NoCursor() 
{
	m_lcdi2c.displaycontrol &= ~LCD_CURSORON;
	Command(LCD_DISPLAYCONTROL | m_lcdi2c.displaycontrol);
}

void LiquidCrystal::Cursor() 
{
	m_lcdi2c.displaycontrol |= LCD_CURSORON;
	Command(LCD_DISPLAYCONTROL | m_lcdi2c.displaycontrol);
}

void LiquidCrystal::NoBlink() 
{
	m_lcdi2c.displaycontrol &= ~LCD_BLINKON;
	Command(LCD_DISPLAYCONTROL | m_lcdi2c.displaycontrol);
}

void LiquidCrystal::Blink() 
{
	m_lcdi2c.displaycontrol |= LCD_BLINKON;
	Command(LCD_DISPLAYCONTROL | m_lcdi2c.displaycontrol);
}

void LiquidCrystal::ScrollDisplayLeft() 
{
	Command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

void LiquidCrystal::ScrollDisplayRight() 
{
	Command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

void LiquidCrystal::LeftToRight() 
{
	m_lcdi2c.displaymode |= LCD_ENTRYLEFT;
	Command(LCD_ENTRYMODESET | m_lcdi2c.displaymode);
}

void LiquidCrystal::RightToLeft() 
{
	m_lcdi2c.displaymode &= ~LCD_ENTRYLEFT;
	Command(LCD_ENTRYMODESET | m_lcdi2c.displaymode);
}

void LiquidCrystal::AutoScroll() 
{
	m_lcdi2c.displaymode |= LCD_ENTRYSHIFTINCREMENT;
	Command(LCD_ENTRYMODESET | m_lcdi2c.displaymode);
}

void LiquidCrystal::NoAutoscroll() 
{
	m_lcdi2c.displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
	Command(LCD_ENTRYMODESET | m_lcdi2c.displaymode);
}

bool LiquidCrystal::CreateChar(uint8_t location, const uint8_t charmap[]) 
{
	location &= 0x7; // we only have 8 locations 0-7
	Command(LCD_SETCGRAMADDR | (location << 3));
	int i;
	for (i = 0; i < 8; i++) 
	{
		if (!Write(charmap[i]))
		{
			return false;
		}
	}
	return true;
}

void LiquidCrystal::NoBacklight() 
{
	m_lcdi2c.backlightval = LCD_NOBACKLIGHT;
	ExpanderWrite(0);
}

void LiquidCrystal::Backlight() 
{
	m_lcdi2c.backlightval = LCD_BACKLIGHT;
	ExpanderWrite(0);
}

bool LiquidCrystal::Command(uint8_t value) 
{
	return Send(value, 0);
}

bool LiquidCrystal::Send(uint8_t value, uint8_t mode) 
{
	uint8_t highnib = value & 0xF0;
	uint8_t lownib = (value << 4) & 0xF0;
		
	if (!Write4bits((highnib) | mode))
	{
		return false;
	}
		
	if (!Write4bits((lownib) | mode))
	{
		return false;
	}
		
	return true;
}

bool LiquidCrystal::Write4bits(uint8_t value) 
{
	if (!ExpanderWrite(value))
	{
		return false;
	}
			
	if (!PulseEnable(value))
	{
		return false;
	}
		
	return true;
}

bool LiquidCrystal::ExpanderWrite(uint8_t data) 
{
	return g_i2c.LCD_expanderWrite(data | m_lcdi2c.backlightval);
}

bool LiquidCrystal::PulseEnable(uint8_t data) 
{
	if (!ExpanderWrite(data | En))	// En high.
	{
		return false;
	}
		
	// Enable pulse must be >450ns.
	DELAY_US(1);

	if (!ExpanderWrite(data & ~En))	// En low.
	{
		return false;
	}
		
	// commands need > 37us to settle.
	DELAY_US(50);

	return true;
}

void LiquidCrystal::LoadCustomCharacter(uint8_t char_num, uint8_t *rows) 
{
	CreateChar(char_num, rows);
}
