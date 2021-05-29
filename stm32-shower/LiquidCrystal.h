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


class LiquidCrystal
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
    bool setup(uint8_t lcd_cols, uint8_t lcd_rows);

    /********** high level commands, for the user! */
    bool clear();
    void setCursor(uint8_t col, uint8_t row);
    void write_String(const char* str);

private:
	
    uint8_t celsius[8] = {
        0b01100,
        0b10010,
        0b10010,
        0b01100,
        0b00000,
        0b00000,
        0b00000,
        0b00000
    };

    uint8_t backslash[8] = { 
        0b00000,
        0b10000,
        0b01000,
        0b00100,
        0b00010,
        0b00001,
        0b00000,
        0b00000
    };

    uint8_t vertical_bar[8] = { 
        0b00100,
        0b00100,
        0b00100,
        0b00100,
        0b00100,
        0b00100,
        0b00100,
        0b00000      
    };
    
    uint8_t right_arrow[8] = { 
        0B00000,
        0B00100,
        0B00010,
        0B11111,
        0B00010,
        0B00100,
        0B00000,
        0B00000   
    };
    
    LiquidCrystal_I2C_Def lcdi2c;
    
    
	bool write(uint8_t value);
    bool begin(uint8_t cols, uint8_t lines);
    bool home();
    // Turn the display on/off (quickly)
    bool noDisplay();
    bool display();
    // Turns the underline cursor on/off
    void noCursor();
    void cursor();
    // Turn on and off the blinking cursor
    void noBlink();
    void blink();
    // These commands scroll the display without changing the RAM
    void scrollDisplayLeft();
    void scrollDisplayRight();
    // This is for text that flows Left to Right
    void leftToRight();
    // This is for text that flows Right to Left
    void rightToLeft();
    // This will 'right justify' text from the cursor
    void autoscroll();
    // This will 'left justify' text from the cursor
    void noAutoscroll();
    // Allows us to fill the first 8 CGRAM locations
    // with custom characters
    bool createChar(uint8_t location, uint8_t charmap[]);
    // Turn the (optional) backlight off/on
    void noBacklight();
    void backlight();
    /*********** mid level commands, for sending data/cmds */
    bool command(uint8_t value);
    /************ low level data pushing commands **********/
    // write either command or data
    bool send(uint8_t value, uint8_t mode);
    bool write4bits(uint8_t value);
    bool expanderWrite(uint8_t data);
    bool pulseEnable(uint8_t data);
    void load_custom_character(uint8_t char_num, uint8_t *rows);
};
