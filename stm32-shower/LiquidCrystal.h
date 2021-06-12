#pragma once
#include "stdint.h"

// commands
constexpr auto LCD_CLEARDISPLAY = 0x01;
constexpr auto LCD_RETURNHOME = 0x02;
constexpr auto LCD_ENTRYMODESET = 0x04;
constexpr auto LCD_DISPLAYCONTROL = 0x08;
constexpr auto LCD_CURSORSHIFT = 0x10;
constexpr auto LCD_FUNCTIONSET = 0x20;
constexpr auto LCD_SETCGRAMADDR = 0x40;
constexpr auto LCD_SETDDRAMADDR = 0x80;

// flags for display entry mode
constexpr auto LCD_ENTRYRIGHT = 0x00;
constexpr auto LCD_ENTRYLEFT = 0x02;
constexpr auto LCD_ENTRYSHIFTINCREMENT = 0x01;
constexpr auto LCD_ENTRYSHIFTDECREMENT = 0x00;

// flags for display on/off control
constexpr auto LCD_DISPLAYON = 0x04;
constexpr auto LCD_DISPLAYOFF = 0x00;
constexpr auto LCD_CURSORON = 0x02;
constexpr auto LCD_CURSOROFF = 0x00;
constexpr auto LCD_BLINKON = 0x01;
constexpr auto LCD_BLINKOFF = 0x00;

// flags for display/cursor shift
constexpr auto LCD_DISPLAYMOVE = 0x08;
constexpr auto LCD_CURSORMOVE = 0x00;
constexpr auto LCD_MOVERIGHT = 0x04;
constexpr auto LCD_MOVELEFT = 0x00;

// flags for function set
constexpr auto LCD_8BITMODE = 0x10;
constexpr auto LCD_4BITMODE = 0x00;
constexpr auto LCD_2LINE = 0x08;
constexpr auto LCD_1LINE = 0x00;
constexpr auto LCD_5x10DOTS = 0x04;
constexpr auto LCD_5x8DOTS = 0x00;

// flags for backlight control
constexpr auto LCD_BACKLIGHT = 0x08;
constexpr auto LCD_NOBACKLIGHT = 0x00;

constexpr auto En = 0x04;		// Enable bit;
constexpr auto Rw = 0x02;		// Read/Write bit;
constexpr auto Rs = 0x01;		// Register select bit;

typedef struct
{
    uint8_t displayfunction;
    uint8_t displaycontrol;
    uint8_t displaymode;
    uint8_t numlines;
    uint8_t cols;
    uint8_t rows;
    uint8_t backlightval;
} LiquidCrystal_I2C_Def;

// Кастомный символ градуса цельсия.
static constexpr uint8_t kCelsius[8] = {
	0b01100,
	0b10010,
	0b10010,
	0b01100,
	0b00000,
	0b00000,
	0b00000,
	0b00000
};

// Кастомный символ косой черты.
static constexpr uint8_t kBackslash[8] = { 
	0b00000,
	0b10000,
	0b01000,
	0b00100,
	0b00010,
	0b00001,
	0b00000,
	0b00000
};

// Кастомный символ - вертикальная черта.
static constexpr uint8_t kVerticalBar[8] = { 
	0b00100,
	0b00100,
	0b00100,
	0b00100,
	0b00100,
	0b00100,
	0b00100,
	0b00000      
};
    
// Кастомный символ - стрелка вправо.
static constexpr uint8_t kRightArrow[8] = { 
	0B00000,
	0B00100,
	0B00010,
	0B11111,
	0B00010,
	0B00100,
	0B00000,
	0B00000   
};

class LiquidCrystal final
{
public:

	/* When the display powers up, it is configured as follows:

	     1. Display clear
	     2. Function set:
	        DL = 1; 8-bit interface data
	        N = 0; 1-line display
	        F = 0; 5x8 dot character font
		 3. Display on/off control:
		    D = 0; Display off
		    C = 0; Cursor off
		    B = 0; Blinking off
		 4. Entry mode set:
		    I/D = 1; Increment by 1
		    S = 0; No shift

	Note, however, that resetting the Arduino doesn't reset the LCD, so we
	can't assume that its in that state when a sketch starts (and the
	LiquidCrystal constructor is called).*/
	bool Setup(uint8_t lcd_cols, uint8_t lcd_rows);

	/********** high level commands, for the user! */
	bool Clear();
	void SetCursor(uint8_t col, uint8_t row);
	void WriteString(const char* str);
	
private:
	
	
    
    LiquidCrystal_I2C_Def m_lcdi2c;

	bool Write(uint8_t value);
    bool Begin(uint8_t cols, uint8_t lines);
    bool Home();
    // Turn the display on/off (quickly).
    bool NoDisplay();
    bool Display();
    // Turns the underline cursor on/off.
    void NoCursor();
    void Cursor();
    // Turn on and off the blinking cursor.
    void NoBlink();
    void Blink();
    // These commands scroll the display without changing the RAM.
    void ScrollDisplayLeft();
    void ScrollDisplayRight();
    // This is for text that flows Left to Right.
    void LeftToRight();
    // This is for text that flows Right to Left.
    void RightToLeft();
    // This will 'right justify' text from the cursor.
    void AutoScroll();
    // This will 'left justify' text from the cursor.
    void NoAutoscroll();
    // Allows us to fill the first 8 CGRAM locations
    // with custom characters.
    bool CreateChar(uint8_t location, const uint8_t charmap[]);
    // Turn the (optional) backlight off/on.
    void NoBacklight();
    void Backlight();
    /*********** mid level commands, for sending data/cmds */
    bool Command(uint8_t value);
    /************ low level data pushing commands **********/
    // write either command or data.
    bool Send(uint8_t value, uint8_t mode);
    bool Write4bits(uint8_t value);
    bool ExpanderWrite(uint8_t data);
    bool PulseEnable(uint8_t data);
    void LoadCustomCharacter(uint8_t char_num, uint8_t *rows);
};
