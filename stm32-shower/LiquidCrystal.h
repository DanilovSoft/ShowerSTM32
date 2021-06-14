#pragma once
#include "Common.h"
#include "FreeRTOS.h"
#include "task.h"

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
    bool Setup(uint8_t lcd_cols, uint8_t lcd_rows)
    {
        m_lcdI2c.cols = lcd_cols;
        m_lcdI2c.rows = lcd_rows;
        m_lcdI2c.backlightval = LCD_BACKLIGHT;
        m_lcdI2c.displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;

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

    /********** high level commands, for the user! */
    bool Clear() 
    {
        bool result = Command(LCD_CLEARDISPLAY);  // clear display, set cursor position to zero
        vTaskDelay(3 / portTICK_PERIOD_MS);   // this command takes a long time!
        return result;
    }
    
    void SetCursor(uint8_t col, uint8_t row) 
    {
        int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
    
        if (row > m_lcdI2c.numlines) 
        {
            row = m_lcdI2c.numlines - 1;     // we count rows starting w/0
        }
        Command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
    }
    
    void WriteString(const char* str) 
    {
        for (uint8_t i = 0; i < m_lcdI2c.cols; i++)
        {
            Write(str[i]);
        }
    }
    
private:
    
    // commands.
    static constexpr auto LCD_CLEARDISPLAY = 0x01;
    static constexpr auto LCD_RETURNHOME = 0x02;
    static constexpr auto LCD_ENTRYMODESET = 0x04;
    static constexpr auto LCD_DISPLAYCONTROL = 0x08;
    static constexpr auto LCD_CURSORSHIFT = 0x10;
    static constexpr auto LCD_FUNCTIONSET = 0x20;
    static constexpr auto LCD_SETCGRAMADDR = 0x40;
    static constexpr auto LCD_SETDDRAMADDR = 0x80;

    // flags for display entry mode.
    static constexpr auto LCD_ENTRYRIGHT = 0x00;
    static constexpr auto LCD_ENTRYLEFT = 0x02;
    static constexpr auto LCD_ENTRYSHIFTINCREMENT = 0x01;
    static constexpr auto LCD_ENTRYSHIFTDECREMENT = 0x00;

    // flags for display on/off control.
    static constexpr auto LCD_DISPLAYON = 0x04;
    static constexpr auto LCD_DISPLAYOFF = 0x00;
    static constexpr auto LCD_CURSORON = 0x02;
    static constexpr auto LCD_CURSOROFF = 0x00;
    static constexpr auto LCD_BLINKON = 0x01;
    static constexpr auto LCD_BLINKOFF = 0x00;

    // flags for display/cursor shift.
    static constexpr auto LCD_DISPLAYMOVE = 0x08;
    static constexpr auto LCD_CURSORMOVE = 0x00;
    static constexpr auto LCD_MOVERIGHT = 0x04;
    static constexpr auto LCD_MOVELEFT = 0x00;

    // flags for function set.
    static constexpr auto LCD_8BITMODE = 0x10;
    static constexpr auto LCD_4BITMODE = 0x00;
    static constexpr auto LCD_2LINE = 0x08;
    static constexpr auto LCD_1LINE = 0x00;
    static constexpr auto LCD_5x10DOTS = 0x04;
    static constexpr auto LCD_5x8DOTS = 0x00;

    // flags for backlight control.
    static constexpr auto LCD_BACKLIGHT = 0x08;
    static constexpr auto LCD_NOBACKLIGHT = 0x00;

    static constexpr auto En = 0x04;  		// Enable bit.
    static constexpr auto Rw = 0x02;  		// Read/Write bit.
    static constexpr auto Rs = 0x01;  		// Register select bit.
    
    LiquidCrystal_I2C_Def m_lcdI2c;
    
    bool Write(uint8_t value) 
    {
        return Send(value, Rs);
    }

    bool Begin(uint8_t cols, uint8_t lines) 
    {
        if (lines > 1)
        {
            m_lcdI2c.displayfunction |= LCD_2LINE;
        }
        
        m_lcdI2c.numlines = lines;

        // for some 1 line displays you can select a 10 pixel high font
        /*	if ((dotsize != 0) && (lines == 1)) {
                _displayfunction |= LCD_5x10DOTS;
        }*/

    // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
    // according to datasheet, we need at least 40ms after power rises above 2.7V
    // before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
    vTaskDelay(50 / portTICK_PERIOD_MS);

        // Now we pull both RS and R/W low to begin commands
        if(!ExpanderWrite(m_lcdI2c.backlightval))	// reset expanderand turn backlight off (Bit 8 =1)
        {
            return false;
        }
        
        //vTaskDelay(1000 / portTICK_PERIOD_MS); // Было
        vTaskDelay(1 / portTICK_PERIOD_MS);  // Проверить

        //put the LCD into 4 bit mode
        // this is according to the hitachi HD44780 datasheet
        // figure 24, pg 46

        // we start in 8bit mode, try to set 4 bit mode
        if(!Write4bits(0x03 << 4))
        {
            return false;
        }
            
        vTaskDelay(5 / portTICK_PERIOD_MS);  // wait min 4.1ms

        // second try
        if(!Write4bits(0x03 << 4))
        {
            return false;
        }
        
        vTaskDelay(5 / portTICK_PERIOD_MS);  // wait min 4.1ms

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
        if(!Command(LCD_FUNCTIONSET | m_lcdI2c.displayfunction))
        {
            return false;
        }

        // turn the display on with no cursor or blinking default
        m_lcdI2c.displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
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
        m_lcdI2c.displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;

        // set the entry mode
        if(!Command(LCD_ENTRYMODESET | m_lcdI2c.displaymode))
        {
            return false;
        }
        
        if (!Home())
        {
            return false;
        }
        
        return true;
    }

    bool Home() 
    {
        bool result = Command(LCD_RETURNHOME);   // set cursor position to zero
        vTaskDelay(3 / portTICK_PERIOD_MS);   // this command takes a long time!
        return result;
    }

    // Turn the display on/off (quickly).
    bool NoDisplay() 
    {
        m_lcdI2c.displaycontrol &= ~LCD_DISPLAYON;
        return Command(LCD_DISPLAYCONTROL | m_lcdI2c.displaycontrol);
    }

    bool Display() 
    {
        m_lcdI2c.displaycontrol |= LCD_DISPLAYON;
        if (!Command(LCD_DISPLAYCONTROL | m_lcdI2c.displaycontrol))
            return false;
        
        return true;
    }

    // Turns the underline cursor on/off.
    void NoCursor() 
    {
        m_lcdI2c.displaycontrol &= ~LCD_CURSORON;
        Command(LCD_DISPLAYCONTROL | m_lcdI2c.displaycontrol);
    }

    void Cursor() 
    {
        m_lcdI2c.displaycontrol |= LCD_CURSORON;
        Command(LCD_DISPLAYCONTROL | m_lcdI2c.displaycontrol);
    }

    // Turn on and off the blinking cursor.
    void NoBlink() 
    {
        m_lcdI2c.displaycontrol &= ~LCD_BLINKON;
        Command(LCD_DISPLAYCONTROL | m_lcdI2c.displaycontrol);
    }

    void Blink() 
    {
        m_lcdI2c.displaycontrol |= LCD_BLINKON;
        Command(LCD_DISPLAYCONTROL | m_lcdI2c.displaycontrol);
    }

    // These commands scroll the display without changing the RAM.
    void ScrollDisplayLeft() 
    {
        Command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
    }

    void ScrollDisplayRight() 
    {
        Command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
    }

    // This is for text that flows Left to Right.
    void LeftToRight() 
    {
        m_lcdI2c.displaymode |= LCD_ENTRYLEFT;
        Command(LCD_ENTRYMODESET | m_lcdI2c.displaymode);
    }

    // This is for text that flows Right to Left.
    void RightToLeft() 
    {
        m_lcdI2c.displaymode &= ~LCD_ENTRYLEFT;
        Command(LCD_ENTRYMODESET | m_lcdI2c.displaymode);
    }

    // This will 'right justify' text from the cursor.
    void AutoScroll() 
    {
        m_lcdI2c.displaymode |= LCD_ENTRYSHIFTINCREMENT;
        Command(LCD_ENTRYMODESET | m_lcdI2c.displaymode);
    }

    // This will 'left justify' text from the cursor.
    void NoAutoscroll() 
    {
        m_lcdI2c.displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
        Command(LCD_ENTRYMODESET | m_lcdI2c.displaymode);
    }

    // Allows us to fill the first 8 CGRAM locations
    // with custom characters.
    bool CreateChar(uint8_t location, const uint8_t charmap[]) 
    {
        location &= 0x7;  // we only have 8 locations 0-7
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

    // Turn the (optional) backlight off/on.
    void NoBacklight() 
    {
        m_lcdI2c.backlightval = LCD_NOBACKLIGHT;
        ExpanderWrite(0);
    }

    void Backlight() 
    {
        m_lcdI2c.backlightval = LCD_BACKLIGHT;
        ExpanderWrite(0);
    }

    /*********** mid level commands, for sending data/cmds */
    bool Command(uint8_t value) 
    {
        return Send(value, 0);
    }

    /************ low level data pushing commands **********/
    // write either command or data.
    bool Send(uint8_t value, uint8_t mode) 
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

    bool Write4bits(uint8_t value) 
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

    bool ExpanderWrite(uint8_t data) 
    {
        return g_i2c.LCD_expanderWrite(data | m_lcdI2c.backlightval);
    }

    bool PulseEnable(uint8_t data) 
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

    void LoadCustomCharacter(uint8_t char_num, uint8_t *rows) 
    {
        CreateChar(char_num, rows);
    }
};
